#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct debug_t {
	int	 max_name_size;
	int	 cur_name_size;
	char name_spacing_buffer[128];

	struct debug_time_t {
#define DEBUG_TIME_MESSAGE_LEVELS 16
#define DEBUG_TIME_MESSAGE_MAX 128
#define DEBUG_TIME_MESSAGE_LENGTH_MAX 256
		int	 level;
		char messages[DEBUG_TIME_MESSAGE_LEVELS][DEBUG_TIME_MESSAGE_MAX]
					 [DEBUG_TIME_MESSAGE_LENGTH_MAX];
		const char messge_popped[DEBUG_TIME_MESSAGE_LENGTH_MAX];

		double last_ms;
	} time;

	double last_ms;
};

extern struct debug_t debug;

long debug_time_now();

int	 debug_time_lvl();
void debug_time_lvl_next();
void debug_time_lvl_prev();

void		debug_time_push(const char* msg);
const char* debug_time_pop(int level);

void   debug_time_set_last_ms(double ms);
double debug_time_last_ms();

#define DEBUG_PRINT_TIME(msg, time) printf("%s took %d ms\n", msg, time);

#define DEBUG_TIME_START(name) \
	{                          \
		debug_time_push(name); \
		debug_time_lvl_next(); \
		long dbgt_start = debug_time_now();

#define DEBUG_TIME_STOP()                                                   \
	long dbgt_diff = debug_time_now() - dbgt_start;                         \
	/*double dbgt_ms	 = (double) dbgt_diff;*/                               \
	long dbgt_ms = dbgt_diff;                                               \
	for(const char* msg = debug_time_pop(debug_time_lvl()); msg != NULL;) { \
		DEBUG_PRINT_TIME(msg, dbgt_ms);                                     \
	}                                                                       \
	DEBUG_PRINT_TIME(debug_time_pop(debug_time_lvl() - 1), dbgt_ms);        \
	debug_time_lvl_prev();                                                  \
	debug_time_set_last_ms(dbgt_ms);                                        \
	}

#endif
