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
  - *This [pull request](https://github.com/seank-com/azure-iot-sdk-tirtos-cc3220/pull/1) demonstrates how we did it.*
- Next take what you learned and create build scripts for new features
  - *This [pull request](https://github.com/seank-com/azure-iot-sdk-tirtos-cc3220/pull/2) demonstrates how we did it.*

## Build Provisioning Sample

- Export ```iothub_client_sample_mqtt...``` to your code composer workspace and build and test it.
- Close Code Composer
- Rename SDKs. This makes testing the new SDK easier. This way all the targets of the samples in your code composer workspace will not need to be updated.
  - Rename ```c:\ti\azure_cc3220_1_00_00_10``` to ```c:\ti\azure_cc3220_1_00_00_10_old```
  - Rename ```c:\ti\azure_cc3220_1_00_00_180302``` to ```c:\ti\azure_cc3220_1_00_00_10```
- Open Code composer
- Right click the ```Iothub_client_sample_mqtt...``` in the Project Explorer and select Copy
- Right click in the empty area of the Project Explorere and select Paste
- Enter ```prov_client_ll_sample_CC3220SF_LAUNCHXL_tirtos_ccs``` for the name
- Rename ```iothub_client_sample_mqtt.h``` to ```prov_dev_client_ll_sample.h```
- Open ```prov_dev_client_ll_sample.h``` and change ```iothub_client_sample_mqtt_run``` to ```prov_dev_client_ll_run```
- Rename ```iothub_client_sample_mqtt.c``` to ```prov_dev_client_ll_sample.c```
- Open ```prov_dev_client_ll_sample.c``` and
  - Delete everything
  - Paste in the contents of ```C:\ti\azure_cc3220_1_00_00_10\source\third_party\azure-iot-sdk-c\provisioning_client\samples\prov_dev_client_ll_sample\prov_dev_client_ll_sample.c```
  - Rename ```main``` to ```prov_dev_client_ll_run```
- Open ```main_tirtos.c```
  - Change ```iothub_client_sample_mqtt_run``` to ```prov_dev_client_ll_run```
- Make other changes as necessary to compile
  - *This [pull request](https://github.com/seank-com/azure-iot-sdk-tirtos-cc3220/pull/3) demonstrates how we did it.*

## Deploy Provisioning Sample

- Plug in CC3220 to USB
- Launch **Device Manager** and look for **User USRT** under **Ports (COM & LPT)** and note the COM port
- Launch UniFlash
  - Click **Serial** to the right of **CC3220SF-LAUNCHXL** in list
  - Click **Start Image Creator**
  - Click **New Project**
    - Enter ```ProvDemo``` for **Name**
    - Select ```CC3220SF``` for **Device Type**
    - Click **Device Mode** until you see ```Develop```
    - Press **Create Project**
  - Click **User Files** in the tree to the left
  - Click **Add File** document icon in middle
    - Navigate to ```C:\ti\simplelink_cc32xx_sdk_1_40_01_00\tools\cc32xx_tools\certificate-playground```
      - Select ```dummy-root-ca-cert``` and click **Write**
  - Repeat for the following
    - ```dummy-trusted-ca-cert```
    - ```dummy-trusted-cert```
  - Click **Service Pack** in the tree to the left
    - Click **Browse** and navigate to ```C:\ti\simplelink_cc32xx_sdk_1_40_01_00\tools\cc32xx_tools\servicepack-cc3x20```
    - Select ```sp_3.4.0.0_2.0.0.0_2.2.0.5.bin```
  - Click **Trusted Root-Certificate Catalog**
    - Uncheck **Use default Trusted Root-Certifcate Catalog**
    - For **Source File**
      - Navigate to ```C:\ti\simplelink_cc32xx_sdk_1_40_01_00\tools\cc32xx_tools\certificate-playground```
      - Select ```certcatalogPlayGround20160911.lst```
    - For **Signature Source File**
      - Navigate to ```C:\ti\simplelink_cc32xx_sdk_1_40_01_00\tools\cc32xx_tools\certificate-playground```
      - Select ```certcatalogPlayGround20160911.lst.signed.bin```
    - Click **UserFiles**
    - Select ```Select MCU Image``` from **Action** dropdown
    - Click **Browse**
      - Navigate to the project in your workspace ```C:\Users\seank\workspace_v8\prov_dev_client_ll_sample_CC3220SF_LAUNCHXL_tirtos_ccs\Debug```
      - Select ```iothub_client_sample_mqtt_CC3220SF_LAUNCHXL_tirtos_ccs.bin```
      - Click **Browse**
      - Navigate to ```C:\ti\simplelink_cc32xx_sdk_1_40_01_00\tools\cc32xx_tools\certificate-playground```
      - Select ```dummy-root-ca-cert-key```
      - Select ```dummy-root-ca-cert``` in **Certication File Name** dropdown
      - Click **Write**
    - Click **Connect**
    - Click **Generate Image**
    - Click **Program Image (Create & Program)**


  - Launch **PuTTY**
    - Select ```Serial```
    - Change **Serial line** to correct COM port
    - Change **Speed** to ```115200```
