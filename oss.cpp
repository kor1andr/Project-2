#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>      // system call data types (pid_t)
#include <sys/wait.h>       // wait()
#include <unistd.h>         // fork(), getpid(), getppid(), getopt()
#include <sys/ipc.h>        // IPC_CREAT, IPC_RMID
#include <sys/shm.h>        // shmget(), shmat(), shmdt(), shmctl()
#include <signal.h>         // signal(), kill()
#include <iomanip>          // std::setfill, std::setw

#define MAX_PROCS 20        // Max number of worker processes

// SIMULATED CLOCK
struct SimClock {
    unsigned int seconds;
    unsigned int nanoseconds;
};

// PROCESS CONTROL BLOCK
struct PCB {
    int occupied;
    pid_t pid;
    unsigned int startSeconds;
    unsigned int startNanoseconds;
};

volatile sig_atomic_t terminateFlag = 0; // Flag to indicate termination signal received

void handle_sigint(int) { terminateFlag = 1; }
void handle_sigalrm(int) { terminateFlag = 1; }

int main(int argc, char* argv[]) {
    int numberOfUsers = 5;
    int simul = 2;
    float timeLimit = 3.0f;
    float interval = 0.1f;
    int opt;

    // Parse command-line options using getopt()
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
    alarm(60);

    // SHARED MEMORY
    int shm_id = shmget(IPC_PRIVATE, sizeof(SimClock), IPC_CREAT | 0666);
    if (shm_id < 0) {
        std::cerr << "[ERROR] shmget failed." << std::endl;
        return 1;
    }
    SimClock* clock = (SimClock*) shmat(shm_id, nullptr, 0);
    if (clock == (void*) -1) {
        std::cerr << "[ERROR] shmat failed." << std::endl;
        shmctl(shm_id, IPC_RMID, nullptr);
        return 1;
    }
    clock->seconds = 0;
    clock->nanoseconds = 0;

    // PROCESS TABLE
    PCB processTable[MAX_PROCS] = {};

    int running = 0;
    // [launched] = var to keep track of total child processes launched
    int launched = 0;
    unsigned int lastLaunchSec = 0;
    unsigned int lastLaunchNano = 0;
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
            lastTablePrintSec = clock->seconds;
            lastTablePrintNano = clock->nanoseconds;
        }

        // Check for terminated child processes
        int status = 0;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            running--;
            for (int i = 0; i < MAX_PROCS; i++) {
                if (processTable[i].occupied && processTable[i].pid == pid) {
                    processTable[i].occupied = 0;
                    break;
                }
            }
        }

        // Launch new child processes if under limit
        if (launched < numberOfUsers && running < simul) {
            unsigned int sinceLastLaunch = (clock->seconds - lastLaunchSec) * 1000000000 + (clock->nanoseconds - lastLaunchNano);
            if (sinceLastLaunch >= (unsigned int)(interval * 1e9)) {
                int pcbIndex = -1;
                for (int i = 0; i < MAX_PROCS; ++i) {
                    if (!processTable[i].occupied) {
                        pcbIndex = i;
                        break;
                    }
                }
                if (pcbIndex != -1) {
                    pid_t cpid = fork();
                    if (cpid == 0) {
                        // Child: Execute worker process with timeLimit arg as seconds/nanoseconds and shm_id
                        int tSec = (int)timeLimit;
                        int tNano = (int)((timeLimit - tSec) * 1e9);
                        char secArg[16], nanoArg[16], shmIdArg[16];
                        snprintf(secArg, sizeof(secArg), "%d", tSec);
                        snprintf(nanoArg, sizeof(nanoArg), "%d", tNano);
                        snprintf(shmIdArg, sizeof(shmIdArg), "%d", shm_id);
                        execlp("./worker", "worker", secArg, nanoArg, shmIdArg, nullptr);
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

    // CLEANUP
    shmdt(clock);
    shmctl(shm_id, IPC_RMID, nullptr);

    std::cout << "OSS PID: " << getpid() << " terminating..." << "\n";
    std::cout << "Total workers launched: " << launched << "\n";
    return 0;
}
