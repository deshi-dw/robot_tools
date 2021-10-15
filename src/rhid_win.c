#include "debug.h"
#include "rhid.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <SetupAPI.h>

// TODO Handle HID disconnection gracefully.
// TODO Rename size to count in places where it refers to an array size to avoid
// confusion.

typedef unsigned long  ulong;
typedef unsigned short ushort;

#define ERROR_TO_STRING_CASE(char_msg, err) \
	case(err):                              \
		(char_msg) = #err;                  \
		break
static const char* _rhid_hidp_err_to_str(ulong status) {
	const char* msg;

	switch(status) {
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_NULL);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_INVALID_PREPARSED_DATA);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_INVALID_REPORT_TYPE);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_INVALID_REPORT_LENGTH);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_USAGE_NOT_FOUND);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_VALUE_OUT_OF_RANGE);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_BAD_LOG_PHY_VALUES);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_BUFFER_TOO_SMALL);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_INTERNAL_ERROR);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_I8042_TRANS_UNKNOWN);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_INCOMPATIBLE_REPORT_ID);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_NOT_VALUE_ARRAY);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_IS_VALUE_ARRAY);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_DATA_INDEX_NOT_FOUND);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_BUTTON_NOT_PRESSED);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_REPORT_DOES_NOT_EXIST);
		ERROR_TO_STRING_CASE(msg, HIDP_STATUS_NOT_IMPLEMENTED);
		default:
			return "NOT_A_HIDP_ERROR";
	}

	return msg;
}

static struct {
	ulong dev_iface_list_size;
	char* dev_iface_list;

	int				  button_caps_count;
	HIDP_BUTTON_CAPS* button_caps;

	int				 value_caps_count;
	HIDP_VALUE_CAPS* value_caps;

	ulong			usages_pages_count;
	USAGE_AND_PAGE* usages_pages;
	USAGE*			usages_ordered;
} _rhid_win_gcache = {0};

// @todo: rhid_get_device_count can be optimized.
// to do this, store a pre-allocated buffer for the interface list in a
// global cache. then, re-allocate it when
// CM_Get_Device_Interface_List_SizeA is greater than the global cache
// size. this way, memory isn't allocated and free every time
// rhid_get_device_count is called.

int rhid_get_device_count() {
	// get the HIDClass devices guid.
	GUID hid_guid = {0};
	HidD_GetHidGuid(&hid_guid);

	// get a list of devices from the devices class.
	HDEVINFO dev_list = SetupDiGetClassDevsA(
		&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if(dev_list == INVALID_HANDLE_VALUE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "failed to get devices from device class. error: %lu\n",
				GetLastError());
#endif
		return -1;
	}

	// jump through the enumeration until we get out of bounds.
	const int jump_count = 5;
	int		  last_index = 0;

	for(int i = jump_count;; i += jump_count) {
		SP_DEVICE_INTERFACE_DATA iface = {0};
		iface.cbSize				   = sizeof(SP_DEVICE_INTERFACE_DATA);
		if(SetupDiEnumDeviceInterfaces(dev_list, NULL, &hid_guid, i, &iface) ==
		   FALSE) {
			if(GetLastError() == ERROR_NO_MORE_ITEMS) {
				// we are too far outside of the array. break out and decriment
				// i until we have reached the last valid index.
				last_index = i;
				break;
			}
			else {
#ifdef RHID_DEBUG_ENABLED
				fprintf(stderr,
						"failed to enumerate through device interfaces. error: "
						"%lu\n",
						GetLastError());
#endif
				SetupDiDestroyDeviceInfoList(dev_list);
				return -1;
			}
		}
	}

	// back-track to the last valid index which will be the final size.
	for(last_index = last_index; last_index >= 0; last_index--) {
		SP_DEVICE_INTERFACE_DATA iface = {0};
		iface.cbSize				   = sizeof(SP_DEVICE_INTERFACE_DATA);
		if(SetupDiEnumDeviceInterfaces(dev_list, NULL, &hid_guid, last_index,
									   &iface) == TRUE) {
			break;
		}
	}

	// clean-up the device list.
	SetupDiDestroyDeviceInfoList(dev_list);

	// return the final count.
	return last_index + 1;
}

