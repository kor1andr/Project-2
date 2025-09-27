#include <iostream>
#include <cstdlib>
#include <unistd.h>         // getpid(), getppid()
#include <sys/ipc.h>        // shmat()
#include <sys/shm.h>        // shmat(), shmdt()


// MAIN
    // Check for # of args: program name, seconds, nanoseconds, shm_id
int main(int argc, char* argv[]) {
    // If not enough args, print error and example usage
    if (argc < 4) {
        std::cerr << "[ERROR] Not enough arguments provided.\n";
        std::cout << "[Example] ./worker <seconds> <nanoseconds> <shm_id>\n";
        return 1;
    }

    // Parse args
        // [std::atoi] = convert arg from string --> int and store in respective variable
    int intervalSec = std::atoi(argv[1]);
    int intervalNano = std::atoi(argv[2]);
    int shm_id = std::atoi(argv[3]);

    // Retrieve and store PID + PPID
    pid_t pid = getpid();
    pid_t ppid = getppid();

    // [shmat] = attach shm_id to worker's address space [nullptr]
        // cast result to int* to access shared memory as an array of 2 ints
    int* clock = (int*)shmat(shm_id, nullptr, 0);
    // If shmat() fails, print error and exit ***
        std::cerr << "[WORKER] shmat failed.\n";
        return 1;

    // Retrieve initial simulate clock values from shared memory
        // [clock[0]] = seconds, [clock[1]] = nanoseconds, records start time
    int startSec = clock[0];
    int startNano = clock[1];

    // Calculate termination time (start time + interval)
    int termSec = startSec + intervalSec;
    int termNano = startNano + intervalNano;
    // Handle nanoseconds overflow; If termNano >= 1 billion, convert excess to seconds ((termNano % 1e9) + termsec)
    if (termNano >= 1000000000) {
        termSec += termNano / 1000000000;
        termNano = termNano % 1000000000;
    }

    // Print startup info (PID, PPID, system clock, term time)
    std::cout << "Worker PID: " << pid << ", PPID: " << ppid << "\n";
    std::cout << "Interval: " << intervalSec << " seconds, " << intervalNano << " nanoseconds\n";
    std::cout << "Start Time: " << startSec << " seconds, " << startNano << " nanoseconds\n";
    std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
    std::cout << "- Starting...\n";
    std::cout << "----------------------------------------\n";

    int lastPrintedSec = startSec;
    // Calculate target termination time (current clock + interval)
    while (true) {
        int currentSec = clock[0];
        int currentNano = clock[1];

    // Check if termination time reached
    if (currentSec > termSec || (currentSec == termSec && currentNano >= termNano)) {
        std::cout << "Worker PID: " << pid << " PPID: " << ppid << "\n";
        std::cout << "SysClock: " << currentSec << " seconds, " << currentNano << " nanoseconds\n";
        std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
        std::cout << "-- Terminating...\n";
        std::cout << "----------------------------------------\n";
        break;
    }

    // Periodic output (every second)
    if (currentSec > lastPrintedSec) {
        std::cout << "Worker PID: " << pid << " PPID: " << ppid << "\n";
        std::cout << "SysClock: " << termSec << " seconds, " << termNano << " nanoseconds\n";
        std::cout << "Termination Time: " << termSec << " seconds, " << termNano << " nanoseconds\n";
        std::cout << "-- " << (currentSec - startSec) << " seconds have passed since starting.\n";
        std::cout << "----------------------------------------\n";
        lastPrintedSec = currentSec;
    }
    // NO SLEEP
}
    // Detach from shared memory
    shmdt(clock);
    return 0;
}
