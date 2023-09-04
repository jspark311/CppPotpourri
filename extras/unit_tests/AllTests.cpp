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

/*
Global unit-testing TODO list:

TODO: Research testing frameworks for C++ again.

TODO: If you won't do that, this is at least a good place to start doing some
  dependency injection for GPIO. If we override the weak references in
  AbstractPlatform, we can fake pin behaviors from a separate thread.

TODO: About that... This program is presumably being run under linux, and so we
  have threads. Apart from mortality, there is no good reason that some directed
  concurrency testing of modules isn't already being done. This won't be an
  _exact_ simulation of ISR behavior, but it is close enough to catch almost
  everything that would happen in that context that C3P is concerned about.
*/

/*******************************************************************************
* Globals
*******************************************************************************/
struct timeval start_micros;


/*******************************************************************************
* Support functions
* TODO: Some of this should be subsumed by the general linux platform.
*   some of what remains should be collected into a general testing framework?
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


/*******************************************************************************
* Test reporting functions that are intended to be called from unit tests...
*******************************************************************************/

int generate_random_text_buffer(StringBuilder* buf, const int RANDOM_BUF_LEN) {
  int ret = 0;
  if ((RANDOM_BUF_LEN > 0) && (nullptr != buf)) {
    uint8_t tmp_buf[RANDOM_BUF_LEN+1] = {0, };
    random_fill(tmp_buf, RANDOM_BUF_LEN);
    for (uint32_t i = 0; i < RANDOM_BUF_LEN; i++) {
      tmp_buf[i] = (0x30 + (tmp_buf[i] % 0x4E));
    }
    ret = RANDOM_BUF_LEN;
    buf->concat(tmp_buf, RANDOM_BUF_LEN);
  }
  return ret;
}


void printTestFailure(const char* module, const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED test: %s.\n", module, test);
  printf("*********************************************\n");
}


/*******************************************************************************
* Something terrible.
* Textual inclusion of CPP files until a testing framework is writen or adopted.
*******************************************************************************/

#include "AsyncSequencerTests.cpp"
#include "StringBuilderTest.cpp"
#include "RingBufferTests.cpp"
#include "KVPTests.cpp"
#include "LinkedListTests.cpp"
#include "FSMTests.cpp"
#include "SchedulerTests.cpp"
#include "TestDataStructures.cpp"
#include "BufferAccepterTests.cpp"
#include "SensorFilterTests.cpp"
#include "ParsingConsoleTest.cpp"
#include "IdentityTest.cpp"
#include "M2MLinkTests.cpp"


/*******************************************************************************
* Aggregation functions that call pieces from each unit test source file.
* Brittle, ugly, hard to understand. Recommend me a test framework...
* I want something that can do dependency injection with a bit more grace than
*   the dabbling I've done for the platform.
*******************************************************************************/

/**
* Prints the sizes of various types. Informational only. No test.
*/
void printTypeSizes() {
  printf("===< Type sizes >=======================================\n-- Primitives:\n");
  printf("\tvoid*                    %u\t%u\n", sizeof(void*),  alignof(void*));
  printf("\tFloat                    %u\t%u\n", sizeof(float),  alignof(float));
  printf("\tDouble                   %u\t%u\n", sizeof(double), alignof(double));
  printf("-- C3P types:\n");
  printf("\tAbstractPlatform         %u\t%u\n", sizeof(AbstractPlatform),     alignof(AbstractPlatform));
  printf("\tVector3<float>           %u\t%u\n", sizeof(Vector3<float>),       alignof(Vector3<float>));
  printf("\tLinkedList<void*>        %u\t%u\n", sizeof(LinkedList<void*>),    alignof(LinkedList<void*>));
  printf("\tElementPool<void*>       %u\t%u\n", sizeof(ElementPool<void*>),   alignof(ElementPool<void*>));
  printf("\tPriorityQueue<void*>     %u\t%u\n", sizeof(PriorityQueue<void*>), alignof(PriorityQueue<void*>));
  printf("\tSensorFilter<float>      %u\t%u\n", sizeof(SensorFilter<float>),  alignof(SensorFilter<float>));
  printf("\tUUID                     %u\t%u\n", sizeof(UUID),             alignof(UUID));
  printf("\tStopWatch                %u\t%u\n", sizeof(StopWatch),        alignof(StopWatch));
  printf("\tGPSWrapper               %u\t%u\n", sizeof(GPSWrapper),       alignof(GPSWrapper));
  printf("\tIdentity                 %u\t%u\n", sizeof(Identity),         alignof(Identity));
  printf("\tIdentityUUID             %u\t%u\n", sizeof(IdentityUUID),     alignof(IdentityUUID));
  print_types_async_sequencer();
  print_types_stringbuilder();
  print_types_ringbuffer();
  print_types_buffer_accepter();
  print_types_parsing_console();
  print_types_scheduler();
  print_types_state_machine();
  print_types_kvp();
  print_types_m2mlink();
}



