/*
File:   AllTests.cpp
Author: J. Ian Lindsay
Date:   2021.09.25

This is the top-level testing program for CppPotpourri (or, C3P, for short).

NOTE: CryptoBurrito has its own test program. All of these tests must pass
  before testing CryptoBurrito.

Global unit-testing TODO list:
--------------------------------------------------------------------------------
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
#include "Console/C3PConsole.h"
#include "ElementPool.h"
#include "RingBuffer.h"
#include "PriorityQueue.h"
#include "C3PValue/KeyValuePair.h"
#include "LightLinkedList.h"
#include "SensorFilter.h"
#include "AsyncSequencer.h"
#include "Vector3.h"
#include "TimerTools/TimerTools.h"
#include "uuid.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"
#include "Identity/IdentityUUID.h"
#include "Identity/Identity.h"
#include "M2MLink/M2MLink.h"


/*******************************************************************************
* Support functions
* TODO: Some of this should be subsumed by the general linux platform.
*   some of what remains should be collected into a general testing framework?
*******************************************************************************/
long unsigned timer_rebase = 0;


uint32_t randomUInt32() {
  uint32_t ret = ((uint8_t) rand()) << 24;
  ret += ((uint8_t) rand()) << 16;
  ret += ((uint8_t) rand()) << 8;
  ret += ((uint8_t) rand());
  return ret;
}

int8_t random_fill(uint8_t* buf, uint32_t len) {
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
  return (micros() / 1000L);
}

/*
* Not provided elsewhere on a linux platform.
*/
long unsigned micros() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  long unsigned raw = (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
  if (timer_rebase) {
    return (raw - timer_rebase);
  }
  timer_rebase = raw;
  return 0;
}


/* Delay functions */
void sleep_ms(uint32_t ms) {
  struct timespec t = {(long) (ms / 1000), (long) ((ms % 1000) * 1000000UL)};
  clock_nanosleep(CLOCK_MONOTONIC, 0, &t, &t);
}

void sleep_us(uint32_t us) {
  struct timespec t = {(long) (us / 1000000), (long) ((us % 1000000) * 1000UL)};
  clock_nanosleep(CLOCK_MONOTONIC, 0, &t, &t);
}


/*******************************************************************************
* Test reporting functions that are intended to be called from unit tests...
*******************************************************************************/

int generate_random_text_buffer(StringBuilder* buf, const int RANDOM_BUF_LEN) {
  int ret = 0;
  if ((RANDOM_BUF_LEN > 0) && (nullptr != buf)) {
    uint8_t tmp_buf[RANDOM_BUF_LEN+1] = {0, };
    random_fill(tmp_buf, (uint32_t) RANDOM_BUF_LEN);
    for (int i = 0; i < RANDOM_BUF_LEN; i++) {
      tmp_buf[i] = (0x30 + (tmp_buf[i] % 0x4E));
    }
    ret = RANDOM_BUF_LEN;
    buf->concat(tmp_buf, RANDOM_BUF_LEN);
  }
  return ret;
}


uint64_t generate_random_uint64() {
  uint64_t ret = 0;
  random_fill((uint8_t*) &ret, (uint32_t) sizeof(uint64_t));
  return ret;
}


int64_t generate_random_int64() {
  int64_t ret = 0;
  random_fill((uint8_t*) &ret, (uint32_t) sizeof(int64_t));
  return ret;
}


bool flip_coin() {
  return (0 != (1 & randomUInt32()));
}


float generate_random_float() {
  return ((float) randomUInt32() / (float) randomUInt32());
}

