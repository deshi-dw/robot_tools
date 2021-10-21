#include "inpt.h"

#include "debug.h"
#include "rhid.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>

// TODO Test all these fffffffuuuuuuuunctions.

static struct inpt_t inpt = {0};

// Hash function from http://www.cse.yorku.ca/~oz/hash.html
static unsigned long inpt_hash(char* str) {
	unsigned long hash = 5381;
	int			  c;

	while((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

LIBINPT const char* inpt_version() {
	// C macros are dumb so macros are stringified before they are evaulated.
	// To solve this, you can use another macro function to stringify and then
	// concatinate.
	// This turns "INPT_VERSION_MAJOR" "."  "INPT_VERSION_MINOR"
	// into "0" "." "1" or whatever the numbers are.
	// That is why all this garbadge is here.
#define STREXPAND(x) #x
#define VER(major, minor) STREXPAND(major) "." STREXPAND(minor)

#ifdef INPT_VERSION_MAJOR
#ifdef INPT_VERSION_MINOR
	return VER(INPT_VERSION_MAJOR, INPT_VERSION_MINOR);
#endif
#endif

#undef STREXPAND
#undef VER

	return "no version";
}

LIBINPT int inpt_start() {
	inpt.is_running = 1;

	while(inpt.is_running) {
		inpt_update();
	}

	return 0;
}

LIBINPT int inpt_stop() {
	inpt.is_running = 0;

	return 0;
}

// TODO move nanosleep function somewhere else (ideally it's own c file)
#ifdef WINDOWS
// Code from https://github.com/coreutils/gnulib/blob/master/lib/nanosleep.c

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* The Windows API function Sleep() has a resolution of about 15 ms and takes
   at least 5 ms to execute.  We use this function for longer time periods.
   Additionally, we use busy-looping over short time periods, to get a
   resolution of about 0.01 ms.  In order to measure such short timespans,
   we use the QueryPerformanceCounter() function.  */

enum { BILLION = 1000 * 1000 * 1000 };

int nanosleep(const struct timespec* requested_delay,
			  struct timespec*		 remaining_delay) {
	static int initialized;
	/* Number of performance counter increments per nanosecond,
	   or zero if it could not be determined.  */
	static double ticks_per_nanosecond;

	if(requested_delay->tv_nsec < 0 || BILLION <= requested_delay->tv_nsec) {
		errno = EINVAL;
		return -1;
	}

	/* For requested delays of one second or more, 15ms resolution is
	   sufficient.  */
	if(requested_delay->tv_sec == 0) {
		if(! initialized) {
			/* Initialize ticks_per_nanosecond.  */
			LARGE_INTEGER ticks_per_second;

			if(QueryPerformanceFrequency(&ticks_per_second))
				ticks_per_nanosecond =
					(double) ticks_per_second.QuadPart / 1000000000.0;

			initialized = 1;
		}
		if(ticks_per_nanosecond) {
			/* QueryPerformanceFrequency worked.  We can use
			   QueryPerformanceCounter.  Use a combination of Sleep and
			   busy-looping.  */
			/* Number of milliseconds to pass to the Sleep function.
			   Since Sleep can take up to 8 ms less or 8 ms more than requested
			   (or maybe more if the system is loaded), we subtract 10 ms.  */
			int sleep_millis = (int) requested_delay->tv_nsec / 1000000 - 10;
			/* Determine how many ticks to delay.  */
			LONGLONG wait_ticks =
				requested_delay->tv_nsec * ticks_per_nanosecond;
			/* Start.  */
			LARGE_INTEGER counter_before;
			if(QueryPerformanceCounter(&counter_before)) {
				/* Wait until the performance counter has reached this value.
				   We don't need to worry about overflow, because the
				   performance counter is reset at reboot, and with a frequency
				   of 3.6E6 ticks per second 63 bits suffice for over 80000
				   years.  */
				LONGLONG wait_until = counter_before.QuadPart + wait_ticks;
				/* Use Sleep for the longest part.  */
				if(sleep_millis > 0)
					Sleep(sleep_millis);
				/* Busy-loop for the rest.  */
				for(;;) {
					LARGE_INTEGER counter_after;
					if(! QueryPerformanceCounter(&counter_after))
						/* QueryPerformanceCounter failed, but succeeded
						   earlier. Should not happen.  */
						break;
					if(counter_after.QuadPart >= wait_until)
						/* The requested time has elapsed.  */
						break;
				}
				goto done;
			}
		}
	}
	/* Implementation for long delays and as fallback.  */
	Sleep(requested_delay->tv_sec * 1000 + requested_delay->tv_nsec / 1000000);

done:
	/* Sleep is not interruptible.  So there is no remaining delay.  */
	if(remaining_delay != NULL) {
		remaining_delay->tv_sec	 = 0;
		remaining_delay->tv_nsec = 0;
	}
	return 0;
}
#endif

LIBINPT int inpt_update() {
	// Update device list.
	// TODO Since this might be expensive, consider doing ever x number of
	// updates instead.
	//		Better yet, do it on a timer.

	// FIXME To continue the above TODO, doing rhid_get_devices will overwrite
	// the previous devices.
	//		 this causes any opened devices to become closed again. (NOT GOOD.)

	if(inpt.dev_selected == NULL) {
		inpt.dev_count = rhid_get_device_count();
		if(inpt.dev_count >= MAX_DEV_COUNT) {
			// TODO Consider erroring out here.
			inpt.dev_count = MAX_DEV_COUNT;
		}

		// TODO do a proper fix. but for now have this fix. What puts a wrench
		// in doing a better fix immediatly
		//		is rhid's incapability to do disconnection detection... so...
		// implement that please.

		// TODO create a dedicated function to get connected devices and only
		// call it when the currently connected device is disconnected.

		// TEMP FIX:
		rhid_device_t tmp_devs[MAX_DEV_COUNT];
		rhid_get_devices(tmp_devs, inpt.dev_count);
		for(int i = 0; i < inpt.dev_count; i++) {
			if(inpt.devs[i].is_open) {
				continue;
			}

			inpt.devs[i] = tmp_devs[i];

			inpt.dev_names[i] = rhid_get_product_name(&inpt.devs[i]);
		}
	}

	// Update HID values.
	if(inpt.dev_selected != NULL) {
		DEBUG_TIME_START("updating HID values");
		DEBUG_TIME_START("HID report");
		rhid_report(inpt.dev_selected, 0);
		DEBUG_TIME_STOP();

		DEBUG_TIME_START("state copy");
		rhid_get_buttons_state(inpt.dev_selected, inpt.hid.btns,
							   inpt.hid.btn_count);
		rhid_get_values_state(inpt.dev_selected, inpt.hid.vals,
							  inpt.hid.val_count);
		DEBUG_TIME_STOP();
		for(int i = 0; i < inpt.hid.btn_count; i++) {
			enum inpt_btn_state_t new_state =
				inpt.hid.btns[i] == 1 && inpt.hid_prev.btns[i] == 0
					? INPT_BTN_PRESSED
				: inpt.hid.btns[i] == 1 && inpt.hid_prev.btns[i] == 1
					? INPT_BTN_HELD
				: inpt.hid.btns[i] == 0 && inpt.hid_prev.btns[i] == 1
					? INPT_BTN_RELEASED
					: 0;

			if(new_state == inpt.btn_states[i]) {
				continue;
			}

			inpt.btn_states[i] = new_state;

			for(int j = 0; j < MAX_HID_BTN_EVENTS; j++) {
				if(inpt.on_hid_btns[j] == NULL) {
					continue;
				}

				inpt.on_hid_btns[j](i, inpt.btn_states[i]);
			}
		}

		for(int i = 0; i < inpt.hid.val_count; i++) {
			if(inpt.hid.vals[i] == inpt.hid_prev.vals[i]) {
				continue;
			}

			for(int j = 0; j < MAX_HID_VAL_EVENTS; j++) {
				if(inpt.on_hid_vals[j] == NULL) {
					continue;
				}

				inpt.on_hid_vals[j](i, inpt.hid.vals[i]);
			}
		}

		DEBUG_TIME_STOP();
	}

	// Update actions.
	DEBUG_TIME_START("updating actions");

	for(int i = 0; i < ACTION_COUNT; i++) {
		const inpt_act_t* action = &inpt.actions[i];

		// Check to see if the action can run in the current state.
		if(action == NULL || ! (BITFLD_GET(action->states, inpt.state_index))) {
			continue;
		}

		// Don't proccess actions that have an input mod but it isn't
		// pressed.
		if(action->input_mod != -1 &&
		   inpt.btn_states[action->input_mod] != INPT_BTN_HELD) {
			continue;
		}

		switch(action->type) {
			case INPT_ACT_STATE_CHANGE: // STATE CHANGE
				if((inpt.btn_states[action->input] & action->flags) == 0) {
					break;
				}

				// Don't update the state if the button value hasn't changed.
				if(inpt.btn_states[action->input] ==
				   inpt.btn_states_prev[action->input]) {
					break;
				}

				// Get the new state index as we switch states.
				for(int i = 0; i < STATE_COUNT; i++) {
					if(inpt.states[i] != action->new_state) {
						continue;
					}

					printf("state changed %ld -> %ld\n",
						   inpt.states[inpt.state_index], inpt.states[i]);

					for(int j = 0; j < MAX_ACT_STATE_CHANGE_EVENTS; j++) {
						if(inpt.on_act_state_changes[j] == NULL) {
							continue;
						}

						inpt.on_act_state_changes[j](
							inpt.states[inpt.state_index], action->new_state);
					}

					// TODO I doubt this is nessesary but it would probably
					// be more correct to queue the change in state so other
					// actions in the array aren't affected
					// until next cycle.
					inpt.state_index = i;

					break;
				}

				break;

			case INPT_ACT_TRIGGER: // TRIGGER
								   // TODO add action->input_mod
				if((inpt.btn_states[action->input] & action->flags) == 0) {
					break;
				}

				// Don't update the action if the button value hasn't changed.
				if(inpt.btn_states[action->input] != INPT_BTN_HELD &&
				   inpt.btn_states[action->input] ==
					   inpt.btn_states_prev[action->input]) {
					break;
				}

				printf("triggered action %s\n", action->name);

				for(int j = 0; j < MAX_ACT_TRIGGER_EVENTS; j++) {
					if(inpt.on_act_triggers[j].event == NULL) {
						continue;
					}

					inpt.on_act_triggers[j].event(
						inpt.btn_states[action->input]);
				}
				break;

			case INPT_ACT_VALUE: // VALUE
				// Don't update the action if the value's value hasn't changed.
				if(inpt.hid.vals[action->input] ==
				   inpt.hid_prev.vals[action->input]) {
					break;
				}

				printf("value action %s set to %d\n", action->name,
					   inpt.hid.vals[action->input]);

				for(int j = 0; j < MAX_ACT_VALUE_EVENTS; j++) {
					if(inpt.on_act_values[j].event == NULL) {
						continue;
					}

					inpt.on_act_values[j].event(inpt.hid.vals[action->input]);
				}
				break;

			default:
				fprintf(stderr,
						"inpt tried to call action '%s' but the action didn't "
						"have a type.\n",
						action->name);
		}
	}
	DEBUG_TIME_STOP();

	// copy current hid data over to the previous hid data in preperation for
	// the next cycle.
	memcpy(&inpt.hid_prev, &inpt.hid, sizeof(inpt_hid_t));
	memcpy(&inpt.btn_states_prev, &inpt.btn_states, sizeof(inpt.btn_states));

	return 0;
}

/**
 * @brief Add a state to the input mapper.
 *
 * @param state the name of the state to add.
 * @return int 0 on success and -1 on failure.
 */
LIBINPT int inpt_state_add(char* state) {
	for(int i = 0; i < STATE_COUNT; i++) {
		if(inpt.states[i] == 0) {
			inpt.states[i] = inpt_hash(state);
			return 0;
		}
	}

	return -1;
}

LIBINPT int inpt_state_del(char* state) {
	unsigned long hash = inpt_hash(state);

	for(int i = 0; i < STATE_COUNT; i++) {
		if(inpt.states[i] == hash) {
			inpt.states[i] = 0;
			return 0;
		}
	}

	return -1;
}

LIBINPT int inpt_state_set(char* state, char* new_state) {
	unsigned long hash = inpt_hash(state);

	for(int i = 0; i < STATE_COUNT; i++) {
		if(inpt.states[i] == hash) {
			inpt.states[i] = inpt_hash(new_state);
			return 0;
		}
	}

	return -1;
}

LIBINPT inpt_act_t* inpt_act_new_state_change(char* name, char** states,
											  int state_count, int input_mod,
											  int input, char* newstate,
											  int flags) {
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i].name == NULL) {
			// set name.
			inpt_act_set_name(&inpt.actions[i], name);
			inpt_act_set_type(&inpt.actions[i], INPT_ACT_STATE_CHANGE);

			// add states.
			for(int j = 0; j < state_count; j++) {
				inpt_act_add_state(&inpt.actions[i], states[j]);
			}

			// set input mod and input.
			inpt_act_set_input_mod(&inpt.actions[i], input_mod);
			inpt_act_set_input(&inpt.actions[i], input);

			// set state change state.
			// TODO add function to set new_state.
			inpt.actions[i].new_state = inpt_hash(newstate);

			// set input flags.
			inpt_act_set_flags(&inpt.actions[i], flags);

			return &inpt.actions[i];
		}
	}

	return NULL;
}

