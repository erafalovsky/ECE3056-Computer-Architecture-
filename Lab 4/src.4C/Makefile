RM        := /bin/rm -rf
SIM       := ./sim
CC        := gcc
CFLAGS    := -O2 -lm -std=gnu99 -W -Wall -Wno-unused-parameter
DFLAGS    := -pg -g
PFLAGS    := -pg



all: 
	${CC} ${CFLAGS} cache.c  sim.c memsys.c dram.c  -o ${SIM}

dbg: 
	${CC} ${CFLAGS} ${DFLAGS} cache.c  sim.c memsys.c dram.c  -o ${SIM}

clean: 
	$(RM) ${SIM} *.o 
