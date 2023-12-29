/*
File:   BusQueue.cpp
Author: J. Ian Lindsay
Date:   2016.05.28

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

#include "../TimerTools/TimerTools.h"
#include "BusQueue.h"


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

// const EnumDef<XferState> _ENUM_LIST_XFER_STATES[] = {
//   { XferState::IDLE,      "IDLE" },
//   { XferState::QUEUED,    "QUEUED" },
//   { XferState::INITIATE,  "INITIATE" },
//   { XferState::ADDR,      "ADDR" },
//   { XferState::RX_WAIT,   "RX-WAIT" },
//   { XferState::TX_WAIT,   "TX-WAIT" },
//   { XferState::STOP,      "STOP" },
//   { XferState::COMPLETE,  "COMPLETE" },
//   { XferState::FAULT,     "FAULT" },
//   { XferState::UNDEF,     "<UNDEF>", (ENUM_WRAPPER_FLAG_CATCHALL)}
// };
// const EnumDefList<XferState> FSM_STATE_LIST(&_ENUM_LIST_XFER_STATES[0], (sizeof(_ENUM_LIST_XFER_STATES) / sizeof(_ENUM_LIST_XFER_STATES[0])));
//
//
// const EnumDef<BusOpcode> _ENUM_LIST_OPCODES[] = {
//   { BusOpcode::RX,              "RX"},
//   { BusOpcode::TX,              "TX"},
//   { BusOpcode::TX_WAIT_RX,      "TX/RX"},
//   { BusOpcode::TX_CMD,          "TX_CMD"},
//   { BusOpcode::TX_CMD_WAIT_RX,  "TX_CMD/RX"},
//   { BusOpcode::UNDEF,           "<UNDEF>", (ENUM_WRAPPER_FLAG_CATCHALL)}
// };
// const EnumDefList<BusOpcode> OPCODE_LIST(&_ENUM_LIST_OPCODES[0], (sizeof(_ENUM_LIST_OPCODES) / sizeof(_ENUM_LIST_OPCODES[0])));
//
//
// const EnumDef<XferFault> _ENUM_LIST_FAULTS[] = {
//   { XferFault::NONE,            "NONE"},
//   { XferFault::NO_REASON,       "NO_REASON", (ENUM_WRAPPER_FLAG_CATCHALL)},
//   { XferFault::TIMEOUT,         "TIMEOUT"},
//   { XferFault::BAD_PARAM,       "BAD_PARAM"},
//   { XferFault::ILLEGAL_STATE,   "ILLEGAL_STATE"},
//   { XferFault::BUS_BUSY,        "BUS_BUSY"},
//   { XferFault::BUS_FAULT,       "BUS_FAULT"},
//   { XferFault::DEV_FAULT,       "DEV_FAULT"},
//   { XferFault::HUNG_IRQ,        "HUNG_IRQ"},
//   { XferFault::DMA_FAULT,       "DMA_FAULT"},
//   { XferFault::DEV_NOT_FOUND,   "DEV_NOT_FOUND"},
//   { XferFault::RO_REGISTER,     "RO_REGISTER"},
//   { XferFault::UNDEFD_REGISTER, "UNDEFD_REGISTER"},
//   { XferFault::IO_RECALL,       "IO_RECALL"},
//   { XferFault::QUEUE_FLUSH,     "QUEUE_FLUSH"}
// };
// const EnumDefList<XferFault> FAULT_CODE_LIST(&_ENUM_LIST_FAULTS[0], (sizeof(_ENUM_LIST_FAULTS) / sizeof(_ENUM_LIST_FAULTS[0])));
//
//
//
// /**
// * Debug and logging support.
// *
// * @return a const char* containing a human-readable representation of a state.
// */
// const char* BusOp::getStateString(XferState state) {
//   return FSM_STATE_LIST.enumStr(state);
// }
//
// /**
// * Debug and logging support.
// *
// * @return a const char* containing a human-readable representation of an opcode.
// */
// const char* BusOp::getOpcodeString(BusOpcode code) {
//   return OPCODE_LIST.enumStr(code);
// }
//
// /**
// * Debug and logging support.
// *
// * @return a const char* containing a human-readable representation of a fault code.
// */
// const char* BusOp::getErrorString(XferFault code) {
//   return FAULT_CODE_LIST.enumStr(code);
// }