double generate_random_double() {
  return ((double) generate_random_uint64() / (double) generate_random_uint64());
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
#include "TestModules/PlatformAssurances.cpp"
#include "TestModules/GlueTests.cpp"
#include "TestModules/AsyncSequencerTests.cpp"
#include "TestModules/StringBuilderTest.cpp"
#include "TestModules/Vector3Tests.cpp"
#include "TestModules/UUIDTests.cpp"
#include "TestModules/RingBufferTests.cpp"
#include "TestModules/C3PTypeTests.cpp"
#include "TestModules/C3PValueTests.cpp"
#include "TestModules/KVPTests.cpp"
#include "TestModules/LinkedListTests.cpp"
#include "TestModules/EnumWrapperTests.cpp"
#include "TestModules/FSMTests.cpp"
#include "TestModules/SchedulerTests.cpp"
#include "TestModules/TimerUtilityTests.cpp"
#include "TestModules/BufferAccepterTests.cpp"
#include "TestModules/Base64CoDecTests.cpp"
#include "TestModules/MultiStringSearchTests.cpp"
#include "TestModules/LineTermCoDecTests.cpp"
#include "TestModules/ImageTests.cpp"
#include "TestModules/SensorFilterTests.cpp"
#include "TestModules/ParsingConsoleTest.cpp"
#include "TestModules/IdentityTest.cpp"
#include "TestModules/M2MLinkTests.cpp"
#include "TestModules/ConfRecordTests.cpp"


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
  printf("\tunsigned int             %u\t%u\t%016x\n", sizeof(unsigned int),  alignof(unsigned int), UINT_MAX);
  printf("\tunsigned long            %u\t%u\t%lu\n", sizeof(unsigned long),  alignof(unsigned long), ULONG_MAX);
  printf("\tFloat                    %u\t%u\n", sizeof(float),  alignof(float));
  printf("\tDouble                   %u\t%u\n", sizeof(double), alignof(double));
  printf("-- C3P types:\n");
  print_types_platform();
  print_types_vector3();
  print_types_stringbuilder();
  print_types_uuid();
  print_types_timer_utils();
  print_types_async_sequencer();
  print_types_ringbuffer();
  printf("\tElementPool<void*>       %u\t%u\n", sizeof(ElementPool<void*>),   alignof(ElementPool<void*>));
  print_types_linked_lists();
  print_types_image();
  print_types_enum_wrapper();
  print_types_scheduler();
  print_types_state_machine();
  print_types_c3p_type();
  print_types_c3p_value();
  print_types_kvp();
  print_types_identity();
  print_types_multisearch();
  print_types_buffer_accepter();
  print_types_c3p_b64();
  print_types_line_term_codec();
  print_types_conf_record();
  print_types_parsing_console();
  print_types_sensorfilter();
  print_types_m2mlink();
}



/*******************************************************************************
* Top-level tests are managed using AsyncSequencer.
* The dependency graph will allow us to order tests in a bottom-up manner, with
*   more sophisticated pieces being run only if base support passes.
* NOTE: The flag value ordering is not important.
*******************************************************************************/
#define CHKLST_STRINGBUILDER_TESTS    0x00000001  // StringBuilder
#define CHKLST_FSM_TESTS              0x00000002  // StateMachine
#define CHKLST_SCHEDULER_TESTS        0x00000004  //
#define CHKLST_TIMER_UTILS_TESTS      0x00000008  // Timer utils
#define CHKLST_SENSORFILTER_TESTS     0x00000010  //
#define CHKLST_IDENTITY_TESTS         0x00000020  //
#define CHKLST_M2MLINK_TESTS          0x00000040  //
#define CHKLST_PARSINGCONSOLE_TESTS   0x00000080  // ParsingConsole
#define CHKLST_RINGBUFFER_TESTS       0x00000100  // RingBuffer
#define CHKLST_BUFFER_ACCEPTER_TESTS  0x00000200  // BufferAccepter contract and test harness.
#define CHKLST_LINKED_LIST_TESTS      0x00000400  // LinkedList
#define CHKLST_KEY_VALUE_PAIR_TESTS   0x00000800  // KeyValuePair
#define CHKLST_PRIORITY_QUEUE_TESTS   0x00001000  // PriorityQueue
#define CHKLST_VECTOR3_TESTS          0x00002000  // Vector3
#define CHKLST_CODEC_B64_TESTS        0x00004000  // Base64Encoder, Base64Decoder
#define CHKLST_CODEC_LINE_TERM_TESTS  0x00008000  // LineEndingCoDec
#define CHKLST_IMAGE_TESTS            0x00010000  // Image
#define CHKLST_C3P_HEADER_TESTS       0x00020000  // CppPotpourri.h
#define CHKLST_MULT_STR_SEARCH_TESTS  0x00040000  // MultiStringSearch
#define CHKLST_CI_PLATFORM_TESTS      0x00080000  // Platform assurances for this test program.
#define CHKLST_UUID_TESTS             0x00100000  // UUID
#define CHKLST_ASYNC_SEQUENCER_TESTS  0x00200000  // AsyncSequencer
#define CHKLST_ENUM_WRAPPER_TESTS     0x00400000  // EnumWrapper
#define CHKLST_CONF_RECORD            0x00800000  // ConfRecord
#define CHKLST_TYPE_CONTAINER_TESTS   0x01000000  // C3PType. C3PValue

