all: OSS child

OSS: OSS.o scheduler1.o help.o
	gcc -std=gnu99 OSS.o scheduler1.o help.o -o OSS
	
OSS.o: OSS.c scheduler.h help.h
	gcc -std=gnu99 -c OSS.c
	
scheduler1.o: scheduler1.c scheduler.h
	gcc -std=gnu99 -c scheduler1.c

help.o: help.c help.h
	gcc -std=gnu99 -c help.c

child: child.o
	gcc -std=gnu99 child.c -o child
	
child.o: child.c
	gcc -std=gnu99 child.c
	
clean:
	rm *.o a.out
