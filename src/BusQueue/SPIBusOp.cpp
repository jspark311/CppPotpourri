/*
File:   SPIBusOp.cpp
Author: J. Ian Lindsay
Date:   2014.07.01

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
#include "SPIAdapter.h"


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor that calls wipe().
*/
SPIBusOp::SPIBusOp() : BusOp(), _bus(nullptr), _max_freq(0), _param_len(0), _cs_pin(255) {
  memset(xfer_params, 0, sizeof(xfer_params));
}



/**
* Constructor that does setup by parameters.
*
* @param  nu_op The opcode that dictates the bus operation we use
* @param  requester  The object to be notified when the bus operation completes with success.
*/
SPIBusOp::SPIBusOp(BusOpcode nu_op, BusOpCallback* requester) : SPIBusOp() {
  set_opcode(nu_op);
  callback = requester;
}


/**
* Constructor that does setup by parameters.
*
* @param  nu_op The opcode that dictates the bus operation we use
* @param  requester  The object to be notified when the bus operation completes with success.
* @param  cs         The pin number for the device's chip-select signal.
* @param  ah         True for an active-high chip-select.
*/
SPIBusOp::SPIBusOp(BusOpcode nu_op, BusOpCallback* requester, uint8_t cs, bool ah) : SPIBusOp(nu_op, requester) {
  _cs_pin = cs;
  csActiveHigh(ah);
}


/**
* Destructor
* Should be nothing to do here. If this is DMA'd, we expect the referenced buffer
*   to be managed by the class that creates these objects.
*
* Moreover, sometimes instances of this class will be preallocated, and never torn down.
*/
SPIBusOp::~SPIBusOp() {
  // TODO: If we want this, it ought to be handled by the adapter.
  // if (profile()) {
  //   StringBuilder debug_log;
  //   debug_log.concat("Destroying an SPI job that was marked for profiling:\n");
  //   printDebug(&debug_log);
  //   Kernel::log(&debug_log);
  // }
}


/**
* This will mark the bus operation complete with a given error code.
*
* @param  cause A failure code to mark the operation with.
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::abort(XferFault cause) {
  set_fault(cause);
  return markComplete();
}


/**
* Marks this bus operation complete.
*
* Need to remember: this gets called in the event of ANY condition that ends this job. And
*   that includes abort() where the bus operation was never begun, and SOME OTHER job has
*   control of the bus.
*
* @return 0 on success. Non-zero on failure.
*/
FAST_FUNC int8_t SPIBusOp::markComplete() {
  //if (csAsserted()) {
    // If this job has bus control, we need to release the bus.
    _assert_cs(false);
  //}
  //time_ended = micros();
  set_state(!hasFault() ? XferState::COMPLETE : XferState::FAULT);
  return 0;
}


FAST_FUNC int8_t  SPIBusOp::bitsPerFrame(SPIFrameSize fsz) {
  switch (fsz) {
    case SPIFrameSize::BITS_8:
    case SPIFrameSize::BITS_9:
    case SPIFrameSize::BITS_16:
    case SPIFrameSize::BITS_24:
    case SPIFrameSize::BITS_32:
      _busop_clear_flag(SPI_XFER_FLAG_FRAME_SIZE_MASK);
      _busop_set_flag((uint16_t) fsz);
      break;
    default:
      return -1;
  }
  return 0;
}


FAST_FUNC uint8_t SPIBusOp::bitsPerFrame() {
  uint8_t ret = 0;
  switch ((SPIFrameSize) (_busop_flags() & SPI_XFER_FLAG_FRAME_SIZE_MASK)) {
    case SPIFrameSize::BITS_32:    ret += 8;
    case SPIFrameSize::BITS_24:    ret += 8;
    case SPIFrameSize::BITS_16:    ret += 7;
    case SPIFrameSize::BITS_9:     ret += 1;
    case SPIFrameSize::BITS_8:     ret += 8;
    default:   break;
  }
  return ret;
}


/**
* Some devices require transfer parameters that are in non-contiguous memory
*   with-respect-to the payload buffer.
*
* @param  p0 The first transfer parameter.
* @param  p1 The second transfer parameter.
* @param  p2 The third transfer parameter.
* @param  p3 The fourth transfer parameter.
* @param  p4 The first transfer parameter.
* @param  p5 The second transfer parameter.
* @param  p6 The third transfer parameter.
* @param  p7 The fourth transfer parameter.
*/
FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6, uint8_t p7) {
  xfer_params[0] = p0;
  xfer_params[1] = p1;
  xfer_params[2] = p2;
  xfer_params[3] = p3;
  xfer_params[4] = p4;
  xfer_params[5] = p5;
  xfer_params[6] = p6;
  xfer_params[7] = p7;
  _param_len     = 8;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6) {
  setParams(p0, p1, p2, p3, p4, p5, p6, 0);
  _param_len     = 7;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5) {
  setParams(p0, p1, p2, p3, p4, p5, 0, 0);
  _param_len     = 6;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
  setParams(p0, p1, p2, p3, p4, 0, 0, 0);
  _param_len     = 5;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3) {
  setParams(p0, p1, p2, p3, 0, 0, 0, 0);
  _param_len     = 4;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2) {
  setParams(p0, p1, p2, 0, 0, 0, 0, 0);
  _param_len     = 3;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0, uint8_t p1) {
  setParams(p0, p1, 0, 0, 0, 0, 0, 0);
  _param_len     = 2;
}

FAST_FUNC void SPIBusOp::setParams(uint8_t p0) {
  setParams(p0, 0, 0, 0, 0, 0, 0, 0);
  _param_len     = 1;
}


/*
*
* P A D | C L*   // P: Pin asserted (not logic level!)
* ------|-----   // A: Active high
* 0 0 0 | 0  1   // D: Desired assertion state
* 0 0 1 | 1  0   // C: Pin changed
* 0 1 0 | 0  0   // L: Pin logic level
* 0 1 1 | 1  1
* 1 0 0 | 1  1   // Therefore...
* 1 0 1 | 0  0   // L  = !(A ^ D)
* 1 1 0 | 1  0   // C  = (P ^ D)
* 1 1 1 | 0  1
*/
FAST_FUNC int8_t SPIBusOp::_assert_cs(bool asrt) {
  if (csAsserted() ^ asrt) {
    csAsserted(asrt);
    setPin(_cs_pin, !(asrt ^ csActiveHigh()));
    return 0;
  }
  return -1;
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
FAST_FUNC void SPIBusOp::wipe() {
  BusOp::_busop_wipe(this);
  // Flags that deal with memory management are untouched.
  _cs_pin     = 255;
  _param_len  = 0;
  xfer_params[0] = 0;
  xfer_params[1] = 0;
  xfer_params[2] = 0;
  xfer_params[3] = 0;
  xfer_params[4] = 0;
  xfer_params[5] = 0;
  xfer_params[6] = 0;
  xfer_params[7] = 0;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param  StringBuilder* The buffer into which this fxn should write its output.
*/
void SPIBusOp::printDebug(StringBuilder* output) {
  BusOp::printBusOp("SPIBusOp", this, output);
  output->concatf("\t param_len         %d\n", _param_len);
  output->concatf("\t cs_pin            %u\n", _cs_pin);
  if (shouldReap())       output->concat("\t Will reap\n");

  if (_param_len > 0) {
    output->concat("\t params            ");
    for (uint8_t i = 0; i < _param_len; i++) {
      output->concatf("0x%02x ", xfer_params[i]);
    }
  }
  output->concat("\n\n");
}
