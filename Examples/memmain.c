#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

const int BUFF_SZ = sizeof(int)*2;
int shm_key;
int shm_id;

int main() {

	int shm_key = ftok("memmain.c",0);
	if (shm_key <= 0 ) {
		fprintf(stderr,"Parent:... Error in ftok\n");
		exit(1);
	}

	int shm_id = shmget(shm_key,BUFF_SZ,0700|IPC_CREAT);	
	if (shm_id <= 0 ) {
		fprintf(stderr,"Parent:... Error in shmget\n");
		exit(1);
	}

	int *clock = (int *)shmat(shm_id,0,0);
	if (clock <= 0) {
		fprintf(stderr,"Parent:... Error in shmat\n");
		exit(1);
	}

	int *sec = &(clock[0]);
	int *nano = &(clock[1]);
	*sec = *nano = 0;

	printf("Parent:\t sec %d , nanosecond %d\n",*sec,*nano);
		
	*sec = 17;
	*nano = 5013;

	printf("Parent:\t sec %d , nanosecond %d\n",*sec,*nano);
	
	// launch worker
	pid_t child_pid = fork();
	if (child_pid == 0) {
		// in child
		char *args[] = {"./memchild"};
		execlp(args[0],args[0],(char *)0);

		fprintf(stderr,"Error in exec after fork\n");
		exit(1);
	}
	else {
		// in parent
	}	

	wait(0);

	printf("Parent:\t sec %d , nanosecond %d\n",*sec,*nano);


	shmdt(clock);
	clock = 0;
	shmctl(shm_id,IPC_RMID,NULL);	
}
