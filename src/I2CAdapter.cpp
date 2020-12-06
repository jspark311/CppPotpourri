/*
File:   I2CAdapter.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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


This class is supposed to be an i2c abstraction layer. The goal is to have
an object of this class that can be instantiated and used to communicate
with i2c devices (as a bus master) regardless of the platform.

This file is the tortured result of growing pains since the beginning of
  ManuvrOS. It has been refactored fifty-eleven times, suffered the brunt
  of all porting efforts, and has reached the point where it must be split
  apart into a more-portable platform-abstraction strategy.
*/

#include "AbstractPlatform.h"
#include "I2CAdapter.h"


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

const char I2CAdapter::_ping_state_chr[4] = {' ', '.', '*', ' '};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

I2CAdapter::I2CAdapter(const I2CAdapterOptions* o) : BusAdapter(o->adapter, I2CADAPTER_MAX_QUEUE_DEPTH), _bus_opts(o) {
  // Some platforms (linux) will ignore pin-assignment values completely.
  _adapter_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _adapter_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);
}


I2CAdapter::~I2CAdapter() {
  busOnline(false);

  /* TODO: The work_queue destructor will take care of its own cleanup, but
     We should abort any open transfers prior to deleting this list. */
  _adapter_clear_flag(I2C_BUS_FLAG_BUS_ERROR | I2C_BUS_FLAG_BUS_ONLINE);
  _adapter_clear_flag(I2C_BUS_FLAG_PING_RUN  | I2C_BUS_FLAG_PINGING);
  bus_deinit();
}


int8_t I2CAdapter::init() {
  _memory_init();
  for (uint16_t i = 0; i < 32; i++) ping_map[i] = 0;   // Zero the ping map.
  return bus_init();
}


