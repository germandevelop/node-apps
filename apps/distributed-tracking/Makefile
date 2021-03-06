# This is the main makefile for RadioCountToLeds firmware

# _______________________ User overridable configuration _______________________

PROJECT_NAME            ?= distributedtracking

VERSION_MAJOR           ?= 1
VERSION_MINOR           ?= 0
VERSION_PATCH           ?= 0
VERSION_DEVEL           ?= "-dev"

DEFAULT_RADIO_CHANNEL   ?= 26

# Set device address at compile time for cases where a signature is not present
DEFAULT_AM_ADDR         ?= 1
# Set destination address at compile time
DESTINATION_GATEWAY_ADDRESS ?= 0xFFFF

# No bootloader, app starts at 0
APP_START               = 0

# Common build options - some of these should be moved to targets/boards
CFLAGS                  += -Wall -std=c99
CFLAGS                  += -ffunction-sections -fdata-sections -ffreestanding -fsingle-precision-constant -Wstrict-aliasing=0
CFLAGS                  += -DconfigUSE_TICKLESS_IDLE=0
CFLAGS                  += -D__START=main -D__STARTUP_CLEAR_BSS
CFLAGS                  += -DVTOR_START_LOCATION=$(APP_START)
LDFLAGS                 += -nostartfiles -Wl,--gc-sections -Wl,--relax -Wl,-Map=$(@:.elf=.map),--cref -Wl,--wrap=atexit
LDLIBS                  += -lgcc -lm
INCLUDES                += -Xassembler -I$(BUILD_DIR) -I.

# If set, disables asserts and debugging, enables optimization
RELEASE_BUILD           ?= 0

# Set the lll verbosity base level
CFLAGS                  += -DBASE_LOG_LEVEL=0xFFFF

# Enable debug messages
VERBOSE                 ?= 0
# Disable info messages
#SILENT                  ?= 1

# This project contains several Makefiles that reference the project root
ROOT_DIR                ?= $(abspath ../..)
ZOO                     ?= $(ROOT_DIR)/zoo
# Destination for build results
BUILD_BASE_DIR          ?= build
# Mark the default target
DEFAULT_BUILD_TARGET    ?= $(PROJECT_NAME)

# Configure how image is programmed to target device
PROGRAM_IMAGE           ?= $(BUILD_DIR)/$(PROJECT_NAME).bin
PROGRAM_DEST_ADDR       ?= $(APP_START)

# Silabs SDK location and version, due to licensing terms, the SDK is not
# distributed with this project and must be installed with Simplicity Studio.
# The variable needs to point at the subdirectory with the version number, set
# it in Makefile.private or through the environment.
SILABS_SDKDIR           ?= $(HOME)/SimplicityStudio_v4/developer/sdks/gecko_sdk_suite/v2.7

# Pull in the developer's private configuration overrides and settings
-include Makefile.private

# _______________________ Non-overridable configuration _______________________

BUILD_DIR                = $(BUILD_BASE_DIR)/$(BUILD_TARGET)
BUILDSYSTEM_DIR         := $(ZOO)/thinnect.node-buildsystem/make
PLATFORMS_DIRS          := $(ZOO)/thinnect.node-buildsystem/make $(ZOO)/thinnect.dev-platforms/make $(ZOO)/smenete-platforms/make
PHONY_GOALS             := all clean
TARGETLESS_GOALS        += clean
UUID_APPLICATION        := d709e1c5-496a-4d31-8957-f389d7fdbb71

VERSION_BIN             := $(shell printf "%02X" $(VERSION_MAJOR))$(shell printf "%02X" $(VERSION_MINOR))$(shell printf "%02X" $(VERSION_PATCH))
VERSION_STR             := "$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)"$(VERSION_DEVEL)
SW_MAJOR_VERSION        := $(VERSION_MAJOR)
SW_MINOR_VERSION        := $(VERSION_MINOR)
SW_PATCH_VERSION        := $(VERSION_PATCH)
BUILD_TIMESTAMP         := $(shell date '+%s')
IDENT_TIMESTAMP         := $(BUILD_TIMESTAMP)

# NODE_PLATFORM_DIR is used by targets to add components to INCLUDES and SOURCES
NODE_PLATFORM_DIR       := $(ZOO)/thinnect.node-platform
MW_CONVERTER_DIR       	:= $(ZOO)/mw_vehicle_detection