static inline void* _rhid_open_device_handle(const char* path,
											 ulong		 access_rights,
											 ulong		 share_mode) {
	void* handle = CreateFileA(path, access_rights, share_mode, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(handle == INVALID_HANDLE_VALUE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "failed to open device \"%s\". error: %lu\n", path,
				GetLastError());
#endif
		return NULL;
	}

	return handle;
}

int rhid_get_devices(rhid_device_t* devices, int count) {
	// get the HIDClass devices guid.
	GUID hid_guid = {0};
	HidD_GetHidGuid(&hid_guid);

	// get a list of devices from the devices class.
	HDEVINFO dev_list = SetupDiGetClassDevsA(
		&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if(dev_list == INVALID_HANDLE_VALUE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "failed to get devices from device class. error: %lu\n",
				GetLastError());
#endif
		return -1;
	}

	for(int i = 0; i < count; i++) {
		// set device values to zero.
		memset(&devices[i], 0, sizeof(rhid_device_t));

		// get the next interface at index i.
		SP_DEVICE_INTERFACE_DATA iface = {0};
		iface.cbSize				   = sizeof(SP_DEVICE_INTERFACE_DATA);

		if(SetupDiEnumDeviceInterfaces(dev_list, NULL, &hid_guid, i, &iface) ==
		   FALSE) {
			if(GetLastError() == ERROR_NO_MORE_ITEMS) {
				break;
			}
			else {
#ifdef RHID_DEBUG_ENABLED
				fprintf(stderr,
						"failed to enumerate through device interfaces. error: "
						"%lu\n",
						GetLastError());
#endif
				SetupDiDestroyDeviceInfoList(dev_list);
				return -1;
			}
		}

		// get the interface detail size.
		ulong iface_info_size = 0;
		if(SetupDiGetDeviceInterfaceDetailA(dev_list, &iface, NULL, 0,
											&iface_info_size, NULL) == FALSE) {
			if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
#ifdef RHID_DEBUG_ENABLED
				fprintf(
					stderr,
					"failed to get device interface detail size. error: %lu\n",
					GetLastError());
#endif
				return -1;
			}
		}

		// get the device interface details.
		SP_DEVICE_INTERFACE_DETAIL_DATA_A* iface_info =
			malloc(iface_info_size * sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A));
		iface_info->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		if(SetupDiGetDeviceInterfaceDetailA(dev_list, &iface, iface_info,
											iface_info_size, NULL,
											NULL) == FALSE) {
#ifdef RHID_DEBUG_ENABLED
			fprintf(stderr,
					"failed to get device interface detail data. error: %lu\n",
					GetLastError());
#endif
			return -1;
		}

		// copy over the device path.
		int next_iface_size = strlen(iface_info->DevicePath) + 1;
		memcpy(devices[i].path, iface_info->DevicePath, next_iface_size);

		if(devices[i].path[next_iface_size - 4] == 'k' &&
		   devices[i].path[next_iface_size - 3] == 'b' &&
		   devices[i].path[next_iface_size - 2] == 'd') {
			memset(devices[i].path + next_iface_size - 5, 0, 5);
		}

		free(iface_info);

#ifdef RHID_DEBUG_ENABLED
		printf("\ngetting device (%i) \"%s\"\n", i, devices[i].path);
#endif
		// open the the device with as little permissions as possible so we can
		// read some attributes.
		devices[i].handle =
			_rhid_open_device_handle(devices[i].path, MAXIMUM_ALLOWED,
									 FILE_SHARE_READ | FILE_SHARE_WRITE);
		if(devices[i].handle == NULL) {
			devices[i].handle = _rhid_open_device_handle(
				devices[i].path, MAXIMUM_ALLOWED, FILE_SHARE_READ);

			if(devices[i].handle == NULL) {
				continue;
			}
		}

		// get general attributes of the device.
		HIDD_ATTRIBUTES attributes = {0};
		if(HidD_GetAttributes(devices[i].handle, &attributes) == TRUE) {
			devices[i].vendor_id  = attributes.VendorID;
			devices[i].product_id = attributes.ProductID;
			devices[i].version	  = attributes.VersionNumber;
		}
