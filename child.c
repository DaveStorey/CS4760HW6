#include<stdio.h>
#include<fcntl.h>
#include<sys/ipc.h>
#include<string.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/msg.h>
#include<signal.h>
#include<time.h>

#define resSize 20
#define available 8

static volatile int keepRunning = 1;

void intHandler(int dummy){
	printf("Process %d caught sigint.\n", getpid());
    keepRunning = 0;
}

struct clock{
	unsigned long nano;
	unsigned int sec;
};

struct mesg_buffer{
	long mesg_type;
	unsigned long processNum;
	int page
	int rdWr;
} message;

int main(int argc, char * argv[]){
	int pageTable[32];
	srand(getpid()*time(0));
	int msgid, msgid1, msgid2, resourcesHeld[resSize];
	for(int k = 0; k < resSize; k++){
		resourcesHeld[k] = 0;
	}
	unsigned int terminates, checkSpanSec = 0;
	char* ptr;
	pid_t pid = getpid();
	struct clock * shmPTR;
	signal(SIGINT, intHandler);
	unsigned long shmID, checkSpanNano;
	unsigned long key = strtoul(argv[0], &ptr, 10);
	unsigned long msgKey = strtoul(argv[1], &ptr, 10);
	unsigned long msgKey1 = strtoul(argv[2], &ptr, 10);
	unsigned long msgKey2 = strtoul(argv[3], &ptr, 10);
	unsigned long logicalNum = strtoul(argv[4], &ptr, 10);
	shmID = shmget(key, sizeof(struct clock), 0);
	shmPTR = (struct clock *) shmat(shmID, (void *)0, 0);
	msgid = msgget(msgKey, 0777 | IPC_CREAT);
	msgid1 = msgget(msgKey1, 0777 | IPC_CREAT);
	msgid2 = msgget(msgKey2, 0777 | IPC_CREAT);
	checkSpanNano = rand() % 250000;
	terminates = rand() % 100;
	message.mesg_type = 1;
	while (terminates > 20 && keepRunning == 1){
		message.processNum = logicalNum;
		message.page = rand() % 31;
		message.offset = rand() % 1023;
		msgsnd(msgid, &message, sizeof(message), 0);
		msgrcv(msgid1, &message, sizeof(message, logicalNum, 0);
	}
	printf("Child %li dying.\n", logicalNum);
	msgsnd(msgid2, & message, sizeof(message), 0);
	shmdt(shmPTR);
}