/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of a state.
*/
const char* BusOp::getStateString(XferState state) {
  switch (state) {
    case XferState::IDLE:        return "IDLE";
    case XferState::QUEUED:      return "QUEUED";
    case XferState::INITIATE:    return "INITIATE";
    case XferState::ADDR:        return "ADDR";
    case XferState::RX_WAIT:     return "RX-WAIT";
    case XferState::TX_WAIT:     return "TX-WAIT";
    case XferState::STOP:        return "STOP";
    case XferState::COMPLETE:    return "COMPLETE";
    case XferState::FAULT:       return "FAULT";
    default:                     return "<UNDEF>";
  }
}

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of an opcode.
*/
const char* BusOp::getOpcodeString(BusOpcode code) {
  switch (code) {
    case BusOpcode::RX:               return "RX";
    case BusOpcode::TX:               return "TX";
    case BusOpcode::TX_WAIT_RX:       return "TX/RX";
    case BusOpcode::TX_CMD:           return "TX_CMD";
    case BusOpcode::TX_CMD_WAIT_RX:   return "TX_CMD/RX";
    default:                          return "<UNDEF>";
  }
}

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of a fault code.
*/
const char* BusOp::getErrorString(XferFault code) {
  switch (code) {
    case XferFault::NONE:            return "NONE";
    case XferFault::NO_REASON:       return "NO_REASON";
    case XferFault::TIMEOUT:         return "TIMEOUT";
    case XferFault::BAD_PARAM:       return "BAD_PARAM";
    case XferFault::ILLEGAL_STATE:   return "ILLEGAL_STATE";
    case XferFault::BUS_BUSY:        return "BUS_BUSY";
    case XferFault::BUS_FAULT:       return "BUS_FAULT";
    case XferFault::DEV_FAULT:       return "DEV_FAULT";
    case XferFault::HUNG_IRQ:        return "HUNG_IRQ";
    case XferFault::DMA_FAULT:       return "DMA_FAULT";
    case XferFault::DEV_NOT_FOUND:   return "DEV_NOT_FOUND";
    case XferFault::RO_REGISTER:     return "RO_REGISTER";
    case XferFault::UNDEFD_REGISTER: return "UNDEFD_REGISTER";
    case XferFault::IO_RECALL:       return "IO_RECALL";
    case XferFault::QUEUE_FLUSH:     return "QUEUE_FLUSH";
    default:                         return "<UNDEF>";
  }
}


void BusOp::printBusOp(const char* print_name, BusOp* op, StringBuilder* output) {
  output->concatf("\t---[ %s %p %s ]---\n", print_name, op, op->getOpcodeString());
  output->concatf("\t xfer_state        %s\n", BusOp::getStateString(op->get_state()));
  if (XferFault::NONE != op->getFault()) {
    output->concatf("\t xfer_fault        %s\n", BusOp::getErrorString(op->getFault()));
  }
  //if (XferState::COMPLETE == op->get_state()) {
  //  output->concatf("\t completed (uS)   %u\n",   (unsigned long) time_ended - time_began);
  //}

  if (op->bufferLen() > 0) {
    output->concatf("\t buf *(%p): (%u bytes)\n", op->buffer(), op->bufferLen());
    //StringBuilder::printBuffer(output, op->buffer(), op->bufferLen(), "\t ");
  }
}


void BusOp::_busop_wipe(BusOp* obj) {
  // NOTE: Does not change _flags.
  obj->callback      = nullptr;
  obj->_buf          = nullptr;
  obj->_buf_len      = 0;
  obj->_extnd_flags  = 0;
  obj->_xfer_state   = XferState::IDLE;
  obj->_xfer_fault   = XferFault::NONE;
  obj->_opcode       = BusOpcode::UNDEF;
}
