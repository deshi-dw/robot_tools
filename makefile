NAME 			= libinpt
VERSION_MAJOR 	= 0
VERSION_MINOR 	= 1
OUTPUT			= DEBUG
CC 				= clang

CFLAGS			= -Wall -pedantic -std=c11 -shared -target x86_64-pc-windows-gnu -DDLL_EXPORT -Iinclude -Llib
LIBS			= 	-lm -lpthread					\
					-lsetupapi						\
					-lhid							\
					-lhidparse						\
					-lcfgmgr32

ifeq ($(OUTPUT), DEBUG)
	CFLAGS += -g -ggdb -O0
endif

SRC			   := $(wildcard src/*.c)
OBJ			   := $(patsubst src/%.c,obj/%.o,$(SRC))

.PHONY: clean

ALL: $(NAME)

include test/test.mk

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o bin/$(NAME).dll

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -DINPT_VERSION_MAJOR=$(VERSION_MAJOR) -DINPT_VERSION_MINOR=$(VERSION_MINOR) -c $< -o $@

clean:
	powershell -c 'rm obj/*.o; rm bin/*'