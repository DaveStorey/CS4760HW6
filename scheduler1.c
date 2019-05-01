#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/msg.h>

#define resSize 20
#define available 10

//Signal handler to catch ctrl-c
static volatile int keepRunning = 1;
static int deadlockDetector(int, int[resSize][2], int[][resSize], int[], FILE *);

void intHandler(int dummy) {
    keepRunning = 0;
}

struct clock{
	unsigned long nano;
	unsigned int sec;
};

struct frame{
	char reference[8];
	char dirty;
	unsigned int offset;
	unsigned int process;
};

struct mesg_buffer{
	long mesg_type;
	unsigned long processNum;
	int page;
	int rdWr;
}message;

void scheduler(char* outfile, int total, int verbFlag){
	unsigned int quantum = 500000;
	int alive = 0, totalSpawn = 0, msgid, msgid1, msgid2, shmID, timeFlag = 0, i = 0, totalFlag = 0, pid[total], status, request[total][resSize], grantFlag = 0, deadlockFlag = 0, processStuck = 0, lastCheck = 0, fileFlag = 0, tableIter = 0, swapFlag = 0;
	int resource[resSize][2], resOut[total][resSize];
	unsigned long increment, timeBetween;
	char * parameter[32], parameter1[32], parameter2[32], parameter3[32], parameter4[32], parameter5[32];
	//Pointer for the shared memory timer
	struct clock * shmPTR;
	struct clock launchTime;
	struct frame frameTable[256];
	for(int i = 0; i < 256; i++){
		frameTable[i].process = 0;
		frameTable[i].dirty = 0;
		frameTable[i].offset = 0;
		for(int j = 0; j < 8; j++){
			frameTable[i].reference[j] = 0;
		}
	}
	//Time variables for the time out function
	time_t when, when2;
	//File pointers for input and output, respectively
	FILE * outPut;
	//Key variable for shared memory access.
	unsigned long key, msgKey, msgKey1, msgKey2;
	srand(time(0));
	timeBetween = (rand() % 100000000) + 1000000;
	key = rand();
	msgKey = ftok("child.c", 65);
	msgKey1 = ftok("child.c", 67);
	msgKey2 = ftok("child.c", 69);
	msgid = msgget(msgKey, 0777 | IPC_CREAT);
	msgid1 = msgget(msgKey1, 0777 | IPC_CREAT);
	msgid2 = msgget(msgKey2, 0777 | IPC_CREAT);
	message.mesg_type = 1;
	//Setting initial time for later check.
	time(&when);
	outPut = fopen(outfile, "w");
	//Check for file error.
	if (outPut == NULL){
		perror("Error");
		printf("Output file could not be created.\n");
		exit(EXIT_SUCCESS);
	}
	printf("\n");
	//Get and access shared memory, setting initial timer state to 0.
	shmID = shmget(key, sizeof(struct clock), IPC_CREAT | IPC_EXCL | 0777);
	shmPTR = (struct clock *) shmat(shmID, (void *) 0, 0);
	shmPTR[0].sec = 0;
	shmPTR[0].nano = 0;
	//initializing resource arrays
	launchTime.sec = 0;
	launchTime.nano = timeBetween;
	//Initializing pids to -1 
	for(int k = 0; k < total; k++){
		pid[k] = -1;
	}
	//Call to signal handler for ctrl-c
	signal(SIGINT, intHandler);
	increment = (rand() % 5000000) + 25000000;
	//While loop keeps running until all children are spawned, ctrl-c, or time is reached.
	while((i < total) && (keepRunning == 1) && (timeFlag == 0) && fileFlag == 0){
		time(&when2);
		if (when2 - when > 10){
			timeFlag = 1;
		}
		if (ftell(outPut) > 10000000){
			fileFlag = 1;
		}
		//Incrementing the timer.
		shmPTR[0].nano += increment;
		if(shmPTR[0].nano >= 1000000000){
			shmPTR[0].sec += 1;
			shmPTR[0].nano -= 1000000000;
		}
		//If statement to spawn child if timer has passed its birth time.
		if((shmPTR[0].sec > launchTime.sec) || ((shmPTR[0].sec == launchTime.sec) && (shmPTR[0].nano > launchTime.nano))){
			if((pid[i] = fork()) == 0){
			//Converting key, shmID and life to char* for passing to exec.
				sprintf(parameter1, "%li", key);
				sprintf(parameter2, "%li", msgKey);
				sprintf(parameter3, "%li", msgKey1);
				sprintf(parameter4, "%li", msgKey2);
				sprintf(parameter5, "%d", i+1);
				srand(getpid() * (time(0) / 3));
				char * args[] = {parameter1, parameter2, parameter3, parameter4, parameter5, NULL};
				execvp("./child\0", args);
			}
			//Updating launch time for next child.
			launchTime.sec = shmPTR[0].sec;
			launchTime.nano = shmPTR[0].nano;
			launchTime.nano += timeBetween;
			if(launchTime.nano >= 1000000000){
				launchTime.sec += 1;
				launchTime.nano -= 1000000000;
			}
			alive++;
			totalSpawn++;
			i++;
		}
		//Checking for message from exiting child
		if (msgrcv(msgid, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			while(frameTable[tableIter].process != 0 && tableIter < 256){
				if(frameTable[tableIter].process == message.processNum){
					if(frameTable[tableIter].page = message.page){
						printf("Process %li Page %d found in frame table\n", message.processNum, message.page);
						frameTable.reference[0] = 1;
						if (message.rdWr){
							frameTable[tableIter].dirty = 1;
						}
						else{
							frameTable[tableIter].dirty = 0;
						}
						break;
					}
				}
				tableIter++;
			}
			tableIter = 0;
			for(int j = 0; j < 256; j++){
				for(int k = 0; k < 8; k++){
					if (frameTable[j].referece[k] == 1){
						if (k > lru){
							lru = k;
							staleProc = j;
							break
						}
					}
				}
			}
			frameTable[staleProc].process = message.processNum;
			frameTable[staleProc].page = message.page;
			printf("Process %li page %d swapped in.\n", message.processNum, message.page);
			frameTable[staleProc.referece[0] = 1;
			for(int j = 1; j < 8; j++){
				frameTable[staleProc].referece[j] = 0;
			}
			if (message.rdWr){
				frameTable[staleProc].dirty = 1;
			}
			else{
				frameTable[staleProc].dirty = 0;
			}
		}
			
