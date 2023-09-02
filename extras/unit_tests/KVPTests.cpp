/*
File:   KVPTests.cpp
Author: J. Ian Lindsay
Date:   2021.10.08

Copyright 2021 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This program runs tests against the KeyValuePair class.
*/


void dump_kvp(KeyValuePair* a) {
  if (a) {
    StringBuilder log;
    a->printDebug(&log);
    printf("%s\n", (char*) log.string());
    log.clear();
  }
  else {
    printf("dump_kvp() was passed a nullptr.\n");
  }
}


void dump_strbldr(StringBuilder* a) {
  if (a) {
    StringBuilder log;
    a->printDebug(&log);
    printf("%s\n", (char*) log.string());
    log.clear();
  }
  else {
    printf("dump_strbldr() was passed a nullptr.\n");
  }
}


/*******************************************************************************
* KVP test routines
*******************************************************************************/

/**
* Test the capability of KeyValuePairs to hold mixed KVP data, test lookup, and
*   to test the mechanics of the pointer-hack on PODs.
* Failure here might result in segfaults. This also needs to be tested against
*   both 32/64-bit builds.
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_KVP() {
  int return_value = -1;
  printf("===< KeyValuePairs KVP >====================================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    randomUInt32()/1000000.0f;
  float    val8  = (float)    randomUInt32()/1000000.0f;
  double   val9  = (double)   (double)randomUInt32()/(double)randomUInt32();
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

  uint32_t ret0 = 0;
  uint16_t ret1 = 0;
  uint8_t  ret2 = 0;
  int32_t  ret3 = 0;
  int16_t  ret4 = 0;
  int8_t   ret5 = 0;
  float    ret6 = 0.0f;
  float    ret8 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  //float    ret9 = 0.0d;

  printf("Adding arguments...\n\n");
  KeyValuePair a(val3);
  a.append(val0)->setKey("value0");
  a.append(val1, "value1");
  a.append(val2);  // NOTE: Mixed in with non-KVP.
  a.append(val4, "value4");
  a.append(val5, "value5");
  a.append(val6, "value6");
  a.append(val8, "value8");
  a.append(&val7)->setKey("value7");
  a.append(val9)->setKey("value9");

  dump_kvp(&a);

  StringBuilder temp_buffer;
  int key_count = a.collectKeys(&temp_buffer);
  printf("\t Breadth-first keyset (%d total keys):   ", key_count);
  for (int i = 0; i < key_count; i++) {
    printf("%s ", temp_buffer.position(i));
  }
  printf("\n");

  temp_buffer.clear();
  a.serialize(&temp_buffer, TCode::BINARY);
  printf("\t temp_buffer is %u bytes long.\n", temp_buffer.length());
  dump_strbldr(&temp_buffer);

  if (a.count() == 10) {
    if ((0 == a.valueWithKey("value6", &ret6)) && (ret6 == val6)) {
      if ((0 == a.valueWithKey("value0", &ret0)) && (ret0 == val0)) {
        if ((0 == a.valueWithKey("value4", &ret4)) && (ret4 == val4)) {
          if ((0 == a.valueWithKey("value5", &ret5)) && (ret5 == val5)) {
            if (0 != a.valueWithKey("non-key", &ret0)) {   // We shouldn't be able to get a value for a key that doesn't exist...
              if (0 != a.valueWithKey(nullptr, &ret0)) {   // Nor for a NULL key...
                if ((0 == a.valueWithIdx(1, &ret0)) && (ret0 == val0)) {
                  if ((0 == a.valueWithIdx(2, &ret1)) && (ret1 == val1)) {
                    if ((0 == a.valueWithIdx(3, &ret2)) && (ret2 == val2)) {
                      if ((0 == a.valueWithIdx(0, &ret3)) && (ret3 == val3)) {
                        if ((0 == a.valueWithIdx(4, &ret4)) && (ret4 == val4)) {
                          if ((0 == a.valueWithIdx(5, &ret5)) && (ret5 == val5)) {
                            return_value = 0;
                          }
                          else printf("int8_t failed (%d vs %d)...\n", val5, ret5);
                        }
                        else printf("int16_t failed (%d vs %d)...\n", val4, ret4);
                      }
                      else printf("int32_t failed (%d vs %d)...\n", val3, ret3);
                    }
                    else printf("uint8_t failed (%u vs %u)...\n", val2, ret2);
                  }
                  else printf("uint16_t failed (%u vs %u)...\n", val1, ret1);
                }
                else printf("uint32_t failed (%u vs %u)...\n", val0, ret0);
              }
              else printf("Found key (nullptr), which should have been nonexistant...\n");
            }
            else printf("Found key 'non-key', which should have been nonexistant...\n");
          }
          else printf("Failed to vet key 'value5'...\n");
        }
        else printf("Failed to vet key 'value4'...\n");
      }
      else printf("Failed to vet key 'value0'...\n");
    }
    else printf("Failed for float (%f vs %f)...\n", (double) val6, (double) ret6);
  }
  else printf("Total KeyValuePairs:  %d\tExpected 10.\n", a.count());

  return return_value;
}


/**
* These tests are for reference handling and proper type-assignment of internal
*   types.
* @return 0 on pass. Non-zero otherwise.
*/
int test_KeyValuePair_InternalTypes() {
  int return_value = 1;
  printf("===< KeyValuePairs Internal Types >=========================\n");
  StringBuilder val0("Some string");
  KeyValuePair a(&val0);

  StringBuilder* ret0 = nullptr;

  dump_kvp(&a);

  if (0 == a.getValueAs(&ret0)) {
    if (&val0 == ret0) {
      return_value = 0;
    }
    else printf("StringBuilder pointer retrieved from KeyValuePair is not the same as what went in. Fail...\n");
  }
  else printf("Failed to retrieve StringBuilder pointer.\n");

  return return_value;
}


