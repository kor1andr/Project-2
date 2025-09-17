#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>      // system call data types (pid_t)
#include <sys/wait.h>       // wait()
#include <unistd.h>         // fork(), getpid(), getppid(), sleep(), getopt()
#include <sys/ipc.h>        // IPC_CREAT, IPC_RMID
#include <sys.shm.h>        // shmget(), shmat(), shmdt(), shmctl()
#include <signal.h>         // signal(), kill()
#include <vector>           // std::vector
#inlude <iomanip>           // std::setfill, std::setw

// SIMULATED CLOCK


// PROCESS TABLE


int main(int argc, char* argv[]) {
    int numberOfUsers = 5;      // Default number of users to launch
    int simul = 2;              // Default max number of simultaneous users
    float timeLimit = 3.0f;     // Default time limit for each user (seconds)
    float interval = 0.1f;      // Default interval between launches (seconds)
    int opt;                    // Option variable for getopt()
}
    // Parse cmd line options using getopt()
    while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
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

    // if numberOfUsers, simul, or iterations are not positive ints, print ERROR and EXIT
    if (numberOfUsers <= 0 || simul <= 0 || timeLimit <= 0.0f || iterval <= 0.0f) {
        std::cerr << "[ERROR] Input must be positive integers.\n";
        return 1;
    }


    // [running] = var to keep track of child processes currently running
    int running = 0;
    // [launched] = var to keep track of total child processes launched
    int launched = 0;


    // while loop continues so long as launched < numberOfUsers [-n]
    while (launched < numberOfUsers) {
        // verify number of currently running processes < allowed simultaneous processes
        if (running < simul) {
            pid_t pid = fork();                                     // Create new child process using fork()
            // if fork() FAILS, print ERROR and EXIT
            if (pid < 0) {
                std::cerr << "[ERROR] Fork failed." << std::endl;
                return EXIT_FAILURE;
            } else if (pid == 0) {                                  // pid == 0 only true for child process
                char arg[10];                                       // [arg] = char array to hold string version of [iterations] var
                snprintf(arg, sizeof(arg), "%d", iterations);       // [snprintf()] = convert [iterations] int to string and store in [arg]
                execlp("./user", "user", arg, nullptr);             // [execlp()] = replace child process with "user" program
                std::cerr << "[ERROR] Exec failed." << std::endl;   // if [execlp()] fails, print ERROR and EXIT
                exit(EXIT_FAILURE);
            } else {
            // Successful fork, increment running and launched counters
                running++;
                launched++;
            }
        } else {
            // If simul limit reached, wait for child process to finish
            wait(nullptr);
            // Decrement running counter so simul limit not exceeded
            running--;
        }
    }

    // Wait for remaining child processes to finish before exiting
    while (running > 0) {
        wait(nullptr);
        running--;
    }

    // Print total number of children launched
    std::cout << "Launched: " << numberOfUsers << " children." << std::endl;
    return EXIT_SUCCESS;
}
