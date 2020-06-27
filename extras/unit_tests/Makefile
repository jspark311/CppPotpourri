###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
# Date:   2020.06.26
###########################################################################
OPTIMIZATION       = -O1
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11
CFLAGS             = -Wall

###########################################################################
# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
export CC      = $(shell which gcc)
export CXX     = $(shell which g++)
export AR      = $(shell which ar)
export SZ      = $(shell which size)
export MAKE    = $(shell which make)
export GCOV    = $(shell which gcov)

# This is where we will store compiled libs and the final output.
export BUILD_ROOT    = $(shell pwd)
export OUTPUT_PATH   = $(BUILD_ROOT)/build/

###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(BUILD_ROOT)/../../src

SOURCES_TESTS_CPP  = TestDataStructures.cpp

SOURCES_CPP = $(SOURCES_TESTS_CPP)
SOURCES_CPP += ../../src/SensorFilter.cpp
SOURCES_CPP += ../../src/StopWatch.cpp
SOURCES_CPP += ../../src/StringBuilder.cpp
SOURCES_CPP += ../../src/uuid.cpp
SOURCES_CPP += ../../src/WakeLock.cpp

CXXFLAGS  = -fsingle-precision-constant -Wdouble-promotion -fno-rtti -fno-exceptions
CXXFLAGS += -D_GNU_SOURCE

LIBS  = -L$(OUTPUT_PATH) -lstdc++ -lm


###########################################################################
# exports, consolidation....
###########################################################################
# Merge our choices and export them to the downstream Makefiles...
OBJS   = $(SOURCES_CPP:.cpp=.o)
TESTS  = $(SOURCES_TESTS_CPP:.cpp=)
COV_FILES = $(SOURCES_CPP:.cpp=.gcda) $(SOURCES_CPP:.cpp=.gcno)
CXXFLAGS += $(INCLUDES) $(OPTIMIZATION) -std=$(CPP_STANDARD)
ANALYZER_FLAGS  = $(INCLUDES)
ANALYZER_FLAGS += --std=$(CPP_STANDARD) --report-progress --force -j6

export ANALYZER_FLAGS
export CXXFLAGS

###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
#
# Eventually, the Argument class and the aparatus surrounding it will need
#   to be validated on a 64-bit build, but for now, we don't want to have
#   to worry about it.
# This is probably just a matter of having a platform-support macro to cast
#   type-sizes to integers, but will also have implications for
#   memory-management surrounding (say) 64-bit integers.
#
# Thanks, estabroo...
# http://www.linuxquestions.org/questions/programming-9/how-can-make-makefile-detect-64-bit-os-679513/
###########################################################################
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	# This is no longer required on 64-bit platforms. But it is being retained in
	#   case 32-bit problems need to be debugged.
  CXXFLAGS += -m32
endif


###########################################################################
# Parameter unification and make targets
###########################################################################

.PHONY: all


all: buildtests coverage
	@echo 'Beginning test execution...'
	@$(foreach test,$(TESTS),./$(OUTPUT_PATH)/$(test);)

buildtests: outputpath $(OBJS)
	@$(foreach test,$(TESTS),$(CXX) $(OBJS) -o $(OUTPUT_PATH)/$(test) $(CXXFLAGS) $(LDFLAGS);)
	@echo 'Built tests:  $(TESTS)'

coverage: outputpath $(TESTS)
	$(GCOV) --demangled-names --preserve-paths --source-prefix $(BUILD_ROOT) $(SOURCES_CPP)
	lcov --capture --directory ../../src --output-file $(OUTPUT_PATH)/coverage/coverage.info
	genhtml $(OUTPUT_PATH)/coverage/coverage.info --output-directory $(OUTPUT_PATH)/coverage

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

outputpath:
	mkdir -p $(OUTPUT_PATH)

clean:
	rm -rf $(OUTPUT_PATH) $(COV_FILES) $(OBJS) *.gcno *.gcda
