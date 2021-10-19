CFLAGS_TEST		=	-Wall -pedantic -std=c11 -Iinclude -Lbin -g -O0 -DWINDOWS
LIBS_TEST		=	-lsetupapi						\
					-lhid							\
					-lcfgmgr32						\
					-llibinpt

SRC			   := $(wildcard test/*.c)
SRC			   += src/debug.c

.PHONY: test

test:
	-powershell -c 'cp lib/* bin'
	$(CC) -v $(CFLAGS_TEST) $(SRC) $(LIBS_TEST) -o bin/test.exe