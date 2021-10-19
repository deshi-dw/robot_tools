NAME 			= libinpt
VERSION_MAJOR 	= 0
VERSION_MINOR 	= 1
OUTPUT			= DEBUG
PLATFORM		= WINDOWS
CC 				= clang

CFLAGS			= -Wall -v -pedantic -std=c11 -shared -DDLL_EXPORT -Iinclude
LIBS			=   -lsetupapi						\
					-lhid							\
					-lcfgmgr32

ifeq ($(OUTPUT), DEBUG)
	CFLAGS += -g -O0
	CFLAGS += -DRHID_DEBUG_ENABLED
#	CFLAGS += -DDEBUG_TIME
endif

SRC			   := $(wildcard src/*.c)
OBJ			   := $(patsubst src/%.c,obj/%.o,$(SRC))

.PHONY: clean

$(NAME): $(NAME)

include test/test.mk

all: $(NAME) test

$(NAME): $(OBJ)
	-mkdir bin
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o bin/$(NAME).dll

obj/%.o: src/%.c
	-mkdir obj
	$(CC) $(CFLAGS) -DINPT_VERSION_MAJOR=$(VERSION_MAJOR) -DINPT_VERSION_MINOR=$(VERSION_MINOR) -D$(PLATFORM) -c $< -o $@

clean:
	-powershell -c 'rm obj/*.o; rm bin/*'