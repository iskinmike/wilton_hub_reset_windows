#include <iostream>


#include <cstring>
#include <string>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
// Include header files
#include <Windows.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>
#endif
#include <iostream>
#include <thread>
#include <string>

#include "wilton/wiltoncall.h"

#include "jansson.h"

namespace example {

#ifdef _WIN32
int reset_smart_hub(std::string pid, std::string vid)
{
    GUID *class_guid = nullptr;
    LPCSTR enumerator = TEXT("USB");
    HWND parent = nullptr;
    DWORD falgs = DIGCF_ALLCLASSES | DIGCF_PRESENT;

    HDEVINFO dev_info;

    dev_info = SetupDiGetClassDevs(NULL, TEXT("USB"), NULL, falgs);
    if (INVALID_HANDLE_VALUE == dev_info) {
        std::cout << "error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "success to get devices info " << std::endl;

    SP_DEVINFO_DATA devinfo_data;
    devinfo_data.cbSize = sizeof(devinfo_data);
    DWORD pos = 1;
    auto res = SetupDiEnumDeviceInfo(dev_info, pos, &devinfo_data);

    while (SetupDiEnumDeviceInfo(dev_info, pos, &devinfo_data)) {

        TCHAR buffer[MAX_DEVICE_ID_LEN];
        ULONG get_flags = 0;
        auto status = CM_Get_Device_ID(devinfo_data.DevInst, buffer, MAX_PATH, get_flags);
        devinfo_data;
        if (status != CR_SUCCESS) {
            std::cout << "error: " << GetLastError() << std::endl;
            eturn 1;
        }

        std::cout << "device [" << pos << "]: " << buffer << std::endl;

        std::string device_path(buffer);
        std::string vendor_id = vid; //{"05E3"};
        std::string product_id = pid; //{ "0608" };

        if (std::string::npos != device_path.find(vendor_id) && std::string::npos != device_path.find(product_id)){
            std::cout << "try to eject first child " << std::endl;
            // CM_Query_And_Remove_SubTree();
            // CM_Request_Device_Eject();
            auto eject_result = CM_Query_And_Remove_SubTree(devinfo_data.DevInst, NULL, NULL, 0, 0);
            if (CR_SUCCESS != eject_result) {
                std::cout << "Error " << eject_result << std::endl;
            }
            else {
                std::cout << "Success" << std::endl;
            }
            //break;
        }

        ++pos;
    }

    // Exit the program
    return 0;
}

#endif

struct hub_settings {
    int vid;
    int pid;
};

std::string reset_hub(const hub_settings& set){
#ifdef _WIN32
    reset_smart_hub(std::to_string(set.pid), std::to_string(set.vid));
#endif
    return std::string{};
}

int get_integer_or_throw(const std::string& key, json_t* value) {
    if (json_is_integer(value)) {
        return static_cast<int>(json_integer_value(value));
    }
    throw std::invalid_argument(std::string{"Error: Key [" + key+ "] don't contains integer value"});
}

char* wrapper_reset_hub(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(hub_settings)> (ctx);

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int pid = not_set;
        int vid = not_set;

        /* obj is a JSON object */
        const char *key = nullptr;
        json_t *value = nullptr;

        json_object_foreach(root, key, value) {
            auto key_str = std::string{key};
            if ("pid" == key_str) {
                pid = get_integer_or_throw(key_str, value);
            } else if ("vid" == key_str) {
                vid = get_integer_or_throw(key_str, value);
            } else {
                std::string err_msg = std::string{"Unknown data field: ["} + key + "]";
                throw std::invalid_argument(err_msg);
            }
        }

        hub_settings set;
        set.pid = pid;
        set.vid = vid;

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
            reinterpret_cast<void*> (example::reset_hub), example::wrapper_reset_hub);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