/**
* [test_KeyValuePair_Value_Placement description]
* @return [description]
*/
int test_KeyValuePair_Value_Placement() {
  int return_value = -1;
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.
  printf("===< KeyValuePair Value Placement >=========================\n");

  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);
  double   val8  = (double)randomUInt32()/(double)randomUInt32();
  bool     val9  = true;
  bool     val10 = false;

  KeyValuePair arg0(val0);
  KeyValuePair arg1(val1);
  KeyValuePair arg2(val2);
  KeyValuePair arg3(val3);
  KeyValuePair arg4(val4);
  KeyValuePair arg5(val5);
  KeyValuePair arg6(val6);
  KeyValuePair arg7(&val7);
  KeyValuePair arg8(val8);
  KeyValuePair arg9(val9);
  KeyValuePair arg10(val10);

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  double   ret8 = 0.0f;
  bool     ret9  = false;
  bool     ret10 = false;

  val0  = (int32_t)  randomUInt32();
  val1  = (int16_t)  randomUInt32();
  val2  = (int8_t)   randomUInt32();
  val3  = (uint32_t) randomUInt32();
  val4  = (uint16_t) randomUInt32();
  val5  = (uint8_t)  randomUInt32();
  val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  val7(
    ((uint32_t) randomUInt32()) / ((float) randomUInt32()),
    ((uint32_t) randomUInt32()) / ((float) randomUInt32()),
    ((uint32_t) randomUInt32()) / ((float) randomUInt32())
  );
  val8  = ((uint32_t) randomUInt32()) / ((double) randomUInt32());
  val9  = !val9;
  val10 = !val10;

  arg0.setValue(val0);
  arg1.setValue(val1);
  arg2.setValue(val2);
  arg3.setValue(val3);
  arg4.setValue(val4);
  arg5.setValue(val5);
  arg6.setValue(val6);
  arg7.setValue(&val7);
  arg8.setValue(val8);
  arg9.setValue(val9);
  arg10.setValue(val10);

  if ((0 == arg0.getValueAs(&ret0)) && (ret0 == val0)) {
    if ((0 == arg1.getValueAs(&ret1)) && (ret1 == val1)) {
      if ((0 == arg2.getValueAs(&ret2)) && (ret2 == val2)) {
        if ((0 == arg3.getValueAs(&ret3)) && (ret3 == val3)) {
          if ((0 == arg4.getValueAs(&ret4)) && (ret4 == val4)) {
            if ((0 == arg5.getValueAs(&ret5)) && (ret5 == val5)) {
              if ((0 == arg6.getValueAs(&ret6)) && (ret6 == val6)) {
                if ((0 == arg8.getValueAs(&ret8)) && (ret8 == val8)) {
                  if ((0 == arg9.getValueAs(&ret9)) && (ret9 == val9)) {
                    if ((0 == arg10.getValueAs(&ret10)) && (ret10 == val10)) {
                      printf("Value placement tests good for all types.\n");
                      return_value = 0;
                    }
                    else printf("Failed to vet bool placement.\n");
                  }
                  else printf("Failed to vet bool placement.\n");
                }
                else printf("Failed to vet key 'value8'... %.20f vs %.20f\n", ret8, val8);
              }
              else printf("Failed to vet key 'value6'... %.3f vs %.3f\n", (double) ret6, (double) val6);
            }
            else printf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
          }
          else printf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
        }
        else printf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
      }
      else printf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
    }
    else printf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
  }
  else printf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);

  if (return_value) {
    //dump_kvp(&a);
  }
  return return_value;
}


