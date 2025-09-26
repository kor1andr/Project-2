#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Access 2 args: max seconds and nanoseconds for worker to run
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "[ERROR] Not enough arguments provided.\n";
        std::cout << "[Example] ./worker <seconds> <nanoseconds> <shm_id>\n";
        return 1;
    }

    int intervalSec = std::atoi(argv[1]);
    int intervalNano = std::atoi(argv[2]);
    int shm_id = std::atoi(argv[3]);

    pid_t pid = getpid();
    pid_t ppid = getppid();

    // Attach to shared memory for system clock
    int* clock = (int*)shmat(shm_id, nullptr, 0);
    std::cerr << "[WORKER] shmat failed.\n";
    return 1;

    int startSec = clock[0];
    int startNano = clock[1];

    int termSec = startSec + intervalSec;
    int termNano = startNano + intervalNano;
    if (termNano >= 1000000000) {
        termSec += termNano / 1000000000;
        termNano = termNano % 1000000000;
    }

    // Print startup info (PID, PPID, system clock, termination time)
    std::cout << "Worker PID: " << pid << ", PPID: " << ppid << "\n";
    std::cout << "Interval: " << intervalSec << " seconds, " << intervalNano << " nanoseconds\n";
    std::cout << "Start Time: " << startSec << " seconds, " << startNano << " nanoseconds\n";
    std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
    std::cout << "----------------------------------------\n";

    int lastPrintedSec = startSec;
    // Calculate target termination time (current clock + interval)
    while (true) {
        int currentSec = clock[0];
        int currentNano = clock[1];

        // Check if termination time reached
        if (currentSec > termSec || (currentSec == termSec && currentNano >= termNano)) {
            std::cout << "Worker PID: " << pid << " PPID: " << ppid << "\n";
            std::cout << "Current Time: " << currentSec << " seconds, " << currentNano << " nanoseconds\n";
            std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
            std::cout << "Terminating...\n";
            break;
        }

        // Print message every second
        if (currentSec > lastPrintedSec) {
            std::cout << "Worker PID: " << pid << " PPID: " << ppid << "\n";
            std::cout << "Current Time: " << currentSec << " seconds, " << currentNano << " nanoseconds\n";
            std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
            std::cout << (currentSec - startSec) << " seconds have passed since starting.\n";
            lastPrintedSec = currentSec;
            std::cout << "----------------------------------------\n";
        }
        // No sleep, just check and print
    }

    // Detach from shared memory
    shmdt(clock);
    return 0;
}