#define CHKLST_GPS_PARSING_TESTS      0x02000000  // TODO:
#define CHKLST_ELEMENT_POOL_TESTS     0x04000000  // TODO: ElementPool<T>
#define CHKLST_BUS_QUEUE_TESTS        0x08000000  // TODO:
#define CHKLST_UNIT_HANDLING_TESTS    0x10000000  // TODO:
#define CHKLST_3_AXIS_PIPE_TESTS      0x40000000  // TODO:
#define CHKLST_LOGGER_TESTS           0x80000000  // TODO: The logging abstraction.



/*
* We're going to do a bit of clutter-control...
* Tier-0: The platform and C3P header. Our primary definitions and ontology.
* Tier-1: The library's basic elements of composition. Data structures, special
*   types with no dependencies, etc.
* Tier-2: The mid-level abstractions and contracts that are formed on the basis
*   of those elements, as well as some optional machinary for solving common
*   problems.
* Tier-3: The composition of those contracts and abstractions into well-defined
*   modules that solve high-value problems in firmware design.
*
* Complex, high-level tests are encouraged to cite one of these tiers as a
*   dependency for brevity. This will save testing complexity by not
*   requiring strict dep-knowledge for a given high-level capability (which
*   probably relies on StringBuilder, and at least one other thing covered
*   by CHKLST_ALL_TIER_1_TESTS).
*/
#define CHKLST_ALL_TIER_0_TESTS ( \
  CHKLST_CI_PLATFORM_TESTS | CHKLST_C3P_HEADER_TESTS)

#define CHKLST_ALL_TIER_1_TESTS ( \
  CHKLST_STRINGBUILDER_TESTS | CHKLST_TIMER_UTILS_TESTS | CHKLST_RINGBUFFER_TESTS | \
  CHKLST_ASYNC_SEQUENCER_TESTS | CHKLST_PRIORITY_QUEUE_TESTS | CHKLST_VECTOR3_TESTS | \
  CHKLST_LINKED_LIST_TESTS | CHKLST_UUID_TESTS | CHKLST_ENUM_WRAPPER_TESTS | \
  CHKLST_IMAGE_TESTS)

#define CHKLST_ALL_TIER_2_TESTS ( \
  CHKLST_FSM_TESTS | CHKLST_SCHEDULER_TESTS | CHKLST_IDENTITY_TESTS | \
  CHKLST_BUFFER_ACCEPTER_TESTS | CHKLST_MULT_STR_SEARCH_TESTS | \
  CHKLST_CODEC_LINE_TERM_TESTS | CHKLST_CODEC_B64_TESTS | CHKLST_KEY_VALUE_PAIR_TESTS)

