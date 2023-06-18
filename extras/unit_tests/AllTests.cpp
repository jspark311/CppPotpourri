#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include <fstream>
#include <iostream>

#include "CppPotpourri.h"
#include "StringBuilder.h"
#include "ParsingConsole.h"
#include "ElementPool.h"
#include "GPSWrapper.h"
#include "RingBuffer.h"
#include "PriorityQueue.h"
#include "KeyValuePair.h"
#include "LightLinkedList.h"
#include "SensorFilter.h"
#include "AsyncSequencer.h"
#include "Vector3.h"
#include "StopWatch.h"
#include "uuid.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"
#include "Identity/IdentityUUID.h"
#include "Identity/Identity.h"
#include "M2MLink/M2MLink.h"


/*******************************************************************************
* Globals
*******************************************************************************/
struct timeval start_micros;


/*******************************************************************************
* Support functions
* TODO: Some of this should be subsumed by the general linux platform.
*   some of what remains should be collected into a general testing framework?
* TODO: Research testing frameworks for C++ again.
*******************************************************************************/

uint32_t randomUInt32() {
  uint32_t ret = ((uint8_t) rand()) << 24;
  ret += ((uint8_t) rand()) << 16;
  ret += ((uint8_t) rand()) << 8;
  ret += ((uint8_t) rand());
  return ret;
}

int8_t random_fill(uint8_t* buf, uint len) {
  uint i = 0;
  while (i < len) {
    *(buf + i++) = ((uint8_t) rand());
  }
  return 0;
}

/*
* Not provided elsewhere on a linux platform.
*/
long unsigned millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}

/*
* Not provided elsewhere on a linux platform.
*/
long unsigned micros() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}


/* Delay functions */
void sleep_ms(uint32_t ms) {
  struct timespec t = {(long) (ms / 1000), (long) ((ms % 1000) * 1000000UL)};
  nanosleep(&t, &t);
}

void sleep_us(uint32_t us) {
  struct timespec t = {(long) (us / 1000000), (long) ((us % 1000000) * 1000000UL)};
  nanosleep(&t, &t);
}


/**
* Prints the sizes of various types. Informational only. No test.
*/
void printTypeSizes() {
  StringBuilder output("===< Type sizes >=======================================\n-- Primitives:\n");
  output.concatf("\tvoid*                 %u\n", sizeof(void*));
  output.concatf("\tFloat                 %u\n", sizeof(float));
  output.concatf("\tDouble                %u\n", sizeof(double));
  output.concat("-- Singletons:\n");
  output.concatf("\tAbstractPlatform      %u\n", sizeof(AbstractPlatform));
  output.concatf("\tParsingConsole        %u\n", sizeof(ParsingConsole));
  output.concat("-- Elemental data structures:\n");
  output.concatf("\tStringBuilder         %u\n", sizeof(StringBuilder));
  output.concatf("\tKeyValuePair          %u\n", sizeof(KeyValuePair));
  output.concatf("\tVector3<float>        %u\n", sizeof(Vector3<float>));
  output.concatf("\tLinkedList<void*>     %u\n", sizeof(LinkedList<void*>));
  output.concatf("\tElementPool<void*>    %u\n", sizeof(ElementPool<void*>));
  output.concatf("\tPriorityQueue<void*>  %u\n", sizeof(PriorityQueue<void*>));
  output.concatf("\tRingBuffer<void*>     %u\n", sizeof(RingBuffer<void*>));
  output.concatf("\tUUID                  %u\n", sizeof(UUID));
  output.concatf("\tStopWatch             %u\n", sizeof(StopWatch));
  output.concatf("\tGPSWrapper            %u\n", sizeof(GPSWrapper));
  output.concatf("\tSensorFilter<float>   %u\n", sizeof(SensorFilter<float>));
  output.concatf("\tIdentity              %u\n", sizeof(Identity));
  output.concatf("\tIdentityUUID          %u\n", sizeof(IdentityUUID));
  output.concat("-- M2M classes:\n");
  output.concatf("\tM2MLink            %u\n", sizeof(M2MLink));
  output.concatf("\tM2MMsg             %u\n", sizeof(M2MMsg));
  output.concatf("\tM2MMsgHdr          %u\n", sizeof(M2MMsgHdr));
  output.concatf("\tM2MLinkOpts        %u\n", sizeof(M2MLinkOpts));
  printf("%s\n\n", (const char*) output.string());
}