# ______________ Build components - sources and includes _______________________

ifneq ($(filter smnt-mb,$(MAKECMDGOALS)),)

	SOURCES += microwave_sensor.c
else
	CFLAGS += -DUSE_SENSORS_SIMULATOR

	SOURCES += fake_sensors.c
	SOURCES += serial_receiver.c
endif

SOURCES += main.c
SOURCES += radio_socket.c
SOURCES += tracking_partner.c
SOURCES += tracking_partner.mapper.c
SOURCES += tracking_session.c
SOURCES += object_tracking.c
SOURCES += speed_calculator.c
SOURCES += tracking_sensors.c
SOURCES += tracking_unit.c


# Use JTAG uart for logging, because opening JTAG port resets device momentarily
CFLAGS += -DLOG_USE_JTAG_UART
# Data will flow through the dedicated uart
#CFLAGS += -DHDLC_USE_JTAG_UART
#CFLAGS += -DHDLC_USE_EXTERNAL_UART

# FreeRTOS
FREERTOS_DIR ?= $(ZOO)/FreeRTOS-Kernel
FREERTOS_INC = -I$(FREERTOS_DIR)/include \
               -I$(ZOO)/thinnect.cmsis-freertos/CMSIS_5/CMSIS/RTOS2/Include \
               -I$(ZOO)/thinnect.cmsis-freertos/CMSIS-FreeRTOS/CMSIS/RTOS2/FreeRTOS/Include \
               -I$(ZOO)/thinnect.cmsis-freertos/$(MCU_ARCH)

