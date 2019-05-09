#include<stdio.h>

void help(){
	printf("This program will simulate a memory manager.  It launches child processes which will then request pages of their logical memory.\nThe simulated memory manager will take these requests and translate them using page tables and a frame table.\nIf the page is not in the frame table, it will simulate a page fault and swap that page into the spot of the least recently used frame.\n");
	printf("There are four options:\n");
	printf("-h, which, as you have just discovered prints a help message.\n");
	printf("-n <number of children>, which specifies the maximum number of total children to be spawned.  The maximum is 18.");
	printf("-o <output file>, which will direct the output to a file designated by you.  The default is output.txt.\n");
	printf("-v, which triggers a more verbose output file which specifies each memory request and whether it is a hit or miss.\n");
	printf("-n, -v and -o, can be used in any combination.\n");
}
