#ifndef RINPT_H
#define RINPT_H

#ifdef DLL_EXPORT
// the dll exports
#define LIBINPT __declspec(dllexport)
#else
// the exe imports
#define LIBINPT __declspec(dllimport)
#endif

#include <stdint.h>
#include <rhid.h>

typedef struct inpt_act_t	 inpt_act_t;
typedef struct inpt_hid_t	 inpt_hid_t;
typedef struct inpt_hid_id_t inpt_hid_id_t;

typedef void (*inpt_hid_btn_evnt_t)(int idx, int flags);
typedef void (*inpt_hid_val_evnt_t)(int idx, int amount);

typedef void (*inpt_act_state_change_evnt_t)(unsigned long current_state,
											 unsigned long new_state);
typedef void (*inpt_act_trigger_evnt_t)(int flag);
typedef void (*inpt_act_value_evnt_t)(double value);

#define BITFLD_GET(bitfield, i) bitfield[i / 8] & (1 << i % 8)
#define BITFLD_SET(bitfield, i) bitfield[i / 8] |= (1 << i % 8)
#define BITFLD_CLR(bitfield, i) bitfield[i / 8] |= ~(1 << i % 8)
#define BITFLD_TGL(bitfield, i) bitfield[i / 8] ^= (1 << i % 8)

enum inpt_btn_state_t {
	INPT_BTN_OFF	  = 0b0000,
	INPT_BTN_PRESSED  = 0b0001,
	INPT_BTN_RELEASED = 0b0010,
	INPT_BTN_HELD	  = 0b0100,
};

// Should be divisible by 8.
#define STATE_COUNT 32

struct inpt_hid_t {
	int btn_count;
	int val_count;

#define MAX_BUTTONS 48
#define MAX_VALUES 32

	uint8_t	 btns[MAX_BUTTONS];
	uint32_t vals[MAX_VALUES];
};

enum inpt_act_type_t {
	INPT_ACT_STATE_CHANGE = 1,
	INPT_ACT_TRIGGER,
	INPT_ACT_VALUE,
};

struct inpt_act_t {
	char*				 name;
	unsigned long		 name_hash;
	enum inpt_act_type_t type;

	uint8_t states[STATE_COUNT / 8];

	int input_mod;
	int input;

	int			  flags;
	unsigned long new_state;

#define INPT_ACT_POINT_COUNT 16
	int point_count;

	struct point_t {
		float x;
		float y;
	} points[INPT_ACT_POINT_COUNT];
};

struct inpt_hid_id_t {
	int vid;
	int pid;
};

// TODO remove excess data duplication in inpt_t struct.
struct inpt_t {
	char version[8];

	unsigned long states[STATE_COUNT];

#define ACTION_COUNT 128
	inpt_act_t actions[ACTION_COUNT];

	int is_running;

	int state_index;

	inpt_hid_t hid;
	inpt_hid_t hid_prev;

#define MAX_HID_BTN_EVENTS 32
	inpt_hid_btn_evnt_t on_hid_btns[MAX_HID_BTN_EVENTS];
#define MAX_HID_VAL_EVENTS 32
	inpt_hid_btn_evnt_t on_hid_vals[MAX_HID_VAL_EVENTS];

#define MAX_ACT_STATE_CHANGE_EVENTS 64
	inpt_act_state_change_evnt_t
		on_act_state_changes[MAX_ACT_STATE_CHANGE_EVENTS];
#define MAX_ACT_TRIGGER_EVENTS 64
	struct {
		inpt_act_trigger_evnt_t event;
		inpt_act_t*				action;
	} on_act_triggers[MAX_ACT_TRIGGER_EVENTS];
#define MAX_ACT_VALUE_EVENTS 64
	struct {
		inpt_act_value_evnt_t event;
		inpt_act_t*			  action;
	} on_act_values[MAX_ACT_VALUE_EVENTS];

	int dev_count;

#define MAX_DEV_COUNT 16
	rhid_device_t devs[MAX_DEV_COUNT];
	const char*	  dev_names[MAX_DEV_COUNT];
	inpt_hid_id_t dev_ids[MAX_DEV_COUNT];

	rhid_device_t* dev_selected;

	enum inpt_btn_state_t btn_states[MAX_BUTTONS];
	enum inpt_btn_state_t btn_states_prev[MAX_BUTTONS];
};

LIBINPT const char* inpt_version();

LIBINPT int inpt_start();
LIBINPT int inpt_stop();
LIBINPT int inpt_update();

LIBINPT int inpt_state_add(char* state);
LIBINPT int inpt_state_del(char* state);
LIBINPT int inpt_state_set(char* state, char* new_state);

LIBINPT inpt_act_t* inpt_act_new_state_change(char* name, char** states,
											  int state_count, int input_mod,
											  int input, char* newstate,
											  int flags);
LIBINPT inpt_act_t* inpt_act_new_trigger(char* name, char** states,
										 int state_count, int input_mod,
										 int input, int flags);
LIBINPT inpt_act_t* inpt_act_new_value(char* name, char** states,
									   int state_count, int input_mod,
									   int input, int flags);
LIBINPT int			inpt_act_del(char* name);
LIBINPT inpt_act_t* inpt_act_get(char* name);

LIBINPT int inpt_act_set_name(inpt_act_t* action, char* name);
LIBINPT int inpt_act_set_type(inpt_act_t* action, int type);

LIBINPT int inpt_act_set_input_mod(inpt_act_t* action, int input_mod);
LIBINPT int inpt_act_set_input(inpt_act_t* action, int input);

LIBINPT int inpt_act_set_flags(inpt_act_t* action, int flags);

LIBINPT int inpt_act_set_point(inpt_act_t* action, int index, int x, int y);

LIBINPT int inpt_act_add_point(inpt_act_t* action, int x, int y);
LIBINPT int inpt_act_del_point(inpt_act_t* action, int index);

LIBINPT int inpt_act_add_state(inpt_act_t* action, char* state);
LIBINPT int inpt_act_del_state(inpt_act_t* action, char* state);

LIBINPT int inpt_act_on_state_change(inpt_act_t*				  action,
									 inpt_act_state_change_evnt_t event);
LIBINPT int inpt_act_on_trigger(inpt_act_t*				action,
								inpt_act_trigger_evnt_t event);
LIBINPT int inpt_act_on_value(inpt_act_t* action, inpt_act_value_evnt_t event);

LIBINPT int			   inpt_hid_count();
LIBINPT inpt_hid_id_t* inpt_hid_list();
LIBINPT char**		   inpt_hid_list_names();
LIBINPT int			   inpt_hid_select(int vid, int pid);

LIBINPT int inpt_hid_is_conn();

LIBINPT int inpt_hid_on_btn(inpt_hid_btn_evnt_t event);
LIBINPT int inpt_hid_on_val(inpt_hid_val_evnt_t event);

#endif