#ifdef RHID_DEBUG_ENABLED
		else {
			fprintf(stderr, "faild to retrieve device attributes.\n");
		}
#endif

		// @todo: think about moving manufacturer_name to global cache.
		// this could potentially optimize the allocation and deletion of the
		// 254 bytes that make up this 2-wide string.

		// get the manufacturer of the device.
		wchar_t manufacturer_name[127];
		if(HidD_GetManufacturerString(devices[i].handle, manufacturer_name,
									  sizeof(manufacturer_name)) == TRUE) {
			wcstombs(devices[i].manufacturer_name, manufacturer_name,
					 sizeof(devices[i].manufacturer_name));
		}
#ifdef RHID_DEBUG_ENABLED
		else {
			fprintf(stderr, "failed to retrieve device manufacturer name.\n");
		}
#endif

		// @todo: think about moving product_name to global cache.
		// this would be for the same reason as manufacturer_name.

		// get the product name of the device.
		wchar_t product_name[127];
		if(HidD_GetProductString(devices[i].handle, product_name,
								 sizeof(product_name)) == TRUE) {
			wcstombs(devices[i].product_name, product_name,
					 sizeof(devices[i].product_name));
		}
#ifdef RHID_DEBUG_ENABLED
		else {
			fprintf(stderr, "failed to retrieve device product name.\n");
		}