/*
* Creates a specially-crafted WorkQueue object that will use the i2c peripheral
*   to discover if a device is active on the bus and addressable.
*/
void I2CAdapter::ping_slave_addr(uint8_t addr) {
  I2CBusOp* nu = new_op(BusOpcode::TX_CMD, this);
  if (nullptr != nu) {
    nu->dev_addr = addr;
    nu->sub_addr = -1;
    nu->setBuffer(nullptr, 0);
    _adapter_set_flag(I2C_BUS_FLAG_PINGING);
    queue_io_job(nu);
  }
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t I2CAdapter::io_op_callahead(BusOp* _op) {
  // Bus adapters don't typically do anything here, other
  //   than permit the transfer.
  return 0;
}


/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return 0 on success, or appropriate error code.
*/
int8_t I2CAdapter::io_op_callback(BusOp* _op) {
  int8_t ret = BUSOP_CALLBACK_NOMINAL;
  I2CBusOp* op = (I2CBusOp*) _op;
  if (op->get_opcode() == BusOpcode::TX_CMD) {
    // The only thing the i2c adapter uses this op-code for is pinging slaves.
    // We only support 7-bit addressing for now.
    set_ping_state_by_addr(op->dev_addr, op->hasFault() ? I2CPingState::NEG : I2CPingState::POS);

    if (_adapter_flag(I2C_BUS_FLAG_PINGING)) {
      if ((op->dev_addr & 0x00FF) < 127) {
        // If the adapter is taking a census, and we haven't pinged all
        //   addresses, ping the next one.
        op->dev_addr++;
        ret = BUSOP_CALLBACK_RECYCLE;
      }
      else {
        _adapter_clear_flag(I2C_BUS_FLAG_PINGING);
        _adapter_set_flag(I2C_BUS_FLAG_PING_RUN);
        #if defined(MANUVR_DEBUG)
        //if (getVerbosity() > 4) local_log.concat("Concluded i2c ping sweep.");
        #endif
      }
    }
  }

  //flushLocalLog();
  return ret;
}


/**
* This is what is called when the class wants to conduct a transaction on the bus.
*
* @param  _op  The bus operation to execute.
* @return Zero on success, or appropriate error code.
*/
int8_t I2CAdapter::queue_io_job(BusOp* op) {
  I2CBusOp* nu = (I2CBusOp*) op;
  //nu->setVerbosity(getVerbosity());
  nu->setAdapter(this);
  //if (current_job) {
    // Something is already going on with the bus. Queue...
    work_queue.insert(nu);
  //}
  //else {
    // Bus is idle. Put this work item in the active slot and start the bus operations...
    //current_job = nu;
    //if ((adapterNumber() >= 0) && busOnline()) {
    //  if (XferFault::NONE == nu->begin()) {
    //    #if defined(__BUILD_HAS_THREADS)
    //    if (_thread_id) wakeThread(_thread_id);
    //    #endif
    //  }
    //}
    //else {
    //  //Kernel::staticRaiseEvent(&_queue_ready);   // Raise an event
    //}
  //}
  return 0;
}



/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t I2CAdapter::advance_work_queue() {
  int8_t return_value = 0;
  bool recycle = busOnline();
  while (recycle) {
    recycle = false;
    if (current_job) {
      switch (current_job->get_state()) {
        // NOTE: Tread lightly. Omission of break; scattered throughout.
        /* These are start states. */

        /* These states are unstable and should decay into a "finish" state. */
        case XferState::IDLE:      // Bus op is allocated and waiting somewhere outside of the queue.
        case XferState::QUEUED:    // Bus op is idle and waiting for its turn. No bus control.
          if (!current_job->has_bus_control()) {
            current_job->begin();
            return_value++;
          }
          break;
        case XferState::INITIATE:  // Waiting for initiation phase.
          current_job->advance();
          break;
        case XferState::ADDR:      // Addressing phase. Sending the address.
        case XferState::TX_WAIT:   // I/O operation in-progress.
        case XferState::RX_WAIT:   // I/O operation in-progress.
        case XferState::STOP:      // I/O operation in cleanup phase.
          //current_job->advance();
          break;

        case XferState::UNDEF:     // Freshly instanced (or wiped, if preallocated).
        default:
          current_job->abort(XferFault::ILLEGAL_STATE);
        /* These are finish states. */
        case XferState::FAULT:     // Fault condition.
        case XferState::COMPLETE:  // I/O op complete with no problems.
          //if (current_job->hasFault()) {
          //  if (getVerbosity() > 3) {
          //    local_log.concat("Destroying failed job.\n");
          //    current_job->printDebug(&local_log);
          //  }
          //}
          switch (current_job->execCB()) {
            case BUSOP_CALLBACK_RECYCLE:
              current_job->markForRequeue();
              queue_io_job(current_job);
              break;
            case BUSOP_CALLBACK_ERROR:
            case BUSOP_CALLBACK_NOMINAL:
            default:
              reclaim_queue_item(current_job);
              break;
          }
          return_value++;
          current_job = nullptr;
          break;
      }
    }

    if (nullptr == current_job) {
      // If there is nothing presently being serviced, we should promote an operation from the
      //   queue into the active slot and initiate it in the block below.
      current_job = work_queue.dequeue();
      recycle = (nullptr != current_job);
    }
  }
  //flushLocalLog();
  return return_value;
}


I2CPingState I2CAdapter::get_ping_state_by_addr(uint8_t addr) {
  return (I2CPingState) ((ping_map[(addr & 0x7F) >> 2] >> ((addr & 0x03) << 1)) & 0x03);
}

void I2CAdapter::set_ping_state_by_addr(uint8_t addr, I2CPingState nu) {
  uint8_t m = (0x03 << ((addr & 0x03) << 1));
  uint8_t a = ping_map[(addr & 0x7F) >> 2] & ~m;
  ping_map[(addr & 0x7F) >> 2] = a | (((uint8_t) nu) << ((addr & 0x03) << 1));
}


/*
* Debug fxn to print the ping map.
*/
void I2CAdapter::printPingMap(StringBuilder *temp) {
  if (temp) {
    temp->concat("\n\n\tPing Map\n\t      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
    char str_buf[33];
    for (uint8_t i = 1; i < 30; i+=2) str_buf[i] = ' ';
    str_buf[31] = '\n';
    str_buf[32] = '\0';
    for (uint8_t i = 0; i < 128; i+=16) {
      temp->concatf("\t0x%02x: ", i);
      str_buf[0]  = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x00)];
      str_buf[2]  = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x01)];
      str_buf[4]  = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x02)];
      str_buf[6]  = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x03)];
      str_buf[8]  = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x04)];
      str_buf[10] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x05)];
      str_buf[12] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x06)];
      str_buf[14] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x07)];
      str_buf[16] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x08)];
      str_buf[18] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x09)];
      str_buf[20] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0A)];
      str_buf[22] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0B)];
      str_buf[24] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0C)];
      str_buf[26] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0D)];
      str_buf[28] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0E)];
      str_buf[30] = _ping_state_chr[(uint8_t) get_ping_state_by_addr(i + 0x0F)];
      temp->concat(str_buf);
    }
  }
  temp->concat("\n");
}


/**
* Debug support method.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CAdapter::printDebug(StringBuilder* output) {
  printAdapter(output);
  printHardwareState(output);
  output->concatf("-- sda/scl             %u/%u\n", _bus_opts.sda_pin, _bus_opts.scl_pin);
  output->concatf("-- bus_error           %s\n", (busError()  ? "yes" : "no"));
  printWorkQueue(output, I2CADAPTER_MAX_QUEUE_PRINT);
}
