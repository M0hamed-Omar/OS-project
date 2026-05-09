#include "MMU.h"

// ─────────────────────────────────────────────────────────────
//  GLOBAL STATE (owned exclusively by the MMU)
// ─────────────────────────────────────────────────────────────

static Frame frame_table[NUM_FRAMES];
static PTE   page_tables[MAX_PROCESSES][MAX_VIRTUAL_PAGES];

// maps process ID → page_tables row index
static int   pid_to_pt_index[1024];
static int   next_pt_index = 0;
static int   disk_end = 0;

// ─────────────────────────────────────────────────────────────
//  INTERNAL HELPERS
// ─────────────────────────────────────────────────────────────

// convert integer address to trimmed binary string
static void to_binary(int value, char *out) {
    for (int i = 9; i >= 0; i--)
        out[9 - i] = ((value >> i) & 1) ? '1' : '0';
    out[10] = '\0';
    char *start = out;
    while (*start == '0' && *(start + 1) != '\0') start++;
    if (start != out) memmove(out, start, strlen(start) + 1);
}

static int get_pt_index_by_pid(int pid) {
    return pid_to_pt_index[pid];
}

// scan frames 0..31 for the first free one
static int find_free_frame() {
    for (int i = 0; i < NUM_FRAMES; i++)
        if (frame_table[i].free) return i;
    return -1;
}

// NRU: class 0→1→2→3, scan frames 0..31, skip page-table frames
static int nru_evict(FILE *mem_log) {
    for (int class = 0; class < 4; class++) {
        for (int f = 0; f < NUM_FRAMES; f++) {
            if (frame_table[f].is_page_table || frame_table[f].pending) continue;

            int frame_class = (frame_table[f].R << 1) | frame_table[f].M;
            if (frame_class != class) continue;

            int old_owner  = frame_table[f].owner_pid;
            int old_vpn    = frame_table[f].vpn;
            int old_pt_idx = get_pt_index_by_pid(old_owner);
            
            page_tables[old_pt_idx][old_vpn].valid = 0;

            frame_table[f].pending = 1;
            return f;
        }
    }
    return -1;  // should never happen per spec
}

// ─────────────────────────────────────────────────────────────
//  PUBLIC API
// ─────────────────────────────────────────────────────────────

// call once at program start
void MMU_init() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_table[i].free         = 1;
        frame_table[i].is_page_table = 0;
        frame_table[i].owner_pid    = -1;
        frame_table[i].vpn          = -1;
        frame_table[i].R            = 0;
        frame_table[i].M            = 0;
    }
    for (int i = 0; i < MAX_PROCESSES; i++)
        for (int j = 0; j < MAX_VIRTUAL_PAGES; j++) {
            page_tables[i][j].valid = 0;
            page_tables[i][j].frame = -1;
            page_tables[i][j].R     = 0;
            page_tables[i][j].M     = 0;
        }
    memset(pid_to_pt_index, -1, sizeof(pid_to_pt_index));
    next_pt_index = 0;
}

// ── called when a process first starts ──────────────────────
// allocates one frame for the page table and loads page 0.
// per spec: this takes no extra time.
void MMU_process_start(FILE *mem_log, PCB *pcb, int now) {
    int pid = pcb->proc->ID;

    // assign a page-table row
    pid_to_pt_index[pid] = next_pt_index++;
    int idx = pid_to_pt_index[pid];

    // initialise all PTEs as invalid
    for (int i = 0; i < MAX_VIRTUAL_PAGES; i++) {
        page_tables[idx][i].valid = 0;
        page_tables[idx][i].frame = -1;
        page_tables[idx][i].R     = 0;
        page_tables[idx][i].M     = 0;
    }

    // allocate frame for page table
    int pt_frame = find_free_frame();
    if (pt_frame == -1) {
        pt_frame = nru_evict(mem_log);
        
        if (frame_table[pt_frame].M) {
            fprintf(mem_log, "Swapping out page %d to disk\n", pt_frame);
            fflush(mem_log);
        }
    }
    else
        fprintf(mem_log, "Free Physical page %d allocated\n", pt_frame);

    frame_table[pt_frame].free         = 0;
    frame_table[pt_frame].is_page_table = 1;
    frame_table[pt_frame].owner_pid    = pid;
    frame_table[pt_frame].vpn          = -1;
    frame_table[pt_frame].R            = 0;
    frame_table[pt_frame].M            = 0;

    pcb->page_table_frame = pt_frame;
    pcb->page_table_index = idx;

    // load first virtual page (VPN 0) — no extra time per spec
    int data_frame = find_free_frame();
    if (data_frame == -1) {
        data_frame = nru_evict(mem_log);
        if (frame_table[data_frame].M) {
            fprintf(mem_log, "Swapping out page %d to disk\n", data_frame);
            fflush(mem_log);
        }
    }
    else
       fprintf(mem_log, "Free Physical page %d allocated\n", data_frame); 

    int disk_addr = pcb->proc->base + 0;

    frame_table[data_frame].free         = 0;
    frame_table[data_frame].is_page_table = 0;
    frame_table[data_frame].owner_pid    = pid;
    frame_table[data_frame].vpn          = 0;
    frame_table[data_frame].R            = 1;
    frame_table[data_frame].M            = 0;

    page_tables[idx][0].valid = 1;
    page_tables[idx][0].frame = data_frame;
    page_tables[idx][0].R     = 1;
    page_tables[idx][0].M     = 0;

    fprintf(mem_log, "At time %d disk address %d for process %d is loaded into memory page %d.\n", now, disk_addr, pid, data_frame);
    fflush(mem_log);
}

