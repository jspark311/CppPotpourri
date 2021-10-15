/*
File:   ManuvrLinkTests.cpp
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


This program runs tests against the M2M communication class.
*/

/*******************************************************************************
* Globals
*******************************************************************************/

KeyValuePair* args_sent = nullptr;
KeyValuePair* args_recd = nullptr;


/*******************************************************************************
* Callbacks and polling functions
*******************************************************************************/

void callback_vlad(uint32_t tag, ManuvrMsg* msg) {
  printf("callback_vlad(0x%x): \n", tag, msg->uniqueId());
}


void callback_carl(uint32_t tag, ManuvrMsg* msg) {
  printf("callback_carl(0x%x): \n", tag, msg->uniqueId());
}


bool poll_until_finished(ManuvrLink* vlad, ManuvrLink* carl) {
  int polling_cycles = 0;
  bool idle = false;
  uint32_t now = millis();
  uint32_t timeout_start = now;
  uint32_t timeout_end   = timeout_start + 5000;
  while ((now < timeout_end) & (!idle)) {
    StringBuilder log_v;
    StringBuilder log_c;
    vlad->poll(&log_v);
    carl->poll(&log_c);
    idle = vlad->linkIdle() & carl->linkIdle();
    if (0 < log_v.length()) {   printf("Vlad (%06d):\n%s\n", polling_cycles, (const char*) log_v.string());  }
    if (0 < log_c.length()) {   printf("Carl (%06d):\n%s\n", polling_cycles, (const char*) log_c.string());  }
    polling_cycles++;
    sleep_ms(1);
    now = millis();
  }
  printf("poll_until_finished completed in %d cycles.\n", polling_cycles);
  return (now < timeout_end);
}


/*******************************************************************************
* ManuvrMsg functionality
*******************************************************************************/

ManuvrMsgHdr connect_header(ManuvrMsgCode::CONNECT, 0, true);

/* Header tests */
int link_tests_message_battery_0() {
  int ret = -1;
  StringBuilder log("===< ManuvrMsg battery 0 (Header) >==========================\n");
  ManuvrMsgHdr msg_valid_with_reply(ManuvrMsgCode::SYNC_KEEPALIVE, 0, true);
  ManuvrMsgHdr msg_valid_without_reply(ManuvrMsgCode::SYNC_KEEPALIVE, 0, false);
  ManuvrMsgHdr msg_valid_reply_without_id(ManuvrMsgCode::CONNECT, 0, true);
  ManuvrMsgHdr msg_invalid_bad_code(ManuvrMsgCode::UNDEFINED, 0, false);

  if (msg_valid_with_reply.isValid()) {
    if (msg_valid_with_reply.expectsReply()) {
      if (!msg_valid_with_reply.isReply()) {
        if (msg_valid_with_reply.msg_id == 0) {
          if (msg_valid_with_reply.isSync()) {
            log.concat("\t msg_valid_with_reply passes tests.\n");
            ret = 0;
          }
          else log.concat("msg_valid_with_reply does not identify as a SYNC.\n");
        }
        else log.concat("With reply: SYNC headers created without IDs should not be assigned one.\n");
      }
      else log.concat("With reply: isReply() should have returned false.\n");
    }
    else log.concat("With reply: expectsReply() should have returned true.\n");
  }
  else log.concat("With reply: A valid header was construed as invalid.\n");

  if (0 == ret) {
    ret--;
    if (msg_valid_without_reply.isValid()) {
      if (!msg_valid_without_reply.expectsReply()) {
        if (!msg_valid_without_reply.isReply()) {
          if (msg_valid_without_reply.msg_id == 0) {
            if (msg_valid_without_reply.isSync()) {
              log.concat("\t msg_valid_without_reply passes tests.\n");
              ret = 0;
            }
            else log.concat("msg_valid_without_reply does not identify as a SYNC.\n");
          }
          else log.concat("Without SYNC headers created without IDs should not be assigned one.\n");
        }
        else log.concat("Without isReply() should have returned false.\n");
      }
      else log.concat("Without expectsReply() should have returned false.\n");
    }
    else log.concat("Without reply: A valid header was construed as invalid.\n");
  }

  if (0 == ret) {
    ret--;
    // Setting the payload length member directly will subvert the class's length
    //   field checks, and will thus not update the flags.
    ManuvrMsgHdr msg_invalid_bad_length(ManuvrMsgCode::CONNECT, 6, false);
    msg_invalid_bad_length.msg_len = 0x1f000;  // Make the length require too many bytes.
    msg_invalid_bad_length.rebuild_checksum();   // Ensure it isn't a checksum fault.
    if (!msg_invalid_bad_length.isValid()) {
      if (!msg_invalid_bad_code.isValid()) {
        // Here, we'll make a change to the header byte, but we won't update the
        //   checksum.
        ManuvrMsgHdr msg_invalid_bad_chksum(ManuvrMsgCode::CONNECT, 0, false);
        msg_invalid_bad_chksum.expectsReply(true);
        if (!msg_invalid_bad_chksum.isValid()) {
          // Replies can't happen without an ID. If the ManuvrMsgHdr constructor
          //   knows that one will be needed, it will generate one. But in this
          //   case, we'll construct the header as requiring no reply, but then
          //   we'll change out mind.
          ManuvrMsgHdr msg_invalid_reply_without_id(ManuvrMsgCode::CONNECT, 0, false);
          msg_invalid_reply_without_id.expectsReply(true);  // ManuvrMsg should accomodate this.
          msg_invalid_reply_without_id.rebuild_checksum();   // Ensure it isn't a checksum fault.
          if (!msg_invalid_reply_without_id.isValid()) {
            log.concat("\t msg_invalid_reply_without_id passes tests.\n");
            ret = 0;
          }
          else log.concat("msg_invalid_reply_without_id was construed as valid.\n");
        }
        else log.concat("msg_invalid_bad_chksum was construed as valid.\n");
      }
      else log.concat("msg_invalid_bad_code was construed as valid.\n");
    }
    else log.concat("msg_invalid_bad_length was construed as valid.\n");
  }

  if (0 == ret) {
    ret--;
    ManuvrMsgHdr stupid_simple_sync(ManuvrMsgCode::SYNC_KEEPALIVE);
    if (stupid_simple_sync.isValid()) {
      msg_valid_with_reply.wipe();
      stupid_simple_sync.rebuild_checksum();
      if (stupid_simple_sync.isValid()) {
        ret = 0;
      }
      else log.concat("stupid_simple_sync was construed as valid following wipe.\n");
    }
    else log.concat("stupid_simple_sync was construed as invalid.\n");
  }

  printf("%s\n\n", (const char*) log.string());
  return ret;
}


