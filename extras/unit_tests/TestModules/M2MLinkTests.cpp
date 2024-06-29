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

#include "M2MLink/M2MLink.h"
#include "M2MLink/LinkUtils/M2MLinkRPC/M2MLinkRPC.h"




/*******************************************************************************
* Globals
*******************************************************************************/
class M2ML_Test_Vehicle;
void callback_link_state(M2MLink*);   // TODO: Work toward elimination?

M2ML_Test_Vehicle* m2ml_test_current;
M2ML_Test_Vehicle* rpc_test_current;


const TCode ARGSET_N[] = {TCode::NONE};
const TCode ARGSET_0[] = {TCode::INT8, TCode::INT8, TCode::NONE};
const TCode ARGSET_1[] = {TCode::INT16, TCode::INT16, TCode::NONE};

const C3PDefinedRPC RPC_TEST_HOST_DEFS[] = {
  { .RP_NAME   = "client_test",
    .RP_ARGS   = ARGSET_N,
    .PROCEDURE = [](C3PRPCContext* cntxt) {
      return 1;
    }
  },
  { .RP_NAME   = "add8",
    .RP_ARGS   = ARGSET_0,
    .PROCEDURE = [](C3PRPCContext* cntxt) {
      return 1;
    }
  },
  { .RP_NAME   = "add16",
    .RP_ARGS   = ARGSET_1,
    .PROCEDURE = [](C3PRPCContext* cntxt) {
      return 1;
    }
  },
};

M2MLinkRPC_Host* svc_host = nullptr;




/*******************************************************************************
* Types to support the testing...
* TODO: Doing this here is UgLy, Finish the test vehicle, and move it into the
*   LinkUtils path in the source tree as a conditionally-built test-fixture.
*******************************************************************************/

/*
* This class saves repeat code associated with M2MLink lifecycle.
*/
class M2ML_Test_Vehicle {
  public:
    M2MLink peer0;   // "Vlad", for historical reasons...
    M2MLink peer1;   // "Carl", for historical reasons...

    /* Constructor */
    M2ML_Test_Vehicle(
      M2MLinkOpts* opts0, M2MMsgCB callback0,
      M2MLinkOpts* opts1, M2MMsgCB callback1,
      uint8_t v = 6
    ) : peer0(opts0), peer1(opts1) {
      // Connect each peer to their respective application callbacks.
      peer0.verbosity(v);    peer0.setCallback(callback0);
      peer1.verbosity(v);    peer1.setCallback(callback1);
    };

    /* Destructor */
    ~M2ML_Test_Vehicle() {
      peer0.setEfferant(nullptr);   // Disconnect the peers.
      peer1.setEfferant(nullptr);   // They will be destroyed next.
    };


    /* Synchronous construction and setup. */
    int8_t prepareTest() {
      printf("===< M2MLink construction and config >=============================\n");
      int8_t ret = -1;
      // Connect Vlad's output to Carl's input, and Carl's output to Vlad's input.
      peer0.setCallback(callback_link_state);  // Both links share the same state callback.
      peer1.setCallback(callback_link_state);  // Both links share the same state callback.
      peer0.setEfferant((BufferAccepter*) &peer1);
      peer1.setEfferant((BufferAccepter*) &peer0);
      ret = 0;
      return ret;
    };


    /*
    * Setup two Link objects, and connect them together.
    * Note that this test is entirely synthetic. The pathway looks like this...
    *   callback_vlad <---> vlad <---> carl <---> callback_carl
    * In a real-world application, it would have a transport in the middle...
    *   callback_vlad <---> vlad <---> UART <---> UART <---> carl <---> callback_carl
    * ...or something similar.
    */
    PollResult connect_peers() {
      printf("===< M2MLink establishment >=================================\n");
      PollResult ret = PollResult::ERROR;
      if (PollResult::ACTION == poll_until_finished()) {
        printf("The peers are syncd and in an established session.\n");
        ret = PollResult::ACTION;
      }
      else {
        _dump_peers();
        printf("\tThe polling loop ran to its maximum extent. Link dead-locked.\n");
      }
      return ret;
    };


    /*
    * Polls both sides of the link until both peers report a stable IDLE state,
    *   or one of them times out, according to the test-fixture's settings.
    */
    PollResult poll_until_finished() {
      int polling_cycles = 0;
      MillisTimeout polling_timeout(5000);
      bool idle = false;
      polling_timeout.reset();
      while (!(polling_timeout.expired() | idle)) {
        _single_poll();
        idle = peer0.linkIdle() & peer1.linkIdle();
        if (0 < log_0.length()) {   printf("Peer0 (%06d):\n%s\n", (_profiler_polling0.executions() - polling_cycles), (const char*) log_0.string());  }
        if (0 < log_1.length()) {   printf("Peer1 (%06d):\n%s\n", (_profiler_polling1.executions() - polling_cycles), (const char*) log_1.string());  }
        // No need to sleep. We are not transport bottle-necked. Just finish the work.
        sleep_ms(1);
      }
      const int POLLS_THIS_RUN_0 = (_profiler_polling0.executions() - polling_cycles);
      const int POLLS_THIS_RUN_1 = (_profiler_polling1.executions() - polling_cycles);
      printf("poll_until_finished completed in %d cycles.\n", strict_max(POLLS_THIS_RUN_0, POLLS_THIS_RUN_1));
      return (idle ? PollResult::ACTION : PollResult::ERROR);
    };


