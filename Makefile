ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m += sysprof.o
sysprof-objs := module/sysprof.o module/data/netfilter.o module/shmem.o module/register.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o daemon/gather -Wall -lsqlite3 daemon/gather.c
	gcc -o daemon/statcalcs -Wall -lm -lsqlite3 daemon/statcalcs.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

gather:
	gcc -o gather -Wall -lsqlite3 daemon/gather.c

statcalcs:
	gcc -o statcalcs -Wall -lm -lsqlite3 daemon/statcalcs.c
