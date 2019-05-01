//David Storey
//CS4760
//Project 5

#include<stdio.h>
#include<fcntl.h>
#include<getopt.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<stdlib.h>
#include "help.h"
#include "scheduler.h"


int main(int argc, char * argv[]){      
	int opt, x = 0, total = 18, hFlag = 0, verbFlag = 0;
	char * outfile = "output.txt";
	//Assigning default case name.
	if((opt = getopt(argc, argv, "-hovn")) != -1){
		do{
			x++;
			switch(opt){
			case 'h':
				help();
				hFlag = 1;
				break;
			case 'o':
				outfile = argv[x+1];
				break;
			case 'n':
				total = atoi(argv[x+1]);
				if (total > 18)
					total = 18;
				break;
			case 'v':
				verbFlag = 1;
				break;
			//Case handles last run through of getopt.
			case 1:
				break;
			}
		} while((opt = getopt(argc, argv, "-hovn")) != -1);
	}
	if (hFlag == 0)
		scheduler(outfile, total, verbFlag);
	return 0; 
}
