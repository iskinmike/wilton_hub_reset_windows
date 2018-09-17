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

std::string hello(const std::string& input) {
    // arbitrary C++ code here
    return input + " from C++!";
}

std::string hello_again(const std::string& input) {
    // arbitrary C++ code here
    return input + " again from C++!";
}

// helper function
char* wrapper_fun(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(const std::string&)> (ctx);
        auto input = std::string(data_in, static_cast<size_t>(data_in_len));
        std::string output = fun(input);
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


#ifdef _WIN32
int rest_hub(std::string pid, std::string vid, int type)
{
    (void) type;
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
        // std::string vendor_id{"FFFF"};
        // std::string product_id{ "0035" };

        // std::string vendor_id{"0590"};
        // std::string product_id{ "0028" };

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

//void reset(std::string pid, std::string vid, std::string type){
//    (void) pid;
//    (void) vid;
//    (void) type;
//}

struct hub_settings {
    int vid;
    int pid;
    int type;
};

std::string reset_hub(const hub_settings& set){
#ifdef _WIN32
    rest_hub(std::to_string(set.pid), std::to_string(set.vid), set.type);
#endif
    return std::string{};
}


char* wrapper_reset_hub(void* ctx, const char* data_in, int data_in_len, char** data_out, int* data_out_len) {
    try {
        auto fun = reinterpret_cast<std::string(*)(const std::string&)> (ctx);
//        auto input = std::string(data_in, static_cast<size_t>(data_in_len));

        json_t *root = nullptr;
        json_error_t error;

        root = json_loadb(data_in, data_in_len, 0, &error);
        if(!root) {
            throw std::invalid_argument("Error: " + std::string{error.text});
        }

        const int not_set = -1;
        int id = 0;


        std::string output = fun(input);
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
    auto reset_hub_name = std::string("example_hello");
    err = wiltoncall_register(reset_hub_name.c_str(), static_cast<int> (reset_hub_name.length()),
            reinterpret_cast<void*> (example::reset_hub), example::wrapper_reset_hub);
    if (nullptr != err) return err;


    // register 'hello' function
    auto name_hello = std::string("example_hello");
    err = wiltoncall_register(name_hello.c_str(), static_cast<int> (name_hello.length()),
            reinterpret_cast<void*> (example::hello), example::wrapper_fun);
    if (nullptr != err) return err;

    // register 'hello_again' function
    auto name_hello_again = std::string("example_hello_again");
    err = wiltoncall_register(name_hello_again.c_str(), static_cast<int> (name_hello_again.length()),
            reinterpret_cast<void*> (example::hello_again), example::wrapper_fun);
    if (nullptr != err) return err;

    // return success
    return nullptr;
}
