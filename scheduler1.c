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

//Signal handler to catch ctrl-c
static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

struct clock{
	unsigned long nano;
	unsigned int sec;
};

struct frame{
	unsigned int reference;
	char dirty;
	int page;
	unsigned int process;
};

struct mesg_buffer{
	long mesg_type;
	unsigned long processNum;
	int request;
	int write;
}message;

void scheduler(char* outfile, int total, int verbFlag){
	int alive = 0, totalSpawn = 0, msgid, msgid1, msgid2, shmID, timeFlag = 0, i = 0, totalFlag = 0, pid[total], fileFlag = 0, swapFlag = 0, pageReq, lru, staleProc, pageCalls = 0;
	int pageTables[18][32];
	unsigned long increment, timeBetween;
	char * parameter[32], parameter1[32], parameter2[32], parameter3[32], parameter4[32], parameter5[32];
	//Pointer for the shared memory timer
	struct clock * shmPTR;
	struct clock launchTime;
	struct frame frameTable[256];
	for(int k = 0; k < 256; k++){
		frameTable[k].process = 0;
		frameTable[k].dirty = 0;
		frameTable[k].page = 0;
		frameTable[k].reference = 0;
	}
	for(int k = 0; k < 18; k++){
		for(int j = 0; j < 32; j++){
			pageTables[k][j] = -1;
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
		//Check on time the 
		if (when2 - when > 5){
			timeFlag = 1;
		}
		//Check on log file size
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
			pageCalls++;
			pageReq = message.request / 1024;
			if (pageTables[message.processNum-1][pageReq] == -1){
				if (verbFlag){
					fprintf(outPut, "%d:%li : Process %li page %d miss!  ", shmPTR[0].sec, shmPTR[0].nano, message.processNum-1, pageReq);
				}
				lru = 1000;
				staleProc = 0;
				for(int j = 0; j < 256; j++){
					if (frameTable[j].reference < lru){
						lru = frameTable[j].reference;
						staleProc = j;
					}
				}
				pageTables[frameTable[staleProc].process][frameTable[staleProc].page] = -1;
				frameTable[staleProc].process = message.processNum-1;
				frameTable[staleProc].page = pageReq;
				frameTable[staleProc].reference = 0;
				pageTables[message.processNum-1][pageReq] = staleProc;
				//Adding additional time for page swap.
				shmPTR[0].nano += increment / 1000;
				if (verbFlag){
					fprintf(outPut, "Swapped in at frame %d\n", staleProc);
				}
				message.mesg_type = message.processNum;
				msgsnd(msgid1, &message, sizeof(message), 0);
			}
			else{
				if (verbFlag){
					fprintf(outPut, "%d:%li : Process %li page %d found at %d\n", shmPTR[0].sec, shmPTR[0].nano, message.processNum-1, pageReq, pageTables[message.processNum-1][pageReq]);
				}
				message.mesg_type = message.processNum;
				msgsnd(msgid1, &message, sizeof(message), 0);
				staleProc = pageTables[message.processNum-1][pageReq];
			}
			if (pageCalls % 90 == 0){
				for(int j = 0; j < 256; j++){
					frameTable[j].reference = frameTable[j].reference / 2;
				}
			}
			//Setting reference and dirty variables
			if (frameTable[staleProc].reference < 128){
				frameTable[staleProc].reference += 128;
			}
			if (message.write){
				frameTable[staleProc].dirty = 1;
			}
			else{
				frameTable[staleProc].dirty = 0;
			}
			//Printing frame table every 150 cycles
			if (pageCalls % 150 == 0){
				fprintf(outPut, "Frame table at %d:%li:\n", shmPTR[0].sec, shmPTR[0].nano);
				for (int k = 0; k < 256; k++){
					fprintf(outPut, "\tFrame %d: Process %d, page %d, reference %d, dirty: %d\n", k, frameTable[k].process, frameTable[k].page, frameTable[k].reference, frameTable[k].dirty);
				}
			}
		}
		//Keeping track of dying children
		if (msgrcv(msgid2, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			alive--;
			pid[message.processNum-1] = -1;
		}
	}
	/*While loop is copy of message checks above, but runs after all
	children have been spawned, until they have all terminated, the time
	flag has been tripped, ctrl-c is pressed, or the file fills up.*/
	while((alive > 0) && (keepRunning == 1) && (timeFlag == 0) && fileFlag == 0){
		time(&when2);
		if (when2 - when > 5){
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
		if (msgrcv(msgid, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			pageCalls++;
			pageReq = message.request / 1024;
			if (pageTables[message.processNum-1][pageReq] == -1){
				if (verbFlag){
					fprintf(outPut, "%d:%li : Process %li page %d miss!  ", shmPTR[0].sec, shmPTR[0].nano, message.processNum-1, pageReq);
				}
				lru = 1000;
				staleProc = 0;
				for(int j = 0; j < 256; j++){
					if (frameTable[j].reference < lru){
						lru = frameTable[j].reference;
						staleProc = j;
					}
				}
				if (verbFlag){
					fprintf(outPut, "Swapped in at frame %d\n", staleProc);
				}
				//Adding additional time for page swap.
				shmPTR[0].nano += increment / 1000;
				pageTables[frameTable[staleProc].process][frameTable[staleProc].page] = -1;
				frameTable[staleProc].process = message.processNum-1;
				frameTable[staleProc].page = pageReq;
				frameTable[staleProc].reference = 0;
				pageTables[message.processNum-1][pageReq] = staleProc;
				message.mesg_type = message.processNum;
				msgsnd(msgid1, &message, sizeof(message), 0);
			}
			else{
				if (verbFlag){
					fprintf(outPut, "%d:%li : Process %li page %d found at %d\n", shmPTR[0].sec, shmPTR[0].nano, message.processNum-1, pageReq, pageTables[message.processNum-1][pageReq]);
				}
				message.mesg_type = message.processNum;
				msgsnd(msgid1, &message, sizeof(message), 0);
				staleProc = pageTables[message.processNum-1][pageReq];
			}
			if (pageCalls % 90 == 0){
				for(int j = 0; j < 256; j++){
					frameTable[j].reference = frameTable[j].reference / 2;
				}
			}
			if (frameTable[staleProc].reference < 128){
				frameTable[staleProc].reference += 128;
			}
			if (message.write){
				frameTable[staleProc].dirty = 1;
			}
			else{
				frameTable[staleProc].dirty = 0;
			}
			if (pageCalls % 150 == 0){
				fprintf(outPut, "Frame table at %d:%li:\n", shmPTR[0].sec, shmPTR[0].nano);
				for (int k = 0; k < 256; k++){
					fprintf(outPut, "\tFrame %d: Process %d, page %d, reference %d, dirty: %d\n", k, frameTable[k].process, frameTable[k].page, frameTable[k].reference, frameTable[k].dirty);
				}
			}
		}
		if (msgrcv(msgid2, &message, sizeof(message), 0, IPC_NOWAIT) !=-1){
			alive--;
			pid[message.processNum-1] = -1;
		}
	}
	if(timeFlag == 1){
		printf("Program has reached its allotted time, exiting.\n");
	}
	if(totalFlag == 1){
		printf("Program has reached its allotted children, exiting.\n");
	}
	if(keepRunning == 0){
		printf("Terminated due to ctrl-c signal.\n");
	}
	if (fileFlag == 1){
		printf("Terminated due to log file length.\n");
	}
	for (int i = 0; i < total; i++){
		if (pid[i] > -1){
			if (kill(pid[i], SIGINT) == 0){
				usleep(100000);
				kill(pid[i], SIGKILL);
			}
		}
	}
	shmdt(shmPTR);
	shmctl(shmID, IPC_RMID, NULL);
	msgctl(msgid, IPC_RMID, NULL);
	msgctl(msgid1, IPC_RMID, NULL);
	msgctl(msgid2, IPC_RMID, NULL);
	wait(NULL);
	fclose(outPut);
}
