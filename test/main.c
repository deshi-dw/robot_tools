#include <stdio.h>
// #include <unistd.h>
#include <stdlib.h>
// #include <pthread.h>

#include "debug.h"
#include "inpt.h"

void on_button(int idx, int flags);
void on_value(int idx, int amount);

int main(int arc, char** argv) {
	printf("inpt version %s\n", inpt_version());

	inpt_state_add("drive");
	inpt_state_add("drive_lock");
	inpt_state_add("shoot");

	inpt_act_new_state_change("drive_to_shoot", (char*[]){"drive", "shoot"}, 2,
							  -1, 7, "shoot", INPT_BTN_PRESSED);
	inpt_act_new_state_change("shoot_to_drive", (char*[]){"drive", "shoot"}, 2,
							  -1, 7, "drive", INPT_BTN_RELEASED);

	inpt_act_new_trigger("shoot", (char*[]){"shoot"}, 1, -1, 2,
						 INPT_BTN_RELEASED);

	inpt_act_new_trigger("drive_forwards", (char*[]){"drive"}, 1, -1, 5,
						 INPT_BTN_HELD);

	inpt_act_new_trigger("drive_backwards", (char*[]){"drive"}, 1, -1, 4,
						 INPT_BTN_HELD);

	DEBUG_TIME_START("inpt_update");
	inpt_update();
	DEBUG_TIME_STOP();

	// pthread_t inpt_thread = {0};
	// pthread_create(&inpt_thread, NULL, (void*(*)(void*))inpt_start, NULL);

	// printf("device count: %i\n", inpt_hid_count());
	int			   count = inpt_hid_count();
	char**		   names = inpt_hid_list_names();
	inpt_hid_id_t* ids	 = inpt_hid_list();
	puts("select a device:");
	for(int i = 0; i < count; i++) {
		printf("\t%i: %s", i, names[i]);
	}
	puts("\n");

	char cin[256];
	gets(cin);
	int selection = atoi(cin);

	if(selection > count) {
		fprintf(stderr, "invalid device selected.\n");
		return -1;
	}

	if(inpt_hid_select(ids[selection].vid, ids[selection].pid) < 0) {
		exit(-1);
	}

	inpt_hid_on_btn(on_button);
	inpt_hid_on_val(on_value);

	while(1) {
		DEBUG_TIME_START("inpt_update");
		inpt_update();
		DEBUG_TIME_STOP();
		if(debug_time_last_ms() > 100.0) {
			exit(0);
		}
		// puts("\n");
	}

	return 0;
}

void on_button(int idx, int flags) {
	// printf("button[%i] set to %i\n", idx, flags);
}

void on_value(int idx, int amount) {
	// printf("value[%i] set to %d\n", idx, amount);
}