void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}



/*******************************************************************************
* Something terrible.
*******************************************************************************/

#include "AsyncSequencerTests.cpp"
#include "StringBuilderTest.cpp"
#include "FSMTests.cpp"
#include "SchedulerTests.cpp"
#include "TestDataStructures.cpp"
#include "SensorFilterTests.cpp"
#include "ParsingConsoleTest.cpp"
#include "IdentityTest.cpp"
#include "M2MLinkTests.cpp"



/*******************************************************************************
* Top-level tests are managed using AsyncSequencer.
* The dependency graph will allow us to order tests in a bottom-up manner, with
*   more sophisticated pieces being run only if base support passes.
*******************************************************************************/
#define CHKLST_STRINGBUILDER_TESTS    0x00000001  // Everything depends on this.
#define CHKLST_FSM_TESTS              0x00000002  //
#define CHKLST_SCHEDULER_TESTS        0x00000004  //
#define CHKLST_DATA_STRUCT_TESTS      0x00000008  //
#define CHKLST_SENSORFILTER_TESTS     0x00000010  //
#define CHKLST_IDENTITY_TESTS         0x00000020  //
#define CHKLST_M2MLINK_TESTS          0x00000040  //
#define CHKLST_PARSINGCONSOLE_TESTS   0x00000080  //
#define CHKLST_ASYNC_SEQUENCER_TESTS  0x80000000  // Everything depends on this.

#define CHKLST_ALL_TESTS ( \
  CHKLST_STRINGBUILDER_TESTS | CHKLST_FSM_TESTS | CHKLST_SCHEDULER_TESTS | \
  CHKLST_DATA_STRUCT_TESTS | CHKLST_SENSORFILTER_TESTS | \
  CHKLST_IDENTITY_TESTS | CHKLST_M2MLINK_TESTS | CHKLST_PARSINGCONSOLE_TESTS | \
  CHKLST_ASYNC_SEQUENCER_TESTS)


const StepSequenceList TOP_LEVEL_TEST_LIST[] = {
  { .FLAG         = CHKLST_ASYNC_SEQUENCER_TESTS,
    .LABEL        = "ASYNC_SEQUENCER_TESTS",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == async_seq_test_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_STRINGBUILDER_TESTS,
    .LABEL        = "STRINGBUILDER_TESTS",
    .DEP_MASK     = (CHKLST_ASYNC_SEQUENCER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == stringbuilder_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_FSM_TESTS,
    .LABEL        = "FSM_TESTS",
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS | CHKLST_DATA_STRUCT_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == fsm_test_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SCHEDULER_TESTS,
    .LABEL        = "SCHEDULER_TESTS",
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS | CHKLST_DATA_STRUCT_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == scheduler_tests_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_DATA_STRUCT_TESTS,
    .LABEL        = "DATA_STRUCT_TESTS",
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS | CHKLST_ASYNC_SEQUENCER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == data_structure_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SENSORFILTER_TESTS,
    .LABEL        = "SENSORFILTER_TESTS",
    .DEP_MASK     = (CHKLST_FSM_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == sensor_filter_tests_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_IDENTITY_TESTS,
    .LABEL        = "IDENTITY_TESTS",
    .DEP_MASK     = (CHKLST_DATA_STRUCT_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == identity_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_M2MLINK_TESTS,
    .LABEL        = "M2MLINK_TESTS",
    .DEP_MASK     = (CHKLST_IDENTITY_TESTS | CHKLST_FSM_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == manuvrlink_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_PARSINGCONSOLE_TESTS,
    .LABEL        = "PARSINGCONSOLE_TESTS",
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == parsing_console_main()) ? 1:-1);  }
  },
};

AsyncSequencer checklist_unit_tests(TOP_LEVEL_TEST_LIST, (sizeof(TOP_LEVEL_TEST_LIST) / sizeof(TOP_LEVEL_TEST_LIST[0])));


/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.
  srand(time(NULL));
  gettimeofday(&start_micros, nullptr);
  printTypeSizes();

  checklist_unit_tests.requestSteps(CHKLST_ALL_TESTS);
  while (!checklist_unit_tests.request_completed() && (0 == checklist_unit_tests.failed_steps(false))) {
    checklist_unit_tests.poll();
  }
  exit_value = (checklist_unit_tests.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  checklist_unit_tests.printDebug(&report_output);
  printf("%s\n", (char*) report_output.string());

  exit(exit_value);
}
