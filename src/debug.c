#include "debug.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct debug_t debug = {0};

#ifdef WINDOWS

#include <windows.h>

#define BILLION (1E9)

static BOOL			 g_first_time = 1;
static LARGE_INTEGER g_counts_per_sec;

int clock_gettime(int dummy, struct timespec* ct) {
	LARGE_INTEGER count;

	if(g_first_time) {
		g_first_time = 0;

		if(0 == QueryPerformanceFrequency(&g_counts_per_sec)) {
			g_counts_per_sec.QuadPart = 0;
		}
	}

	if((NULL == ct) || (g_counts_per_sec.QuadPart <= 0) ||
	   (0 == QueryPerformanceCounter(&count))) {
		return -1;
	}

	ct->tv_sec	= count.QuadPart / g_counts_per_sec.QuadPart;
	ct->tv_nsec = ((count.QuadPart % g_counts_per_sec.QuadPart) * BILLION) /
				  g_counts_per_sec.QuadPart;

	return 0;
}

// #define CLOCK_REALTIME 0

// struct timespec { long tv_sec; long tv_nsec; };

#else
#include <time.h>
#endif

long debug_time_now() {
	struct timespec tp;
	clock_gettime(0, &tp);

	return tp.tv_nsec / 1000000 + tp.tv_sec * 1000;
}

int debug_time_lvl() {
	return debug.time.level;
}

void debug_time_lvl_next() {
	debug.time.level++;
}

void debug_time_lvl_prev() {
	debug.time.level--;
}

void debug_time_push(const char* msg) {
	for(int i = 0; i < DEBUG_TIME_MESSAGE_MAX; i++) {
		if(debug.time.messages[debug.time.level][i][0] != '\0') {
			continue;
		}

		for(int j = 0; j < debug.time.level; j++) {
			debug.time.messages[debug.time.level][i][j] = '\t';
		}

		memcpy(debug.time.messages[debug.time.level][i] + debug.time.level, msg,
			   strlen(msg));
		break;
	}
}

const char* debug_time_pop(int level) {
	for(int i = 0; i < DEBUG_TIME_MESSAGE_MAX; i++) {
		if(debug.time.messages[level][i][0] == '\0') {
			continue;
		}

		memcpy(debug.time.messge_popped, debug.time.messages[level][i],
			   DEBUG_TIME_MESSAGE_LENGTH_MAX);
		memset(debug.time.messages[level][i], '\0',
			   DEBUG_TIME_MESSAGE_LENGTH_MAX);
		return debug.time.messge_popped;
	}

	return NULL;
}

void debug_time_set_last_ms(double ms) {
	debug.time.last_ms = ms;
}
double debug_time_last_ms() {
	return debug.time.last_ms;
}
