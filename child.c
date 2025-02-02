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
	int request;
	int write;
} message;

int main(int argc, char * argv[]){
	srand(getpid()*time(0));
	int msgid, msgid1, msgid2, i = 0;
	unsigned int terminates;
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
	terminates = rand() % 100;
	message.mesg_type = 1;
	while (terminates > 20 && keepRunning == 1){
		i++;
		message.processNum = logicalNum;
		message.request = rand() % 32767;
		if (message.request % 100 <20){
			message.write = 1;
		}
		else{
			message.write = 0;
		}
		msgsnd(msgid, &message, sizeof(message), 0);
		msgrcv(msgid1, &message, sizeof(message), logicalNum, 0);
		//If statement ensures sufficient page requests
		if (i % 15 == 0)
			terminates = rand() % 100;
	}
	printf("Child %li dying.\n", logicalNum);
	message.processNum = logicalNum;
	msgsnd(msgid2, & message, sizeof(message), 0);
	shmdt(shmPTR);
}