#define CHKLST_ALL_TIER_3_TESTS ( \
  CHKLST_PARSINGCONSOLE_TESTS | CHKLST_SENSORFILTER_TESTS | \
  CHKLST_LOGGER_TESTS | CHKLST_M2MLINK_TESTS | CHKLST_CONF_RECORD)

#define CHKLST_ALL_TESTS ( \
  CHKLST_ALL_TIER_0_TESTS | CHKLST_ALL_TIER_1_TESTS | \
  CHKLST_ALL_TIER_2_TESTS | CHKLST_ALL_TIER_3_TESTS)


/*
* Top level test definitions.
* Each of these blocks defines a top-level series of tests on the named module
*   of the library.
*
* NOTE: These tests are listed in their dependency order for clarity only. Their
*   ordering in this list is arbitrary with respect to the outcome. All tests
*   with sated dependencies will be given a chance to run, even if the test that
*   just ran failed.
*/
const StepSequenceList TOP_LEVEL_TEST_LIST[] = {
  // The tests of the test program's implementation of AbstractPlatform. Nothing
  //   else will make any sense if this fails. It is ultimately a dependency for
  //   everything being tested, in one way or another.
  // TODO: Verification of correct operation of any dependency injection
  //   features should fall into this block as well.
  { .FLAG         = CHKLST_CI_PLATFORM_TESTS,
    .LABEL        = "Test program ontology",
    .DEP_MASK     = (0),   // Bottom Turtle
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == platform_assurance_test_main()) ? 1:-1);  }
  },

  // After that, we'll want to test some of our library glue.
  // These tests handle
  // CHKLST_CI_PLATFORM_TESTS:   Test needs the RNG
  { .FLAG         = CHKLST_C3P_HEADER_TESTS,
    .LABEL        = "CppPotpourri.h",
    .DEP_MASK     = (CHKLST_CI_PLATFORM_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_header_test_main()) ? 1:-1);  }
  },

  //////////////////////////////////////////////////////////////////////////////
  // Now, we can begin TIER-1 tests. These tests usually require the RNG and/or
  //   timer apperatus to be in good working order. So each should depend upon
  //   all TIER-0 tests passing (in addition to whatever they need amongst each
  //   other).

  // Most programs need to implement simple delays and one-shots, and profile
  //   execution. Fundamental needs based on timers are covered by these tests.
  { .FLAG         = CHKLST_TIMER_UTILS_TESTS,
    .LABEL        = "Timer tools",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timer_utilities_main()) ? 1:-1);  }
  },

  // 3-space is a really common thing to deal with. Test our vector class.
  { .FLAG         = CHKLST_VECTOR3_TESTS,
    .LABEL        = "Vector3<T>",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == vector3_test_main()) ? 1:-1);  }
  },

  // UUID is a common thing to handle.
  { .FLAG         = CHKLST_UUID_TESTS,
    .LABEL        = "UUID",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == uuid_test_main()) ? 1:-1);  }
  },

  // Test the data structure that we are using to execute these test blocks.
  // AsyncSequencer is used for (lazy and asyncronous) resolution of complex
  //   dependency trees.
  { .FLAG         = CHKLST_ASYNC_SEQUENCER_TESTS,
    .LABEL        = "AsyncSequencer",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == async_seq_test_main()) ? 1:-1);  }
  },

  // StringBuilder is the library's abstraction of "strings", in the general
  //   sense of the word. It is used for both binary data that is not
  //   null-terminated, as well as C-style printable strings. Its API allows
  //   safe and seamless cross-interpretation of strings and buffers, with
  //   features that wrap complex dynamic memory use into a uniform contract.
  // StringBuilder is the elemental representation of dynamic buffers throughout
  //   the library. Prinable, null-terminated, or otherwise.
  { .FLAG         = CHKLST_STRINGBUILDER_TESTS,
    .LABEL        = "StringBuilder",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == stringbuilder_main()) ? 1:-1);  }
  },

  // RingBuffer is our template for a heap-resident, fixed length FIFO.
  // It forms the underpinning of the FiniteStateMachine template, as well as
  //   many termini of BufferAccepter chains.
  { .FLAG         = CHKLST_RINGBUFFER_TESTS,
    .LABEL        = "RingBuffer<T>",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == ringbuffer_main()) ? 1:-1);  }
  },

  // LinkedList and Priority queue are sister templates with _almost_ matching
  //   APIs and implementations. Both are heap-resident.
  // One or the other of these classes is the library's go-to for orderd lists
  //   of things.
  { .FLAG         = CHKLST_LINKED_LIST_TESTS,
    .LABEL        = "LinkedList<T>",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_LinkedList()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_PRIORITY_QUEUE_TESTS,
    .LABEL        = "PriorityQueue<T>",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_PriorityQueue()) ? 1:-1);  }
  },

  // Image is a high-complexity API that is used as the basis for frame-buffer
  //   APIs, and wrapping specific image libraries.
  { .FLAG         = CHKLST_IMAGE_TESTS,
    .LABEL        = "Image",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS | CHKLST_STRINGBUILDER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_image_test_main()) ? 1:-1);  }
  },

  // EnumWrapper is used to extend certain compile-time enum value assurances
  //   into run-time.
  { .FLAG         = CHKLST_ENUM_WRAPPER_TESTS,
    .LABEL        = "EnumDefList",
    .DEP_MASK     = (CHKLST_ALL_TIER_0_TESTS | CHKLST_STRINGBUILDER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_enum_wrapper_test_main()) ? 1:-1);  }
  },

  //////////////////////////////////////////////////////////////////////////////
  // Now moving into TIER-2, where we start testing some higher-level things.
  // These classes (and possibly the tests themselves) depend upon one or more
  //   elements from TIER-1.

  // These are the tests of the BufferAccepter interface (used to govern
  //   buffer transfer by contract).
  { .FLAG         = CHKLST_BUFFER_ACCEPTER_TESTS,
    .LABEL        = "BufferAccepter",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == buffer_accepter_main()) ? 1:-1);  }
  },

  // The finite state machine template. Many classes of varying degrees of
  //   constraint rely on this class to exhibit controlled state evolution.
  // Such classes tend to be the ones with wide failure nets (many parameters),
  //   and/or have asynchronicity concerns forced upon them. Often these are
  //   hardware drivers, but also any pure-software classes that want to spread
  //   their local complexity out over a defined state-space and polling cycles.
  { .FLAG         = CHKLST_FSM_TESTS,
    .LABEL        = "StateMachine<T>",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == fsm_test_main()) ? 1:-1);  }
  },

  // Tests the ability to do concurrent string search. This has very simple
  //   dependencies (basically StringBuilder), but is a difficult problem to
  //   solve, in practice.
  { .FLAG         = CHKLST_MULT_STR_SEARCH_TESTS,
    .LABEL        = "MultiStringSearch",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_multisearch_test_main()) ? 1:-1);  }
  },

  // Identity tests in this block are the trivial types, and base-class
  //   implementations of Identity. Notions of Identity that have cryptographic
  //   backing should be tested in the (TODO) cryptography tests,
  { .FLAG         = CHKLST_IDENTITY_TESTS,
    .LABEL        = "Identity",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == identity_main()) ? 1:-1);  }
  },

  // C3PValue is the class that facilitates type-wrapping for single values.
  // C++ wouldn't be worth using if not for its hard-types. But for simplicity
  //   elsewhere, we often like to forget that fact and write code that handles
  //   data generally (or en masse) no matter its specific type composition.
  //
  // NOTE: This test block also covers parsing and packing of all supported
  //   types. So don't be confused if changes in a seemingly unrelated place
  //   break these tests. In such a case, check the type's parse/pack functions.
  // NOTE: This test block also covers CBOR implementation.
  { .FLAG         = CHKLST_TYPE_CONTAINER_TESTS,
    .LABEL        = "C3PValue",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() {
      if ((0 == c3p_type_test_main()) ? 1:-1) {
        if ((0 == c3p_value_test_main()) ? 1:-1) {
          return 1;
        }
      }
      return -1;
    }
  },

  // KVP is responsible for implementing a map data type with full
  //   type-resolution and memory management of the library's
  //   internally-enumerated types. It is thus the library's highest complexity
  //   class, in the sense that it must be aware of the contracts of the highest
  //   numbers of types and their APIs.
  // Fortunately, it is also entirely optional. If you just want a map, use the
  //   one in the standard library. If, however, you want to conduct exchange of
  //   sophisticated objects in terms of their string representations, use this.
  // NOTE: This test block also covers CBOR implementation.
  { .FLAG         = CHKLST_KEY_VALUE_PAIR_TESTS,
    .LABEL        = "KeyValuePair",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS | CHKLST_TYPE_CONTAINER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_KeyValuePair()) ? 1:-1);  }
  },

  // This test block is for the scheduler.
  // Most firmware designs are simple. They only do a handful of things, are not
  //   I/O-bound, and can afford sloppy top-level scheduling (or no scheduling
  //   at all).
  // Other firmware designs can get by with a smattering of cheap and simple
  //   PeriodicTimeout instances.
  // But some firmware designs have to cope with multiple real-time demands
  //   which sometime conflict, and have to do so without threads, and at the
  //   scale of microseconds.
  // For this final class of firmware designs, we have C3PScheduler.
  // CHKLST_CI_PLATFORM_TESTS:   Test needs the system time
  { .FLAG         = CHKLST_SCHEDULER_TESTS,
    .LABEL        = "C3PScheduler",
    .DEP_MASK     = (CHKLST_ALL_TIER_1_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == scheduler_tests_main()) ? 1:-1);  }
  },

  // By now, we'll be able to test some of our top-level abstractions that deal
  //   with the outside world. It can be said that the true purpose of the unit
  //   tests is to have confidence in the things being tested below. Not only
  //   because thier dep complexities are the highest in the library, but
  //   also because these pieces are exposed to input from the outside world
  //   (which is always in a state of anarchy).
  // Test our Base64 implementation...
  { .FLAG         = CHKLST_CODEC_B64_TESTS,
    .LABEL        = "Base64 CoDec",
    .DEP_MASK     = (CHKLST_BUFFER_ACCEPTER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_b64_test_main()) ? 1:-1);  }
  },

  // Textual buffers are commonly imbued with a protocol meant for typewriters,
  //   for the benefit of brains. We call the atomic end-result of the protocol
  //   a "line".
  // In theory, it doesn't matter what line-terminator you use. But in practice,
  //   sometimes those terminal sequences are observed by machines in a manner
  //   that locally makes sense in the face of its practical constraints, but
  //   isn't the same choice that local firmware made.
  // LineEndingCoDec is intended to make this library a neutral party in this
  //   particular Holy War, and to optionally align line-breaks with call-time
  //   semantics in a given program.
  { .FLAG         = CHKLST_CODEC_LINE_TERM_TESTS,
    .LABEL        = "LineEndingCoDec",
    .DEP_MASK     = (CHKLST_BUFFER_ACCEPTER_TESTS | CHKLST_MULT_STR_SEARCH_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_line_codec_test_main()) ? 1:-1);  }
  },

  //////////////////////////////////////////////////////////////////////////////
  // Thusly begins TIER-3.
  // These classes are some of the top-turtles in the library, and any program
  //   that uses them will be flexing significant features in C3P, possibly
  //   without knowing it.

  // Not all programs log. So this class is optional. But any logging done
  //   within C3P will require routing and setup from the application.
  // NOTE: This test program records class logs, and optionally saves them to
  //   disk as a report. But it does not rely on the logging abstraction for
  //   output in any way.
  { .FLAG         = CHKLST_LOGGER_TESTS,
    .LABEL        = "Logging abstraction",
    .DEP_MASK     = (CHKLST_CODEC_LINE_TERM_TESTS | CHKLST_SCHEDULER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }   // TODO
  },

  // Not all programs want to store non-volatile KVP data in a type-safe,
  //   high-assurance manner. But those that do can do so with a minimum of
  //   concern by using the ConfRecord class.
  { .FLAG         = CHKLST_CONF_RECORD,
    .LABEL        = "ConfRecord",
    .DEP_MASK     = (CHKLST_KEY_VALUE_PAIR_TESTS | CHKLST_ENUM_WRAPPER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_conf_record_test_main()) ? 1:-1);  }
  },

  // Another common thing to have in firmware is a console relationship with
  //   the engineer (at least). Rather than forcing the consideration of a given
  //   program's needs from the console, this class is provided as a ready
  //   solution so that work can be done in places that are probably of higher
  //   value early on the design cycle.
  // Many of the sophisticated modules in C3P (and drivers written against it)
  //   contain built-in console handlers that can either be ignored or re-used
  //   by an application-custom implementation of ParsingConsole.
  // These handlers would ordinarilly be dropped by the linker if unused, along
  //   with the significant burden of the data section content to support them.
  //   They also prevent the linker's garbage collection from dropping code
  //   branches that are never otherwise called.
  // So if you are pressed for code size, dropping console support for drivers
  //   you are done debugging will go a long way to limiting the size and
  //   complexity of the resulting build. But no other top-level class in C3P
  //   does more to eliminate the boilerplate of making a firmware program.
  { .FLAG         = CHKLST_PARSINGCONSOLE_TESTS,
    .LABEL        = "ParsingConsole",
    .DEP_MASK     = (CHKLST_CODEC_LINE_TERM_TESTS | CHKLST_SCHEDULER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == parsing_console_main()) ? 1:-1);  }
  },

  // TODO: SensorFilter is under tremendous strain right now. It's API and
  //   contract are not finalized, and it will likely undergo fission into
  //   better-contained (and better-defined) pieces. It will probably be used
  //   as a dep for KVP, rather than as now (the other way around), and will
  //   probably have a memory implementation rooted in RingBuffer.
  { .FLAG         = CHKLST_SENSORFILTER_TESTS,
    .LABEL        = "SensorFilter<T>",
    .DEP_MASK     = (CHKLST_FSM_TESTS | CHKLST_KEY_VALUE_PAIR_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == sensor_filter_tests_main()) ? 1:-1);  }
  },

  // By nature of its purpose, this constellation of classes is the most
  //   complicated in the entire library. It basically forms an object
  //   translation aperture with another machine using this library (SOAP or
  //   REST, depending on usage). It must potentially handle any of the
  //   KVP-supported types within the library.
  { .FLAG         = CHKLST_M2MLINK_TESTS,
    .LABEL        = "M2MLink",
    .DEP_MASK     = (CHKLST_ALL_TIER_2_TESTS | CHKLST_PARSINGCONSOLE_TESTS | CHKLST_LOGGER_TESTS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == manuvrlink_main()) ? 1:-1);  }
  },
};

AsyncSequencer checklist_unit_tests(TOP_LEVEL_TEST_LIST, (sizeof(TOP_LEVEL_TEST_LIST) / sizeof(TOP_LEVEL_TEST_LIST[0])));


/*******************************************************************************
* The top-level main function.                                                 *
*******************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.
  srand(time(NULL));
  printTypeSizes();

  checklist_unit_tests.requestSteps(CHKLST_ALL_TESTS);
  while (!checklist_unit_tests.request_completed() && (0 == checklist_unit_tests.failed_steps(false))) {
    checklist_unit_tests.poll();
  }
  exit_value = (checklist_unit_tests.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  checklist_unit_tests.printDebug(&report_output, "Final test report");
  printf("%s\n", (char*) report_output.string());

  exit(exit_value);
}
