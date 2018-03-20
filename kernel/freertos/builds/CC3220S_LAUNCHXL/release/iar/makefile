#
#  ======== makefile ========
#

AZURE_CC32XX_INSTALL_DIR ?= ../../../../../..

include $(AZURE_CC32XX_INSTALL_DIR)/imports.mak

ifneq ($(CC32XXWARE_INSTALL_DIR),)
    DRIVERLIB_DIR = $(CC32XXWARE_INSTALL_DIR)
    FREERTOSPORT_DIR = $(TIDRIVERS_INSTALL_DIR)
else
    DRIVERLIB_DIR = $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)
    FREERTOSPORT_DIR = $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)
endif

VPATH = $(FREERTOS_INSTALL_DIR)/FreeRTOS/Source:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/IAR/ARM_CM3:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/MemMang:$(FREERTOSPORT_DIR)/kernel/freertos/posix:$(FREERTOSPORT_DIR)/kernel/freertos/dpl:$(FREERTOSPORT_DIR)/kernel/freertos/startup

CC = "$(IAR_ARMCOMPILER)/bin/iccarm"
ASM = "$(IAR_ARMCOMPILER)/bin/iasmarm"
AR = "$(IAR_ARMCOMPILER)/bin/iarchive"

CFLAGS = -I.. "-I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/include" "-I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/IAR/ARM_CM3" "-I$(FREERTOSPORT_DIR)/source" "-I$(DRIVERLIB_DIR)/source" "-I$(IAR_ARMCOMPILER)/include" -Dewarm  --debug --silent --aeabi --cpu=Cortex-M4 --diag_suppress=Pa050,Go005 --endian=little -e --thumb

AFLAGS = -I.. "-I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/include" -S --cpu Cortex-M4 --endian little --thumb

CFILES = \
    croutine.c \
    event_groups.c \
    list.c \
    queue.c \
    tasks.c \
    timers.c \
    port.c \
    heap_4.c \
    clock_freertos.c \
    memory_freertos.c \
    mqueue_freertos.c \
    pthread_barrier_freertos.c \
    pthread_cond_freertos.c \
    pthread_freertos.c \
    pthread_mutex_freertos.c \
    pthread_rwlock_freertos.c \
    sched_freertos.c \
    semaphore_freertos.c \
    sleep_freertos.c \
    timer_freertos.c \
    ClockP_freertos.c \
    DebugP_freertos.c \
    MutexP_freertos.c \
    SemaphoreP_freertos.c \
    SystemP_freertos.c \
    HwiPCC32XX_freertos.c \
    PowerCC32XX_freertos.c \
    startup_cc32xx_iar.c

AFILES = \
    portasm.s

OBJCFILES = $(CFILES:.c=.obj)
OBJAFILES = $(AFILES:.s=.obj)

freertos.lib: $(OBJCFILES) $(OBJAFILES)
	@ echo ar $@ ...
	@ $(RM) $@ > $(DEVNULL) 2>&1
	@ $(AR) --silent -o $@ $(OBJCFILES) $(OBJAFILES)

$(CFILES) $(AFILES):
	@ echo "Error: can't find $@, please ensure FREERTOS_INSTALL_DIR in $(realpath $(SIMPLELINK_CC32XX_SDK_INSTALL_DIR)/imports.mak) is correct (it is '$(FREERTOS_INSTALL_DIR)')"
	@ exit 1

%.obj:%.c
	@ echo cc $@ ...
	@ $(CC) $(CFLAGS) $< -o $@

%.obj:%.s
	@ echo asm $@ ...
	@ $(ASM) $(AFLAGS) $< -o $@

clean:
	@ echo cleaning ...
	@ $(RM) *.obj > $(DEVNULL) 2>&1
	@ $(RM) *.lib > $(DEVNULL) 2>&1
