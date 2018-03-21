# azure-iot-sdk-tirtos-cc3220
The Azure IoT C SDK for the TI RTOS cc3220

## Building

- Edit C:\ti\azure_cc3220_1_00_00_10\source\third_party\azure-iot-sdk-c\build_all\tirtos\products.mak with the following changes

```
XDC_INSTALL_DIR ?= c:/ti/xdctools_3_50_02_20_core
SIMPLELINK_CC32XX_SDK_INSTALL_DIR ?= C:/ti/simplelink_cc32xx_sdk_1_40_01_00
NS_INSTALL_DIR ?= C:/ti/azure_cc3220_1_00_00_10
ti.targets.arm.elf.M4 ?= C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.1.LTS
```

- run the following commands to build

```
c:
cd \ti\azure_cc3220_1_00_00_10\source\third_party\azure-iot-sdk-c\build_all\tirtos
path %PATH%;C:\ti\xdctools_3_50_02_20_core
gmake all
```

## Prep for WinDiff

- To get a copy of Microsoft source synced to TI's release of their sdk do the following from Git Bash

```bash
$ cd /C/azure
$ git clone https://github.com/Azure/azure-iot-sdk-c.git
$ git checkout tags/2017-06-30
$ git submodule update --init --recursive
$ git status
```

- Make sure there are no changes tracked or untracked
- Now you can compare this to the code in ```C:\ti\azure_cc3220_1_00_00_10``` and see where TI added code.
- copy ```c:\azure\azure-iot-sdk-c``` to ```c:\azure\base``` and remove ```c:\azure\base\.git```
- copy ```c:\ti\azure_cc3220_1_00_00_10``` to ```c:\ti\azure_cc3220_1_00_00_base```
- now run windiff and compare ```c:\azure\base``` with ```C:\ti\azure_cc3220_1_00_00_base\source\third_party\azure-iot-sdk-c```
- delete all files that are unchanged between ```c:\azure\base``` and ```C:\ti\azure_cc3220_1_00_00_base\source\third_party\azure-iot-sdk-c```

*It may be helpful to download [AutoHotKey](https://autohotkey.com/) and use the ```windiff.ahk``` script.*

## Create a new SDK

- copy ```C:\ti\azure_cc3220_1_00_00_10``` to ```C:\ti\azure_cc3220_1_00_00_180302```
- delete ```C:\ti\azure_cc3220_1_00_00_180302\source\third_party\azure-iot-sdk-c```
- run the following commands from Git Bash

```bash
$ cd /c/azure/azure-iot-sdk-c
$ git checkout tags/2018-03-02
$ git submodule update --init --recursive
$ git status
```

- Make sure there are no changes tracked or untracked
- copy ```c:\azure\azure-iot-sdk-c``` to ```C:\ti\azure_cc3220_1_00_00_180302\source\third_party\azure-iot-sdk-c```
- delete ```C:\ti\azure_cc3220_1_00_00_180302\source\third_party\azure-iot-sdk-c\.git```
- Now, using the WinDiff above update the new source with the changes indicated

This [pull request](https://github.com/seank-com/azure-iot-sdk-tirtos-cc3220/pull/1) demonstrates how we did it.