LIBINPT inpt_act_t* inpt_act_new_trigger(char* name, char** states,
										 int state_count, int input_mod,
										 int input, int flags) {
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i].name == NULL) {
			// set name.
			inpt_act_set_name(&inpt.actions[i], name);
			inpt_act_set_type(&inpt.actions[i], INPT_ACT_TRIGGER);

			// add states.
			for(int j = 0; j < state_count; j++) {
				inpt_act_add_state(&inpt.actions[i], states[j]);
			}

			// set input mod and input.
			inpt_act_set_input_mod(&inpt.actions[i], input_mod);
			inpt_act_set_input(&inpt.actions[i], input);

			// set input flags.
			inpt_act_set_flags(&inpt.actions[i], flags);

			return &inpt.actions[i];
		}
	}

	return NULL;
}

LIBINPT inpt_act_t* inpt_act_new_value(char* name, char** states,
									   int state_count, int input_mod,
									   int input, int flags) {
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i].name == NULL) {
			// set name.
			inpt_act_set_name(&inpt.actions[i], name);
			inpt_act_set_type(&inpt.actions[i], INPT_ACT_VALUE);

			// add states.
			for(int j = 0; j < state_count; j++) {
				inpt_act_add_state(&inpt.actions[i], states[j]);
			}

			// set input mod and input.
			inpt_act_set_input_mod(&inpt.actions[i], input_mod);
			inpt_act_set_input(&inpt.actions[i], input);

			// set input flags.
			inpt_act_set_flags(&inpt.actions[i], flags);

			return &inpt.actions[i];
		}
	}

	return NULL;
}

