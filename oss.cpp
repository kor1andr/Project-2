#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>      // pid_t
#include <sys/wait.h>       // waitpid(), WNOHANG
#include <unistd.h>         // fork(), getpid(), getppid(), getopt()
#include <sys/ipc.h>        // IPC_CREAT, IPC_PRIVATE, IPC_RMID
#include <sys/shm.h>        // shmget(), shmat(), shmdt(), shmctl()
#include <signal.h>         // signal(), SIGINT, SIGALRM, sig_atomic_t
#include <iomanip>          // std::cout

#define MAX_PROCS 20        // Max number of worker processes

// SIMULATED CLOCK
struct SimClock {
    unsigned int seconds;           // unsigned int for only non-negative values
    unsigned int nanoseconds;
};

// PROCESS CONTROL BLOCK
struct PCB {
    int occupied;                   // flag to indicate if entry currently in use; 1 = occupied, 0 = free
    pid_t pid;                      // stores PID of child process
    unsigned int startSeconds;      // time when process started
    unsigned int startNanoseconds;
};

// flag to indicate termination signal received
    // volatile = tells compiler the terminateFlag value may change at any time, so do not cache its value
    // sig_atomic_t = int type that guarantees read/write operations are atomic (not interruptible by main program or signal handler)
volatile sig_atomic_t terminateFlag = 0;

// SIGNAL HANDLERS for graceful termination (release resources)
void handle_sigint(int) { terminateFlag = 1; }          // handle_sigint(): sets terminateFlag to 1 when SIGINT received
void handle_sigalrm(int) { terminateFlag = 1; }         // handle_sigalrm(): sets terminateFlag to 1 when SIGALRM received

