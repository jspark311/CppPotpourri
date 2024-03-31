###########################################################################
# Makefile for tests
# Author: J. Ian Lindsay
# Date:   2020.06.26
###########################################################################
OPTIMIZATION       = -O0 -g --coverage -fno-omit-frame-pointer
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

BUILD_ROOT    = .
OUTPUT_PATH   = $(BUILD_ROOT)/build
COVERAGE_PATH = $(OUTPUT_PATH)/coverage
ANALYZER_PATH = $(OUTPUT_PATH)/static-analysis


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(BUILD_ROOT)/../../src

# Use the line below for discrete test executables.
# TODO: Find a way to agglomerate the coverage data from the running of several
#   executables. Until then, we run all the tests in a single translation unit.
#SOURCES_TESTS_CPP  = StringBuilderTest.cpp TestDataStructures.cpp ParsingConsoleTest.cpp IdentityTest.cpp
SOURCES_TESTS_CPP  = AllTests.cpp

SOURCES_CPP  = $(wildcard ../../src/AbstractPlatform.cpp)
SOURCES_CPP += $(wildcard ../../src/AsyncSequencer.cpp)
SOURCES_CPP += $(wildcard ../../src/CppPotpourri.cpp)
SOURCES_CPP += $(wildcard ../../src/EnumeratedTypeCodes.cpp)
SOURCES_CPP += $(wildcard ../../src/MultiStringSearch.cpp)
SOURCES_CPP += $(wildcard ../../src/SensorFilter.cpp)
SOURCES_CPP += $(wildcard ../../src/StringBuilder.cpp)
SOURCES_CPP += $(wildcard ../../src/uuid.cpp)
SOURCES_CPP += $(wildcard ../../src/BusQueue/BusQueue.cpp)
SOURCES_CPP += $(wildcard ../../src/Storage/SimpleDataRecord.cpp)
SOURCES_CPP += $(wildcard ../../src/Storage/RecordTypes/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/GPSWrapper/GPSWrapper.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/Base64/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/C3PTypePipe/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/LineCoDec/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/BufferAccepter/TestFixtures/*.cpp)
SOURCES_CPP += $(wildcard ../../src/TimerTools/*.cpp)
SOURCES_CPP += $(wildcard ../../src/C3PValue/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Image/*.cpp)
SOURCES_CPP += $(wildcard ../../src/TimeSeries/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Console/*.cpp)
SOURCES_CPP += $(wildcard ../../src/cbor-cpp/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Pipes/TripleAxisPipe/*.cpp)
SOURCES_CPP += $(wildcard ../../src/M2MLink/*.cpp)
SOURCES_CPP += $(wildcard ../../src/M2MLink/LinkUtils/M2MLinkRPC/*.cpp)
SOURCES_CPP += $(wildcard ../../src/Identity/*.cpp)

# TODO: These things are not yet under unit tests, but we build them anyway so
#   we can at least get the benefit of basic compilability checking.
# TODO: Need a bit of dep-injection before this is workable...
#SOURCES_CPP += ../../src/BusQueue/I2CAdapter.cpp
#SOURCES_CPP += ../../src/BusQueue/SPIBusOp.cpp
#SOURCES_CPP += ../../src/BusQueue/SPIAdapter.cpp
#SOURCES_CPP += ../../src/BusQueue/UARTAdapter.cpp

CXXFLAGS  = $(INCLUDES) $(OPTIMIZATION)
CXXFLAGS += $(WARNING_BEHAVIOR) -D_GNU_SOURCE -fprofile-dir=$(COVERAGE_PATH)
CXXFLAGS += -fsingle-precision-constant -Wdouble-promotion -fno-rtti -fno-exceptions
CXXFLAGS += -static
CXXFLAGS += -DCONFIG_C3P_CBOR -DCONFIG_C3P_M2M_SUPPORT
CXXFLAGS += -DCONFIG_C3P_IMG_SUPPORT
CXXFLAGS += -DCONFIG_C3P_TRACE_ENABLED

LIBS  = -L. -lCppPotpourri -lstdc++ -lm

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


###########################################################################
# exports, consolidation....
###########################################################################
OBJS       = $(SOURCES_CPP:.cpp=.o)      # List of the object files to build.
DEPS       = $(SOURCES_CPP:.cpp=.d)      # List of the dependency files.
GCNO_FILES = $(OBJS:.o=.gcno)            # These are generated by coverage reporting.
GCDA_FILES = $(OBJS:.o=.gcda)            # These are generated by coverage reporting.
# TODO: Multi-binary tests.
#TESTS  = $(SOURCES_TESTS_CPP:.cpp=)

GCOV_FLAGS  = --demangled-names
GCOV_FLAGS += --object-directory $(COVERAGE_PATH)
GCOV_FLAGS += --source-prefix $(COVERAGE_PATH)
GCOV_FLAGS += --preserve-paths
GCOV_FLAGS += --branch-probabilities
GCOV_FLAGS += --branch-counts
GCOV_FLAGS += --display-progress
GCOV_FLAGS += --function-summaries
GCOV_FLAGS += --relative-only

ANALYZER_FLAGS  = $(INCLUDES)
ANALYZER_FLAGS += --std=$(CPP_STANDARD) -j6 --cppcheck-build-dir=$(ANALYZER_PATH)
ANALYZER_FLAGS += --enable=all --report-progress --force


###########################################################################
# Parameter unification and make targets
###########################################################################

.PHONY: all

all: alltests coverage check

%.o : %.cpp
	$(CXX) -std=$(CPP_STANDARD) $(CXXFLAGS) -c -o $@ $^

%.o : %.c
	$(CC) -std=$(C_STANDARD) $(CFLAGS) -c -o $(OUTPUT_PATH)/$(@F) $^

%.o : %.S
	$(CC) -std=$(C_STANDARD) -x assembler-with-cpp $(CFLAGS) -c -o $(OUTPUT_PATH)/$(@F) $^

outputpath:
	mkdir -p $(OUTPUT_PATH)
	mkdir -p $(COVERAGE_PATH)
	mkdir -p $(ANALYZER_PATH)

# Build a static library.
lib: outputpath $(OBJS)
	$(AR) -rvs libCppPotpourri.a $(wildcard $(OUTPUT_PATH)/*.o)

alltests: lib
	@$(foreach test,$(TESTS),$(CXX) -Wl,--gc-sections $(CXXFLAGS) $(LIBS) $(OBJS) $(test).cpp -o $(OUTPUT_PATH)/$(test);)
	@echo 'Built tests:  $(TESTS)'
	@echo 'Beginning test execution...'
	@$(foreach test,$(TESTS),$(OUTPUT_PATH)/$(test);)
	#@$(foreach test,$(TESTS),$(OUTPUT_PATH)/$(test) >> $(OUTPUT_PATH)/unit_test.log;)
	#cat $(OUTPUT_PATH)/unit_test.log
	@echo 'Unit testing complete.'

coverage: alltests
	#mv $(OUTPUT_PATH)/*.gcno $(GCNO_FILES) $(COVERAGE_PATH)
	$(GCOV) $(GCOV_FLAGS) $(SOURCES_CPP)
	#mv *.gcov $(COVERAGE_PATH)
	lcov --capture --directory $(COVERAGE_PATH) --output-file $(COVERAGE_PATH)/coverage.info
	genhtml $(COVERAGE_PATH)/coverage.info --output-directory $(COVERAGE_PATH)
	@echo 'Coverage measurement complete.'

check: outputpath
	@echo 'Doing static analysis...'
	find ../../src -type f \( -name \*.cpp -o -name \*.c -o -name \*.h \) -exec wc -l {} + > $(OUTPUT_PATH)/line_counts.log
	cppcheck $(SOURCES_CPP) $(ANALYZER_FLAGS) > $(OUTPUT_PATH)/report.log
	@echo 'Static analysis complete.'

clean:
	rm -rf $(OUTPUT_PATH) $(DEPS) $(OBJS) $(GCNO_FILES) $(GCDA_FILES) *.gcno *.gcda *.gcov *.o
