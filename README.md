# azure-iot-sdk-tirtos-cc3220
The Azure IoT C SDK for the TI RTOS cc3220

# Building

- Edit C:\ti\azure_cc3220_1_00_00_10\source\third_party\azure-iot-sdk-c\build_all\tirtos\products.mak with the following changes

```
XDC_INSTALL_DIR ?= c:/ti/xdctools_3_50_02_20_core
SIMPLELINK_CC32XX_SDK_INSTALL_DIR ?= C:/ti/simplelink_cc32xx_sdk_1_40_01_00
NS_INSTALL_DIR     ?= C:\ti\azure_cc3220_1_00_00_10
ti.targets.arm.elf.M4 ?= C:\ti\ccsv8\tools\compiler\ti-cgt-arm_18.1.1.LTS
```

- run the following commands to build

```
c:
cd \ti\azure_cc3220_1_00_00_10\source\third_party\azure-iot-sdk-c\build_all\tirtos
path %PATH%;C:\ti\xdctools_3_50_02_20_core
gmake all
```
