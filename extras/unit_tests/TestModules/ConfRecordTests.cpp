/*
File:   ConfRecordTests.cpp
Author: J. Ian Lindsay
Date:   2023.10.09

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This program tests ConfRecord.
*/

#include "Storage/RecordTypes/ConfRecord.h"


/*******************************************************************************
* The program has a set of configurations that it defines and loads at runtime.
* This defines everything required to handle that conf fluidly and safely.
*
* TODO: Pending mitosis into a header file....
*******************************************************************************/
// First, we define (as an enum) what keys we want in the data, and an invalid
//   marker so that we can use the sanitizer in EnumDefList.
enum class ExampleConf : uint8_t {
  PROG_KEY_0,
  PROG_KEY_1,
  PROG_KEY_2,
  PROG_KEY_3,
  PROG_KEY_4,
  PROG_KEY_5,
  PROG_KEY_6,
  PROG_KEY_7,
  PROG_KEY_8,
  PROG_KEY_9,
  PROG_KEY_A,
  PROG_KEY_B,
  PROG_KEY_C,
  PROG_KEY_D,
  PROG_KEY_E,
  PROG_KEY_F,
  INVALID
};

// Then, we bind those enum values each to a type code, and to a semantic string
//   suitable for storage or transmission to a counterparty.
// ConfRecord uses the context byte in the enum wrapper to store the value's
//   underlying type. We define a type rainbow for testing.
// We set the flags of the INVALID marker such that we can fail safely, and also
//   to not have it show as a configuration key.
const EnumDef<ExampleConf> EX_CONF_KEY_LIST[] = {
  { ExampleConf::PROG_KEY_0,  "PROG_KEY_0",     0, (uint8_t) TCode::BOOLEAN    },
  { ExampleConf::PROG_KEY_1,  "PROG_KEY_1",     0, (uint8_t) TCode::BOOLEAN    },
  { ExampleConf::PROG_KEY_2,  "PROG_KEY_2",     0, (uint8_t) TCode::BOOLEAN    },
  //{ ExampleConf::PROG_KEY_3,  "PROG_KEY_3",     0, (uint8_t) TCode::STR        },
  { ExampleConf::PROG_KEY_4,  "PROG_KEY_4",     0, (uint8_t) TCode::UINT32     },
  { ExampleConf::PROG_KEY_5,  "PROG_KEY_5",     0, (uint8_t) TCode::UINT16     },
  { ExampleConf::PROG_KEY_6,  "PROG_KEY_6",     0, (uint8_t) TCode::UINT8      },
  { ExampleConf::PROG_KEY_7,  "PROG_KEY_7",     0, (uint8_t) TCode::FLOAT      },
  //{ ExampleConf::PROG_KEY_8,  "PROG_KEY_8",     0, (uint8_t) TCode::VECT_3_FLOAT     },
  //{ ExampleConf::PROG_KEY_9,  "PROG_KEY_9",     0, (uint8_t) TCode::VECT_3_UINT32    },
  //{ ExampleConf::PROG_KEY_A,  "PROG_KEY_A",     0, (uint8_t) TCode::VECT_3_INT8      },
  { ExampleConf::PROG_KEY_B,  "PROG_KEY_B",     0, (uint8_t) TCode::DOUBLE     },
  { ExampleConf::PROG_KEY_C,  "PROG_KEY_C",     0, (uint8_t) TCode::UINT64     },
  { ExampleConf::PROG_KEY_D,  "PROG_KEY_D",     0, (uint8_t) TCode::INT64      },
  //{ ExampleConf::PROG_KEY_E,  "PROG_KEY_E",     0, (uint8_t) TCode::BINARY     },
  //{ ExampleConf::PROG_KEY_F,  "PROG_KEY_F",     0, (uint8_t) TCode::STR        },
  { ExampleConf::INVALID, "INVALID", (ENUM_FLAG_MASK_INVALID_CATCHALL), (uint8_t) TCode::NONE}
};

// The top-level enum wrapper binds the above definitions into a tidy wad
//   of contained concerns. The string argument names this kind of configuration
//   record (but not this specific instance).
const EnumDefList<ExampleConf> EX_CONF_LIST(
  EX_CONF_KEY_LIST, (sizeof(EX_CONF_KEY_LIST) / sizeof(EX_CONF_KEY_LIST[0])),
  "ExampleConf"  // Doesn't _need_ to be the enum name...
);

// After all that definition, we can finally create the conf object.
ConfRecordValidation<ExampleConf> example_conf(0, &EX_CONF_LIST);



/*******************************************************************************
* Support functions
*******************************************************************************/

void print_conf_record_to_stdout(ConfRecord* record) {
  StringBuilder tmp_str;
  StringBuilder serialized;
  record->printConfRecord(&tmp_str);
  printf("Serializing conf returns %d.\n", record->serialize(&serialized, TCode::CBOR));
  serialized.printDebug(&tmp_str);
  printf("%s\n", (char*) tmp_str.string());
}


void print_types_conf_record() {
  printf("\tConfRecord                   %u\t%u\n", sizeof(ConfRecord), alignof(ConfRecord));
  printf("\tConfRecordValidation<T>      %u\t%u\n", sizeof(ConfRecordValidation<ExampleConf>), alignof(ConfRecordValidation<ExampleConf>));
}



/*******************************************************************************
* Tests for ConfRecord
*******************************************************************************/


int naked_conf_record_basic_tests() {
  int ret = -1;
  printf("Running basic tests on a naked ConfRecord...\n");
  printf("\tThe list contains the expected number of definitions...\n");
  ret = 0;   // TDOO

  if (0 != ret) {
    printf(" Fail.\n");
  }
  print_conf_record_to_stdout(&example_conf);
  return ret;
}


int naked_conf_record_advanced_tests() {
  int ret = -1;
  printf("Running advanced tests on a naked ConfRecord...\n");
  printf("\tSerializing the ConfRecord results in success...\n");
  printf("\tThe size of the resulting buffer closely matches the expectation value...\n");
  printf("\tUsing the serialized buffer to deserialize a new ConfRecord results in success...\n");
  printf("\tAll values in the records match...\n");
  ret = 0;   // TDOO

  if (0 != ret) {
    printf(" Fail.\n");
  }
  return ret;
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_conf_record_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "ConfRecord";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == naked_conf_record_basic_tests()) {
    if (0 == naked_conf_record_advanced_tests()) {
      ret = 0;
    }
    else printTestFailure(MODULE_NAME, "Advanced tests (naked record)");
  }
  else printTestFailure(MODULE_NAME, "Basic tests (naked record)");

  return ret;
}
