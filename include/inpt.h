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

typedef struct inpt_act_t inpt_act_t;
typedef struct inpt_hid_t inpt_hid_t;

typedef void (*inpt_hid_btn_evnt_t)(int idx, int flags);
typedef void (*inpt_hid_val_evnt_t)(int idx, int amount);

#define BITFLD_GET(bitfield, i) bitfield[i / 8] & (1 << i % 8)
#define BITFLD_SET(bitfield, i) bitfield[i / 8] |= (1 << i % 8)
#define BITFLD_CLR(bitfield, i) bitfield[i / 8] |= ~(1 << i % 8)
#define BITFLD_TGL(bitfield, i) bitfield[i / 8] ^= (1 << i % 8)

struct inpt_hid_t {
	int btn_count;
	int val_count;

#define MAX_BUTTONS 48
#define MAX_VALUES 32

	uint8_t	 btns[MAX_BUTTONS];
	uint32_t vals[MAX_VALUES];
};

struct inpt_t {
	char version[8];

// Should be divisible by 8.
#define STATE_COUNT 32
	unsigned long states[STATE_COUNT];

// TODO Consider making the actions ptr array into just an actions array.
//		Instead of functions testing if actions[i] == NULL they could instead
//		do actions[i].name == NULL.
//		The advantage here would be 0 allocations and all the action memory
//		would reside in one place making for optimal cache performance.
//		(I Think)
//		Something like inpt_act_get could still return an action pointer like
//		this: &inpt.actions[i]
#define ACTION_COUNT 128
	inpt_act_t* actions[ACTION_COUNT];

	int is_running;

	int state_index;

	inpt_hid_t hid;
	inpt_hid_t hid_prev;

#define MAX_HID_BTN_EVENTS 32
	inpt_hid_btn_evnt_t on_hid_btns[MAX_HID_BTN_EVENTS];
#define MAX_HID_VAL_EVENTS 32
	inpt_hid_btn_evnt_t on_hid_vals[MAX_HID_VAL_EVENTS];

	int dev_count;

#define MAX_DEV_COUNT 16
	rhid_device_t devs[MAX_DEV_COUNT];
	const char* dev_names[MAX_DEV_COUNT];

	rhid_device_t* dev_selected;
};

struct inpt_act_t {
	char*		  name;
	unsigned long name_hash;
	int			  type;

	uint8_t states[STATE_COUNT / 8];

	int input_mod;
	int input;

	int			  flags;
	unsigned long new_state;

#define INPT_ACT_POINT_COUNT 16
	int			   point_count;

	struct point_t {
		float x;
		float y;
	} points[INPT_ACT_POINT_COUNT];
};

LIBINPT const char* inpt_version();

LIBINPT int inpt_start();
LIBINPT int inpt_stop();
LIBINPT int inpt_update();

LIBINPT int inpt_state_add(char* state);
LIBINPT int inpt_state_del(char* state);
LIBINPT int inpt_state_set(char* state, char* new_state);

// TODO Consider making act_add have parameters for the action as arguments.
//      This goes along with making the inpt.actions into a struct array instead
//      of a pointer array.
//      This would also be just convinient in general.
LIBINPT int inpt_act_add(inpt_act_t* action, char* name);
LIBINPT int inpt_act_del(inpt_act_t* action);
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

LIBINPT int inpt_act_on_state_change(inpt_act_t* action,
									 void (*event)(char* current_state,
												   char* new_state));
LIBINPT int inpt_act_on_trigger(inpt_act_t* action, void (*event)());
LIBINPT int inpt_act_on_value(inpt_act_t* action, void (*event)(int amount));

LIBINPT int inpt_hid_count();
LIBINPT char** inpt_hid_list();
LIBINPT int inpt_hid_select(int index);

LIBINPT int inpt_hid_is_conn();

LIBINPT int inpt_hid_on_btn(inpt_hid_btn_evnt_t event);
LIBINPT int inpt_hid_on_val(inpt_hid_val_evnt_t event);

#endif
