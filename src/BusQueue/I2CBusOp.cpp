/*
File:   I2CBusOp.cpp
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

*/

#include "AbstractPlatform.h"
#include "../TimerTools/TimerTools.h"
#include "I2CAdapter.h"

/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp() : BusOp() {
  set_state(XferState::IDLE);
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, BusOpCallback* requester) : I2CBusOp() {
  set_opcode(nu_op);
  callback = requester;
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t* buffer, uint16_t len) : I2CBusOp() {
  dev_addr     = dev_addr;
  sub_addr     = sub_addr;
  set_opcode(nu_op);
  setBuffer(buffer, len);
};


I2CBusOp::~I2CBusOp() {}



/* Call to mark something completed that may not be. Also sends a stop. */
int8_t I2CBusOp::abort(XferFault er) {
  markComplete();
  set_fault(er);
  return 0;
}

/*
*
*/
void I2CBusOp::markComplete() {
  set_state(XferState::COMPLETE);
}



/*******************************************************************************
* ___     _                              These members are mandatory overrides
*  |   / / \ o     |  _  |_              from the BusOp class.
* _|_ /  \_/ o   \_| (_) |_)
*******************************************************************************/

/**
* Wipes this bus operation so it can be reused.
* Be careful not to blow away the flags that prevent us from being reaped.
*/
void I2CBusOp::wipe() {
  BusOp::_busop_wipe(this);
  // Flags that deal with memory management are untouched.
  device   = nullptr;
  sub_addr = -1;
  dev_addr =  0;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CBusOp::printDebug(StringBuilder* output) {
  BusOp::printBusOp("I2COp", this, output);
  output->concatf("\t device          0x%02x\n", dev_addr);
  if (sub_addr != -1) {
    output->concatf("\t subaddress      0x%02x\n", sub_addr);
  }
  output->concat("\n\n");
}
