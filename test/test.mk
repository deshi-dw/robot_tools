CFLAGS_TEST		=	-Wall -pedantic -std=c11 -target x86_64-pc-windows-gnu -Iinclude -Lbin
LIBS_TEST		= 	-lm -lpthread					\
					-lsetupapi						\
					-lhid							\
					-lhidparse						\
					-lcfgmgr32						\
					-linpt

ifeq ($(OUTPUT), DEBUG)
	CFLAGS += -g -ggdb -O0
endif

SRC			   := $(wildcard test/*.c)

.PHONY: test

test:
	$(CC) $(CFLAGS_TEST) $(SRC) $(LIBS_TEST) -o bin/test.exe