#*******************************************************************************
#
# Makefile - Rules for building the SAIL library.
#
#  Copyright (C) 2015 Texas Instruments Incorporated - http:#www.ti.com/
#
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#*******************************************************************************

#
# Include the common make definitions.
#
include Makefile.defs

.PHONY: ccs_tirtos ccs_freertos help all ccs iar gcc \
clean_all clean clean_ccs clean_iar clean_gcc \
setlocal enabledelayedexpansion

ifndef BUILD_TOOL
abort:
	@echo  "-------------------------------------------------------------------"
	@echo  "Build Tool (ccs,iar,gcc,all) not specified"
	@echo  "-------------------------------------------------------------------"
	make help
endif


MCU_PLATFORMS      ?= cc32xx msp432 cc26x0 cc13x0
MCU_TOOLS          ?= ccs gcc ewarm
CURRENT_BUILD_TOOL ?=

#
# The rule to create the sail library
#
build: $(BUILDDIR) $(OBJ) .$(LIBRARY)

#
# The rule to display the available options
#
help:
	@echo Makefile to build SAIL component for specified platform and tool
	@echo   goal                       description
	@echo  -------------------------------------------------------------------------------------------
	@echo  "make all                   Builds using CCS, IAR and GCC for all supported platforms"
	@echo  "make ccs          MCU=<>   Builds sail library for tirtos and freertos using CCS for given SimpleLink MCU"
	@echo  "make ccs_tirtos   MCU=<>   Builds sail library for tirtos using CCS for given SimpleLink MCU"
	@echo  "make ccs_freertos MCU=<>   Builds sail library for freertos using CCS for given SimpleLink MCU"
	@echo  "make iar          MCU=<>   Builds sail library for tirtos and freertos using IAR for given SimpleLink MCU"
	@echo  "make iar_tirtos   MCU=<>   Builds sail library for tirtos using IAR for given SimpleLink MCU"
	@echo  "make iar_freertos MCU=<>   Builds sail library for freertos using IAR for given SimpleLink MCU"
	@echo  "make gcc          MCU=<>   Builds sail library for tirtos and freertos using gcc for given SimpleLink MCU"
	@echo  "make gcc_tirtos   MCU=<>   Builds sail library for tirtos using IAR for given SimpleLink MCU"
	@echo  "make gcc_freertos MCU=<>   Builds sail library for freertos using IAR for given SimpleLink MCU"
	@echo  "make gcc          MCU=<>   Builds using GCC for given SimpleLink MCU"
	@echo  "make clean_all             Cleans lib using CCS, IAR and GCC for all platforms"
	@echo  "make clean_ccs    MCU=<>   Cleans sail lib using CCS for specified SimpleLink MCU"
	@echo  "make clean_iar    MCU=<>   Cleans sail lib using IAR for specified SimpleLink MCU"
	@echo  "make clean_gcc    MCU=<>   Cleans sail lib using GCC for specified SimpleLink MCU"
	@echo  "make help                  Displays this description"
	@echo  -------------------------------------------------------------------------------------------
	@echo  "Supported SimpleLink MCUs are - msp432, cc3220, cc26x0 cc13x0"
	@echo  "For Eg.-    make MCU=msp432 ccs    ; MCU can take one value at a time"
	@echo  -------------------------------------------------------------------------------------------

#
# The rule to create the build directory
#
$(BUILDDIR):
	@$(MKDIR) $(BUILDDIR)

#
# The rule to clean the build and output directories
#
clean:
	@$(RMDIR) $(BUILDDIR)
#
# The rule to build the libraries using CCS, IAR and GCC tools
#
#
.PHONY: $(MCU_PLATFORMS)
all: $(MCU_PLATFORMS)

$(MCU_PLATFORMS):
	@$(MAKE) BUILD_TOOL=ccs RTOS=tirtos MCU=$@
	@$(MAKE) BUILD_TOOL=gcc RTOS=tirtos MCU=$@
	@$(MAKE) BUILD_TOOL=ewarm RTOS=tirtos MCU=$@
	@if [ $@ != "cc26x0" ] && [ $@ != "cc13x0" ]; then                        \
		$(MAKE) BUILD_TOOL=ccs RTOS=freertos MCU=$@    ;\
		$(MAKE) BUILD_TOOL=gcc RTOS=freertos MCU=$@    ;\
		$(MAKE) BUILD_TOOL=ewarm RTOS=freertos MCU=$@  ;\
	fi



#
# The rule to clean the libraries using CCS, IAR and GCC tool
#
clean_all: clean_ccs clean_iar clean_gcc

#
# The rule to build the os library using CCS tool
#
ccs:
ifeq ($(MCU),$(filter $(MCU),cc32xx msp432))
	$(MAKE) BUILD_TOOL=ccs RTOS=freertos
	$(MAKE) BUILD_TOOL=ccs RTOS=tirtos
else
	$(MAKE) BUILD_TOOL=ccs RTOS=tirtos
endif

ccs_freertos:
	$(MAKE) BUILD_TOOL=ccs RTOS=freertos

ccs_tirtos:
	$(MAKE) BUILD_TOOL=ccs RTOS=tirtos

#
# The rule to clean the os library using CCS tool
#
clean_ccs:
	$(MAKE) BUILD_TOOL=ccs RTOS=freertos clean
	$(MAKE) BUILD_TOOL=ccs RTOS=tirtos clean
#
# The rule to build the os library using IAR tool
#
iar:
ifeq ($(MCU),$(filter $(MCU),cc32xx msp432))
	$(MAKE) BUILD_TOOL=ewarm RTOS=freertos
	$(MAKE) BUILD_TOOL=ewarm RTOS=tirtos
else
	$(MAKE) BUILD_TOOL=ewarm RTOS=tirtos
endif

iar_tirtos:
	$(MAKE) BUILD_TOOL=ewarm RTOS=tirtos

iar_freertos:
	$(MAKE) BUILD_TOOL=ewarm RTOS=freertos


#
# The rule to clean the os library using IAR tool
#
clean_iar:
	$(MAKE) BUILD_TOOL=ewarm RTOS=freertos clean
	$(MAKE) BUILD_TOOL=ewarm RTOS=tirtos clean

#
# The rule to build the os library using GCC tool
#
gcc:
ifeq ($(MCU),$(filter $(MCU),cc32xx msp432))
	$(MAKE) BUILD_TOOL=gcc RTOS=freertos
	$(MAKE) BUILD_TOOL=gcc RTOS=tirtos
else
	$(MAKE) BUILD_TOOL=gcc RTOS=tirtos
endif

gcc_tirtos:
	$(MAKE) BUILD_TOOL=gcc RTOS=tirtos

gcc_freertos:
	$(MAKE) BUILD_TOOL=gcc RTOS=freertos

#
# The rule to clean the os library using GCC tool
#
clean_gcc:
	$(MAKE) BUILD_TOOL=gcc RTOS=freertos clean
	$(MAKE) BUILD_TOOL=gcc RTOS=tirtos clean
