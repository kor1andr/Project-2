// Acces 2 arg: max seconds and nanoseconds for worker to run
// Attach to shared memory for system clock
// Print startup info (PID, PPID, system clock, termination time)
// Calculate target termination time (current clock + interval)
// LOOP
    // Check system clock for termination time (no sleep)
    // On each new second, print periodic message
// On termination, print final message and EXIT
