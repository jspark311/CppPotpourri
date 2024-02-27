/*
File:   M2MLinkTests.cpp
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

M2MLink* vlad = nullptr;
M2MLink* carl = nullptr;

KeyValuePair* args_sent_vlad = nullptr;
KeyValuePair* args_sent_carl = nullptr;
KeyValuePair* args_recd_vlad = nullptr;
KeyValuePair* args_recd_carl = nullptr;

bool vlad_reply_lockout = false;
bool carl_reply_lockout = false;

int vlad_replies_rxd = 0;
int carl_replies_rxd = 0;


/*******************************************************************************
* Callbacks, value-checking, and polling functions
*******************************************************************************/

void check_that_kvps_match(StringBuilder* log, KeyValuePair* k0, KeyValuePair* k1) {
  if (nullptr != k0) {
    log->concat("\n\tKVP Sent:\n\t------------------\n");
    k0->printDebug(log);
  }
  if (nullptr != k1) {
   log->concat("\n\tKVP Received:\n\t--------------\n");
   k1->printDebug(log);
  }
}


void callback_link_state(M2MLink* cb_link) {
  StringBuilder log;
  log.concatf("Link (0x%x) entered state %s\n", cb_link->linkTag(), M2MLink::sessionStateStr(cb_link->getState()));
  printf("%s\n\n", (const char*) log.string());
}


void callback_vlad(uint32_t tag, M2MMsg* msg) {
  StringBuilder log;
  KeyValuePair* kvps_rxd = nullptr;
  log.concatf("callback_vlad(0x%x): \n", tag, msg->uniqueId());
  msg->printDebug(&log);
  msg->getPayload(&kvps_rxd);
  check_that_kvps_match(&log, args_sent_carl, kvps_rxd);
  args_recd_vlad = kvps_rxd;
  if (msg->isReply()) {
    vlad_replies_rxd++;
  }
  if ((!vlad_reply_lockout) && msg->expectsReply()) {
    log.concatf("\ncallback_vlad ACK's %d.\n", msg->ack());
  }
  printf("%s\n\n", (const char*) log.string());
}


void callback_carl(uint32_t tag, M2MMsg* msg) {
  StringBuilder log;
  KeyValuePair* kvps_rxd = nullptr;
  log.concatf("callback_carl(0x%x): \n", tag, msg->uniqueId());
  msg->printDebug(&log);
  msg->getPayload(&kvps_rxd);
  check_that_kvps_match(&log, args_sent_vlad, kvps_rxd);
  args_recd_carl = kvps_rxd;
  if (msg->isReply()) {
    carl_replies_rxd++;
  }
  if ((!carl_reply_lockout) && msg->expectsReply()) {
    log.concatf("\ncallback_carl ACK's %d.\n", msg->ack());
  }
  printf("%s\n\n", (const char*) log.string());
}


bool poll_until_disconnected(M2MLink* vlad, M2MLink* carl) {
  int polling_cycles = 0;
  bool idle = false;
  uint32_t now = millis();
  uint32_t timeout_start = now;
  uint32_t timeout_end   = timeout_start + 1000;
  while ((now < timeout_end) & (!idle)) {
    StringBuilder log_v;
    StringBuilder log_c;
    vlad->poll(&log_v);
    carl->poll(&log_c);
    idle = !(vlad->isConnected() | carl->isConnected());
    if (0 < log_v.length()) {   printf("Vlad (%06d):\n%s\n", polling_cycles, (const char*) log_v.string());  }
    if (0 < log_c.length()) {   printf("Carl (%06d):\n%s\n", polling_cycles, (const char*) log_c.string());  }
    polling_cycles++;
    sleep_ms(1);
    now = millis();
  }
  printf("poll_until_disconnected completed in %d cycles.\n", polling_cycles);
  return (now < timeout_end);
}