LIBINPT int inpt_act_del(char* name) {
	long hash = inpt_hash(name);
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i].name_hash == hash) {
			memset(&inpt.actions[i], 0, sizeof(inpt_act_t));
			return 0;
		}
	}

	return -1;
}

inpt_act_t* inpt_act_get(char* name) {
	unsigned long hash = inpt_hash(name);

	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i].name_hash == hash) {
			return &inpt.actions[i];
		}
	}

	return NULL;
}

LIBINPT int inpt_act_set_name(inpt_act_t* action, char* name) {
	action->name	  = name;
	action->name_hash = inpt_hash(name);
	return 0;
}

LIBINPT int inpt_act_set_type(inpt_act_t* action, int type) {
	action->type = type;
	return 0;
}

LIBINPT int inpt_act_set_input_mod(inpt_act_t* action, int input_mod) {
	action->input_mod = input_mod;
	return 0;
}

LIBINPT int inpt_act_set_input(inpt_act_t* action, int input) {
	action->input = input;
	return 0;
}

LIBINPT int inpt_act_set_flags(inpt_act_t* action, int flags) {
	action->flags = flags;
	return 0;
}

LIBINPT int inpt_act_set_point(inpt_act_t* action, int index, int x, int y) {
	if(index >= action->point_count || index >= INPT_ACT_POINT_COUNT) {
		return -1;
	}

	// TODO Make the maximum x value something that makes sense.
	//      Currently the 255 in the line below is used to set the maximum x
	//      value of the graph. 255 is an abitrary number. Change this to
	//      something more sensible.

	// TODO Change 255 to 1.0 if points change from integers to a decimal value
	// type.

	// Make sure that the first point and the last point stay at either end of
	// the curve. AKA the first point is always at x = 0 and the last point is
	// always at x = 1.
	action->points[index].x = index == 0						 ? 0
							  : index == action->point_count - 1 ? 255
																 : x;

	// TODO Clamp y to a minimum and maximum value.
	action->points[index].y = y;

	return 0;
}

