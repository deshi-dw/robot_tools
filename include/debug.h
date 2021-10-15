#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <string.h>
#include <time.h>

// TODO Add a "Level" system. Maybe a static list of static lists and an int for the level of timer.
//      Then, we could print the levels as such:
//      some_func_at_lvl1(arg1, arg2)               took 100 ms
//          some_iternal_func(args)                     took 25 ms
//          some_iternal_func2(args)                    took 25 ms
//              some_further_internal_func(args)            took 25 ms
//          some_internal_func3(args)                   took 25 ms

/*
DEBUG_TIME(name, a)
    debug.level++;

    ...

    debug.level--;
    foreach(debug.time_message[debug.level + 1]) {
        puts(debug.time_message[debug.level + 1][index]);
    }

*/

struct {
	int	 max_name_size;
	int	 cur_name_size;
	char name_spacing_buffer[128];

	double last_ms;
} debug = {0};

#define DEBUG_TIME(a) DEBUG_TIME2(#a, a);

#define DEBUG_TIME2(name, a)                                      \
	{                                                             \
		long start = clock();                                     \
		a;                                                        \
		long   diff = clock() - start;                            \
		double ms	= (double) diff * 1000 / CLOCKS_PER_SEC;      \
		if(strlen(name) > debug.max_name_size) {                  \
			debug.max_name_size = strlen(name);                   \
		}                                                         \
		debug.cur_name_size = debug.max_name_size - strlen(name); \
		for(int i = 0; i < debug.cur_name_size; i++) {            \
			debug.name_spacing_buffer[i] = ' ';                   \
		}                                                         \
		debug.name_spacing_buffer[debug.cur_name_size] = '\0';    \
		printf(name "%s took %10.2f milliseconds\n",              \
			   debug.name_spacing_buffer, ms);                    \
		debug.last_ms = ms;                                       \
	}

#endif
