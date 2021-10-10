#ifndef RHID_H
#define RHID_H

#include <stdint.h>

// @todo: replace RHID_MAX_BUTTON_CAPS and RHID_MAX_VALUE_CAPS with a more
// dynamic solution.
// these maximum are to define the size of button_report_ids and
// value_report_ids so they can be allocated automatically by the compiler.
// this isn't a great method since I don't think there is such
// limit in the HID specs but it will work for now.
#define RHID_MAX_BUTTON_CAPS 32
#define RHID_MAX_VALUE_CAPS 32

enum RHID_PAGE {
	RHID_PAGE_UNDEFINED = (0x00),
	RHID_PAGE_GENERIC = (0x01),
	RHID_PAGE_SIMULATION = (0x02),
	RHID_PAGE_VR = (0x03),
	RHID_PAGE_SPORT = (0x04),
	RHID_PAGE_GAME = (0x05),
	RHID_PAGE_GENERIC_DEVICE = (0x06),
	RHID_PAGE_KEYBOARD = (0x07),
	RHID_PAGE_LED = (0x08),
	RHID_PAGE_BUTTON = (0x09),
	RHID_PAGE_ORDINAL = (0x0A),
	RHID_PAGE_TELEPHONY = (0x0B),
	RHID_PAGE_CONSUMER = (0x0C),
	RHID_PAGE_DIGITIZER = (0x0D),
	RHID_PAGE_HAPTICS = (0x0E),
	RHID_PAGE_PID = (0x0F),
	RHID_PAGE_UNICODE = (0x10),
	RHID_PAGE_ALPHANUMERIC = (0x14),
	RHID_PAGE_SENSOR = (0x20),
	RHID_PAGE_LIGHTING_ILLUMINATION = (0x59),
	RHID_PAGE_BARCODE_SCANNER = (0x8C),
	RHID_PAGE_WEIGHING_DEVICE = (0x8D),
	RHID_PAGE_MAGNETIC_STRIPE_READER = (0x8E),
	RHID_PAGE_CAMERA_CONTROL = (0x90),
	RHID_PAGE_ARCADE = (0x91),
	RHID_PAGE_MICROSOFT_BLUETOOTH_HANDSFREE = (0xFFF3),
	RHID_PAGE_VENDOR_DEFINED_BEGIN = (0xFF00),
	RHID_PAGE_VENDOR_DEFINED_END = (0xFFFF)
};

enum RHID_USAGE {
	// Generic Desktop Page (0x01)
	RHID_USAGE_GENERIC_POINTER = (0x01),
	RHID_USAGE_GENERIC_MOUSE = (0x02),
	RHID_USAGE_GENERIC_JOYSTICK = (0x04),
	RHID_USAGE_GENERIC_GAMEPAD = (0x05),
	RHID_USAGE_GENERIC_KEYBOARD = (0x06),
	RHID_USAGE_GENERIC_KEYPAD = (0x07),
	RHID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER = (0x08),
	RHID_USAGE_GENERIC_TABLET_PC_SYSTEM_CTL = (0x09),
	RHID_USAGE_GENERIC_PORTABLE_DEVICE_CONTROL = (0x0D),
	RHID_USAGE_GENERIC_INTERACTIVE_CONTROL = (0x0E),
	RHID_USAGE_GENERIC_COUNTED_BUFFER = (0x3A),
	RHID_USAGE_GENERIC_SYSTEM_CTL = (0x80),

