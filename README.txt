Name: Megan Garrett
Date: 09/16/2025
Environment: VS Code, Linux
Version Control: GitHub https://github.com/kor1andr/Project-2
- - - - - - - - - -
[AI] GPT 4.1
    [Prompts]
    - Help me draft a bare-bones starting template for implementing shared memory based on the following requirements.
    - What formula can I use to handle nanosecond overflow?
    - Why am I receiving the following message sometimes in the output? "[WORKER] shmat failed"
    - 
[Summary]
    Found to be very useful, unfortunately... I'm not a fan of the increasing over-reliance on generative AI and
    do not feel it should be a used as a replacement, as it so often is pushed to be in professional settings--but
    rather an aid. Still, it's been some years since I have worked with C/C++, so it has been especially useful in
    helping me refresh quickly and reducing testing/debug time.
- - - - - - - - - -
[How to Compile]
    - In the terminal, type "make"
[How to Run]
    1) Run the main program 'oss' with default parameters: ./ oss
        - This will launch 5 workers, up to 2 at a time, each for 3 econds, with 0.1 seconds between launches.
        - OR -
    2) Type "-h" for detailed instructions/help.
    3) Input total # of workers to launch "-n": -n 3
    4) Input max # of workers to run simultaneously after "-s": -s 1
    5) Input amount of SIMULATED time before terminated (float, seconds) after "-t": -t 2.5
    6) Input minimum interval between launches (float, seconds) after "-i": -i 0.2
        Example: ./oss  -n 3    -s 1    -t 2.5  -i 0.2
        - This will launch 3 workers, up to 1 at a time, each for 2.5 seconds, with 0.2 seconds between launches.
