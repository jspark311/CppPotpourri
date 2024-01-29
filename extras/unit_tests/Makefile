###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
# Date:   2020.06.26
###########################################################################
OPTIMIZATION       = -O0 -g -fprofile-arcs -ftest-coverage --coverage -fno-omit-frame-pointer
C_STANDARD         = gnu99
CPP_STANDARD       = c++17


###########################################################################
# This is the build policy for the behavior of warnings in the library,
#   and should reflect the contractual assurances given at that level.
###########################################################################
WARNING_BEHAVIOR   = -Wall -Wextra              # Enable all non-ABI warnings.
WARNING_BEHAVIOR  += -Wno-unused-variable       # Sloppy, but not a risk.
WARNING_BEHAVIOR  += -Wno-unused-parameter      # Not a problem.
WARNING_BEHAVIOR  += -Wno-unused-value          # Sometimes things are volatile.
WARNING_BEHAVIOR  += -Wno-ignored-qualifiers    # We might want to ignore qualifiers.
WARNING_BEHAVIOR  += -Wno-implicit-fallthrough  # Code often reads better with this.

# The warnings below will be considered build-failing problems.
#WARNING_BEHAVIOR  += -Werror   # One day....
#WARNING_BEHAVIOR  += -Werror=sign-compare
#WARNING_BEHAVIOR  += -Werror=strict-aliasing          # Type-punning needs to be explicitly noted.
WARNING_BEHAVIOR  += -Werror=delete-non-virtual-dtor  # Unacceptable risk that is easy to close.
WARNING_BEHAVIOR  += -Werror=format-extra-args        # Something else is broken.
WARNING_BEHAVIOR  += -Werror=type-limits              # Exposes bad assumptions about type suitability.
WARNING_BEHAVIOR  += -Werror=parentheses              # Algebra and C aren't the same thing.


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

SOURCES_CPP  = ../../src/*.cpp
SOURCES_CPP += ../../src/BusQueue/BusQueue.cpp
SOURCES_CPP += ../../src/Storage/SimpleDataRecord.cpp
SOURCES_CPP += ../../src/Storage/RecordTypes/*.cpp
SOURCES_CPP += ../../src/Pipes/BufferAccepter/GPSWrapper/GPSWrapper.cpp
SOURCES_CPP += ../../src/Pipes/BufferAccepter/*.cpp
SOURCES_CPP += ../../src/Pipes/BufferAccepter/Base64/*.cpp
SOURCES_CPP += ../../src/Pipes/BufferAccepter/LineCoDec/*.cpp
SOURCES_CPP += ../../src/Pipes/BufferAccepter/TestFixtures/*.cpp
SOURCES_CPP += ../../src/TimerTools/*.cpp
SOURCES_CPP += ../../src/C3PValue/*.cpp
SOURCES_CPP += ../../src/Image/*.cpp
SOURCES_CPP += ../../src/TimeSeries/*.cpp
SOURCES_CPP += ../../src/Console/*.cpp
SOURCES_CPP += ../../src/cbor-cpp/*.cpp
SOURCES_CPP += ../../src/Pipes/TripleAxisPipe/*.cpp
SOURCES_CPP += ../../src/M2MLink/*.cpp
SOURCES_CPP += ../../src/Identity/*.cpp

# TODO: These things are not yet under unit tests, but we build them anyway so
#   we can at least get the benefit of basic compilability checking.
# TODO: Need a bit of dep-injection before this is workable...
#SOURCES_CPP += ../../src/BusQueue/I2CAdapter.cpp
#SOURCES_CPP += ../../src/BusQueue/SPIBusOp.cpp
#SOURCES_CPP += ../../src/BusQueue/SPIAdapter.cpp
#SOURCES_CPP += ../../src/BusQueue/UARTAdapter.cpp

CXXFLAGS  = $(WARNING_BEHAVIOR) -D_GNU_SOURCE -fprofile-dir=$(COVERAGE_PATH)
CXXFLAGS += -fsingle-precision-constant -Wdouble-promotion -fno-rtti -fno-exceptions
CXXFLAGS += -DCONFIG_C3P_CBOR -DCONFIG_C3P_M2M_SUPPORT
CXXFLAGS += -DCONFIG_C3P_IMG_SUPPORT
CXXFLAGS += -DCONFIG_C3P_TRACE_ENABLED

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