	RHID_USAGE_GENERIC_X = (0x30),
	RHID_USAGE_GENERIC_Y = (0x31),
	RHID_USAGE_GENERIC_Z = (0x32),
	RHID_USAGE_GENERIC_RX = (0x33),
	RHID_USAGE_GENERIC_RY = (0x34),
	RHID_USAGE_GENERIC_RZ = (0x35),
	RHID_USAGE_GENERIC_SLIDER = (0x36),
	RHID_USAGE_GENERIC_DIAL = (0x37),
	RHID_USAGE_GENERIC_WHEEL = (0x38),
	RHID_USAGE_GENERIC_HATSWITCH = (0x39),
	RHID_USAGE_GENERIC_BYTE_COUNT = (0x3B),
	RHID_USAGE_GENERIC_MOTION_WAKEUP = (0x3C),
	RHID_USAGE_GENERIC_START = (0x3D),
	RHID_USAGE_GENERIC_SELECT = (0x3E),
	RHID_USAGE_GENERIC_VX = (0x40),
	RHID_USAGE_GENERIC_VY = (0x41),
	RHID_USAGE_GENERIC_VZ = (0x42),
	RHID_USAGE_GENERIC_VBRX = (0x43),
	RHID_USAGE_GENERIC_VBRY = (0x44),
	RHID_USAGE_GENERIC_VBRZ = (0x45),
	RHID_USAGE_GENERIC_VNO = (0x46),
	RHID_USAGE_GENERIC_FEATURE_NOTIFICATION = (0x47),
	RHID_USAGE_GENERIC_RESOLUTION_MULTIPLIER = (0x48),
	RHID_USAGE_GENERIC_SYSCTL_POWER = (0x81),
	RHID_USAGE_GENERIC_SYSCTL_SLEEP = (0x82),
	RHID_USAGE_GENERIC_SYSCTL_WAKE = (0x83),
	RHID_USAGE_GENERIC_SYSCTL_CONTEXT_MENU = (0x84),
	RHID_USAGE_GENERIC_SYSCTL_MAIN_MENU = (0x85),
	RHID_USAGE_GENERIC_SYSCTL_APP_MENU = (0x86),
	RHID_USAGE_GENERIC_SYSCTL_HELP_MENU = (0x87),
	RHID_USAGE_GENERIC_SYSCTL_MENU_EXIT = (0x88),
	RHID_USAGE_GENERIC_SYSCTL_MENU_SELECT = (0x89),
	RHID_USAGE_GENERIC_SYSCTL_MENU_RIGHT = (0x8A),
	RHID_USAGE_GENERIC_SYSCTL_MENU_LEFT = (0x8B),
	RHID_USAGE_GENERIC_SYSCTL_MENU_UP = (0x8C),
	RHID_USAGE_GENERIC_SYSCTL_MENU_DOWN = (0x8D),
	RHID_USAGE_GENERIC_SYSCTL_COLD_RESTART = (0x8E),
	RHID_USAGE_GENERIC_SYSCTL_WARM_RESTART = (0x8F),
	RHID_USAGE_GENERIC_DPAD_UP = (0x90),
	RHID_USAGE_GENERIC_DPAD_DOWN = (0x91),
	RHID_USAGE_GENERIC_DPAD_RIGHT = (0x92),
	RHID_USAGE_GENERIC_DPAD_LEFT = (0x93),
	RHID_USAGE_GENERIC_SYSCTL_DISMISS_NOTIFICATION = (0x9A),
	RHID_USAGE_GENERIC_SYSCTL_DOCK = (0xA0),
	RHID_USAGE_GENERIC_SYSCTL_UNDOCK = (0xA1),
	RHID_USAGE_GENERIC_SYSCTL_SETUP = (0xA2),
	RHID_USAGE_GENERIC_SYSCTL_SYS_BREAK = (0xA3),
	RHID_USAGE_GENERIC_SYSCTL_SYS_DBG_BREAK = (0xA4),
	RHID_USAGE_GENERIC_SYSCTL_APP_BREAK = (0xA5),
	RHID_USAGE_GENERIC_SYSCTL_APP_DBG_BREAK = (0xA6),
	RHID_USAGE_GENERIC_SYSCTL_MUTE = (0xA7),
	RHID_USAGE_GENERIC_SYSCTL_HIBERNATE = (0xA8),
	RHID_USAGE_GENERIC_SYSCTL_DISP_INVERT = (0xB0),
	RHID_USAGE_GENERIC_SYSCTL_DISP_INTERNAL = (0xB1),
	RHID_USAGE_GENERIC_SYSCTL_DISP_EXTERNAL = (0xB2),
	RHID_USAGE_GENERIC_SYSCTL_DISP_BOTH = (0xB3),
	RHID_USAGE_GENERIC_SYSCTL_DISP_DUAL = (0xB4),
	RHID_USAGE_GENERIC_SYSCTL_DISP_TOGGLE = (0xB5),
	RHID_USAGE_GENERIC_SYSCTL_DISP_SWAP = (0xB6),
	RHID_USAGE_GENERIC_SYSCTL_DISP_AUTOSCALE = (0xB7),
	RHID_USAGE_GENERIC_SYSTEM_DISPLAY_ROTATION_LOCK_BUTTON = (0xC9),
	RHID_USAGE_GENERIC_SYSTEM_DISPLAY_ROTATION_LOCK_SLIDER_SWITCH = (0xCA),
	RHID_USAGE_GENERIC_CONTROL_ENABLE = (0xCB)
};

