###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
# Date:   2020.06.26
###########################################################################
OPTIMIZATION       = -O0 -g -fprofile-arcs -ftest-coverage --coverage
C_STANDARD         = gnu99
CPP_STANDARD       = c++17
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
export OUTPUT_PATH   = $(BUILD_ROOT)/build
export COVERAGE_PATH = $(OUTPUT_PATH)/coverage
export ANALYZER_PATH = $(OUTPUT_PATH)/static-analysis


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(BUILD_ROOT)/../../src

# Use the line below for discrete test executables.
# TODO: Find a way to agglomerate the coverage data from the running of several
#   executables. Until then, we run all the tests in a single translation unit.
#SOURCES_TESTS_CPP  = StringBuilderTest.cpp TestDataStructures.cpp ParsingConsoleTest.cpp IdentityTest.cpp
SOURCES_TESTS_CPP  = AllTests.cpp

SOURCES_CPP  = ../../src/SensorFilter.cpp
SOURCES_CPP += ../../src/StopWatch.cpp
SOURCES_CPP += ../../src/StringBuilder.cpp
SOURCES_CPP += ../../src/EnumeratedTypeCodes.cpp
SOURCES_CPP += ../../src/KeyValuePair.cpp
SOURCES_CPP += ../../src/AsyncSequencer.cpp
SOURCES_CPP += ../../src/uuid.cpp
SOURCES_CPP += ../../src/BusQueue.cpp
SOURCES_CPP += ../../src/GPSWrapper.cpp
SOURCES_CPP += ../../src/ParsingConsole.cpp
SOURCES_CPP += ../../src/Quaternion.cpp
SOURCES_CPP += ../../src/Storage/SimpleDataRecord.cpp
SOURCES_CPP += ../../src/AbstractPlatform.cpp
SOURCES_CPP += ../../src/BufferAccepter/*.cpp
SOURCES_CPP += ../../src/BufferAccepter/Base64/*.cpp
SOURCES_CPP += ../../src/BufferAccepter/LineCoDec/*.cpp
SOURCES_CPP += ../../src/BufferAccepter/TestFixtures/*.cpp
SOURCES_CPP += ../../src/Image/*.cpp
SOURCES_CPP += ../../src/cbor-cpp/*.cpp
SOURCES_CPP += ../../src/C3PEvents/*.cpp
SOURCES_CPP += ../../src/TripleAxisPipe/*.cpp
SOURCES_CPP += ../../src/M2MLink/*.cpp
SOURCES_CPP += ../../src/Identity/*.cpp

CXXFLAGS  = -fsingle-precision-constant -Wdouble-promotion -fno-rtti -fno-exceptions
CXXFLAGS += -D_GNU_SOURCE -fprofile-dir=$(COVERAGE_PATH)
CXXFLAGS += -DCONFIG_C3P_CBOR -DCONFIG_C3P_M2M_SUPPORT
CXXFLAGS += -DCONFIG_C3P_IMG_SUPPORT

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
ANALYZER_FLAGS += --std=$(CPP_STANDARD) -j6 --cppcheck-build-dir=$(ANALYZER_PATH)
ANALYZER_FLAGS += --enable=all --report-progress --force

###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
#
# TODO: we need to make both 32-bit and 64-bit builds. The library is used under both conditions.
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

export ANALYZER_FLAGS
export CXXFLAGS


###########################################################################
# Parameter unification and make targets
###########################################################################

.PHONY: all

all: alltests coverage check

alltests: outputpath
	@$(foreach test,$(TESTS),$(CXX) -Wl,--gc-sections -static $(CXXFLAGS) $(LIBS) $(SOURCES_CPP) $(test).cpp -o $(OUTPUT_PATH)/$(test);)
	@echo 'Built tests:  $(TESTS)'
	@echo 'Beginning test execution...'
	@$(foreach test,$(TESTS),$(OUTPUT_PATH)/$(test);)
	#@$(foreach test,$(TESTS),$(OUTPUT_PATH)/$(test) >> $(OUTPUT_PATH)/unit_test.log;)
	#cat $(OUTPUT_PATH)/unit_test.log
	@echo 'Unit testing complete.'

coverage: outputpath
	$(GCOV) --object-directory $(COVERAGE_PATH) -m --preserve-paths --demangled-names $(SOURCES_CPP)
	mv *.gcno $(COVERAGE_PATH)
	lcov --capture --directory $(COVERAGE_PATH) --output-file $(COVERAGE_PATH)/coverage.info
	genhtml $(COVERAGE_PATH)/coverage.info --output-directory $(COVERAGE_PATH)
	@echo 'Coverage measurement complete.'

lib: $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/CppPotpourri.a $(OBJS)

outputpath:
	mkdir -p $(OUTPUT_PATH)
	mkdir -p $(COVERAGE_PATH)
	mkdir -p $(ANALYZER_PATH)

check: outputpath
	@echo 'Doing static analysis...'
	find ../../src -type f \( -name \*.cpp -o -name \*.c -o -name \*.h \) -exec wc -l {} + > $(OUTPUT_PATH)/line_counts.log
	cppcheck $(SOURCES_CPP) $(ANALYZER_FLAGS) > $(OUTPUT_PATH)/report.log
	@echo 'Static analysis complete.'

clean:
	rm -rf $(OUTPUT_PATH) $(OBJS) *.gcno *.gcda *.gcov *.o
