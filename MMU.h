#ifndef MMU_H
#define MMU_H

#include "headers.h"

// ─────────────────────────────────────────────────────────────
//  CONSTANTS
// ─────────────────────────────────────────────────────────────
#define NUM_FRAMES        32
#define PAGE_SIZE         16
#define MAX_VIRTUAL_PAGES 64
#define MAX_PROCESSES     20

// ─────────────────────────────────────────────────────────────
//  DATA STRUCTURES
// ─────────────────────────────────────────────────────────────

typedef struct {
    int free;            // 1 = available
    int is_page_table;   // 1 = reserved for a page table, never evict
    int owner_pid;       // process ID that owns this frame (-1 if free)
    int vpn;             // virtual page number loaded here (-1 if free/PT)
    int R;               // referenced bit
    int M;               // modified bit
    int pending;
} Frame;

typedef struct {
    int valid;           // 1 = page is resident in RAM
    int frame;           // physical frame number (-1 if not resident)
    int R;               // referenced bit (mirrors frame_table)
    int M;               // modified bit   (mirrors frame_table)
} PTE;
// ─────────────────────────────────────────────────────────────
//  PUBLIC FUNCTION
// ─────────────────────────────────────────────────────────────

void MMU_init();

void MMU_process_start(FILE *mem_log, PCB *pcb, int now);

int MMU_translate(PCB *pcb, int virtual_address, char rw);

int MMU_handle_fault(FILE *mem_log, PCB *pcb, int virtual_address, char rw, int now);

void MMU_complete_page_load(FILE *mem_log, PCB *pcb, int now);

void MMU_process_finish(PCB *pcb);

void MMU_reset_r_bits();

#endif