enum RHID_DEVICE_TYPE {
	RHID_DEVICE_POINTER = (1 << 0),
	RHID_DEVICE_MOUSE = (1 << 1),
	RHID_DEVICE_JOYSTICK = (1 << 2),
	RHID_DEVICE_GAMEPAD = (1 << 3),
	RHID_DEVICE_KEYBOARD = (1 << 4),
	RHID_DEVICE_KEYPAD = (1 << 5)
};

enum RHID_BUTTON_STATE {
	RHID_BUTTON_OFF = 0,
	RHID_BUTTON_PRESSED = 1,
	RHID_BUTTON_HELD = 2,
	RHID_BUTTON_RELEASED = 3
};

typedef struct {
	int is_open;

	char path[256];
	void* handle;

	void* _preparsed;

	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t version;

	uint16_t usage;
	uint16_t usage_page;

	char product_name[127];
	char manufacturer_name[127];

	int report_size;
	char* report;

	int cap_button_count;
	int cap_value_count;

	uint8_t button_report_ids[RHID_MAX_BUTTON_CAPS];
	uint8_t value_report_ids[RHID_MAX_VALUE_CAPS];

	uint16_t value_pages[RHID_MAX_VALUE_CAPS];
	uint16_t value_usages[RHID_MAX_VALUE_CAPS];

	int button_count;
	int value_count;

	uint8_t* buttons;

	uint32_t* values;
} rhid_device_t;

typedef int (*rhid_select_func_t)(uint16_t page, uint16_t usage);

int rhid_get_device_count();
int rhid_get_devices(rhid_device_t* devices, int count);

int rhid_select_count(rhid_device_t* devices, int count,
					  rhid_select_func_t select_func);

int rhid_select_devices(rhid_device_t* devices, int count,
						rhid_device_t** selected, int selected_count,
						rhid_select_func_t select_func);

// @todo: add a function to filer devices from rhid_get_devices.

int rhid_open(rhid_device_t* device);
int rhid_close(rhid_device_t* device);

int rhid_report_buttons(rhid_device_t* device, uint8_t report_id);
int rhid_report_values(rhid_device_t* device, uint8_t report_id);

int rhid_get_buttons_state(rhid_device_t* device, uint8_t* buttons, int size);
int rhid_get_values_state(rhid_device_t* device, uint32_t* values, int size);

int rhid_get_button(rhid_device_t* device, int index);
int rhid_get_value(rhid_device_t* device, int index);

int rhid_get_button_count(rhid_device_t* device);
int rhid_get_value_count(rhid_device_t* device);

uint16_t rhid_get_vendor_id(rhid_device_t* device);
uint16_t rhid_get_product_id(rhid_device_t* device);
uint16_t rhid_get_product_id(rhid_device_t* device);

uint16_t rhid_get_usage_page(rhid_device_t* device);
uint16_t rhid_get_usage(rhid_device_t* device);

const char* rhid_get_manufacturer_name(rhid_device_t* device);
const char* rhid_get_product_name(rhid_device_t* device);

#endif
