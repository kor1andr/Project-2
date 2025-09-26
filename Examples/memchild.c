#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

const int BUFF_SZ = sizeof(int)*2;
int shm_key;
int shm_id;

int main() {

	int shm_key = ftok("memmain.c",0);
	if (shm_key <= 0 ) {
		fprintf(stderr,"Child:... Error in ftok\n");
		exit(1);
	}

	int shm_id = shmget(shm_key,BUFF_SZ,0700);	
	if (shm_id <= 0 ) {
		fprintf(stderr,"child:... Error in shmget\n");
		exit(1);
	}

	int *clock = (int *)shmat(shm_id,0,0);
	if (clock <= 0) {
		fprintf(stderr,"Child:... Error in shmat\n");
		exit(1);
	}

	int *sec = &(clock[0]);
	int *nano = &(clock[1]);

	printf("Child:\t sec %d , nanosecond %d\n",*sec, *nano);
	printf("Changing clock to 5 , 13\n");
	
	*sec = 5;
	*nano = 13;

	printf("Child:\t sec %d , nanosecond %d\n",*sec, *nano);

	printf("Child terminating\n");
	
	shmdt(clock);	
}