/*******************************************************************************
* Top-level tests are managed using AsyncSequencer.
* The dependency graph will allow us to order tests in a bottom-up manner, with
*   more sophisticated pieces being run only if base support passes.
*******************************************************************************/
#define CHKLST_STRINGBUILDER_TESTS    0x00000001  // Everything depends on this.
#define CHKLST_FSM_TESTS              0x00000002  //
#define CHKLST_SCHEDULER_TESTS        0x00000004  //
#define CHKLST_DATA_STRUCT_TESTS      0x00000008  // Unorganized tests on data structures.
#define CHKLST_SENSORFILTER_TESTS     0x00000010  //
#define CHKLST_IDENTITY_TESTS         0x00000020  //
#define CHKLST_M2MLINK_TESTS          0x00000040  //
#define CHKLST_PARSINGCONSOLE_TESTS   0x00000080  //
#define CHKLST_RINGBUFFER_TESTS       0x00000100  //
#define CHKLST_BUFFER_ACCEPTER_TESTS  0x00000200  //
#define CHKLST_LINKED_LIST_TESTS      0x00000400  // LinkedList
#define CHKLST_KEY_VALUE_PAIR_TESTS   0x00000800  // KeyValuePair
#define CHKLST_PRIORITY_QUEUE_TESTS   0x00001000  // PriorityQueue
#define CHKLST_VECTOR3_TESTS          0x00002000  // Vector3
#define CHKLST_ASYNC_SEQUENCER_TESTS  0x80000000  // Everything depends on this.


#define CHKLST_ALL_TESTS ( \
  CHKLST_STRINGBUILDER_TESTS | CHKLST_FSM_TESTS | CHKLST_SCHEDULER_TESTS | \
  CHKLST_DATA_STRUCT_TESTS | CHKLST_SENSORFILTER_TESTS | CHKLST_RINGBUFFER_TESTS | \
  CHKLST_IDENTITY_TESTS | CHKLST_M2MLINK_TESTS | CHKLST_PARSINGCONSOLE_TESTS | \
  CHKLST_ASYNC_SEQUENCER_TESTS | CHKLST_BUFFER_ACCEPTER_TESTS | \
  CHKLST_PRIORITY_QUEUE_TESTS | CHKLST_VECTOR3_TESTS | \
  CHKLST_KEY_VALUE_PAIR_TESTS | CHKLST_LINKED_LIST_TESTS)


const StepSequenceList TOP_LEVEL_TEST_LIST[] = {
  { .FLAG         = CHKLST_ASYNC_SEQUENCER_TESTS,
    .LABEL        = "ASYNC_SEQUENCER_TESTS",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == async_seq_test_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_RINGBUFFER_TESTS,
    .LABEL        = "RINGBUFFER_TESTS",
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == ringbuffer_main()) ? 1:-1);  }
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
    .DEP_MASK     = (CHKLST_STRINGBUILDER_TESTS | CHKLST_ASYNC_SEQUENCER_TESTS | CHKLST_RINGBUFFER_TESTS | CHKLST_PRIORITY_QUEUE_TESTS | CHKLST_VECTOR3_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == data_structure_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_BUFFER_ACCEPTER_TESTS,
    .LABEL        = "BUFFER_ACCEPTER_TESTS",
    .DEP_MASK     = (CHKLST_DATA_STRUCT_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == buffer_accepter_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SENSORFILTER_TESTS,
    .LABEL        = "SENSORFILTER_TESTS",
    .DEP_MASK     = (CHKLST_FSM_TESTS | CHKLST_VECTOR3_TESTS),
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
    .DEP_MASK     = (CHKLST_IDENTITY_TESTS | CHKLST_FSM_TESTS | CHKLST_BUFFER_ACCEPTER_TESTS | CHKLST_KEY_VALUE_PAIR_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == manuvrlink_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_PARSINGCONSOLE_TESTS,
    .LABEL        = "PARSINGCONSOLE_TESTS",
    .DEP_MASK     = (CHKLST_BUFFER_ACCEPTER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == parsing_console_main()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_LINKED_LIST_TESTS,
    .LABEL        = "LINKED_LIST_TESTS",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_LinkedList()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_KEY_VALUE_PAIR_TESTS,
    .LABEL        = "KEY_VALUE_PAIR_TESTS",
    .DEP_MASK     = (CHKLST_DATA_STRUCT_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_KeyValuePair()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_PRIORITY_QUEUE_TESTS,
    .LABEL        = "PRIORITY_QUEUE_TESTS",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_PriorityQueue()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_VECTOR3_TESTS,
    .LABEL        = "VECTOR3_TESTS",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == vector3_test_main()) ? 1:-1);  }
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