#endif

		// get preparsed data from the device.
		PHIDP_PREPARSED_DATA preparsed = {0};
		if(HidD_GetPreparsedData(devices[i].handle, &preparsed) == FALSE) {
#ifdef RHID_DEBUG_ENABLED
			fprintf(stderr, "failed to get pre-parsed data from device.\n");
#endif
			CloseHandle(devices[i].handle);
			devices[i].handle = NULL;
			continue;
		}

		// get device capabilities for various relatively useful attributes.
		// although this is probably the last HidP_GetCaps, HidP_GetButtonsCaps
		// and HidP_GetValueCaps will probably get called again when reading
		// input reports.
		HIDP_CAPS dev_caps;
		{
			ulong ret =
				HidP_GetCaps((PHIDP_PREPARSED_DATA) preparsed, &dev_caps);
			if(ret != HIDP_STATUS_SUCCESS) {
#ifdef RHID_DEBUG_ENABLED
				fprintf(stderr,
						"failed to get device's capabilities. error: %s\n",
						_rhid_hidp_err_to_str(ret));
#endif
				CloseHandle(devices[i].handle);
				devices[i].handle = NULL;
				HidD_FreePreparsedData(preparsed);
				continue;
			}
		}

		// get button caps.
		if(dev_caps.NumberInputButtonCaps > 0) {
			if(_rhid_win_gcache.button_caps_count <
			   dev_caps.NumberInputButtonCaps) {
				if(_rhid_win_gcache.button_caps == NULL) {
					_rhid_win_gcache.button_caps =
						malloc(sizeof(HIDP_BUTTON_CAPS) *
							   dev_caps.NumberInputButtonCaps);
				}
				else {
					_rhid_win_gcache.button_caps =
						realloc(_rhid_win_gcache.button_caps,
								sizeof(HIDP_BUTTON_CAPS) *
									dev_caps.NumberInputButtonCaps);
				}

				_rhid_win_gcache.button_caps_count =
					dev_caps.NumberInputButtonCaps;
			}
			HIDP_BUTTON_CAPS* button_caps = _rhid_win_gcache.button_caps;
			devices[i].cap_button_count	  = dev_caps.NumberInputButtonCaps;
			{
				ulong ret = HidP_GetButtonCaps(
					HidP_Input, button_caps,
					(PUSHORT) &devices[i].cap_button_count, preparsed);
				if(ret != HIDP_STATUS_SUCCESS) {
#ifdef RHID_DEBUG_ENABLED
					fprintf(stderr,
							"failed to get device's button capabilities. "
							"error: %s\n",
							_rhid_hidp_err_to_str(ret));
#endif
				}
				else {
					// assign button report ids.
					if(dev_caps.NumberInputButtonCaps > RHID_MAX_BUTTON_CAPS) {
#ifdef RHID_DEBUG_ENABLED
						fprintf(stderr, "the number of button caps is larger "
										"than the maximum supported.\n");
#endif
						CloseHandle(devices[i].handle);
						devices[i].handle = NULL;
						HidD_FreePreparsedData(preparsed);
						continue;
					}

					for(int k = 0; k < dev_caps.NumberInputButtonCaps; k++) {
						devices[i].button_report_ids[k] =
							button_caps[k].ReportID;
					}
				}
			}
		}

		// get value caps.
		if(dev_caps.NumberInputValueCaps > 0) {
			if(_rhid_win_gcache.value_caps_count <
			   dev_caps.NumberInputValueCaps) {
				if(_rhid_win_gcache.value_caps == NULL) {
					_rhid_win_gcache.value_caps =
						malloc(sizeof(HIDP_VALUE_CAPS) *
							   dev_caps.NumberInputValueCaps);
				}
				else {
					_rhid_win_gcache.value_caps =
						realloc(_rhid_win_gcache.value_caps,
								sizeof(HIDP_VALUE_CAPS) *
									dev_caps.NumberInputValueCaps);
				}

				_rhid_win_gcache.value_caps_count =
					dev_caps.NumberInputValueCaps;
			}

			HIDP_VALUE_CAPS* value_caps = _rhid_win_gcache.value_caps;
			devices[i].cap_value_count	= dev_caps.NumberInputValueCaps;
			{
				ulong ret = HidP_GetValueCaps(
					HidP_Input, value_caps,
					(PUSHORT) &devices[i].cap_value_count, preparsed);
				if(ret != HIDP_STATUS_SUCCESS) {
#ifdef RHID_DEBUG_ENABLED
					fprintf(stderr,
							"failed to get device's value capabilities. "
							"error: %s\n",
							_rhid_hidp_err_to_str(ret));
#endif
				}
				else {
					// assign value report ids.
					if(dev_caps.NumberInputValueCaps > RHID_MAX_VALUE_CAPS) {
#ifdef RHID_DEBUG_ENABLED
						fprintf(stderr, "the number of value caps is larger "
										"than the maximum supported.\n");
#endif
						CloseHandle(devices[i].handle);
						devices[i].handle = NULL;
						HidD_FreePreparsedData(preparsed);
						continue;
					}

					for(int k = 0; k < dev_caps.NumberInputValueCaps; k++) {
						devices[i].value_report_ids[k] = value_caps[k].ReportID;
						devices[i].value_pages[k] = value_caps[k].UsagePage;

						if(value_caps[k].IsRange == TRUE) {
#ifdef RHID_DEBUG_ENABLED
							fprintf(stderr, "ranged values not supported.\n");
#endif
							devices[i].value_usages[k] =
								value_caps[k].Range.UsageMax;
						}
						else {
							devices[i].value_usages[k] =
								value_caps[k].NotRange.Usage;
						}
					}
				}
			}
		}

		devices[i].usage_page = dev_caps.UsagePage;
		devices[i].usage	  = dev_caps.Usage;

		// devices[i].cap_button_count = dev_caps.NumberInputButtonCaps;
		// devices[i].cap_value_count = dev_caps.NumberInputValueCaps;

		devices[i].button_count =
			HidP_MaxUsageListLength(HidP_Input, 0, preparsed);
		devices[i].value_count = dev_caps.NumberInputValueCaps;

		// note that we shouldn't allocate the report array here as that
		// wouldn't make all that much sense to the user. instead, allocate the
		// report in rhid_device_open.
		devices[i].report_size = dev_caps.InputReportByteLength;

		HidD_FreePreparsedData(preparsed);

		CloseHandle(devices[i].handle);
		devices[i].handle = NULL;
	}

	// free device list.
	SetupDiDestroyDeviceInfoList(dev_list);

	return 0;
}

int rhid_select_count(rhid_device_t* devices, int count,
					  rhid_select_func_t select_func) {
	int new_count = 0;

	// count the space required for this new list.
	for(int i = 0; i < count; i++) {
		if(select_func(devices[i].usage_page, devices[i].usage) == 1) {
			new_count++;
		}
	}

	return new_count;
}