int main(int argc, char* argv[]) {
    int numberOfUsers = 5;          // set default # of workers to launch
    int simul = 2;                  // set default max # of simultaneous workers
    float timeLimit = 3.0f;         // set default time limit for each worker (seconds)
    float interval = 0.1f;          // set default min interval between worker launches (seconds)
    int opt;                        // variable to hold opt haracter returned with getopt()

    // PARSE CMD LINE OPTIONS: getopt()
    while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
        switch (opt) {
            case 'h':
                std::cout << "Usage: " << argv[0] << " [-h] [-n proc] [-s simul] [-t timeLimit] [-i interval]\n";
                std::cout << "  -h:             Help\n";
                std::cout << "  -n proc:        Number of total workers to launch\n";
                std::cout << "  -s simul:       Max number of simultaneous workers\n";
                std::cout << "  -t timelimitForChildren:            Amount of SIMULATED time before terminated (float, seconds)\n";
                std::cout << "  -i intervalsInMsToLaunchChildren:   Min interval between launches (float, seconds)\n";
                return 0;
            case 'n':
                // [std::atoi] = convert arg to int and store in respective variable
                numberOfUsers = std::atoi(optarg);
                break;
            case 's':
                simul = std::atoi(optarg);
                break;
            case 't':
                timeLimit = std::atof(optarg);
                break;
            case 'i':
                interval = std::atof(optarg);
                break;
            default:
                std::cerr << "ERROR: Invalid option. Use -h for help.\n";
                return 1;
        }
    }

    // Validate cmd line args
    if (numberOfUsers <= 0 || simul <= 0 || timeLimit <= 0.0f || interval <= 0.0f) {
        std::cerr << "[ERROR] Input must be positive values.\n";
        return 1;
    }

    std::cout << "OSS starting... PID: " << getpid() << ", PPID: " << getppid() << "\n";
    std::cout << "Called with:\n";
    std::cout << "-n: " << numberOfUsers << "\n";
    std::cout << "-s: " << simul << "\n";
    std::cout << "-t: " << timeLimit << "\n";
    std::cout << "-i: " << interval << "\n";

    // SIGNAL HANDLERS
    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_sigalrm);
    alarm(60);          // set timer to send SIGALRM singal after 60 sec real time -- so program doesn't run indefinitely    

    // SHARED MEMORY
        // shmget(): request shm segment == size of SimClock struct
        // IPC_PRIVATE = create new, unique shared memory segment
        // IPC_CREAT = create segment if it doesn't already exist
        // 0666 = read and write permissions for all users
    int shm_id = shmget(IPC_PRIVATE, sizeof(SimClock), IPC_CREAT | 0666);
    if (shm_id < 0) {
        std::cerr << "[ERROR] shmget failed." << std::endl;
        return 1;
    }
    // SimClock* = pointer to SimClock struct to access shared memory as a clock
    // shmat(): attach shm segment to process's address space so it can be accessed
    // nullptr, 0 = let system choose address to attach segment with default flags
    SimClock* clock = (SimClock*) shmat(shm_id, nullptr, 0);
    // If shmat() fails, remove the shared memory segment to prevent memory leaks
    if (clock == (void*) -1) {
        std::cerr << "[ERROR] shmat failed." << std::endl;
        shmctl(shm_id, IPC_RMID, nullptr);          // IPC_RMID = remove shared memory segment
        return 1;
    }
    // Initialize simulated clock to 0, so it begins at known state
    clock->seconds = 0;
    clock->nanoseconds = 0;

    // PROCESS TABLE
    PCB processTable[MAX_PROCS] = {};

    // Total simulated runtime of all worker processes
        // long long = 64-bit integer type to prevent overflow for large values
    unsigned long long totalWorkerSeconds = 0;
    unsigned long long totalWorkerNanoseconds = 0;

    int running = 0;
    // [launched] = var to keep track of total child processes launched
    int launched = 0;
    // Variables to track time since last launch
    unsigned int lastLaunchSec = 0;
    unsigned int lastLaunchNano = 0;
    // Variables to track time since last process table print
    unsigned int lastTablePrintSec = 0;
    unsigned int lastTablePrintNano = 0;

    // MAIN LOOP
    while ((launched < numberOfUsers || running > 0) && !terminateFlag) {
        // Increment clock 1 ms per loop iteration
        clock->nanoseconds += 1000000;
        if (clock->nanoseconds >= 1000000000) {
            clock->seconds += 1;
            clock->nanoseconds -= 1000000000;
        }

        // PRINT PROCESS TABLE every 0.5 simulated seconds
        // Calculate elapsed time since last print in nanosec
        unsigned int elapsedNano = (clock->seconds - lastTablePrintSec) * 1000000000 + (clock->nanoseconds - lastTablePrintNano);
        if (elapsedNano >= 500000000) {
            std::cout << "OSS PID: " << getpid() << std::endl;
            std::cout << "Seconds: " << clock->seconds << "\n";
            std::cout << "Nanoseconds: " << clock->nanoseconds << "\n";
            std::cout << "- - - Process Table - - -" << std::endl;
            std::cout << "Entry | Occupied | PID | StartSeconds | StartNano" << std::endl;
            for (int i = 0; i < MAX_PROCS; i++) {
                std::cout << i << " " << processTable[i].occupied
                          << " " << processTable[i].pid
                          << " " << processTable[i].startSeconds
                          << " " << processTable[i].startNanoseconds << std::endl;
            }
            // Update last print time to current clock time
            lastTablePrintSec = clock->seconds;
            lastTablePrintNano = clock->nanoseconds;
        }

        // Check for terminated child processes
        int status = 0;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {     // [waitpid] + [WNOHANG] = non-blocking check for finished child processes
            running--;
            // Find and update PCB for terminated child
            for (int i = 0; i < MAX_PROCS; i++) {
                if (processTable[i].occupied && processTable[i].pid == pid) {
                    // Calculate how long this worker ran (simulated time)
                    unsigned int startSec = processTable[i].startSeconds;
                    unsigned int startNano = processTable[i].startNanoseconds;
                    unsigned int endSec = clock->seconds;
                    unsigned int endNano = clock->nanoseconds;
                    unsigned int runSec = endSec - startSec;
                    int runNano = endNano - startNano;
                    if (runNano < 0) {
                        runSec -= 1;
                        runNano += 1000000000;
                    }
                    totalWorkerSeconds += runSec;
                    totalWorkerNanoseconds += runNano;
                    // Mark slot as free
                    processTable[i].occupied = 0;
                    break;
                }
            }
        }

    // LAUNCH NEW CHILD WORKER PROCESSES
        // check if total # [launched] < desired # [numberOfUsers]
        // check if [running] < allowed limit [simul]
        if (launched < numberOfUsers && running < simul) {
            // calculate amount of sim time passed since last worker launch
            unsigned int sinceLastLaunch = (clock->seconds - lastLaunchSec) * 1000000000 + (clock->nanoseconds - lastLaunchNano);
            if (sinceLastLaunch >= (unsigned int)(interval * 1e9)) {
                int pcbIndex = -1;
                for (int i = 0; i < MAX_PROCS; ++i) {
                    // loop through process table entries to find unoccupied slot
                    if (!processTable[i].occupied) { 
                        pcbIndex = i;
                        break;
                    }
                }
                // If available slot found in process table, oss to fork() then exec() off one worker to do its task
                if (pcbIndex != -1) {
                    pid_t cpid = fork();
                    if (cpid == 0) {
                        int tSec = (int)timeLimit;                          // calculate time limit for worker in seconds
                        int tNano = (int)((timeLimit - tSec) * 1e9);        // calculate time limit for worker in nanoseconds
                        char secArg[16], nanoArg[16], shmIdArg[16];         // arrays to store args' string equivalent, which will be passed to worker process
                        snprintf(secArg, sizeof(secArg), "%d", tSec);       // [snprintf]: convert calculated values and shm_id to strings
                        snprintf(nanoArg, sizeof(nanoArg), "%d", tNano);
                        snprintf(shmIdArg, sizeof(shmIdArg), "%d", shm_id);
                        execlp("./worker", "worker", secArg, nanoArg, shmIdArg, nullptr);   // [execlp]: replaces child process with worker executable
                    } else {
                        processTable[pcbIndex].occupied = 1;
                        processTable[pcbIndex].pid = cpid;
                        processTable[pcbIndex].startSeconds = clock->seconds;
                        processTable[pcbIndex].startNanoseconds = clock->nanoseconds;
                        running++;
                        launched++;
                        lastLaunchSec = clock->seconds;
                        lastLaunchNano = clock->nanoseconds;
                    }
                }
            }
        }
    }

    // Convert excess nanosec to seconds so value is < 1 billion
    totalWorkerSeconds += totalWorkerNanoseconds / 1000000000;
    totalWorkerNanoseconds = totalWorkerNanoseconds % 1000000000;

    // CLEANUP
    shmdt(clock);
    shmctl(shm_id, IPC_RMID, nullptr);

    std::cout << "OSS PID: " << getpid() << " Terminating" << "\n";
    std::cout << launched << " workers were launched and terminated\n";
    std::cout << "Workers ran for a combined time of " << totalWorkerSeconds << " seconds "
              << totalWorkerNanoseconds << " nanoseconds.\n";

    return 0;
}
