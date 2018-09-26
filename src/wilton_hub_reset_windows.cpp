/*
 * Copyright 2018, mike at myasnikov.mike@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
// Include header files
#include <Windows.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <cstdio>
#include <winusb.h>
#include <usbiodef.h>
#include <memory>
#include <sstream>
#endif

#include "wilton/wiltoncall.h"

#include "jansson.h"

namespace smart_hub_reset {

#ifdef _WIN32
namespace {
std::string errcode_to_string(uint32_t code){
    if (0 == code) return std::string{};
    char* buf_p = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<char*> (&buf_p), 0, nullptr);
    if (0 == size) {
        return "Cannot format code: [" + std::to_string(code) + "]" +
            " into message, error code: [" + std::to_string(GetLastError()) + "]";
    }
    if (size <= 2) {
        LocalFree(buf_p);
        return "code: [" + std::to_string(code) + "], message: []";
    }
    auto msg = std::string(buf_p, size-2);
    LocalFree(buf_p);
    return "code: [" + std::to_string(code) + "], message: [" + msg + "]";
}
std::string cm_errcode_to_string(uint32_t cm_code){
    auto error_description = std::string{};
    switch(cm_code){
        case CR_REMOVE_VETOED: {
            error_description = "Remove operation vetoed by other process or function.";
            break;
        }
        case CR_INVALID_DEVNODE: {
            error_description = "Invalid devnode or devinst of device. Device has errors at initialization or device_info structure contains wrong data. Check if device connected.";
            break;
        }
        case CR_SUCCESS:{
            error_description = "Success operation.";
            break;
        }
        case CR_ACCESS_DENIED:{
            error_description = "Access denied. You need Administrator rights to do reset.";
            break;
        }
        default: {
            error_description = "Error [" + std::to_string(cm_code) + "] check error code at cfgmgr32.h";
        }
    }
    return error_description;
}
std::string get_json_error(const std::string error_message){
    std::string json = "{ \"error\": \"" +  error_message + "\"}";
    return json;
}
std::string get_veto_type_name(PNP_VETO_TYPE veto_type){
    auto type_name = std::string{};
    switch (veto_type)
    {
    case PNP_VetoTypeUnknown: {
        type_name = "PNP_VetoTypeUnknown";
        break; }
    case PNP_VetoLegacyDevice: {
        type_name = "PNP_VetoLegacyDevice";
        break; }
    case PNP_VetoPendingClose: {
        type_name = "PNP_VetoPendingClose";
        break; }
    case PNP_VetoWindowsApp: {
        type_name = "PNP_VetoWindowsApp";
        break; }
    case PNP_VetoWindowsService: {
        type_name = "PNP_VetoWindowsService";
        break; }
    case PNP_VetoOutstandingOpen: {
        type_name = "PNP_VetoOutstandingOpen";
        break; }
    case PNP_VetoDevice: {
        type_name = "PNP_VetoDevice";
        break; }
    case PNP_VetoDriver: {
        type_name = "PNP_VetoDriver";
        break; }
    case PNP_VetoIllegalDeviceRequest: {
        type_name = "PNP_VetoIllegalDeviceRequest";
        break; }
    case PNP_VetoInsufficientPower: {
        type_name = "PNP_VetoInsufficientPower";
        break; }
    case PNP_VetoNonDisableable: {
        type_name = "PNP_VetoNonDisableable";
        break; }
    case PNP_VetoLegacyDriver: {
        type_name = "PNP_VetoLegacyDriver";
        break; }
    case PNP_VetoInsufficientRights: {
        type_name = "PNP_VetoInsufficientRights";
        break; }
    default: break;
    }
    return type_name;
}

enum class device_types
{
    hub, usb
};

struct devices_info
{
    std::string pid; 
    std::string vid;
    std::string path;
    devices_info(const std::string& vid, const std::string& pid, const std::string& path) 
            : vid(vid), pid(pid), path(path){}
};
static std::string tohex(uint16_t num) {
    std::stringstream ss{};
    ss << "0x" << std::hex << num;
    return ss.str();
}
std::string reset_smart_hub(int pid, int vid, device_types type)
{
	LPCSTR enumerator = nullptr; // may be used to get all HID or USB if set as TEXT("HID") oa TEXT("USB") but not nescessary if we get class_guid
	GUID class_guid = GUID_CLASS_USBHUB; // by default
    switch (type) {
    case device_types::usb: {
        class_guid = GUID_CLASS_USB_DEVICE;
        break;
    }
    case device_types::hub:
    default: {
        class_guid = GUID_CLASS_USBHUB;
    }
    };
    
    HWND parent = nullptr; // Not nescessary to our goals. Poiner to the top-level window to be used for a user interface that is associated with installing a device instance in the device information set
	// DIGCF_DEVICEINTERFACE allows get devices with realized interfaces
    DWORD falgs = DIGCF_DEVICEINTERFACE; // other possible values: DIGCF_ALLCLASSES  DIGCF_DEVICEINTERFACE  DIGCF_PRESENT DIGCF_PROFILE

    HDEVINFO dev_info;
	dev_info = SetupDiGetClassDevs(&class_guid, enumerator, parent, falgs);
    if (INVALID_HANDLE_VALUE == dev_info) {
        return get_json_error("Error 'SetupDiGetClassDevs' [" + errcode_to_string(GetLastError()) + "]");
    }

    SP_DEVINFO_DATA dev_info_data;
    dev_info_data.cbSize = sizeof(dev_info_data);
    dev_info_data.ClassGuid;
    DWORD pos = 1; // count from 1, not from 0
    bool successfull_reset = false;
    auto error_message = std::string{};

    std::vector<devices_info> cecked_devices;

    const size_t id_leng = 4;
    const int hex_base = 16;
    char* proxy_pend = nullptr;
    auto vid_prefix = std::string{"VID_"};
    auto pid_prefix = std::string{"PID_"};
    while (SetupDiEnumDeviceInfo(dev_info, pos++, &dev_info_data)) {

        TCHAR buffer[MAX_DEVICE_ID_LEN];
        ULONG get_flags = 0;
        auto status = CM_Get_Device_ID(dev_info_data.DevInst, buffer, MAX_PATH, get_flags);
        dev_info_data;
        if (status != CR_SUCCESS) {
            continue;
        }

        std::string device_path(buffer);

        size_t pos_vid = device_path.find(vid_prefix);
        size_t pos_pid = device_path.find(pid_prefix);
		int test_vid = 0;
		int test_pid = 0;
		auto vid_str = std::string{ "none" };
		auto pid_str = std::string{ "none" };
		if (std::string::npos != pos_vid) {
			vid_str = device_path.substr(pos_vid + vid_prefix.size(), id_leng);
			test_vid = strtol(vid_str.c_str(), &proxy_pend, hex_base);
		}
		if (std::string::npos != pos_vid) {
			pid_str = device_path.substr(pos_pid + pid_prefix.size(), id_leng);
			test_pid = strtol(pid_str.c_str(), &proxy_pend, hex_base);
		}

        if (vid == test_vid && pid == test_pid) {
            PNP_VETO_TYPE veto_type;
            std::vector<char> veto_reason(MAX_PATH);
            auto eject_result = CM_Query_And_Remove_SubTree(dev_info_data.DevInst, std::addressof(veto_type), static_cast<LPSTR>(veto_reason.data()), MAX_PATH, 0);
            if (CR_SUCCESS != eject_result) {
                auto veto_type_name = get_veto_type_name(veto_type);
                error_message += "Error, device: [" + device_path +
                        "], message: [" + cm_errcode_to_string(eject_result) + 
                        "], Veto type: [" + veto_type_name + 
                        "], Veto reason: [" + std::string{veto_reason.data()} + "]\n";
            }
            else {
                successfull_reset = true;
                break;
            }
        } else {
            cecked_devices.emplace_back(vid_str, pid_str, device_path);
        }
    }

    SetupDiDeleteDeviceInfo(dev_info, &dev_info_data);
    SetupDiDestroyDeviceInfoList(dev_info);

    if (!successfull_reset) {
        if (error_message.empty()) {
            error_message += "Cannot find USB device with VID: [" + tohex(vid) + "], PID: [" + 
                            tohex(pid) + "], found devices [";
            for (auto& el : cecked_devices) {
                error_message += "{ \"vendorId\": \"" + el.vid +
                                 "\", \"productId\": \"" + el.pid + 
                                 "\", \"device_path\": \"" + el.path + "\"}\n";
            }
            error_message += "]";
        }
        return get_json_error(error_message);
    }

    return std::string{};
}
} // anonymus namespace
#endif

struct hub_settings {
    int vid;
    int pid;
	device_types type;
};

std::string reset_hub(const hub_settings& set){
#ifdef _WIN32
	return reset_smart_hub(set.pid, set.vid, set.type);
#endif
    return std::string{};
}

int get_integer_or_throw(const std::string& key, json_t* value) {
    if (json_is_integer(value)) {
        return static_cast<int>(json_integer_value(value));
    }
    throw std::invalid_argument(std::string{"Error: Key [" + key+ "] don't contains integer value"});
}
std::string get_string_or_throw(const std::string& key, json_t* value) {
	if (json_is_string(value)) {
		return std::string{ json_string_value(value) };
	}
	throw std::invalid_argument(std::string{ "Error: Key [" + key + "] don't contains string value" });
}

char* wrapper_reset_hub(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(const hub_settings&)> (ctx);
        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int pid = not_set;
        int vid = not_set;
		device_types type = device_types::hub; // by default search hubs
		auto type_name = std::string{};

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("pid" == key_str) {
                pid = get_integer_or_throw(key_str, value);
            } else if ("vid" == key_str) {
                vid = get_integer_or_throw(key_str, value);
            } 
			else if ("type" == key_str) {
				type_name = get_string_or_throw(key_str, value);
			} else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        hub_settings set;
        set.pid = pid;
        set.vid = vid;
		if ("usb" == type_name) {
			type = device_types::usb;
		}
		set.type = type;

        std::string output = fun(set);
        if (!output.empty()) {
            // nul termination here is required only for JavaScriptCore engine
            *data_out = wilton_alloc(static_cast<int>(output.length()) + 1);
            std::memcpy(*data_out, output.c_str(), output.length() + 1);
        } else {
            *data_out = nullptr;
        }
        *data_out_len = static_cast<int>(output.length());
        return nullptr;
    } catch (...) {
        auto what = std::string("CALL ERROR"); // std::string(e.what());
        char* err = wilton_alloc(static_cast<int>(what.length()) + 1);
        std::memcpy(err, what.c_str(), what.length() + 1);
        return err;
    }
}

} // namespace

// this function is called on module load,
// must return NULL on success
extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
char* wilton_module_init() {
    char* err = nullptr;


    // register 'hello' function
    auto reset_hub_name = std::string("reset_hub");
    err = wiltoncall_register(reset_hub_name.c_str(), static_cast<int> (reset_hub_name.length()),
        reinterpret_cast<void*> (smart_hub_reset::reset_hub), smart_hub_reset::wrapper_reset_hub);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
