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
#include "Vector3.h"
#include "StopWatch.h"
#include "uuid.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"
#include "Identity/IdentityUUID.h"
#include "Identity/Identity.h"


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

unsigned long micros() {
	uint32_t ret = 0;
	struct timeval current;
	gettimeofday(&current, nullptr);
	return (current.tv_usec - start_micros.tv_usec);
}

unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}


/**
* Prints the sizes of various types. Informational only. No test.
*/
void printTypeSizes(StringBuilder* output) {
  output->concat("===< Type sizes >=======================================\n-- Primitives:\n");
  output->concatf("\tvoid*                 %u\n", sizeof(void*));
  output->concatf("\tFloat                 %u\n", sizeof(float));
  output->concatf("\tDouble                %u\n", sizeof(double));
  output->concat("-- Singletons:\n");
  output->concatf("\tAbstractPlatform      %u\n", sizeof(AbstractPlatform));
  output->concatf("\tParsingConsole        %u\n", sizeof(ParsingConsole));
  output->concat("-- Elemental data structures:\n");
  output->concatf("\tStringBuilder         %u\n", sizeof(StringBuilder));
  output->concatf("\tKeyValuePair          %u\n", sizeof(KeyValuePair));
  output->concatf("\tVector3<float>        %u\n", sizeof(Vector3<float>));
  output->concatf("\tLinkedList<void*>     %u\n", sizeof(LinkedList<void*>));
  output->concatf("\tElementPool<void*>    %u\n", sizeof(ElementPool<void*>));
  output->concatf("\tPriorityQueue<void*>  %u\n", sizeof(PriorityQueue<void*>));
  output->concatf("\tRingBuffer<void*>     %u\n", sizeof(RingBuffer<void*>));
  output->concatf("\tUUID                  %u\n", sizeof(UUID));
  output->concatf("\tStopWatch             %u\n", sizeof(StopWatch));
  output->concatf("\tGPSWrapper            %u\n", sizeof(GPSWrapper));
  output->concatf("\tSensorFilter<float>   %u\n", sizeof(SensorFilter<float>));
  output->concatf("\tIdentity              %u\n", sizeof(Identity));
  output->concatf("\tIdentityUUID          %u\n", sizeof(IdentityUUID));
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

#include "StringBuilderTest.cpp"
#include "TestDataStructures.cpp"
#include "ParsingConsoleTest.cpp"
#include "IdentityTest.cpp"



/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.
  srand(time(NULL));
  gettimeofday(&start_micros, nullptr);

  StringBuilder out;
  printTypeSizes(&out);
  printf("%s\n\n", (const char*) out.string());
  out.clear();

  if (0 == stringbuilder_main()) {
    if (0 == data_structure_main()) {
      if (0 == parsing_console_main()) {
        if (0 == identity_main()) {
          exit_value = 0;
        }
      }
    }
  }

  exit(exit_value);
}
