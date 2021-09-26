/*
File:   IdentityTest.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests on Identity elements.
*/


/*******************************************************************************
* Globals
*******************************************************************************/
IdentityUUID* id_uuid = nullptr;



/*******************************************************************************
* Test routines
*******************************************************************************/

int UUID_IDENT_TESTS() {
  int return_value = -1;
  StringBuilder log("===< UUID_IDENT_TESTS >=================================\n");
  // Create identities from nothing...
  IdentityUUID id_uuid("testUUID");
  log.concat("\t Creating a new identity...\n");
  Identity::staticToString(&id_uuid, &log);
  log.concat("\n\t Loading from buffer...\n");

  // Create identities from serialized representations.
  uint8_t buf[] = {0, 24, 0, 0, (uint8_t) IdentFormat::UUID, 65, 65, 0,   1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  IdentityUUID* ident0 = (IdentityUUID*) Identity::fromBuffer(buf, sizeof(buf));
  if (ident0) {
    Identity::staticToString(ident0, &log);
    log.concat("\n");
    // Serialize identity.
    int rep_len = ident0->length();
    if (rep_len == sizeof(buf)) {
      uint8_t* buf0 = (uint8_t*) alloca(rep_len);
      int ser_len = ident0->serialize(buf0, rep_len);
      if (ser_len == rep_len) {
        return_value = 0;
        for (int i = 0; i < ser_len; i++) {
          if (buf[i] != buf0[i]) {
            log.concatf("Index %d mismatch. Found 0x%02x, expected 0x%02x.\n", i, buf0[i], buf[i]);
            return return_value;
          }
        }
        return_value = 0;  // PASS
      }
      else {
        log.concatf("Serialized length is %d bytes. Should be %d bytes.\n", ser_len, rep_len);
      }
    }
    else {
      log.concatf("Reported length is %d bytes. Should be %d bytes.\n", rep_len, sizeof(buf));
    }
  }
  else {
    log.concatf("Failed to deserialize.\n");
  }

  log.concat("\n\n");
  printf("%s\n", (const char*) log.string());
  return return_value;
}


/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int identity_main() {
  int ret = 1;   // Failure is the default result.
  if (0 == UUID_IDENT_TESTS()) {
    printf("**********************************\n");
    printf("*  Identity tests all pass       *\n");
    printf("**********************************\n");
    ret = 0;
  }
  else printTestFailure("UUID_IDENT_TESTS");

  return ret;
}