    PollResult poll_until_disconnected() {
      uint32_t polling_cycles = _profiler_polling0.executions();
      MillisTimeout polling_timeout(5000);
      bool idle = false;
      while (!(polling_timeout.expired() | idle)) {
        _single_poll();
        idle = !(peer0.isConnected() | peer1.isConnected());
        if (0 < log_0.length()) {   printf("Peer0 (%06d):\n%s\n", (_profiler_polling0.executions() - polling_cycles), (const char*) log_0.string());  }
        if (0 < log_1.length()) {   printf("Peer1 (%06d):\n%s\n", (_profiler_polling1.executions() - polling_cycles), (const char*) log_1.string());  }
        sleep_ms(1);
      }
      const int POLLS_THIS_RUN_0 = (_profiler_polling0.executions() - polling_cycles);
      const int POLLS_THIS_RUN_1 = (_profiler_polling1.executions() - polling_cycles);
      if (!idle) {
        _dump_peers();
      }
      printf("poll_until_disconnected completed in %d cycles.\n", strict_max(POLLS_THIS_RUN_0, POLLS_THIS_RUN_1));
      return (idle ? PollResult::ACTION : PollResult::ERROR);
    };


    /*
    * Uses the previously-setup links to move some messages.
    */
    PollResult simple_messages() {
      StringBuilder log("===< M2MLink Simple messages >====================================\n");
      PollResult ret = PollResult::ERROR;
      int ret_local = -1;
      if (peer0.linkIdle() & peer1.linkIdle()) {
        KeyValuePair a("time_ms", (uint32_t) millis());
        a.append((uint32_t) randomUInt32(), "rand");
        ret_local = peer0.send(&a);
        if (0 <= ret_local) {
          _args_sent_0 = &a;
          if (PollResult::ACTION == poll_until_finished()) {
            KeyValuePair b("time_ms", (uint32_t) millis());
            b.append((uint32_t) randomUInt32(), "reply_test");
            ret_local = peer0.send(&b, true);
            if (0 <= ret_local) {
              _args_sent_0 = &b;
              if (PollResult::ACTION == poll_until_finished()) {
                //peer0.printDebug(&log);
                //peer1.printDebug(&log);
                if (_replies_rxd_0 == 1) {
                  //args_recd_peer0.printDebug(&log);
                  _reply_lockout_1 = true;
                  ret_local = peer0.send(&b, true);
                  if (0 <= ret_local) {
                    if (PollResult::ACTION == poll_until_finished()) {
                      if (peer0.replyTimeouts() == 1) {
                        log.concat("\tSimple messages pass tests.\n");
                        ret = PollResult::ACTION;
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

      peer0.poll(&log);
      peer1.poll(&log);
      _reply_lockout_0 = false;
      _reply_lockout_1 = false;

      printf("%s\n\n", (const char*) log.string());
      return ret;
    };


    PollResult remote_log_insertion() {
      StringBuilder log("===< M2MLink remote log insertion >=============================\n");
      PollResult ret = PollResult::ERROR;
      if (peer0.isConnected() && peer1.isConnected()) {
        StringBuilder sendlog_vlad("This is a log from Vlad (no reply).");
        StringBuilder sendlog_carl("This is a log from Carl (no reply).");
        if (0 == peer0.writeRemoteLog(&sendlog_vlad, false)) {
          sendlog_vlad.concat("This is a log from Vlad (demands reply this time).");
          if (PollResult::ACTION == poll_until_finished()) {
            if (0 == peer0.writeRemoteLog(&sendlog_vlad, true)) {
              if (PollResult::ACTION == poll_until_finished()) {
                if (0 == peer1.writeRemoteLog(&sendlog_carl, false)) {
                  sendlog_carl.concat("This is a log from Carl (demands reply this time).");
                  if (PollResult::ACTION == poll_until_finished()) {
                    if (0 == peer1.writeRemoteLog(&sendlog_carl, true)) {
                      if (PollResult::ACTION == poll_until_finished()) {
                        log.concat("\tRemote log insertion passes tests.\n");
                        ret = PollResult::ACTION;
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
      peer0.poll(&log);
      peer1.poll(&log);
      printf("%s\n\n", (const char*) log.string());
      return ret;
    };


    PollResult reestablish_after_hangup() {
      printf("===< M2MLink re-establish after hangup >========================\n");
      PollResult ret = PollResult::ERROR;
      int ret_local = -1;
      if (!(peer0.isConnected() | peer1.isConnected())) {
        if (0 == peer1.reset()) {
          if (0 == peer0.reset()) {
            if (PollResult::ACTION == poll_until_finished()) {
              printf("\tRe-establish after hangup passes tests.\n");
              ret = PollResult::ACTION;
            }
            else printf("Failed to send. Link dead-locked.\n");
          }
          else printf("Vlad failed to reset()\n");
        }
        else printf("Carl failed to reset()\n");
      }
      else printf("Either Vlad or Carl is not ready for the test.\n");

      return ret;
    };


    PollResult hangup_gentle() {
      printf("===< M2MLink gentle hangup >====================================\n");
      PollResult ret = PollResult::ERROR;
      int ret_local = -1;
      if (peer0.linkIdle() && peer1.linkIdle()) {
        ret_local = peer1.hangup();
        if (0 == ret_local) {
          if (PollResult::ACTION == poll_until_disconnected()) {
            printf("\tGentle hangup passes tests.\n");
            ret = PollResult::ACTION;
          }
          else printf("Failed to HANGUP. Link dead-locked.\n");
        }
        else printf("Carl failed to HANGUP. Returned %d\n", ret_local);
      }
      else printf("Either Vlad or Carl is not ready for the test.\n");

      return ret;
    };


    /*
    * Feed garbage into the stream, and make sure the link resyncs.
    */
    PollResult corrupted_transport() {
      StringBuilder log("===< M2MLink corrupted transport >====================================\n");
      PollResult ret = PollResult::ERROR;
        uint32_t buf_0[4] = {randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32()};
        uint32_t buf_1[4] = {randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32()};
        StringBuilder garbage_for_vlad;
        StringBuilder garbage_for_carl;
        garbage_for_vlad.concat((uint8_t*) &buf_0[0], 16);
        garbage_for_carl.concat((uint8_t*) &buf_1[0], 16);
        peer0.pushBuffer(&garbage_for_vlad);
        peer1.pushBuffer(&garbage_for_carl);
        if (PollResult::ACTION == poll_until_finished()) {
          peer1.pushBuffer(&garbage_for_carl);
          peer1.pushBuffer(&garbage_for_carl);
          peer1.pushBuffer(&garbage_for_carl);
          peer1.pushBuffer(&garbage_for_carl);
          if (PollResult::ACTION == poll_until_finished()) {
            log.concat("Vlad and Carl resyncd after being fed garbage.\n");
            ret = PollResult::ACTION;
          }
          else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");
        }
        else log.concat("The polling loop ran to its maximum extent. Link dead-locked.\n");

      printf("%s\n\n", (const char*) log.string());
      return ret;
    };


    PollResult message_flood() {
      int ret = -1;
      printf("===< M2MLink message flood >====================================\n");
      return ((0 == ret) ? PollResult::ACTION : PollResult::ERROR);
    };


    PollResult hangup_abrupt() {
      int ret = -1;
      printf("===< M2MLink abrupt hangup >====================================\n");
      return ((0 == ret) ? PollResult::ACTION : PollResult::ERROR);
    };

    void callback_shunt(bool is_peer1, uint32_t tag, M2MMsg* msg) {
      StringBuilder* log      = (is_peer1 ? &log_1 : &log_0);
      KeyValuePair** kvps_rxd = (is_peer1 ? &_args_rxd_1 : &_args_rxd_0);
      const char* NAME_IN_LOG = (is_peer1 ? "carl":"vlad");
      //msg->printDebug(&log);
      msg->getPayload(kvps_rxd);
      if (msg->isReply()) {
        if (is_peer1) {
          _replies_rxd_1++;
        }
        else {
          _replies_rxd_0++;
        }
      }
      const bool REPLY_LOCKOUT = (is_peer1 ? _reply_lockout_1 : _reply_lockout_0);
      if ((!REPLY_LOCKOUT) && msg->expectsReply()) {
        log->concatf("%s received Msg(0x%x)", NAME_IN_LOG, msg->uniqueId());
        log->concatf(" ACKing returns %d.\n", msg->ack());
      }
    };



  private:
    StopWatch     _profiler_polling0;
    StopWatch     _profiler_polling1;
    KeyValuePair* _args_sent_0 = nullptr;   // Non-ownership references to the
    KeyValuePair* _args_sent_1 = nullptr;   //   most recent KVPs in versus out
    KeyValuePair* _args_rxd_0  = nullptr;   //   for each end of the link.
    KeyValuePair* _args_rxd_1  = nullptr;
    StringBuilder log_0;
    StringBuilder log_1;
    uint32_t      _replies_rxd_0 = 0;
    uint32_t      _replies_rxd_1 = 0;
    bool          _reply_lockout_0 = false;
    bool          _reply_lockout_1 = false;


    PollResult _single_poll() {
      PollResult ret = PollResult::ERROR;
      _profiler_polling0.markStart();
      peer0.poll(&log_0);
      _profiler_polling0.markStop();
      _profiler_polling1.markStart();
      peer1.poll(&log_1);
      _profiler_polling1.markStop();
      ret = PollResult::ACTION;
      return ret;
    }

    /*
    * Takes any logs from the peers, and renders it to the test log.
    * TODO: Make recursive with bit-shifted bailout and mask parameter.
    */
    void _dump_peers() {
      peer0.printDebug(&log_0);
      peer1.printDebug(&log_1);
      if (0 < log_0.length()) {   printf("---\n---Peer0\n----------------------------------\n%s\n", (const char*) log_0.string());  }
      if (0 < log_1.length()) {   printf("---\n---Peer1\n----------------------------------\n%s\n", (const char*) log_1.string());  }
    };


    /*
    * Takes any logs from the peers, and renders it to the test log.
    */
    void _dump_last_kvps() {
      StringBuilder tmp;
      if (nullptr != _args_sent_1) {
        tmp.concat("\n_args_sent_1:\n\t------------------\n");
        _args_sent_1->printDebug(&tmp);
      }
      if (nullptr != _args_rxd_0) {
       tmp.concat("\n_args_rxd_0:\n\t-------------------\n");
       _args_rxd_0->printDebug(&tmp);
      }
      if (nullptr != _args_sent_0) {
        tmp.concat("\nargs_sent_0:\n\t------------------\n");
        _args_sent_0->printDebug(&tmp);
      }
      if (nullptr != _args_rxd_1) {
       tmp.concat("\n_args_rxd_1:\n\t-------------------\n");
       _args_rxd_1->printDebug(&tmp);
      }
      printf("%s\n", (const char*) tmp.string());
    };
};



/*******************************************************************************
* Callbacks, value-checking, and polling functions
*******************************************************************************/

/*
* TODO: This test program doesn't observe the Link state callback. But then
*   again, no other software I can think of does, either.
* If not for the possibility of re-use with a differnt counterparty, I should
*   like to collapse this role into the M2MMsg callback..
*/
void callback_link_state(M2MLink* cb_link) {
  printf("Link (0x%x) entered state %s\n", cb_link->linkTag(), M2MLink::sessionStateStr(cb_link->currentState()));
}

/*
* Ordinarilly, the application would fill in these functions. But because this
*   is test code, both halves of the link are pushed into the test fixture,
*   where they can be evaluated.
* Services running within the link will intercept and manage their own messages.
*   Only unknown messages make it to this callback.
*/
void callback_vlad(uint32_t tag, M2MMsg* msg) {  m2ml_test_current->callback_shunt(false, tag, msg);  }
void callback_carl(uint32_t tag, M2MMsg* msg) {  m2ml_test_current->callback_shunt(true, tag, msg);   }


/*
* This is the callback for the RPC host.
*/
void callback_rpc_host(uint32_t tag, M2MMsg* msg) {
  printf("callback_rpc_host() received Msg(0x%x) with tag %u\n", msg->uniqueId(), tag);
}


/*
* This is the callback for the RPC client.
*/
void callback_rpc_client(uint32_t tag, M2MMsg* msg) {
  printf("callback_rpc_client() received Msg(0x%x) with tag %u\n", msg->uniqueId(), tag);
}


/*******************************************************************************
* M2MMsg functionality
*******************************************************************************/

/* Header tests */
int link_tests_message_battery_0() {
  int ret = -1;
  StringBuilder log("\tM2MMsg battery 0 (Header)\n");
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
  printf("\tM2MMsg battery 1 (Parse-pack)\n");
  const uint32_t TEST_BUF_LEN    = (19 + (randomUInt32() % 9));
  uint32_t now     = millis();
  uint32_t rand    = randomUInt32();
  StringBuilder test_string;
  // Generate a test string of (TEST_BUF_LEN-1), because the wrapper will
  //   report the binary length of the contained data. Which includes the
  //   null-terminator for a C-style string.
  generate_random_text_buffer(&test_string, (TEST_BUF_LEN-1));
  const char* VAL_STR = (const char*) test_string.string();
  float    val_flt = generate_random_float();
  double   val_dbl = generate_random_double();
  Vector3<float> vect(generate_random_float(), generate_random_float(), generate_random_float());
  KeyValuePair a("time_ms", now);
  a.append(rand, "rand");
  a.append(val_flt, "val_flt");
  a.append(val_dbl, "val_dbl");
  a.append(VAL_STR, "my_key");
  a.append(&vect, "vect");

  M2MMsgHdr hdr_parse_pack_0(M2MMsgCode::APPLICATION, 0, true);
  M2MMsg* msg_parse_pack_0 = new M2MMsg(&hdr_parse_pack_0, BusOpcode::TX);
  printf("\t\tCan construct a TX message... ");
  if (msg_parse_pack_0) {
    printf("Pass\n\t\tCan attach a payload... ");
    if (0 == msg_parse_pack_0->setPayload(&a)) {
      StringBuilder msg_0_serial;
      printf("Pass\n\t\tCan serialize the message... ");
      if (0 == msg_parse_pack_0->serialize(&msg_0_serial)) {
        printf("Pass\n\t\tCan serialize the message... ");
        if (!msg_0_serial.isEmpty()) {
          //msg_0_serial.printDebug(&log);
          printf("Pass\n\t\tCan deserialize the message... ");
          M2MMsg* msg_parse_pack_1 = M2MMsg::unserialize(&msg_0_serial);
          if (nullptr != msg_parse_pack_1) {
            printf("Pass\n\t\trxComplete() is set... ");
            if (msg_parse_pack_1->rxComplete()) {
              KeyValuePair* pl = nullptr;
              msg_parse_pack_1->getPayload(&pl);
              printf("Pass\n\t\tPayload is retrievable... ");
              if (nullptr != pl) {
                printf("Pass\n\t\tPayload contains all the keys with matching values...");
                // Did all of the arguments come across unscathed?
                uint32_t now_ret     = millis();
                uint32_t rand_ret    = randomUInt32();
                char*    val_str_ret = nullptr;
                float    val_flt_ret = generate_random_float();
                double   val_dbl_ret = generate_random_double();
                Vector3<float> vect_ret(generate_random_float(), generate_random_float(), generate_random_float());
                printf("\n\t\t\t\"time_ms\"... ");
                int8_t fetch_ret    = pl->valueWithKey("time_ms", &now_ret);
                bool   values_match = (now_ret == now);
                if ((0 == fetch_ret) && values_match) {
                  printf("Pass\n\t\t\t\"rand\"... ");
                  fetch_ret    = pl->valueWithKey("rand", &rand_ret);
                  values_match = (rand_ret == rand);
                  if ((0 == fetch_ret) && values_match) {
                    printf("Pass\n\t\t\t\"my_key\"... ");
                    fetch_ret    = pl->valueWithKey("my_key", &val_str_ret);
                    values_match = ((0 == fetch_ret) && (0 == StringBuilder::strcasecmp(VAL_STR, val_str_ret)));
                    if ((0 == fetch_ret) && values_match) {
                      printf("Pass\n\t\t\t\"val_flt\"... ");
                      fetch_ret    = pl->valueWithKey("val_flt", &val_flt_ret);
                      values_match = (val_flt_ret == val_flt);
                      if ((0 == fetch_ret) && values_match) {
                        printf("Pass\n\t\t\t\"val_dbl\"... ");
                        fetch_ret    = pl->valueWithKey("val_dbl", &val_dbl_ret);
                        values_match = (val_dbl_ret == val_dbl);
                        if ((0 == fetch_ret) && values_match) {
                          printf("Pass\n\t\t\t\"vect\"... ");
                          fetch_ret    = pl->valueWithKey("vect", &vect_ret);
                          values_match = (vect_ret == vect);
                          if ((0 == fetch_ret) && values_match) {
                            printf("\t\tParse-pack tests pass.\n");
                            ret = 0;
                          }
                        }
                      }
                    }
                  }
                }

                if (0 != ret) {
                  if (0 != fetch_ret) {     printf("Fetch value (%d). ", fetch_ret);  }
                  else if (values_match) {  printf("Values don't match. ");           }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\nInput payload:\n");
    dump_kvp(&a);
  }
  return ret;
}





/*******************************************************************************
* M2MLink test plan
*
* NOTE: Due to the large mounts of global state in these tests, the checklist
*   cannot be run in parallel. It must be run one step after another.
*******************************************************************************/
#define CHKLST_M2ML_TEST_MSG_HEADER     0x00000001  // This is the bottom of the Link abstraction.
#define CHKLST_M2ML_TEST_MSG_PARSE_PACK 0x00000002  // This is the bottom of the Link abstraction.
#define CHKLST_M2ML_TEST_PREPARE_CBOR   0x00000004  // Prepare simulated peers and connect them.
#define CHKLST_M2ML_TEST_SIMPLE_MSGS    0x00000008  // The peers can exchange simple messages.
#define CHKLST_M2ML_TEST_ACKD_MSGS      0x00000010  // The peers can exchange messages with delivery assurance.
#define CHKLST_M2ML_TEST_CORRUPT_XPORT  0x00000020  // The Link detects and recovers from a corrupted stream.
#define CHKLST_M2ML_TEST_GENTLE_HANGUP  0x00000040  // The Link can be torn down by procedure.
#define CHKLST_M2ML_TEST_REESTABLISH    0x00000080  // The Link can be re-established following a hangup.
#define CHKLST_M2ML_TEST_REMOTE_LOG     0x00000100  // The log insertion feature operates as expected.
#define CHKLST_M2ML_TEST_NO_MWEO        0x00000200  // "No mutally-workable encoding options"
#define CHKLST_M2ML_TEST_ABRUPT_HANGUP  0x00000400  // Sometimes, a peer just needs to drop the link.
#define CHKLST_M2ML_TEST_XPORT_DROP     0x00000800  // Tests behavior when the underlying transport fails. Common.
#define CHKLST_M2ML_TEST_MTU_SHEAR      0x00001000  // What happens when messages only fit in one of the two peers?
#define CHKLST_M2ML_TEST_MSG_FLOOD      0x00002000  // Both host and client
#define CHKLST_M2ML_TEST_PING_PONG      0x00004000  // The peers can bounce a message back-and-forth indefinitely.
#define CHKLST_M2ML_TEST_CONCURRENCY_0  0x00008000  // Peers can play ping-pong with many messages concurrently.
#define CHKLST_M2ML_TEST_CONCURRENCY_1  0x00010000  // Spin up a thread to make things more interesting.
#define CHKLST_M2ML_TEST_AUTH_ONE_WAY   0x00020000  // Unidirectional authentication.
#define CHKLST_M2ML_TEST_AUTH_NOMINAL   0x00040000  // Auth flows succeed if they ought to.
#define CHKLST_M2ML_TEST_AUTH_FAIL      0x00080000  // Auth rejection flows.

/* Tests of the RPC M2MService */
// TODO: These ought to be split out into their own checklist,
//   and possibly even their own source file. Use BufferAccepter
//   as a temporay guide.
#define CHKLST_M2ML_RPC_INIT_SESSION    0x01000000  // Sets up a new link for the RPC test.
#define CHKLST_M2ML_RPC_CLIENT_MSGS     0x02000000  // Request messages from the client are correctly-formed.
#define CHKLST_M2ML_RPC_HOST_MSGS       0x04000000  // Response messages from the host are correctly-formed.
#define CHKLST_M2ML_RPC_RP_LIST         0x08000000  // The client can fetch the RPC listing from the host.
#define CHKLST_M2ML_RPC_NOMINAL_FLOW    0x10000000  // RPCs work properly under conditions of proper use.
#define CHKLST_M2ML_RPC_MALFORMED_ARGS  0x20000000  // Host and client detect and respond to semantic skew.
#define CHKLST_M2ML_RPC_SPLIT_REQUEST   0x40000000  // Client requests are split and rejoined correctly.
#define CHKLST_M2ML_RPC_SPLIT_RESPONSE  0x80000000  // Host responses are split and rejoined correctly.

#define CHKLST_M2ML_RPC_TESTS_ALL ( \
  CHKLST_M2ML_RPC_INIT_SESSION | CHKLST_M2ML_RPC_CLIENT_MSGS | \
  CHKLST_M2ML_RPC_HOST_MSGS | CHKLST_M2ML_RPC_RP_LIST | \
  CHKLST_M2ML_RPC_NOMINAL_FLOW | CHKLST_M2ML_RPC_MALFORMED_ARGS | \
  CHKLST_M2ML_RPC_SPLIT_REQUEST | CHKLST_M2ML_RPC_SPLIT_RESPONSE)

#define CHKLST_M2ML_TESTS_ALL ( \
  CHKLST_M2ML_TEST_MSG_HEADER | CHKLST_M2ML_TEST_MSG_PARSE_PACK | \
  CHKLST_M2ML_TEST_PREPARE_CBOR | CHKLST_M2ML_TEST_SIMPLE_MSGS | \
  CHKLST_M2ML_TEST_NO_MWEO | \
  CHKLST_M2ML_TEST_ABRUPT_HANGUP | CHKLST_M2ML_TEST_XPORT_DROP | \
  CHKLST_M2ML_TEST_MTU_SHEAR | CHKLST_M2ML_TEST_MSG_FLOOD | \
  CHKLST_M2ML_TEST_PING_PONG | CHKLST_M2ML_TEST_CONCURRENCY_0 \
)

  /*
  CHKLST_M2ML_TEST_CORRUPT_XPORT | CHKLST_M2ML_TEST_GENTLE_HANGUP | \
  CHKLST_M2ML_TEST_REESTABLISH | CHKLST_M2ML_TEST_REMOTE_LOG | \
  CHKLST_M2ML_TEST_CONCURRENCY_1 | CHKLST_M2ML_TEST_AUTH_ONE_WAY | \
  CHKLST_M2ML_TEST_AUTH_NOMINAL | CHKLST_M2ML_TEST_AUTH_FAIL \
  */


const StepSequenceList TOP_LEVEL_M2ML_TEST_LIST[] = {
  { .FLAG         = CHKLST_M2ML_TEST_MSG_HEADER,
    .LABEL        = "M2MMsgHdr",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == link_tests_message_battery_0()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_M2ML_TEST_MSG_PARSE_PACK,
    .LABEL        = "M2MMsg (parse/pack}",
    .DEP_MASK     = (CHKLST_M2ML_TEST_MSG_HEADER),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == link_tests_message_battery_1()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_M2ML_TEST_PREPARE_CBOR,
    .LABEL        = "Test preparation (CBOR)",
    .DEP_MASK     = (CHKLST_M2ML_TEST_MSG_PARSE_PACK),
    .DISPATCH_FXN = []() {
      int8_t ret = -1;
      // Construct the first test conditions.
      M2MLinkOpts opts_vlad(
        100,           // ACK timeout is 100ms. Vlad is patient.
        2000,          // Send a KA every 2s.
        2048,          // MTU for this link is 2 kibi.
        TCode::CBOR,   // Payloads should be CBOR encoded.
        (M2MLINK_FLAG_ALLOW_LOG_WRITE)
      );
      M2MLinkOpts opts_carl(
        40,            // ACK timeout is 40ms.
        2000,          // Send a KA every 2s.
        1024,          // MTU for this link is 1 kibi.
        TCode::CBOR,   // Payloads should be CBOR encoded.
        0              // No flags.
      );
      m2ml_test_current = new M2ML_Test_Vehicle(
        &opts_vlad, callback_vlad,
        &opts_carl, callback_carl
      );
      if (nullptr != m2ml_test_current) {             // If allocation worked...
        if (0 == m2ml_test_current->prepareTest()) {  // ...and the peers are setup...
          ret = 1;                                    // ...dispatch succeeds.
        }
      }
      return ret;
    },
    .POLL_FXN     = []() {
      // Connection is (in reality) a long-running and asynchronous process. We
      //   simulate an I/O channel that is effectively instantaneous, and poll
      //   both peers in an alternating fashion until the both report state stability.
      return (int8_t) m2ml_test_current->connect_peers();
    }
  },

  // Test block to ensure that the raw KVP API is operational between the peers,
  //   and that non-link messages can be exchanged in both directions.
  { .FLAG         = CHKLST_M2ML_TEST_SIMPLE_MSGS,
    .LABEL        = "Corrupted transport",
    .DEP_MASK     = (CHKLST_M2ML_TEST_PREPARE_CBOR),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return (int8_t) m2ml_test_current->simple_messages();  }
  },

  // Message ACK mechanism works correctly, and the sender notices/retries when
  //   ACK fails to happen. Receiver correctly handles repeat messages in cases
  //   where latency exceeds ACK timeout, but no bytes are lost.
  { .FLAG         = CHKLST_M2ML_TEST_ACKD_MSGS,
    .LABEL        = "Messsage ACK mechanism",
    .DEP_MASK     = (CHKLST_M2ML_TEST_SIMPLE_MSGS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  { .FLAG         = CHKLST_M2ML_TEST_GENTLE_HANGUP,
    .LABEL        = "Gentle hangup",
    .DEP_MASK     = (CHKLST_M2ML_TEST_ACKD_MSGS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return (int8_t) m2ml_test_current->hangup_gentle();  }
  },

  { .FLAG         = CHKLST_M2ML_TEST_REESTABLISH,
    .LABEL        = "Re-establishment after hangup",
    .DEP_MASK     = (CHKLST_M2ML_TEST_GENTLE_HANGUP),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return (int8_t) m2ml_test_current->reestablish_after_hangup();  }
  },

  { .FLAG         = CHKLST_M2ML_TEST_REMOTE_LOG,
    .LABEL        = "Remote log insertion",
    .DEP_MASK     = (CHKLST_M2ML_TEST_REESTABLISH),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return (int8_t) m2ml_test_current->remote_log_insertion();  }
  },

  // This test injects small amounts of garbage into the byte stream that
  //   connects the two peers. A passing test means the the receiving-side of
  //   the link notices, and takes corrective action for the link as-a-whole.
  // Also tests that corrupted messages don't become lost in the bilateral state
  //   dance that the link will perform in the course of re-establishing sync.
  { .FLAG         = CHKLST_M2ML_TEST_CORRUPT_XPORT,
    .LABEL        = "Corrupted transport (sync recovery)",
    .DEP_MASK     = (CHKLST_M2ML_TEST_REMOTE_LOG),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return (int8_t) m2ml_test_current->corrupted_transport();  }
  },

  // Tests behavior when the peers can't find a mutally-workable payload
  //   encoding scheme.
  { .FLAG         = CHKLST_M2ML_TEST_NO_MWEO,
    .LABEL        = "Encoding negotiation failure case",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Sometimes, a peer just needs to drop the link.
  { .FLAG         = CHKLST_M2ML_TEST_ABRUPT_HANGUP,
    .LABEL        = "Abrupt hangup",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Tests behavior when the underlying transport fails. Common.
  { .FLAG         = CHKLST_M2ML_TEST_XPORT_DROP,
    .LABEL        = "Transport failure",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // What happens when messages only fit in one of the two peers?
  { .FLAG         = CHKLST_M2ML_TEST_MTU_SHEAR,
    .LABEL        = "MTU shear",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Both host and client
  { .FLAG         = CHKLST_M2ML_TEST_MSG_FLOOD,
    .LABEL        = "Message flood handling",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // The peers can bounce a message back-and-forth indefinitely.
  { .FLAG         = CHKLST_M2ML_TEST_PING_PONG,
    .LABEL        = "Message ping-pong",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Peers can play ping-pong with many messages concurrently.
  { .FLAG         = CHKLST_M2ML_TEST_CONCURRENCY_0,
    .LABEL        = "Multi-message concurrency",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Messages in a state of ping-pong correctly conclude.
  { .FLAG         = CHKLST_M2ML_TEST_CONCURRENCY_1,
    .LABEL        = "Multi-message concurrency cleanup",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Unidirectional authentication.
  { .FLAG         = CHKLST_M2ML_TEST_AUTH_ONE_WAY,
    .LABEL        = "Auth unidirectional flows",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Auth flows succeed if they ought to.
  { .FLAG         = CHKLST_M2ML_TEST_AUTH_NOMINAL,
    .LABEL        = "Auth nominal flows",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Auth rejection flows.
  { .FLAG         = CHKLST_M2ML_TEST_AUTH_FAIL,
    .LABEL        = "Auth rejection flows",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },


  /*
  * The remaining tests are for the RPC mechanism that is built on top of
  *   M2MLink. It is not required to use M2MLink, and it belongs in its own
  *   subgroup. TODO: Move them.
  *
  */
  // Construct the RPC test conditions.
  { .FLAG         = CHKLST_M2ML_RPC_INIT_SESSION,
    .LABEL        = "Test preparation (RPC)",
    .DEP_MASK     = (CHKLST_M2ML_TEST_ACKD_MSGS),  // If we can handle ACKd messages, we can test RPC.
    .DISPATCH_FXN = []() {
      int8_t ret = -1;
      // The link is isotropic WRT options used on each side, with timeout
      //   values that might be reasonable for a real UART or TCP socket.
      // ACK timeout is 10ms.
      // Send a KA every 2s.
      // MTU for this link is 4 kibi.
      // Payloads should be CBOR encoded.
      // No flags
      M2MLinkOpts opts_rpc_host(10, 2000, 4096, TCode::CBOR, 0);
      M2MLinkOpts opts_rpc_clnt(10, 2000, 4096, TCode::CBOR, 0);
      rpc_test_current = new M2ML_Test_Vehicle(
        &opts_rpc_host, callback_rpc_host,
        &opts_rpc_clnt, callback_rpc_client
      );
      if (nullptr != rpc_test_current) {             // If allocation worked...
        if (0 == rpc_test_current->prepareTest()) {  // ...and the peers are setup...
          svc_host = new M2MLinkRPC_Host(&rpc_test_current->peer0, RPC_TEST_HOST_DEFS);
          ret = (nullptr == svc_host) ? -1 : 1;
        }
      }
      return ret;
    },
    .POLL_FXN     = []() {
      // Connection is (in reality) a long-running and asynchronous process. We
      //   simulate an I/O channel that is effectively instantaneous, and poll
      //   both peers in an alternating fashion until the both report state stability.
      return (int8_t) rpc_test_current->connect_peers();
    }
  },

  // Request messages from the client are correctly-formed.
  { .FLAG         = CHKLST_M2ML_RPC_CLIENT_MSGS,
    .LABEL        = "RPC client operation",
    .DEP_MASK     = (CHKLST_M2ML_RPC_INIT_SESSION),
    .DISPATCH_FXN = []() { return 1;  },  // TODO: Setup a new link for this test.
    .POLL_FXN     = []() { return 1;  }
  },

  // Response messages from the host are correctly-formed.
  { .FLAG         = CHKLST_M2ML_RPC_HOST_MSGS,
    .LABEL        = "RPC host operation",
    .DEP_MASK     = (CHKLST_M2ML_RPC_CLIENT_MSGS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // The client can fetch the RPC listing from the host.
  { .FLAG         = CHKLST_M2ML_RPC_RP_LIST,
    .LABEL        = "RPC list procedure",
    .DEP_MASK     = (CHKLST_M2ML_RPC_HOST_MSGS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // RPCs work properly under conditions of proper use.
  { .FLAG         = CHKLST_M2ML_RPC_NOMINAL_FLOW,
    .LABEL        = "RPC nominal flow",
    .DEP_MASK     = (CHKLST_M2ML_RPC_RP_LIST),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Host and client detect and respond to semantic skew.
  { .FLAG         = CHKLST_M2ML_RPC_MALFORMED_ARGS,
    .LABEL        = "RPC malformed arguments",
    .DEP_MASK     = (CHKLST_M2ML_RPC_NOMINAL_FLOW),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Client requests are split and rejoined correctly.
  { .FLAG         = CHKLST_M2ML_RPC_SPLIT_REQUEST,
    .LABEL        = "RPC split request",
    .DEP_MASK     = (CHKLST_M2ML_RPC_MALFORMED_ARGS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },

  // Host responses are split and rejoined correctly.
  { .FLAG         = CHKLST_M2ML_RPC_SPLIT_RESPONSE,
    .LABEL        = "RPC split response",
    .DEP_MASK     = (CHKLST_M2ML_RPC_SPLIT_REQUEST),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
};

AsyncSequencer m2ml_test_plan(TOP_LEVEL_M2ML_TEST_LIST, (sizeof(TOP_LEVEL_M2ML_TEST_LIST) / sizeof(TOP_LEVEL_M2ML_TEST_LIST[0])));





/*******************************************************************************
* The top-level of the M2MLink tests
*******************************************************************************/

void print_types_m2mlink() {
  printf("\tM2MLinkOpts           %u\t%u\n", sizeof(M2MLinkOpts), alignof(M2MLinkOpts));
  printf("\tM2MLink               %u\t%u\n", sizeof(M2MLink), alignof(M2MLink));
  printf("\tM2MMsg                %u\t%u\n", sizeof(M2MMsg), alignof(M2MMsg));
  printf("\tM2MMsgHdr             %u\t%u\n", sizeof(M2MMsgHdr), alignof(M2MMsgHdr));
  printf("\tM2MLinkRPC_Host       %u\t%u\n", sizeof(M2MLinkRPC_Host), alignof(M2MLinkRPC_Host));
  printf("\tM2MLinkRPC_Client     %u\t%u\n", sizeof(M2MLinkRPC_Client), alignof(M2MLinkRPC_Client));
  printf("\tC3PDefinedRPC         %u\t%u\n", sizeof(C3PDefinedRPC), alignof(C3PDefinedRPC));
  printf("\tC3PRPCContext         %u\t%u\n", sizeof(C3PRPCContext), alignof(C3PRPCContext));
}


/**
* This is the root of the M2MLink tests.
*
* @return 0 on success. Nonzero otherwise.
*/
int m2mlink_test_main() {
  const char* const MODULE_NAME = "M2MLink";
  printf("===< %s >=======================================\n", MODULE_NAME);

  m2ml_test_plan.requestSteps(CHKLST_M2ML_RPC_TESTS_ALL | CHKLST_M2ML_TESTS_ALL);
  while (!m2ml_test_plan.request_completed() && (0 == m2ml_test_plan.failed_steps(false))) {
    m2ml_test_plan.poll();
  }
  int ret = (m2ml_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  m2ml_test_plan.printDebug(&report_output, "M2MLink test report");
  printf("%s\n", (char*) report_output.string());

  if (nullptr != m2ml_test_current) {
    delete m2ml_test_current;
    m2ml_test_current = nullptr;
  }
  if (nullptr != rpc_test_current) {
    delete rpc_test_current;
    rpc_test_current = nullptr;
  }
  return ret;
}