LIBINPT int inpt_act_add_point(inpt_act_t* action, int x, int y) {
	if(action->point_count >= INPT_ACT_POINT_COUNT) {
		return -1;
	}

	// Insert point based on its x value. This keeps the points array in a
	// sorted order.
	for(int i = 0; i < action->point_count; i++) {
		if(action->points[i].x < x) {
			continue;
		}

		// Shift all the points over by one to make room for the new point.
		memmove(action->points + i + 1, action->points + i,
				action->point_count - i);

		action->points[i] = (struct point_t){x, y};
		action->point_count++;

		return 0;
	}

	// There was no place for the point to fit into which is strange considering
	// the last point should always be after all other points so if we get here,
	// something is seriously fucked up.
	return -1;
}

LIBINPT int inpt_act_del_point(inpt_act_t* action, int index) {
	if(index >= action->point_count || index >= INPT_ACT_POINT_COUNT) {
		return -1;
	}

	// Do not allow the deletion of the first or last points because that would
	// be stupid. The first point should always be x = 0 and the last point
	// should always be x = 1. Of course, we could make a system that assumes
	// there is always a point at 0 and 1 and that might be a smarter system but
	// who cares.

	// TODO Make the points array assume that there is a point at x = 0 and
	// another at x = 1 without them being explicitly in the array.
	if(index == 0 || index == action->point_count - 1) {
		return -1;
	}

	// Move the end of the points array overtop of the item at index,
	// effectively overwriting it, a`la total annihilation. Horray.
	memmove(action->points + index - 1, action->points + index,
			action->point_count - index);

	return 0;
}