// ── translate a virtual address ──────────────────────────────
// returns physical address on hit, -1 on page fault.
// sets R (and M if write) on hit.
int MMU_translate(PCB *pcb, int virtual_address, char rw) {
    int vpn    = virtual_address >> 4;
    int offset = virtual_address & 0xF;
    int idx    = pcb->page_table_index;

    if (vpn >= pcb->proc->limit)
        return -2;  // out of bounds — ignore it totally

    PTE *entry = &page_tables[idx][vpn];

    if (!entry->valid)
        return -1;  // PAGE FAULT

    // hit — update reference bits
    entry->R = 1;
    frame_table[entry->frame].R = 1;

    if (rw == 'w') {
        entry->M = 1;
        frame_table[entry->frame].M = 1;
    }

    return entry->frame * PAGE_SIZE + offset;
}

// ── begin handling a page fault ──────────────────────────────
// logs the fault, claims a frame, stores pending info in PCB.
// returns total disk ticks needed (10 clean, 20 if dirty victim).
int MMU_handle_fault(FILE *mem_log, PCB *pcb, int virtual_address, char rw, int now) {
    int vpn = virtual_address >> 4;
    int pid = pcb->proc->ID;
    int disk_ticks = 10;

    char bin_str[16];
    to_binary(virtual_address, bin_str);
    fprintf(mem_log, "PageFault upon VA %s from process %d\n", bin_str, pid);
    fflush(mem_log);

    int frame = find_free_frame();

    if (frame != -1) {
        fprintf(mem_log, "Free Physical page %d allocated\n", frame);
        fflush(mem_log);
        frame_table[frame].free = 0;
    } else {
        // NRU eviction
        frame = nru_evict(mem_log);

        if (frame_table[frame].M) {
            fprintf(mem_log, "Swapping out page %d to disk\n", frame);
            fflush(mem_log);
            disk_ticks += 10;   // write-back + load = 20 ticks
        }
    }
    
    int pt_idx = get_pt_index_by_pid(pid);
    
    page_tables[pt_idx][vpn].valid = 1;
    page_tables[pt_idx][vpn].frame = frame;
    page_tables[pt_idx][vpn].R = 1;
    page_tables[pt_idx][vpn].M = (rw == 'w') ? 1 : 0;

    // // clear frame metadata — caller will repopulate
    frame_table[frame].owner_pid = pid;
    frame_table[frame].vpn       = vpn;
    frame_table[frame].free      = 0; 
    frame_table[frame].is_page_table = 0;
    frame_table[frame].R            = 1;
    frame_table[frame].M            = page_tables[pt_idx][vpn].M;

    // store pending fault info in PCB for scheduler
    pcb->pending_frame = frame;
    pcb->pending_vpn   = vpn;
    pcb->fault_rw      = rw;
    pcb->disk_ticks    = disk_ticks;
    if(now > disk_end) 
        pcb->disk_done_at = now + disk_ticks;
    else 
        pcb->disk_done_at = disk_end + disk_ticks;
    disk_end = pcb->disk_done_at;

    return disk_ticks;
}

// ── complete a page load after disk finishes ─────────────────
// called by scheduler when getClk() >= pcb->disk_done_at.
void MMU_complete_page_load(FILE *mem_log, PCB *pcb, int now) {
    int idx       = pcb->page_table_index;
    int vpn       = pcb->pending_vpn;
    int frame     = pcb->pending_frame;
    int disk_addr = pcb->proc->base + vpn;
    int pid       = pcb->proc->ID;

    // // update page table entry — page is now resident
    // page_tables[idx][vpn].valid = 1;
    // page_tables[idx][vpn].frame = frame;
    // page_tables[idx][vpn].R     = 1;
    // page_tables[idx][vpn].M     = (pcb->fault_rw == 'w') ? 1 : 0;

    // // update frame table
    // frame_table[frame].R = 1;
    // frame_table[frame].M = page_tables[idx][vpn].M;

    frame_table[frame].pending = 0;

    fprintf(mem_log, "At time %d disk address %d for process %d is loaded into memory page %d.\n", now, disk_addr, pid, frame);
    fflush(mem_log);

    // clear pending fields
    pcb->pending_frame = -1;
    pcb->pending_vpn   = -1;
    pcb->disk_done_at  = -1;
}

// ── free all memory owned by a finished process ──────────────
void MMU_process_finish(PCB *pcb) {
    int idx = pcb->page_table_index;

    // free all data frames
    for (int i = 0; i < MAX_VIRTUAL_PAGES; i++) {
        if (!page_tables[idx][i].valid) continue;
        int f = page_tables[idx][i].frame;
        frame_table[f].free         = 1;
        frame_table[f].is_page_table = 0;
        frame_table[f].owner_pid    = -1;
        frame_table[f].vpn          = -1;
        frame_table[f].R            = 0;
        frame_table[f].M            = 0;
        page_tables[idx][i].valid   = 0;
        page_tables[idx][i].frame   = -1;
        page_tables[idx][i].R       = 0;
        page_tables[idx][i].M       = 0;
    }

    // free the page-table frame
    int pt = pcb->page_table_frame;
    frame_table[pt].free         = 1;
    frame_table[pt].is_page_table = 0;
    frame_table[pt].owner_pid    = -1;

    // recycle the row for future processes
    pid_to_pt_index[pcb->proc->ID] = -1;
}

// ── reset R bits every K quantums ───────────────────────────
void MMU_reset_r_bits() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_table[i].free)          continue;
        if (frame_table[i].is_page_table) continue;

        frame_table[i].R = 0;

        int owner  = frame_table[i].owner_pid;
        int vpn    = frame_table[i].vpn;
        if (owner < 0 || vpn < 0) continue;
        int pt_idx = get_pt_index_by_pid(owner);
        if (pt_idx < 0) continue;
        page_tables[pt_idx][vpn].R = 0;
    }
}