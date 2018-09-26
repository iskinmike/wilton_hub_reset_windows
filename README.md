# wilton_hub_reset_windows module

Used to reset smarthub D-LINK DUB-H4.

Tested and not working with hubs:
 - Ritmix CR-3402

ATTENTION!!!
After succesfull reset call you need to wait some time before hub will be powered again.

### Build
Environment variable WILTON_HOME need s to be setted at wilton distributive directory.

Build for x64 architect:
```sh
	cmake .. -G "Visual Studio 12 2013 Win64"
	cmake --build . --config Release
```

Build for x32 architect:
```sh
	cmake .. -G "Visual Studio 12 2013 Win64" -DARCHITECT_X32
	cmake --build . --config Release
```


### Functions

| Function | Description |
| ---- | ---- |
| **reset\_hub(params{vid, pid, type})**| Tryes to call reset to usb device with specified pid, vid. Type defines device search category. "hub" by default. |

##### Params example
```javascript
{
	"vid": 0x05e4, // Vendor Id. Required
	"pid": 0x0608, // Product Id. Required
	"type":"hub"   // Specifies device search category. Values "hub" or "usb". Optional. "hub" value by default.
}
```

##### Errors output.
In case of error returns json string:
 ```javascript
{
	"error": "Error description" 
}
```