int rhid_select_devices(rhid_device_t* devices, int count,
						rhid_device_t** selected, int selected_count,
						rhid_select_func_t select_func) {
	int select_index = 0;

	// populate the select list with pointers to devices who match the selection
	// requirments demended from the select function.
	for(int i = 0; i < count; i++) {
		if(select_func(devices[i].usage_page, devices[i].usage) == 1) {
			if(select_index > selected_count) {
#ifdef RHID_DEBUG_ENABLED
				fprintf(stderr, "couldn't select all devices as the selection "
								"count was not big enough.\n");
#endif
				return -1;
			}
			selected[select_index] = &devices[i];
			select_index++;
		}
	}

	return 0;
}

int rhid_open(rhid_device_t* device) {
	// open the file while trying different share options.
	device->handle = _rhid_open_device_handle(
		device->path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ);
	if(device->handle == NULL) {
		device->handle =
			_rhid_open_device_handle(device->path, GENERIC_READ | GENERIC_WRITE,
									 FILE_SHARE_READ | FILE_SHARE_WRITE);

		if(device->handle == NULL) {
			return -1;
		}
	}

	// get preparsed data from the device.
	PHIDP_PREPARSED_DATA preparsed = {0};
	if(HidD_GetPreparsedData(device->handle, &preparsed) == FALSE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "failed to get pre-parsed data from device.\n");
#endif
		CloseHandle(device->handle);
		device->handle = NULL;
		return -1;
	}

	device->_preparsed = preparsed;
	device->report	   = malloc(device->report_size);

	device->buttons = calloc(device->button_count, sizeof(uint8_t));
	device->values	= calloc(device->value_count, sizeof(uint32_t));

	device->is_open = 1;

	return 0;
}

int rhid_close(rhid_device_t* device) {
	if(device->is_open == 0) {
		return -1;
	}

	if(device->handle != NULL) {
		CloseHandle(device->handle);
		device->handle = NULL;
	}

	if(device->_preparsed != NULL) {
		HidD_FreePreparsedData(device->_preparsed);
		device->_preparsed = NULL;
	}

	if(device->report != NULL) {
		free(device->report);
		device->report = NULL;
	}

	if(device->buttons != NULL) {
		free(device->buttons);
		device->buttons = NULL;
	}

	if(device->values != NULL) {
		free(device->values);
		device->values = NULL;
	}

	device->is_open = 0;

	return 0;
}

int rhid_report_buttons(rhid_device_t* device, uint8_t report_id) {
	if(device->handle == NULL || device->is_open == 0) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "can't get a report because the device isn't open.\n");
#endif
		return -1;
	}

// get raw data from the device.
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_START("rhid 'get raw button data from device'");
#endif
	device->report[0] = report_id;
	if(HidD_GetInputReport(device->handle, device->report,
						   (u_long) device->report_size) == FALSE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "didn't receive a device report. error: %lu\n",
				GetLastError());
#endif
		return -1;
	}
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_STOP();
#endif

	// parse report into actives buttons list.
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_START("rhid 'reallocating cache'");
#endif
	if(_rhid_win_gcache.usages_pages_count < device->button_count) {
		if(_rhid_win_gcache.usages_pages == NULL) {
			_rhid_win_gcache.usages_pages =
				malloc(device->button_count * sizeof(USAGE_AND_PAGE));

			if(_rhid_win_gcache.usages_ordered == NULL) {
				_rhid_win_gcache.usages_ordered =
					malloc(device->button_count * sizeof(USAGE_AND_PAGE));
			}
		}
		else {
			_rhid_win_gcache.usages_pages =
				realloc(_rhid_win_gcache.usages_pages,
						device->button_count * sizeof(USAGE_AND_PAGE));

			_rhid_win_gcache.usages_ordered =
				realloc(_rhid_win_gcache.usages_ordered,
						device->button_count * sizeof(USAGE_AND_PAGE));
		}

		_rhid_win_gcache.usages_pages_count = device->button_count;
	}
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_STOP();
#endif

