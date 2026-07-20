#pragma once

#include <winioctl.h>

namespace UchihaSpoofer::Driver {

// IOCTL Constants - Định nghĩa các lệnh điều khiển driver kernel
#define IOCTL_SPOOF_CPU  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SPOOF_BIOS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Device name
const char* DRIVER_DEVICE_NAME = "\\\\.\\UchihaDriverDevice";
const char* DRIVER_SERVICE_NAME = "UchihaDriver";
const char* DRIVER_TEMP_FILE = "UchihaDriver_temp.sys";

} // namespace UchihaSpoofer::Driver
