#
#  ======== makefile ========
#

AZURE_CC32XX_INSTALL_DIR ?= ../../../../../..

include $(AZURE_CC32XX_INSTALL_DIR)/imports.mak

ifneq ($(BIOS_INSTALL_DIR),)
    XDCPATH = $(TIDRIVERS_INSTALL_DIR)/source;$(TIDRIVERS_INSTALL_DIR)/kernel/tirtos/packages;$(BIOS_INSTALL_DIR)/packages;$(UIA_INSTALL_DIR)/packages;
else
    XDCPATH = $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source;$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages;$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages/ti/sysbios/posix;
endif

#
# TI-RTOS application configuration file
#
CFG_FILE   ?= ../release.cfg

#
# Platform to build
#
PLATFORM   ?= ti.platforms.simplelink:CC3220S

#
# Compiler options for the build
#
CFLAGS =

#
# Set XDCOPTIONS.  Use -v for a verbose build.
#
#XDCOPTIONS=v
export XDCOPTIONS

#######################################################
## Shouldn't have to modify anything below this line ##
#######################################################
XS = $(XDC_INSTALL_DIR)/xs
XDC = $(XDC_INSTALL_DIR)/xdc

all:
	@ $(XS) --xdcpath="$(XDCPATH)" xdc.tools.configuro -o ../iar \
            -t iar.targets.arm.M4 -p $(PLATFORM) -c "$(IAR_ARMCOMPILER)" \
            --compileOptions "$(CFLAGS)" $(CFG_FILE)

clean:
	@ echo cleaning ...
ifeq ($(wildcard package.xdc), package.xdc)
	@ $(XDC) clean > $(DEVNULL) 2>&1
endif
	@ $(RM) compiler.opt* > $(DEVNULL) 2>&1
	@ $(RM) config.bld > $(DEVNULL) 2>&1
	@ $(RM) custom.mak > $(DEVNULL) 2>&1
	@ $(RM) package.* > $(DEVNULL) 2>&1
	@ $(RM) linker.cmd > $(DEVNULL) 2>&1