bool poll_until_finished(M2MLink* vlad, M2MLink* carl) {
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
* M2MMsg functionality
*******************************************************************************/

/* Header tests */
int link_tests_message_battery_0() {
  int ret = -1;
  StringBuilder log("===< M2MMsg battery 0 (Header) >==========================\n");
  M2MMsgHdr msg_valid_with_reply(M2MMsgCode::SYNC_KEEPALIVE, 0, true);
  M2MMsgHdr msg_valid_without_reply(M2MMsgCode::SYNC_KEEPALIVE, 0, false);
  M2MMsgHdr msg_valid_reply_without_id(M2MMsgCode::CONNECT, 0, true);
  M2MMsgHdr msg_invalid_bad_code(M2MMsgCode::UNDEFINED, 0, false);

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
    M2MMsgHdr msg_invalid_bad_length(M2MMsgCode::CONNECT, 6, false);
    msg_invalid_bad_length.msg_len = 0x1f000;  // Make the length require too many bytes.
    msg_invalid_bad_length.rebuild_checksum();   // Ensure it isn't a checksum fault.
    if (!msg_invalid_bad_length.isValid()) {
      if (!msg_invalid_bad_code.isValid()) {
        // Here, we'll make a change to the header byte, but we won't update the
        //   checksum.
        M2MMsgHdr msg_invalid_bad_chksum(M2MMsgCode::CONNECT, 0, false);
        msg_invalid_bad_chksum.expectsReply(true);
        if (!msg_invalid_bad_chksum.isValid()) {
          // Replies can't happen without an ID. If the M2MMsgHdr constructor
          //   knows that one will be needed, it will generate one. But in this
          //   case, we'll construct the header as requiring no reply, but then
          //   we'll change out mind.
          M2MMsgHdr msg_invalid_reply_without_id(M2MMsgCode::CONNECT, 0, false);
          msg_invalid_reply_without_id.expectsReply(true);  // M2MMsg should accomodate this.
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
    M2MMsgHdr stupid_simple_sync(M2MMsgCode::SYNC_KEEPALIVE);
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
  StringBuilder log("===< M2MMsg battery 1 (Parse-pack) >=======================\n");
  M2MMsgHdr hdr_parse_pack_0(M2MMsgCode::APPLICATION, 0, true);
  M2MMsg* msg_parse_pack_0 = new M2MMsg(&hdr_parse_pack_0, BusOpcode::TX);
  if (msg_parse_pack_0) {
    uint32_t now     = millis();
    uint32_t rand    = randomUInt32();
    const char* val_str = "my_value";
    float    val_flt = (float) randomUInt32()/1000000.0f;
    double   val_dbl = (double) randomUInt32() / (double) randomUInt32();
    Vector3<float> vect(randomUInt32()/1000000.0f, randomUInt32()/1000000.0f, randomUInt32()/1000000.0f);

    KeyValuePair a("time_ms", now);
    a.append(rand, "rand");
    a.append(val_str, "my_key");
    a.append(val_flt, "val_flt");
    a.append(val_dbl, "val_dbl");
    a.append(&vect, "vect");
    //a.printDebug(&log);
    if (0 == msg_parse_pack_0->setPayload(&a)) {
      StringBuilder msg_0_serial;
      if (0 == msg_parse_pack_0->serialize(&msg_0_serial)) {
        msg_parse_pack_0->printDebug(&log);
        if (!msg_0_serial.isEmpty()) {
          msg_0_serial.printDebug(&log);
          M2MMsg* msg_parse_pack_1 = M2MMsg::unserialize(&msg_0_serial);
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
            else log.concat("M2MMsg::unserialize() returned an incomplete message.\n");
          }
          else log.concat("M2MMsg::unserialize() failed.\n");
        }
        else log.concat("Serializer produced an empty string.\n");
      }
      else log.concat("Failed to serialize message.\n");
    }
    else log.concat("Failed to set payload.\n");
  }
  else log.concat("Failed to allocate message.\n");

  printf("%s\n\n", (const char*) log.string());
  return ret;
}



/*******************************************************************************
* Basic M2MLink functionality
*******************************************************************************/

/*
* Setup two Link objects, and connect them together.
* Note that this test is entirely synthetic. The pathway looks like this...
*   callback_vlad <---> vlad <---> carl <---> callback_carl
* In a real-world application, it would have a transport in the middle...
*   callback_vlad <---> vlad <---> UART <---> UART <---> carl <---> callback_carl
* ...or something similar.
*/
int link_tests_build_and_connect(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink Build and connect >====================================\n");
  int ret = -1;
  if ((nullptr != vlad) & (nullptr != carl)) {
    // Connect Vlad's output to Carl's input, and Carl's output to Vlad's input.
    vlad->setEfferant(carl);
    carl->setEfferant(vlad);
    // Now connect each of them to their respective application callbacks.
    vlad->setCallback(callback_vlad);
    carl->setCallback(callback_carl);
    vlad->setCallback(callback_link_state);  // Both links share the same state callback.
    carl->setCallback(callback_link_state);  // Both links share the same state callback.

    if (poll_until_finished(vlad, carl)) {
      log.concat("Vlad and Carl are syncd and in an established session.\n");
      ret = 0;
    }
    else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");
    log.concat("\n");
    vlad->printDebug(&log);
    carl->printDebug(&log);
  }
  else log.concat("Failed to allocate two M2MLinks.\n");
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


/*
* Uses the previously-setup links to move some messages.
*/
int link_tests_simple_messages(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink Simple messages >====================================\n");
  int ret = -1;
  int ret_local = -1;
  if ((nullptr != vlad) & (nullptr != carl) && vlad->linkIdle() && carl->linkIdle()) {
    KeyValuePair a("time_ms", (uint32_t) millis());
    a.append((uint32_t) randomUInt32(), "rand");
    ret_local = vlad->send(&a);
    if (0 <= ret_local) {
      args_sent_vlad = &a;
      if (poll_until_finished(vlad, carl)) {
        KeyValuePair b("time_ms", (uint32_t) millis());
        b.append((uint32_t) randomUInt32(), "reply_test");
        ret_local = vlad->send(&b, true);
        if (0 <= ret_local) {
          args_sent_vlad = &b;
          if (poll_until_finished(vlad, carl)) {
            //vlad->printDebug(&log);
            //carl->printDebug(&log);
            if (vlad_replies_rxd == 1) {
              //args_recd_vlad->printDebug(&log);
              carl_reply_lockout = true;
              ret_local = vlad->send(&b, true);
              if (0 <= ret_local) {
                if (poll_until_finished(vlad, carl)) {
                  if (vlad->replyTimeouts() == 1) {
                    log.concat("\tSimple messages pass tests.\n");
                    ret = 0;
                  }
                  else log.concat("Vlad should have given up sending a message that got no reply.\n");
                }
                else log.concat("Failed to send. Link dead-locked.\n");
              }
              else log.concat("Vlad failed to send a second message that needed a reply.\n");
            }
            else log.concat("Vlad should have received a reply, and didn't.\n");
          }
          else log.concat("Failed to send. Link dead-locked.\n");
        }
        else log.concatf("Vlad failed to send a reply-required message to Carl. send() returned %d.\n", ret_local);
      }
      else log.concat("Failed to send. Link dead-locked.\n");
    }
    else log.concatf("Vlad failed to send to Carl. send() returned %d.\n", ret_local);
  }
  else log.concat("Either Vlad or Carl is not ready for the test.\n");

  vlad->poll(&log);
  carl->poll(&log);
  carl_reply_lockout = false;

  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_complex_messages(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink complex messages >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_exotic_encodings(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink exotic encodings >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_message_flood(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink message flood >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_remote_log_insertion(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink remote log insertion >=============================\n");
  int ret = -1;
  if ((nullptr != vlad) & (nullptr != carl) && vlad->isConnected() && carl->isConnected()) {
    StringBuilder sendlog_vlad("This is a log from Vlad (no reply).");
    StringBuilder sendlog_carl("This is a log from Carl (no reply).");
    if (0 == vlad->writeRemoteLog(&sendlog_vlad, false)) {
      sendlog_vlad.concat("This is a log from Vlad (demands reply this time).");
      if (poll_until_finished(vlad, carl)) {
        if (0 == vlad->writeRemoteLog(&sendlog_vlad, true)) {
          if (poll_until_finished(vlad, carl)) {
            if (0 == carl->writeRemoteLog(&sendlog_carl, false)) {
              sendlog_carl.concat("This is a log from Carl (demands reply this time).");
              if (poll_until_finished(vlad, carl)) {
                if (0 == carl->writeRemoteLog(&sendlog_carl, true)) {
                  if (poll_until_finished(vlad, carl)) {
                    log.concat("\tRemote log insertion passes tests.\n");
                    ret = 0;
                  }
                  else log.concat("Failed to send. Link dead-locked.\n");
                }
                else log.concat("Carl failed to send LOG with reply.\n");
              }
              else log.concat("Failed to send. Link dead-locked.\n");
            }
            else log.concat("Carl failed to send LOG without reply.\n");
          }
          else log.concat("Failed to send. Link dead-locked.\n");
        }
        else log.concat("Vlad failed to send LOG with reply.\n");
      }
      else log.concat("Failed to send. Link dead-locked.\n");
    }
    else log.concat("Vlad failed to send LOG without reply.\n");
  }
  else log.concat("Either Vlad or Carl is not ready for the test.\n");
  vlad->poll(&log);
  carl->poll(&log);
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_reestablish_after_hangup(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink re-establish after hangup >========================\n");
  int ret = -1;
  int ret_local = -1;
  if ((nullptr != vlad) & (nullptr != carl) && !(vlad->isConnected() | carl->isConnected())) {
    if (0 == carl->reset()) {
      if (0 == vlad->reset()) {
        if (poll_until_finished(vlad, carl)) {
          log.concat("\tRe-establish after hangup passes tests.\n");
          ret = 0;
        }
        else log.concat("Failed to send. Link dead-locked.\n");
      }
      else log.concat("Vlad failed to reset()\n");
    }
    else log.concat("Carl failed to reset()\n");
  }
  else log.concat("Either Vlad or Carl is not ready for the test.\n");

  vlad->poll(&log);
  carl->poll(&log);
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_hangup_gentle(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink gentle hangup >====================================\n");
  int ret = -1;
  int ret_local = -1;
  if ((nullptr != vlad) & (nullptr != carl) && vlad->linkIdle() && carl->linkIdle()) {
    ret_local = carl->hangup();
    if (0 == ret_local) {
      if (poll_until_disconnected(vlad, carl)) {
        log.concat("\tGentle hangup passes tests.\n");
        ret = 0;
      }
      else log.concat("Failed to HANGUP. Link dead-locked.\n");
    }
    else log.concatf("Carl failed to HANGUP. Returned %d\n", ret_local);
  }
  else log.concat("Either Vlad or Carl is not ready for the test.\n");

  vlad->poll(&log);
  carl->poll(&log);
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_hangup_abrupt(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink abrupt hangup >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


int link_tests_interrupted_transport(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink interrupted transport >====================================\n");
  int ret = -1;
  printf("%s\n\n", (const char*) log.string());
  return ret;
}


/*
* Feed garbage into the stream, and make sure the link resyncs.
*/
int link_tests_corrupted_transport(M2MLink* vlad, M2MLink* carl) {
  StringBuilder log("===< M2MLink corrupted transport >====================================\n");
  int ret = -1;
  if ((nullptr != vlad) & (nullptr != carl)) {
    uint32_t buf_0[4] = {randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32()};
    uint32_t buf_1[4] = {randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32()};
    StringBuilder garbage_for_vlad;
    StringBuilder garbage_for_carl;
    garbage_for_vlad.concat((uint8_t*) &buf_0[0], 16);
    garbage_for_carl.concat((uint8_t*) &buf_1[0], 16);
    vlad->pushBuffer(&garbage_for_vlad);
    carl->pushBuffer(&garbage_for_carl);
    if (poll_until_finished(vlad, carl)) {
      carl->pushBuffer(&garbage_for_carl);
      carl->pushBuffer(&garbage_for_carl);
      carl->pushBuffer(&garbage_for_carl);
      carl->pushBuffer(&garbage_for_carl);
      if (poll_until_finished(vlad, carl)) {
        log.concat("Vlad and Carl resyncd after being fed garbage.\n");
        ret = 0;
      }
      else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");
    }
    else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");
    log.concat("\n");
    vlad->printDebug(&log);
    carl->printDebug(&log);
  }
  else log.concat("Failed to allocate two M2MLinks.\n");
  printf("%s\n\n", (const char*) log.string());
  return ret;
}



void print_types_m2mlink() {
  printf("\tM2MLinkOpts           %u\t%u\n", sizeof(M2MLinkOpts), alignof(M2MLinkOpts));
  printf("\tM2MLink               %u\t%u\n", sizeof(M2MLink), alignof(M2MLink));
  printf("\tM2MMsg                %u\t%u\n", sizeof(M2MMsg), alignof(M2MMsg));
  printf("\tM2MMsgHdr             %u\t%u\n", sizeof(M2MMsgHdr), alignof(M2MMsgHdr));
}


/**
* This is the root of the M2MLink tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int manuvrlink_main() {
  M2MLinkOpts opts_vlad(
    100,   // ACK timeout is 100ms. Vlad is patient.
    2000,  // Send a KA every 2s.
    2048,  // MTU for this link is 2 kibi.
    TCode::CBOR,   // Payloads should be CBOR encoded.
    M2MLINK_FLAG_ALLOW_LOG_WRITE
  );
  M2MLinkOpts opts_carl(
    40,    // ACK timeout is 40ms.
    2000,  // Send a KA every 2s.
    1024,  // MTU for this link is 1 kibi.
    TCode::CBOR,   // Payloads should be CBOR encoded.
    0      // No flags.
  );
  vlad = new M2MLink(&opts_vlad);  // One half of the link.
  carl = new M2MLink(&opts_carl);  // One half of the link.
  vlad->verbosity(6);
  carl->verbosity(6);
  int ret = -1;
  const char* const MODULE_NAME = "M2MLink";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == link_tests_message_battery_0()) {
    if (0 == link_tests_message_battery_1()) {
      if (0 == link_tests_build_and_connect(vlad, carl)) {
        if (0 == link_tests_simple_messages(vlad, carl)) {
          if (0 == link_tests_corrupted_transport(vlad, carl)) {
            if (0 == link_tests_hangup_gentle(vlad, carl)) {
              if (0 == link_tests_reestablish_after_hangup(vlad, carl)) {
                if (0 == link_tests_remote_log_insertion(vlad, carl)) {
                  ret = 0;
                }
                else printTestFailure(MODULE_NAME, "link_tests_remote_log_insertion");
              }
              else printTestFailure(MODULE_NAME, "link_tests_reestablish_after_hangup");
            }
            else printTestFailure(MODULE_NAME, "link_tests_hangup_gentle");
          }
          else printTestFailure(MODULE_NAME, "link_tests_corrupted_transport");
        }
        else printTestFailure(MODULE_NAME, "link_tests_simple_messages");
      }
      else printTestFailure(MODULE_NAME, "link_tests_build_and_connect");
    }
    else printTestFailure(MODULE_NAME, "link_tests_message_battery_1");
  }
  else printTestFailure(MODULE_NAME, "link_tests_message_battery_0");

  if (0 == ret) {
    printf("**********************************\n");
    printf("*  M2MLink tests all pass     *\n");
    printf("**********************************\n");
  }
  vlad->setEfferant(nullptr);
  carl->setEfferant(nullptr);
  delete vlad;
  delete carl;
  return ret;
}