#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_START("rhid 'parse report into actives buttons list'");
#endif

	ulong			active_count = device->button_count;
	USAGE_AND_PAGE* usages_pages = _rhid_win_gcache.usages_pages;
	{
		ulong ret = HidP_GetUsagesEx(HidP_Input, 0, usages_pages, &active_count,
									 (PHIDP_PREPARSED_DATA) device->_preparsed,
									 device->report, device->report_size);
		if(ret != HIDP_STATUS_SUCCESS) {
#ifdef RHID_DEBUG_ENABLED
			fprintf(stderr,
					"failed to parse button data from report. error: %s\n",
					_rhid_hidp_err_to_str(ret));
#endif
			return -1;
		}
	}

	USAGE* usages_ordered = _rhid_win_gcache.usages_ordered;
	memset(usages_ordered, 0, device->button_count * sizeof(USAGE));

	for(int i = 0; i < active_count; i++) {
		if(usages_pages[i].UsagePage == HID_USAGE_PAGE_BUTTON) {
			usages_ordered[usages_pages[i].Usage] = 1;
		}
	}

	for(int i = 0; i < device->button_count; i++) {
		if(usages_ordered[i] == 1) {
			if(device->buttons[i] == RHID_BUTTON_OFF ||
			   device->buttons[i] == RHID_BUTTON_RELEASED) {
				device->buttons[i] = RHID_BUTTON_PRESSED;
			}
			else {
				device->buttons[i] = RHID_BUTTON_HELD;
			}
		}
		else if(device->buttons[i] == RHID_BUTTON_HELD) {
			device->buttons[i] = RHID_BUTTON_RELEASED;
		}
		else {
			device->buttons[i] = RHID_BUTTON_OFF;
		}
	}

#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_STOP();
#endif

	return 0;
}

int rhid_report_values(rhid_device_t* device, uint8_t report_id) {
	if(device->handle == NULL || device->is_open == 0) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "can't get a report because the device isn't open.\n");
#endif
		return -1;
	}

	// get raw data from the device.
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_START("rhid 'get raw value data from the device'");
#endif
	device->report[0] = report_id;
	if(HidD_GetInputReport(device->handle, device->report,
						   (u_long) device->report_size) == FALSE) {
#ifdef RHID_DEBUG_ENABLED
		fprintf(stderr, "didn't receive a device report. error: %lu\n",
				GetLastError());
#endif
		return -1;
	}

	for(int i = 0; i < device->value_count; i++) {
		{
			ulong ret = HidP_GetUsageValue(
				HidP_Input, device->value_pages[i], 0, device->value_usages[i],
				(PULONG) &device->values[i], device->_preparsed, device->report,
				device->report_size);

			if(ret != HIDP_STATUS_SUCCESS) {
				fprintf(stderr,
						"failed to parse value data from report. error: %s\n",
						_rhid_hidp_err_to_str(ret));
			}
		}
	}
#ifdef RHID_DEBUG_ENABLED
	DEBUG_TIME_STOP();
#endif

	return 0;
}

int rhid_get_buttons_state(rhid_device_t* device, uint8_t* buttons, int size) {
	if(size < device->button_count) {
		return -1;
	}

	memcpy(buttons, device->buttons, device->button_count);

	return 0;
}
int rhid_get_values_state(rhid_device_t* device, uint32_t* values, int size) {
	if(size < device->value_count) {
		return -1;
	}

	memcpy(values, device->buttons, device->value_count * sizeof(uint32_t));

	return 0;
}

int rhid_get_button(rhid_device_t* device, int index) {
	return 0;
}
int rhid_get_value(rhid_device_t* device, int index) {
	return 0;
}

int rhid_get_button_count(rhid_device_t* device) {
	return device->button_count;
}
int rhid_get_value_count(rhid_device_t* device) {
	return device->value_count;
}

uint16_t rhid_get_vendor_id(rhid_device_t* device) {
	return device->vendor_id;
}
uint16_t rhid_get_product_id(rhid_device_t* device) {
	return device->product_id;
}

uint16_t rhid_get_usage_page(rhid_device_t* device) {
	return device->usage_page;
}
uint16_t rhid_get_usage(rhid_device_t* device) {
	return device->usage;
}

const char* rhid_get_manufacturer_name(rhid_device_t* device) {
	return device->manufacturer_name;
}
const char* rhid_get_product_name(rhid_device_t* device) {
	return device->product_name;
}