/* Message pack-parse tests */
int link_tests_message_battery_1() {
  int ret = -1;
  StringBuilder log("===< ManuvrMsg battery 1 (Parse-pack) >=======================\n");
  ManuvrMsgHdr hdr_parse_pack_0(ManuvrMsgCode::APPLICATION, 0, true);
  ManuvrMsg* msg_parse_pack_0 = new ManuvrMsg(&hdr_parse_pack_0, BusOpcode::TX);
  if (msg_parse_pack_0) {
    uint32_t now     = millis();
    uint32_t rand    = randomUInt32();
    const char* val_str = "my_value";
    float    val_flt = (float) randomUInt32()/1000000.0f;
    double   val_dbl = (double) randomUInt32() / (double) randomUInt32();
    Vector3<float> vect(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

    KeyValuePair a(now, "time_ms");
    a.append(rand, "rand");
    a.append(val_str, "my_key");
    a.append(val_flt, "val_flt");
    a.append(val_dbl, "val_dbl");
    a.append(&vect, "vect");
    //a.printDebug(&log);
    msg_parse_pack_0->setPayload(&a);
    StringBuilder msg_0_serial;
    if (0 == msg_parse_pack_0->serialize(&msg_0_serial)) {
      msg_parse_pack_0->printDebug(&log);
      if (!msg_0_serial.isEmpty()) {
        msg_0_serial.printDebug(&log);
        ManuvrMsg* msg_parse_pack_1 = ManuvrMsg::unserialize(&msg_0_serial);
        if (nullptr != msg_parse_pack_1) {
          if (msg_parse_pack_1->rxComplete()) {
            KeyValuePair* pl = nullptr;
            msg_parse_pack_1->getPayload(&pl);
            if (nullptr != pl) {
              // Did all of the arguments come across unscathed?
              uint32_t now_ret     = millis();
              uint32_t rand_ret    = randomUInt32();
              char* val_str_ret = nullptr;
              float    val_flt_ret = (float) randomUInt32()/1000000.0f;
              double   val_dbl_ret = (double) randomUInt32() / (double) randomUInt32();
              Vector3<float> vect_ret(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);
              pl->printDebug(&log);
              if ((0 == a.valueWithKey("time_ms", &now_ret)) && (now_ret == now)) {
                if ((0 == a.valueWithKey("rand", &rand_ret)) && (rand_ret == rand)) {
                  if ((0 == a.valueWithKey("my_key", &val_str_ret)) && (0 == strcasecmp(val_str, val_str_ret))) {
                    if ((0 == a.valueWithKey("val_flt", &val_flt_ret)) && (val_flt_ret == val_flt)) {
                      if ((0 == a.valueWithKey("val_dbl", &val_dbl_ret)) && (val_dbl_ret == val_dbl)) {
                        if ((0 == a.valueWithKey("vect", &vect_ret)) && (vect_ret == vect)) {
                          log.concat("\tParse-pack tests pass.\n");
                          ret = 0;
                        }
                        else log.concat("Failed to vet vect\n");
                      }
                      else log.concat("Failed to vet val_dbl\n");
                    }
                    else log.concat("Failed to vet val_flt\n");
                  }
                  else log.concat("Failed to vet my_key\n");
                }
                else log.concat("Failed to vet rand\n");
              }
              else log.concat("Failed to vet time_ms.\n");
            }
            else log.concat("Failed to retrieve payload.\n");
          }
          else log.concat("ManuvrMsg::unserialize() returned an incomplete message.\n");
        }
        else log.concat("ManuvrMsg::unserialize() failed.\n");
      }
      else log.concat("Serializer produced an empty string.\n");
    }
    else log.concat("Failed to serialize message.\n");
  }
  else log.concat("Failed to allocate message.\n");

  printf("%s\n\n", (const char*) log.string());
  return ret;
}



/*******************************************************************************
* Basic ManuvrLink functionality
*******************************************************************************/

/*
* Setup two Link objects, and connect them together.
* Note that this test is entirely synthetic. The pathway looks like this...
*   callback_vlad <---> vlad <---> carl <---> callback_carl
* In a real-world application, it would have a transport in the middle...
*   callback_vlad <---> vlad <---> UART <---> UART <---> carl <---> callback_carl
* ...or something similar.
*/
int link_tests_build_and_connect(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink Build and connect >====================================\n");
  int ret = -1;
  if ((nullptr != vlad) & (nullptr != carl)) {
    // Connect Vlad's output to Carl's input, and Carl's output to Vlad's input.
    vlad->setOutputTarget(carl);
    carl->setOutputTarget(vlad);
    // Now connect each of them to their respective application callbacks.
    vlad->setCallback(callback_vlad);
    carl->setCallback(callback_carl);
    if (poll_until_finished(vlad, carl)) {
      log.concat("Vlad and Carl are syncd and in an established session.\n");
      ret = 0;
    }
    else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");
    log.concat("\n");
    vlad->printDebug(&log);
    carl->printDebug(&log);
  }
  else log.concat("Failed to allocate two ManuvrLinks.\n");
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_simple_messages(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink Simple messages >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_complex_messages(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink complex messages >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_exotic_encodings(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink exotic encodings >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_message_flood(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink message flood >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_interrupted_transport(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink interrupted transport >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_corrupted_transport(ManuvrLink* vlad, ManuvrLink* carl) {
  StringBuilder log("===< ManuvrLink corrupted transport >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}



/**
* This is the root of the ManuvrLink tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int manuvrlink_main() {
  ManuvrLinkOpts opts_vlad(
    100,   // ACK timeout is 100ms. Vlad is patient.
    2000,  // Send a KA every 2s.
    2048,  // MTU for this link is 2 kibi.
    0      // No flags.
  );
  ManuvrLinkOpts opts_carl(
    40,    // ACK timeout is 40ms.
    2000,  // Send a KA every 2s.
    1024,  // MTU for this link is 1 kibi.
    0      // No flags.
  );
  ManuvrLink* vlad = new ManuvrLink(&opts_vlad);  // One half of the link.
  ManuvrLink* carl = new ManuvrLink(&opts_carl);  // One half of the link.
  int ret = -1;
  if (0 == link_tests_message_battery_0()) {
    if (0 == link_tests_message_battery_1()) {
      if (0 == link_tests_build_and_connect(vlad, carl)) {
        //ret = link_tests_simple_messages(vlad, carl);
        //if (0 == ret) {
          ret = 0;
        //}
        //else printTestFailure("link_tests_simple_messages");
      }
      else printTestFailure("link_tests_build_and_connect");
    }
    else printTestFailure("link_tests_message_battery_1");
  }
  else printTestFailure("link_tests_message_battery_0");

  if (0 == ret) {
    printf("**********************************\n");
    printf("*  ManuvrLink tests all pass     *\n");
    printf("**********************************\n");
  }
  return ret;
}