FREERTOS_SRC = $(wildcard $(FREERTOS_DIR)/*.c) \
               $(ZOO)/thinnect.cmsis-freertos/CMSIS-FreeRTOS/CMSIS/RTOS2/FreeRTOS/Source/cmsis_os2.c

INCLUDES += $(FREERTOS_PORT_INC) $(FREERTOS_INC)
SOURCES += $(FREERTOS_PORT_SRC) $(FREERTOS_SRC)

# CMSIS_CONFIG_DIR is used to add default CMSIS and FreeRTOS configs to INCLUDES
CMSIS_CONFIG_DIR ?= $(ZOO)/thinnect.cmsis-freertos/$(MCU_ARCH)/config

# MoteXML components
MOTEXML_DIR ?= $(ZOO)/prolab.motexml
DTTYPES_DIR ?= $(ZOO)/prolab.dt-types
INCLUDES += -I$(MOTEXML_DIR)/lib -I$(DTTYPES_DIR)
SOURCES += $(MOTEXML_DIR)/lib/MLE.c $(MOTEXML_DIR)/lib/MLD.c $(MOTEXML_DIR)/lib/MLI.c
CFLAGS += -DLIBEXPORT=""

# Silabs EMLIB, RAIL, radio
INCLUDES += \
    -I$(SILABS_SDKDIR)/hardware/kit/common/drivers \
    -I$(SILABS_SDKDIR)/platform/halconfig/inc/hal-config \
    -I$(SILABS_SDKDIR)/platform/emlib/inc \
    -I$(SILABS_SDKDIR)/platform/emdrv/sleep/inc \
    -I$(SILABS_SDKDIR)/platform/emdrv/dmadrv/inc \
    -I$(SILABS_SDKDIR)/platform/emdrv/dmadrv/config \
    -I$(SILABS_SDKDIR)/platform/emdrv/common/inc \
    -I$(SILABS_SDKDIR)/platform/emdrv/gpiointerrupt/inc \
    -I$(SILABS_SDKDIR)/platform/radio/rail_lib/hal \
    -I$(SILABS_SDKDIR)/platform/radio/rail_lib/common \
    -I$(SILABS_SDKDIR)/platform/radio/rail_lib/protocol/ieee802154 \
    -I$(SILABS_SDKDIR)/platform/radio/rail_lib/plugin/pa-conversions \
    -I$(NODE_PLATFORM_DIR)/include

SOURCES += \
    $(SILABS_SDKDIR)/util/silicon_labs/silabs_core/queue/circular_queue.c \
    $(SILABS_SDKDIR)/hardware/kit/common/drivers/retargetserial.c \
    $(SILABS_SDKDIR)/hardware/kit/common/drivers/retargetio.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_system.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_core.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_emu.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_cmu.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_rmu.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_gpio.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_i2c.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_adc.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_prs.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_iadc.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_ldma.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_usart.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_msc.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_rtcc.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_timer.c \
    $(SILABS_SDKDIR)/platform/emlib/src/em_wdog.c \
    $(SILABS_SDKDIR)/platform/emdrv/sleep/src/sleep.c \
    $(SILABS_SDKDIR)/platform/emdrv/dmadrv/src/dmadrv.c \
    $(SILABS_SDKDIR)/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c \
    $(SILABS_SDKDIR)/platform/radio/rail_lib/hal/hal_common.c \
    $(NODE_PLATFORM_DIR)/common/platform_mutex.c \
    $(NODE_PLATFORM_DIR)/silabs/radio_rtos.c \
    $(NODE_PLATFORM_DIR)/common/radio_seqNum.c \
    $(NODE_PLATFORM_DIR)/common/sys_panic.c

# logging
ifneq ($(filter Lsmnt-mb,$(MAKECMDGOALS)),)

	CFLAGS	+= -DLOGGER_TIMESTAMP
	CFLAGS  += -DLOGGER_LDMA -DLOGGER_LDMA_DMADRV
	SOURCES += $(NODE_PLATFORM_DIR)/silabs/logger_ldma.c
	SOURCES += $(ZOO)/thinnect.lll/logging/loggers_ext.c
	INCLUDES += -I$(ZOO)/thinnect.lll/logging
else
	CFLAGS  += -DLOGGER_FWRITE
	SOURCES += $(NODE_PLATFORM_DIR)/silabs/logger_fwrite.c
	SOURCES += $(ZOO)/thinnect.lll/logging/loggers_ext.c
	INCLUDES += -I$(ZOO)/thinnect.lll/logging
endif


# device signature
INCLUDES += -I$(ZOO)/thinnect.device-signature/signature \
            -I$(ZOO)/thinnect.device-signature/area
SOURCES  += $(ZOO)/thinnect.device-signature/signature/DeviceSignature.c \
            $(ZOO)/thinnect.device-signature/area/silabs/SignatureArea.c

# Generally useful external tools
INCLUDES += -I$(ZOO)/lammertb.libcrc/include \
            -I$(ZOO)/jtbr.endianness \
            -I$(ZOO)/graphitemaster.incbin
SOURCES += $(ZOO)/lammertb.libcrc/src/crcccitt.c

# mistcomm
INCLUDES += -I$(ZOO)/thinnect.mist-comm/include -I$(ZOO)/thinnect.mist-comm/include/compat
SOURCES += $(wildcard $(ZOO)/thinnect.mist-comm/am/*.c) \
           $(wildcard $(ZOO)/thinnect.mist-comm/api/*.c) \
           $(wildcard $(ZOO)/thinnect.mist-comm/routing/*.c) \
           $(wildcard $(ZOO)/thinnect.mist-comm/cmsis/*.c)

# Microwave sensor
ifneq ($(filter smnt-mb,$(MAKECMDGOALS)),)

    	SOURCES += $(ZOO)/smenete-platforms/src/boards/smnt-mb/mw_setup_sm3.c

	INCLUDES += -I$(ZOO)/mw_vehicle_detection/include
	INCLUDES += -I$(ZOO)/mw_vehicle_detection/include/sensor
	SOURCES += $(ZOO)/mw_vehicle_detection/src/signal_normalizer.c
	SOURCES += $(ZOO)/mw_vehicle_detection/src/FFT.c
	SOURCES += $(ZOO)/mw_vehicle_detection/src/vehicle_speed_detector.c
	SOURCES += $(ZOO)/mw_vehicle_detection/src/sensor/mw_vehicle_sensor.c

	# DMA ADC
	INCLUDES += -I$(ZOO)/dmadrv-adc/include/silabs
	SOURCES += $(ZOO)/dmadrv-adc/src/silabs/adc_dmadrv.c

	SOURCES += $(NODE_PLATFORM_DIR)/common/platform_adc.c
else
	# serial layer
	SOURCES  += $(NODE_PLATFORM_DIR)/common/serial_protocol.c
	SOURCES  += $(NODE_PLATFORM_DIR)/common/hdlc_tools.c
	SOURCES  += $(NODE_PLATFORM_DIR)/silabs/serial_hdlc.c
endif

# ------------------------------------------------------------------------------

# Pull in the grunt work
include $(BUILDSYSTEM_DIR)/Makerules
# ------------------------------------------------------------------------------

# Print some build parameters
$(info DEFAULT_AM_ADDR=$(DEFAULT_AM_ADDR))
$(info DEFAULT_RADIO_CHANNEL=$(DEFAULT_RADIO_CHANNEL))
$(info DEFAULT_PAN_ID=$(DEFAULT_PAN_ID))
$(info DEFAULT_RFPOWER_DBM=$(DEFAULT_RFPOWER_DBM))
# ------------------------------------------------------------------------------

$(call passVarToCpp,CFLAGS,VERSION_MAJOR)
$(call passVarToCpp,CFLAGS,VERSION_MINOR)
$(call passVarToCpp,CFLAGS,VERSION_PATCH)
$(call passVarToCpp,CFLAGS,VERSION_STR)
$(call passVarToCpp,CFLAGS,SW_MAJOR_VERSION)
$(call passVarToCpp,CFLAGS,SW_MINOR_VERSION)
$(call passVarToCpp,CFLAGS,SW_PATCH_VERSION)
$(call passVarToCpp,CFLAGS,IDENT_TIMESTAMP)

$(call passVarToCpp,CFLAGS,DEFAULT_AM_ADDR)
$(call passVarToCpp,CFLAGS,DEFAULT_RADIO_CHANNEL)
$(call passVarToCpp,CFLAGS,DEFAULT_PAN_ID)
$(call passVarToCpp,CFLAGS,DESTINATION_GATEWAY_ADDRESS)

UUID_APPLICATION_BYTES = $(call uuidToCstr,$(UUID_APPLICATION))
$(call passVarToCpp,CFLAGS,UUID_APPLICATION_BYTES)

$(call passVarToCpp,CFLAGS,BASE_LOG_LEVEL)

# _______________________________ Project rules _______________________________

all: $(BUILD_DIR)/$(PROJECT_NAME).bin

# header.bin should be recreated if a build takes place
$(OBJECTS): $(BUILD_DIR)/header.bin

$(BUILD_DIR)/$(PROJECT_NAME).elf: Makefile | $(BUILD_DIR)

$(BUILD_DIR)/header.bin: Makefile | $(BUILD_DIR)
	$(call pInfo,Creating application header block [$@])
	$(HEADEREDIT) -c -v softtype,1 -v firmaddr,$(APP_START) -v firmsizemax,$(APP_MAX_LEN) \
	    -v version,$(VERSION_STR) -v versionbin,$(VERSION_BIN) \
	    -v uuid,$(UUID_BOARD) -v uuid2,$(UUID_PLATFORM) -v uuid3,$(UUID_APPLICATION) \
	    -v timestamp,$(BUILD_TIMESTAMP) \
	    -v name,$(PROJECT_NAME) \
	    -v size -v crc "$@"

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJECTS)
	$(call pInfo,Linking [$@])
	$(HIDE_CMD)$(CC) $(CFLAGS) $(INCLUDES) $(OBJECTS) $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(PROJECT_NAME).bin: $(BUILD_DIR)/$(PROJECT_NAME).elf
	$(call pInfo,Exporting [$@])
	$(HIDE_CMD)$(TC_SIZE) --format=Berkeley $<
	$(HIDE_CMD)$(TC_OBJCOPY) --strip-all -O binary "$<" "$@"
	$(HIDE_CMD)$(HEADEREDIT) -v size -v crc $@

$(PROJECT_NAME): $(BUILD_DIR)/$(PROJECT_NAME).bin

# _______________________________ Utility rules ________________________________

$(BUILD_DIR):
	$(call pInfo,Creating [$@])
	@mkdir -p "$@"

clean:
	$(call pInfo,Nuking everything in [$(BUILD_BASE_DIR)])
	@-rm -rf "$(BUILD_BASE_DIR)"

.PHONY: $(PHONY_GOALS)