LIBINPT int inpt_act_add_state(inpt_act_t* action, char* state) {
	unsigned long hash = inpt_hash(state);

	// If the state string is "all" or the more novel "ALL!!!!", enable all of
	// the states. Again if this was a bitfield instead we could just use a
	// memset and be good to go.
	if(hash == inpt_hash("all") || hash == inpt_hash("ALL")) {
		// for(int i = 0; i < STATE_COUNT; i++) {
		//     action->states[i] = inpt.states[i];
		// }

		memset(action->states, 1, sizeof(action->states));

		return 0;
	}

	for(int i = 0; i < STATE_COUNT; i++) {
		if(inpt.states[i] != hash) {
			continue;
		}

		BITFLD_SET(action->states, i);
		return 0;
	}

	return -1;
}

LIBINPT int inpt_act_del_state(inpt_act_t* action, char* state) {
	unsigned long hash = inpt_hash(state);

	// If state is one of all of the Alls, set everything to zero.
	// See, we are being clever because all is usually used to allow everthing
	// but if you delete all you have nothing. It isn't that clever actually...
	if(hash == inpt_hash("all") || hash == inpt_hash("ALL")) {
		memset(action->states, 0, sizeof(action->states));

		return 0;
	}

	// Use search and destroy technology to find that hash and delete it.
	for(int i = 0; i < STATE_COUNT; i++) {
		// if(action->states[i] != hash) {
		//     continue;
		// }

		// action->states[i] = 0;
		// return 0;

		if(inpt.states[i] != hash) {
			continue;
		}

		BITFLD_CLR(action->states, i);
		return 0;
	}

	return -1;
}

// TODO Listeners are difficult so I empore you to do them later.
LIBINPT int inpt_act_on_state_change(inpt_act_t*				  action,
									 inpt_act_state_change_evnt_t event) {
	for(int i = 0; i < MAX_ACT_STATE_CHANGE_EVENTS; i++) {
		if(inpt.on_act_state_changes[i] != NULL) {
			continue;
		}

		inpt.on_act_state_changes[i] = event;
	}
	return 0;
}
LIBINPT int inpt_act_on_trigger(inpt_act_t*				action,
								inpt_act_trigger_evnt_t event) {
	for(int i = 0; i < MAX_ACT_TRIGGER_EVENTS; i++) {
		if(inpt.on_act_triggers[i].event != NULL) {
			continue;
		}

		inpt.on_act_triggers[i].event  = event;
		inpt.on_act_triggers[i].action = action;
	}

	return 0;
}
LIBINPT int inpt_act_on_value(inpt_act_t* action, inpt_act_value_evnt_t event) {
	for(int i = 0; i < MAX_ACT_VALUE_EVENTS; i++) {
		if(inpt.on_act_values[i].event != NULL) {
			continue;
		}

		inpt.on_act_values[i].event	 = event;
		inpt.on_act_values[i].action = action;
	}

	return 0;
}