/**
* The KVP API allows values to be type-degraded transparently. These tests
*   verify that such behavior is correct.
*
* @return [description]
*/
int test_KeyValuePair_Value_Translation() {
  int return_value = -1;
  printf("===< KeyValuePair Value Translation >=========================\n");

  uint32_t val0  = (uint32_t) randomUInt32();
  uint16_t val1  = (uint16_t) randomUInt32();
  uint8_t  val2  = (uint8_t)  randomUInt32();
  int32_t  val3  = (int32_t)  randomUInt32();
  int16_t  val4  = (int16_t)  randomUInt32();
  int8_t   val5  = (int8_t)   randomUInt32();
  float    val6  = (float)    randomUInt32()/1000000.0f;
  double   val7  = (double)randomUInt32()/(double)randomUInt32();
  Vector3<float> val8(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

  KeyValuePair a(val0, "uint32");
  a.append(val1, "uint16");
  a.append(val2, "uint8");
  a.append(val3, "int32");
  a.append(val4, "int16");
  a.append(val5, "int8");
  a.append(val6, "float");
  a.append(val7, "double");
  a.append(&val8, "Vector3<f>");
  dump_kvp(&a);

  // Experimental values.
  double   ret0 = 0;
  uint32_t ret1 = 0;
  uint16_t ret2 = 0;
  double   ret3 = 0;
  int32_t  ret4 = 0;
  int16_t  ret5 = 0;
  int8_t   ret6 = 0.0f;
  int32_t  ret7 = 0.0f;
  Vector3<int32_t> ret8(0, 0, 0);

  // Control values.
  double   compare0 = (double)   val0;
  uint32_t compare1 = (uint32_t) val1;
  uint16_t compare2 = (uint16_t) val2;
  double   compare3 = (double)   val3;
  int32_t  compare4 = (int32_t)  val4;
  int16_t  compare5 = (int16_t)  val5;
  int8_t   compare6 = (int8_t)   val6;
  int32_t  compare7 = (double)   val7;
  Vector3<int32_t> compare8((int32_t) val8.x, (int32_t) val8.y, (int32_t) val8.z);

  if ((0 == a.valueWithKey("uint32", &ret0)) && (ret0 == compare0)) {
    if ((0 == a.valueWithKey("uint16", &ret1)) && (ret1 == compare1)) {
      if ((0 == a.valueWithKey("uint8", &ret2)) && (ret2 == compare2)) {
        if ((0 == a.valueWithKey("int32", &ret3)) && (ret3 == compare3)) {
          if ((0 == a.valueWithKey("int16", &ret4)) && (ret4 == compare4)) {
            if ((0 == a.valueWithKey("int8", &ret5)) && (ret5 == compare5)) {
              if ((0 == a.valueWithKey("float", &ret6)) && (ret6 == compare6)) {
                if ((0 == a.valueWithKey("double", &ret7)) && (ret7 == compare7)) {
                  if ((0 == a.valueWithKey("Vector3<f>", &ret8)) && (ret8 == compare8)) {
                    printf("Value Translation tests pass.\n");
                    return_value = 0;
                  }
                  else printf("Failed to vet Vector3<float> --> Vector3<int32>\n");
                }
                else printf("Failed to vet double --> int32_t\n");
              }
              else printf("Failed to vet float --> int8\n");
            }
            else printf("Failed to vet int8 --> int16\n");
          }
          else printf("Failed to vet int16 --> int32\n");
        }
        else printf("Failed to vet int32 --> double\n");
      }
      else printf("Failed to vet uint8_t --> uint16_t\n");
    }
    else printf("Failed to vet uint16_t --> uint32_t\n");
  }
  else printf("Failed to vet uint32_t --> double\n");

  // TODO: If the safe translations passed, try the ones that should be error-cases.
  if (0 == return_value) {
  }

  return return_value;
}



/**
* This is the test of key-related edge-cases.
*
* @return [description]
*/
int test_KeyValuePair_Key_Abuse() {
  int return_value = -1;
  printf("===< KeyValuePair Key Abuse >=========================\n");

  const char* feed_str = "mallocd_key";
  uint32_t m_str_len = strlen(feed_str);

  uint32_t val0 = (uint32_t) randomUInt32();
  uint32_t val1 = (uint32_t) randomUInt32();
  uint32_t val2 = (uint32_t) randomUInt32();
  uint32_t val3 = (uint32_t) randomUInt32();
  uint32_t val4 = (uint32_t) randomUInt32();
  uint32_t val5 = (uint32_t) randomUInt32();
  uint32_t val6 = (uint32_t) randomUInt32();
  uint32_t val7 = (uint32_t) randomUInt32();
  uint32_t val8 = (uint32_t) randomUInt32();
  uint32_t val9 = (uint32_t) randomUInt32();

  // Experimental values.
  uint32_t ret0 = 0;
  uint32_t ret1 = 0;
  uint32_t ret2 = 0;
  uint32_t ret3 = 0;
  uint32_t ret4 = 0;
  uint32_t ret5 = 0;
  uint32_t ret6 = 0;
  uint32_t ret7 = 0;
  uint32_t ret8 = 0;
  uint32_t ret9 = 0;

  const char* key0 = "safe";       // A safe test key.
  const char* key1 = "\t \n\r  ";  // Exotic whitespace is also valid.
  const char* key2 = "duplicate";  // Duplicate keys are allowed, but the second
  const char* key3 = "duplicate";  //   key will only be accessible by index.
  const char* key4 = nullptr;      // This should be the same as not passing a key.
  const char* key5 = "";           // Empty string is a valid key.
  const char* key6 = "test6";
  const char* key7 = "test7";
  const char* key8 = "test8";
  char* key9 = (char*) malloc(m_str_len);
  memcpy(key9, feed_str, m_str_len);
  *(key9 + m_str_len) = '\0';

  KeyValuePair a(val0, key0);
  if (a.append(val1, key1)) {
    if (a.append(val2, key2)) {
      if (a.append(val3, key3)) {
        if (a.append(val4, key4)) {
          if (a.append(val5, key5)) {
            if (a.append(val6, key6)) {
              if (a.append(val7, key7)) {
                if (a.append(val8, key8)) {
                  if (a.append(val9, key9)) {
                    if ((0 == a.valueWithKey(key0, &ret0)) && (ret0 == val0)) {
                      if ((0 == a.valueWithKey(key1, &ret1)) && (ret1 == val1)) {
                        if ((0 == a.valueWithKey(key2, &ret2)) && (ret2 == val2)) {
                          if ((0 == a.valueWithKey(key3, &ret3)) && (ret3 == val3)) {
                            if ((0 == a.valueWithKey(key4, &ret4)) && (ret4 == val4)) {
                              if ((0 == a.valueWithKey(key5, &ret5)) && (ret5 == val5)) {
                                if ((0 == a.valueWithKey(key6, &ret6)) && (ret6 == val6)) {
                                  if ((0 == a.valueWithKey(key7, &ret7)) && (ret7 == val7)) {
                                    if ((0 == a.valueWithKey(key8, &ret8)) && (ret8 == val8)) {
                                      if ((0 == a.valueWithKey(key9, &ret9)) && (ret9 == val9)) {
                                        printf("Key abuse tests pass.\n");
                                        return_value = 0;
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  dump_kvp(&a);

  return return_value;
}



#if defined(CONFIG_C3P_CBOR)
/**
* [test_CBOR_KVP description]
* @return [description]
*/
int test_CBOR_KeyValuePair() {
  int return_value = -1;
  printf("===< KVPs CBOR >===================================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = ((uint32_t) randomUInt32()) / ((float) randomUInt32());
  Vector3<float> val7(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);
  double   val8  = ((uint32_t) randomUInt32()) / ((double) randomUInt32());

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;
  float    ret6 = 0.0f;
  Vector3<float> ret7(0.0f, 0.0f, 0.0f);
  double   ret8 = 0.0f;

  KeyValuePair a(val0, "val0");
  a.append(val1)->setKey("val1");
  a.append(val2)->setKey("val2");
  a.append(val3)->setKey("val3");
  a.append(val4)->setKey("val4");
  a.append(val5)->setKey("val5");
  a.append(val6)->setKey("val6");
  a.append(&val7)->setKey("val7");
  a.append(val8, "val8");
  dump_kvp(&a);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);
  KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);

  if (0 == ret_local) {
    printf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    dump_strbldr(&shuttle);

    if (nullptr != r) {
      printf("CBOR decoded:\n");
      dump_kvp(r);

      if ((0 == r->valueWithIdx(0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->valueWithIdx(1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->valueWithIdx(2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->valueWithIdx(3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->valueWithIdx(4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->valueWithIdx(5, &ret5)) && (ret5 == val5)) {
                  if ((0 == r->valueWithIdx(6, &ret6)) && (ret6 == val6)) {
                    if ((0 == r->valueWithKey("val8", &ret8)) && (ret8 == val8)) {
                      if (r->count() == a.count()) {
                        return_value = 0;
                      }
                      else printf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
                    }
                    else printf("Failed to vet key 'value8'... %.6f vs %.6f\n", ret8, val8);
                  }
                  else printf("Failed to vet key 'value6'... %.3f vs %.3f\n", (double) ret6, (double) val6);
                }
                else printf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else printf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else printf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else printf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else printf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else printf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else printf("Failed to decode KVP chain from CBOR...\n");
  }
  else printf("Failed to encode KVP chain into CBOR: %d\n", ret_local);

  return return_value;
}


/**
* These are values that give the CBOR implementation special flavors of grief.
* Usually, this is due to a boundary condition in the CBOR spec being
*   implemented poorly. All such known cases are implemented here.
*
* [test_CBOR_Problematic_KeyValuePair description]
* @return [description]
*/
int test_CBOR_Problematic_KeyValuePair() {
  int return_value = -1;
  printf("===< KeyValuePairs CBOR Minefield >=========================\n");
  StringBuilder shuttle;  // We will transport the CBOR encoded-bytes through this.

  int32_t  val0  = (int32_t)  -65500;
  int16_t  val1  = (int16_t)  -230;
  int8_t   val2  = (int8_t)   -23;
  uint32_t val3  = (uint32_t) 3643900856;
  uint16_t val4  = (uint16_t) 59041;
  uint8_t  val5  = (uint8_t)  250;

  int32_t  ret0 = 0;
  int16_t  ret1 = 0;
  int8_t   ret2 = 0;
  uint32_t ret3 = 0;
  uint16_t ret4 = 0;
  uint8_t  ret5 = 0;

  KeyValuePair a(val0);

  a.setKey("val0");
  a.append(val1)->setKey("val1");
  a.append(val2)->setKey("val2");  // NOTE: Mixed in with non-KVP.
  a.append(val3)->setKey("val3");
  a.append(val4)->setKey("val4");
  a.append(val5)->setKey("val5");

  dump_kvp(&a);

  int8_t ret_local = a.serialize(&shuttle, TCode::CBOR);
  if (0 == ret_local) {
    printf("CBOR encoding occupies %d bytes\n\t", shuttle.length());
    dump_strbldr(&shuttle);

    KeyValuePair* r = KeyValuePair::unserialize(shuttle.string(), shuttle.length(), TCode::CBOR);
    if (nullptr != r) {
      printf("CBOR decoded:\n");
      dump_kvp(r);

      if ((0 == r->valueWithIdx((uint8_t) 0, &ret0)) && (ret0 == val0)) {
        if ((0 == r->valueWithIdx((uint8_t) 1, &ret1)) && (ret1 == val1)) {
          if ((0 == r->valueWithIdx((uint8_t) 2, &ret2)) && (ret2 == val2)) {
            if ((0 == r->valueWithIdx((uint8_t) 3, &ret3)) && (ret3 == val3)) {
              if ((0 == r->valueWithIdx((uint8_t) 4, &ret4)) && (ret4 == val4)) {
                if ((0 == r->valueWithIdx((uint8_t) 5, &ret5)) && (ret5 == val5)) {
                    if (r->count() == a.count()) {
                      return_value = 0;
                    }
                    else printf("Arg counts don't match: %d vs %d\n", r->count(), a.count());
                }
                else printf("Failed to vet key 'value5'... %u vs %u\n", ret5, val5);
              }
              else printf("Failed to vet key 'value4'... %u vs %u\n", ret4, val4);
            }
            else printf("Failed to vet key 'value3'... %u vs %u\n", ret3, val3);
          }
          else printf("Failed to vet key 'value2'... %u vs %u\n", ret2, val2);
        }
        else printf("Failed to vet key 'value1'... %u vs %u\n", ret1, val1);
      }
      else printf("Failed to vet key 'value0'... %u vs %u\n", ret0, val0);
    }
    else printf("Failed to decode KeyValuePair chain from CBOR...\n");
  }
  else printf("Failed to encode KeyValuePair chain into CBOR...\n");

  if (return_value) {
    dump_kvp(&a);
  }

  return return_value;
}
#endif  // CONFIG_C3P_CBOR


/**
* This is the test of KVP's ability to accept the types it claims to support.
*
* @return [description]
*/
int test_KeyValuePair_Build_Polytyped_KVP(KeyValuePair* a) {
  int return_value = -1;
  printf("===< KeyValuePair Build_Polytyped_KVP >=========================\n");

  int32_t  val0  = (int32_t)  randomUInt32();
  int16_t  val1  = (int16_t)  randomUInt32();
  int8_t   val2  = (int8_t)   randomUInt32();
  uint32_t val3  = (uint32_t) randomUInt32();
  uint16_t val4  = (uint16_t) randomUInt32();
  uint8_t  val5  = (uint8_t)  randomUInt32();
  float    val6  = (randomUInt32() / (float) randomUInt32());
  double   val7  = (double)randomUInt32()/(double)randomUInt32();
  char*    val8  = (char*) "A non-const test string";
  bool     val9  = true;
  Vector3<float> val10(
    randomUInt32()/(float)randomUInt32(),
    randomUInt32()/(float)randomUInt32(),
    randomUInt32()/(float)randomUInt32()
  );
  Vector3<uint32_t> val11(randomUInt32(), randomUInt32(), randomUInt32());
  const int TEST_BUFFER_SIZE = 16;
  void* val20 = malloc(TEST_BUFFER_SIZE);
  for (uint8_t i = 0; i < TEST_BUFFER_SIZE; i++) {   *((uint8_t*) val20 + i) = i;   }

  if (nullptr != a->append(val0, "int32")) {
    if (nullptr != a->append(val1, "int16")) {
      if (nullptr != a->append(val2, "int8")) {
        if (nullptr != a->append(val3, "uint32")) {
          if (nullptr != a->append(val4, "uint16")) {
            if (nullptr != a->append(val5, "uint8")) {
              if (nullptr != a->append(val6, "float")) {
                if (nullptr != a->append(val7, "double")) {
                  if (nullptr != a->append(val8, "char*")) {
                    if (nullptr != a->append(val9, "bool")) {
                      if (nullptr != a->append(&val10, "Vector3<f>")) {
                        if (nullptr != a->append(&val11, "Vector3<u32>")) {
                          KeyValuePair* raw_buf_kvp = a->append(val20, TEST_BUFFER_SIZE, "raw_buf");
                          if (nullptr != raw_buf_kvp) {
                            raw_buf_kvp->reapValue(true);
                            printf("Successfully built a test KVP:\n");
                            dump_kvp(a);
                            return_value = 0;
                          }
                          else printf("Failed to append a void*/len\n");
                        }
                        else printf("Failed to append a Vector3<u32>\n");
                      }
                      else printf("Failed to append a Vector3<f>\n");
                    }
                    else printf("Failed to append a bool\n");
                  }
                  else printf("Failed to append a char*\n");
                }
                else printf("Failed to append a double\n");
              }
              else printf("Failed to append a float\n");
            }
            else printf("Failed to append a uint8\n");
          }
          else printf("Failed to append a uint16\n");
        }
        else printf("Failed to append a uint32\n");
      }
      else printf("Failed to append a int8\n");
    }
    else printf("Failed to append a int16\n");
  }
  else printf("Failed to append a int32\n");

  return return_value;
}


void print_types_kvp() {
  printf("\tKeyValuePair          %u\t%u\n", sizeof(KeyValuePair),   alignof(KeyValuePair));
}


/**
* This is the root of the KeyValuePair tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int test_KeyValuePair() {
  const char* const MODULE_NAME = "KeyValuePair";
  printf("===< %s >=======================================\n", MODULE_NAME);

  KeyValuePair a("A const test string", "constchar*");    // const char* test
  int return_value = test_KeyValuePair_Build_Polytyped_KVP(&a);
  if (0 == return_value) {
    return_value = test_KeyValuePair_KVP();
    if (0 == return_value) {
      return_value = test_KeyValuePair_InternalTypes();
      if (0 == return_value) {
        return_value = test_KeyValuePair_Value_Placement();
        if (0 == return_value) {
          //return_value = test_KeyValuePair_Key_Abuse();
          if (0 == return_value) {
            //return_value = test_KeyValuePair_Value_Translation();
            if (0 == return_value) {
            #if defined(CONFIG_C3P_CBOR)
              return_value = test_CBOR_KeyValuePair();
              if (0 == return_value) {
                return_value = test_CBOR_Problematic_KeyValuePair();
                if (0 == return_value) {
                  printf("**********************************\n");
                  printf("*  KeyValuePair tests all pass   *\n");
                  printf("**********************************\n");
                }
                else printTestFailure(MODULE_NAME, "KVP_CBOR_Problematic_KeyValuePair");
              }
              else printTestFailure(MODULE_NAME, "test_CBOR_KeyValuePair");
              #endif  // CONFIG_C3P_CBOR
            }
            else printTestFailure(MODULE_NAME, "test_Value_Translation");
          }
          else printTestFailure(MODULE_NAME, "KeyValuePair_Key_Abuse");
        }
        else printTestFailure(MODULE_NAME, "KVP_Value_Placement");
      }
      else printTestFailure(MODULE_NAME, "KVP_InternalTypes");
    }
    else printTestFailure(MODULE_NAME, "KVP_value_retrieval");
  }
  else printTestFailure(MODULE_NAME, "Build_Polytyped_KVP");

  if (0 == return_value) {
  }
  return return_value;
}
