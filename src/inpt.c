#include "inpt.h"

#include "debug.h"
#include "rhid.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// TODO Thread the program and then unthread it.
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

		// FIXME Calling rhid_get_devices causes massive slowdown every so often
		// (I THINK). Probably do
		//		 to accessing various database-like structures in order to
		// generate all the info needed. 		 None-the-less, move
		// rhid_get_devices out of the update cycle, don't just reduce the
		// amount of times it is called because even if it is only called a
		// couple of times, it could 		 still freeze the update cycle. This
		// goes
		// back to needing a proper way to detect when a 		 is removed.

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
		rhid_report_buttons(inpt.dev_selected, 0);
		// FIXME If there is not a slight delay here, the application will
		// 		 freeze up for 5 seconds, seemingly with no decernable reason.
		usleep(1000 * 1);
		rhid_report_values(inpt.dev_selected, 0);
		DEBUG_TIME_STOP();

		DEBUG_TIME_START("state copy");
		rhid_get_buttons_state(inpt.dev_selected, inpt.hid.btns,
							   inpt.hid.btn_count);
		rhid_get_values_state(inpt.dev_selected, inpt.hid.vals,
							  inpt.hid.val_count);
		DEBUG_TIME_STOP();
		for(int i = 0; i < inpt.hid.btn_count; i++) {
			if(inpt.hid.btns[i] == inpt.hid_prev.btns[i]) {
				continue;
			}

			// TODO Do on button update / change / trigger or whatever.

			for(int j = 0; j < MAX_HID_BTN_EVENTS; j++) {
				if(inpt.on_hid_btns[j] == NULL) {
					continue;
				}

				inpt.on_hid_btns[j](i, inpt.hid.btns[i]);
			}
		}

		for(int i = 0; i < inpt.hid.val_count; i++) {
			if(inpt.hid.vals[i] == inpt.hid_prev.vals[i]) {
				continue;
			}

			// TODO Do on value update.
			for(int j = 0; j < MAX_HID_VAL_EVENTS; j++) {
				if(inpt.on_hid_vals[j] == NULL) {
					continue;
				}

				inpt.on_hid_vals[j](i, inpt.hid.vals[i]);
			}
		}

		memcpy(&inpt.hid_prev, &inpt.hid, sizeof(inpt_hid_t));
		DEBUG_TIME_STOP();
	}

	// Update actions.
	DEBUG_TIME_START("updating actions");

	for(int i = 0; i < ACTION_COUNT; i++) {
		inpt_act_t* action = inpt.actions[i];

		// Check to see if the action can run in the current state.
		if(action == NULL || ! (BITFLD_GET(action->states, inpt.state_index))) {
			continue;
		}

		// TODO Refactor.

		// Don't proccess actions that have an input mod but it isn't
		// pressed.
		if(action->input_mod != -1 && inpt.hid.btns[action->input_mod] == 0) {
			continue;
		}

		switch(action->type) {
			case 0: // STATE CHANGE
				if(inpt.hid.btns[action->input] != 1) {
					break;
				}

				// Get the new state index as we switch states.
				for(int i = 0; i < STATE_COUNT; i++) {
					if(inpt.states[i] != action->new_state) {
						continue;
					}

					// TODO I doubt this is nessesary but it would probably
					// be more correct to queue the change in state so other
					// actions in the array aren't affected
					// until next cycle.
					inpt.state_index = i;
					break;
				}

				break;

			case 1: // TRIGGER
				if(inpt.hid.btns[action->input] != 1) {
					break;
				}

				// TODO Call the trigger action.
				break;

			case 2: // VALUE
				// TODO Call the value.
				break;
		}
	}
	DEBUG_TIME_STOP();

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

LIBINPT int inpt_act_add(inpt_act_t* action, char* name) {
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i] == 0) {
			action->name	  = name;
			action->name_hash = inpt_hash(name);
			inpt.actions[i]	  = action;
			return 0;
		}
	}

	return -1;
}

LIBINPT int inpt_act_del(inpt_act_t* action) {
	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i] == action) {
			inpt.actions[i] = NULL;
			return 0;
		}
	}

	return -1;
}

inpt_act_t* inpt_act_get(char* name) {
	unsigned long hash = inpt_hash(name);

	for(int i = 0; i < ACTION_COUNT; i++) {
		if(inpt.actions[i]->name_hash == hash) {
			return inpt.actions[i];
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
		// if(action->states[i] != 0) {
		//     continue;
		// }

		// action->states[i] = hash;
		// return 0;

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
LIBINPT int inpt_act_on_state_change(inpt_act_t* action,
									 void (*event)(char* current_state,
												   char* new_state)) {
	return 0;
}
LIBINPT int inpt_act_on_trigger(inpt_act_t* action, void (*event)()) {
	return 0;
}
LIBINPT int inpt_act_on_value(inpt_act_t* action, void (*event)(int amount)) {
	return 0;
}

// TODO Copy rhid_win.c and rhid.h from the big computer to the little computer
// so I can implement these functions.
LIBINPT int inpt_hid_count() {
	return inpt.dev_count;
}
LIBINPT char** inpt_hid_list() {
	// FIXME This will return a list of null pointers if inpt_update hasn't been
	// called yet.
	return (char**) inpt.dev_names;
}

LIBINPT int inpt_hid_select(int index) {
	if(index >= inpt.dev_count) {
		return -1;
	}

	if(inpt.dev_selected != NULL) {
		rhid_close(inpt.dev_selected);
	}

	inpt.dev_selected = &inpt.devs[index];
	rhid_open(inpt.dev_selected);

	inpt.hid.btn_count = rhid_get_button_count(inpt.dev_selected);
	inpt.hid.val_count = rhid_get_value_count(inpt.dev_selected);

	// Make hid_prev the same as the hid so we can do comparisons latter.
	memcpy(&inpt.hid_prev, &inpt.hid, sizeof(inpt_hid_t));

	return 0;
}

LIBINPT int inpt_hid_is_conn() {
	// TODO Find a better way to do connection check. This is prone to fail if
	//		the device list isn't updated every time inpt_update is called.
	//		(which it shouldn't be.)
	for(int i = 0; i < inpt.dev_count; i++) {
		if(&inpt.devs[i] == inpt.dev_selected) {
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