static int inpt_hid_open_and_select(rhid_device_t* device) {
	inpt.dev_selected = device;
	if(rhid_open(inpt.dev_selected) < 0) {
		return -1;
	}

	inpt.hid.btn_count = rhid_get_button_count(inpt.dev_selected);
	inpt.hid.val_count = rhid_get_value_count(inpt.dev_selected);

	// Make hid_prev the same as the hid so we can do comparisons latter.
	memcpy(&inpt.hid_prev, &inpt.hid, sizeof(inpt_hid_t));

	return 0;
}

static int inpt_hid_update_connected() {
	int vid = 0;
	int pid = 0;
	if(inpt.dev_selected != NULL) {
		vid = inpt.dev_selected->vendor_id;
		pid = inpt.dev_selected->product_id;
		rhid_close(inpt.dev_selected);
	}

	inpt.dev_count = rhid_get_device_count();
	if(inpt.dev_count >= MAX_DEV_COUNT) {
		inpt.dev_count = MAX_DEV_COUNT;
	}

	for(int i = 0; i < inpt.dev_count; i++) {
		if(rhid_get_vendor_id(&inpt.devs[i]) == vid &&
		   rhid_get_product_id(&inpt.devs[i]) == pid) {
			inpt_hid_open_and_select(&inpt.devs[i]);

			break;
		}

		inpt.dev_names[i]	= rhid_get_product_name(&inpt.devs[i]);
		inpt.dev_ids[i].vid = rhid_get_vendor_id(&inpt.devs[i]);
		inpt.dev_ids[i].vid = rhid_get_product_id(&inpt.devs[i]);
	}

	return 0;
}

LIBINPT int inpt_hid_count() {
	int real_count = rhid_get_device_count();
	return real_count >= MAX_DEV_COUNT ? MAX_DEV_COUNT : real_count;
}

LIBINPT inpt_hid_id_t* inpt_hid_list() {
	inpt_hid_update_connected();

	if(inpt.dev_names[0] == NULL) {
		return NULL;
	}

	return (inpt_hid_id_t*) inpt.dev_ids;
}

LIBINPT char** inpt_hid_list_names() {
	inpt_hid_update_connected();

	if(inpt.dev_names[0] == NULL) {
		return NULL;
	}

	return (char**) inpt.dev_names;
}

LIBINPT int inpt_hid_select(int vid, int pid) {
	if(inpt.dev_selected != NULL) {
		rhid_close(inpt.dev_selected);
		inpt.dev_selected = NULL;
	}

	for(int i = 0; i < inpt.dev_count; i++) {
		if(rhid_get_vendor_id(&inpt.devs[i]) == vid &&
		   rhid_get_product_id(&inpt.devs[i]) == pid) {
			break;
		}

		inpt_hid_open_and_select(&inpt.devs[i]);
		return 0;
	}

	return -1;
}

LIBINPT int inpt_hid_is_conn() {
	// TODO Find a better way to do connection check.
	for(int i = 0; i < inpt.dev_count; i++) {
		if(inpt.devs[i].product_id == inpt.dev_selected->product_id &&
		   inpt.devs[i].vendor_id == inpt.dev_selected->vendor_id) {
			return 1;
		}
	}

	return 0;
}

LIBINPT int inpt_hid_on_btn(inpt_hid_btn_evnt_t event) {
	for(int i = 0; i < MAX_HID_BTN_EVENTS; i++) {
		if(inpt.on_hid_btns[i] != NULL) {
			continue;
		}

		inpt.on_hid_btns[i] = event;
		return 0;
	}

	return -1;
}
LIBINPT int inpt_hid_on_val(inpt_hid_val_evnt_t event) {
	for(int i = 0; i < MAX_HID_VAL_EVENTS; i++) {
		if(inpt.on_hid_vals[i] != NULL) {
			continue;
		}

		inpt.on_hid_vals[i] = event;
		return 0;
	}

	return -1;
}
