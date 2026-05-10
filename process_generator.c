#include "headers.h"

int msgid = -1;
int semid = -1;

void clearResources(int signum)
{
    if (msgid != -1)
        msgctl(msgid, IPC_RMID, NULL);

    if (semid != -1)
        semctl(semid, 0, IPC_RMID);

    destroyClk(true);
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read input file (argv[1] or default to "processes.txt")
    char *filename = (argc > 1) ? argv[1] : "processes.txt";

    FILE *input_file = fopen(filename, "r");
    if (input_file == NULL)
    {
        perror("Error opening input file");
        return 1;
    }

    char line[256];
    int id, arrivalTime, runTime, priority, base, limit;
    Process proc;

    Queue processes_queue;
    initQueue(&processes_queue);

    while (fgets(line, sizeof(line), input_file))
    {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        if (sscanf(line, "%d\t%d\t%d\t%d\t%d\t%d",
                   &id, &arrivalTime, &runTime, &priority, &base, &limit) == 6)
        {
            proc.ID          = id;
            proc.PID         = -1;
            proc.arrivalTime = arrivalTime;
            proc.runTime     = runTime;
            proc.priority    = priority;
            proc.base        = base;
            proc.limit       = limit;

            enqueue(&processes_queue, proc);
        }
        else
        {
            fprintf(stderr, "Error parsing line: %s\n", line);
            return 1;
        }
    }
    fclose(input_file);

    // 2. Ask only for quantum and K
    int quantum, K;
    printf("Enter quantum: ");
    scanf("%d", &quantum);
    printf("Enter K (R-bit reset interval in quantums): ");
    scanf("%d", &K);

    // 3. Create IPC resources
    msgid = msgget(111, 0666 | IPC_CREAT);
    semid = create_semaphore(111, 0);

    if (msgid == -1)
    {
        perror("msgget failed");
        exit(1);
    }

    // 4. Fork clock
    pid_t clk_pid = fork();
    if (clk_pid == 0)
    {
        execl("./clk.out", "clk.out", NULL);
        perror("Clock exec failed");
        exit(1);
    }

    // Init clock
    initClk();

    // 5. Fork scheduler (always RR)
    pid_t scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char q_arg[20], k_arg[20];
        sprintf(q_arg, "%d", quantum);
        sprintf(k_arg, "%d", K);
        execl("./scheduler.out", "scheduler.out", q_arg, k_arg, NULL);
        perror("Scheduler exec failed");
        exit(1);
    }

    struct msbuf message;
    message.mtype = 1;

    int now;
    int prev_time = 0;

    while (!isQueueEmpty(&processes_queue))
    {
        now = getClk();

        // tick gate — only act once per clock tick
        if (now == prev_time) continue;
        prev_time = now;

        // send all processes that arrived at or before current time
        while (!isQueueEmpty(&processes_queue))
        {
            Process p;
            peekQueue(&processes_queue, &p);

            if (p.arrivalTime <= now)
            {
                dequeue(&processes_queue, &p);
                message.proc = p;
                if (msgsnd(msgid, &message, sizeof(p), 0) == -1)
                    perror("msgsnd failed");
            }
            else
                break;
        }
        if (isQueueEmpty(&processes_queue)) {
            Process end_proc;
            end_proc.ID = -1;
            message.proc = end_proc;
            msgsnd(msgid, &message, sizeof(end_proc), 0);
        }
        sem_release(semid, 0);
    }

    // wait for scheduler to finish
    waitpid(scheduler_pid, NULL, 0);

    clearResources(0);
    return 0;
}