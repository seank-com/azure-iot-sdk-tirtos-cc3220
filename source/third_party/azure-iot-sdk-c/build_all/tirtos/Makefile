#
# ==== Makefile ====
#

#
# ==== Standard Variables ====
#

include ./products.mak

#
# Set XDCOPTIONS.  Use -v for a verbose build.
#
#XDCOPTIONS=v
export XDCOPTIONS

# Set XDCARGS to some of the variables above.  XDCARGS are passed
# to the XDC build engine... which will load ns.bld... which will
# extract these variables and use them to determine what to build and which
# toolchains to use.
#
# Note that not all of these variables need to be set to something valid.
# Unfortunately, since these vars are unconditionally assigned, your build line
# will be longer and more noisy than necessary (e.g., it will include C66
# assignment even if you're just building for C64P).
#
# Some background is here:
#     http://rtsc.eclipse.org/docs-tip/Command_-_xdc#Environment_Variables
#
XDCARGS= \
    CC32XXSDK=\"$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)\" \
    BIOS=\"$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos\" \
    ti.targets.arm.elf.M4=\"$(ti.targets.arm.elf.M4)\" \


# Set XDCPATH to contain necessary repositories.
XDCPATH = $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/source;$(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/kernel/tirtos/packages;$(NS_INSTALL_DIR)/source;
export XDCPATH

#
# Set XDC executable command
# Note that XDCBUILDCFG points to the ns.bld file which uses
# the arguments specified by XDCARGS
#
XDC = $(XDC_INSTALL_DIR)/xdc $(JOBS) XDCARGS="$(XDCARGS)" XDCBUILDCFG=./azureiot.bld
XS = $(XDC_INSTALL_DIR)/xs

######################################################
## Shouldnt have to modify anything below this line ##
######################################################

all:
	@echo building packages...
	@$(XDC) -Pr ../../c-utility
	@$(XDC) -Pr ../../

clean:
	@echo cleaning packages ...
	@$(XDC) clean -Pr ../../c-utility
	@$(XDC) clean -Pr ../../
