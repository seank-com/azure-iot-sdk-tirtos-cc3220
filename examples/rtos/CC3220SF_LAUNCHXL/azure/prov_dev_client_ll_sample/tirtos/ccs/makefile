AZURE_CC32XX_INSTALL_DIR ?= $(abspath ../../../../../../..)

include $(AZURE_CC32XX_INSTALL_DIR)/imports.mak

KERNEL_BUILD := $(AZURE_CC32XX_INSTALL_DIR)/kernel/tirtos/builds/CC3220SF_LAUNCHXL/release

CC = "$(CCS_ARMCOMPILER)/bin/armcl"
LNK = "$(CCS_ARMCOMPILER)/bin/armcl"

OBJECTS = netwifi.obj startsntp.obj certs.obj UARTUtils.obj main_tirtos.obj prov_dev_client_ll_sample.obj CC3220SF_LAUNCHXL.obj

CONFIGPKG = $(KERNEL_BUILD)/ccs

NAME = iothub_client_sample_mqtt

CFLAGS = -I../.. "-I$(AZURE_CC32XX_INSTALL_DIR)/source" "-I$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/umqtt/inc" "-I$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/c-utility/inc" "-I$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/c-utility/inc/azure_c_shared_utility" "-I$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/provisioning_client/inc" "-I$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/iothub_client/inc" --c99 -DNET_SL -DTI "-I$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source" "-I$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages/ti/sysbios/posix" --silicon_version=7M4 --code_state=16 --little_endian --display_error_number --diag_warning=255 --diag_wrap=off --gen_func_subsections=on --float_support=vfplib --symdebug:dwarf

LFLAGS = "-l$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/iothub_client/build/tirtos/lib/iotclient_sl.aem4" "-l$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/provisioning_client/build_all/tirtos/lib/provisioning_client_sl.aem4" "-l$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/c-utility/build_all/tirtos/lib/common_sl.aem4" "-l$(AZURE_CC32XX_INSTALL_DIR)/source/ti/net/sntp/lib/sntp_sl.aem4" "-l$(AZURE_CC32XX_INSTALL_DIR)/source/ti/net/lib/network_sl.aem4" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source/ti/drivers/net/wifi/ccs/rtos/simplelink.a" "-l$(AZURE_CC32XX_INSTALL_DIR)/source/third_party/azure-iot-sdk-c/umqtt/build_all/tirtos/lib/umqtt_sl.aem4" "-l$(KERNEL_BUILD)/ccs/linker.cmd" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source/ti/display/lib/display.aem4" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source/ti/drivers/lib/drivers_cc32xx.aem4" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source/third_party/fatfs/lib/fatfs.aem4" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages/ti/dpl/lib/dpl_cc32xx.aem4" "-l$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source/ti/devices/cc32xx/driverlib/ccs/Release/driverlib.a" ../../tirtos/ccs/CC3220SF_LAUNCHXL_TIRTOS.cmd "-m$(NAME).map" --warn_sections --display_error_number --diag_wrap=off --rom_model "-i$(CCS_ARMCOMPILER)/lib" -llibc.a

all: $(NAME).out

$(CONFIGPKG)/linker.cmd $(CONFIGPKG)/compiler.opt:
	@ $(ECHOBLANKLINE)
	@ echo $(abspath $(CONFIGPKG)) is not built.
	@ echo You can build it by issuing $(MAKE) in $(abspath $(CONFIGPKG)).
	@ $(ECHOBLANKLINE)

netwifi.obj: ../../netwifi.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

startsntp.obj: ../../startsntp.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

certs.obj: ../../certs.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

UARTUtils.obj: ../../UARTUtils.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

main_tirtos.obj: ../../tirtos/main_tirtos.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

iothub_client_sample_mqtt.obj: ../../prov_dev_client_ll_sample.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

CC3220SF_LAUNCHXL.obj: ../../CC3220SF_LAUNCHXL.c $(CONFIGPKG)/compiler.opt
	@ echo Building $@
	@ $(CC) $(CFLAGS) $< --cmd_file=$(CONFIGPKG)/compiler.opt --output_file=$@

$(NAME).out: $(OBJECTS) $(CONFIGPKG)/linker.cmd
	@ echo linking...
	@ $(LNK) $(CFLAGS) -z $(OBJECTS) $(LFLAGS) -o $(NAME).out

clean:
	@ echo Cleaning...
	@ $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	@ $(RM) $(NAME).out > $(DEVNULL) 2>&1
	@ $(RM) $(NAME).map > $(DEVNULL) 2>&1
