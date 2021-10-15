#include "debug.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

struct debug_t debug = {0};

long debug_time_now() {
    struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);

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
