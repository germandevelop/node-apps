
C_COMPILER=gcc

CFLAGS=-std=c11
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
#CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
#CFLAGS += -Wno-misleading-indentation
CFLAGS += -DUNITY_INCLUDE_DOUBLE

ROOT_DIR=$(abspath ../../..)
ZOO=$(ROOT_DIR)/zoo
UNITY_ROOT=$(ZOO)/unity.unit-tests
BUILD_DIR=../build/tests
RUNNER_DIR=$(BUILD_DIR)/runners

# object_tracking.c testing
TARGET_1=object_tracking_test
SRC_FILES_1=$(UNITY_ROOT)/src/unity.c ../speed_calculator.c ../object_tracking.c ./motion_data_examples.c ./object_tracking.test.c  $(RUNNER_DIR)/object_tracking.test_runner.c
INC_DIRS=-I./ -I../ -I$(UNITY_ROOT)/src

all: clean generate_test_runners default

default:
	$(C_COMPILER) $(CFLAGS) $(INC_DIRS) $(SRC_FILES_1) -o $(BUILD_DIR)/$(TARGET_1)
	$(BUILD_DIR)/$(TARGET_1)

generate_test_runners:
	mkdir -p $(RUNNER_DIR)
	ruby $(UNITY_ROOT)/auto/generate_test_runner.rb ./object_tracking.test.c $(RUNNER_DIR)/object_tracking.test_runner.c

clean:
	rm -rf $(BUILD_DIR)

ci: CFLAGS += -Werror
ci: default
