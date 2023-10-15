/*
File:   I2CDevice.cpp
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
#include "I2CAdapter.h"


/*
* Constructor takes a pointer to the bus we are going to be using, and the slave
*   address of the implementing class.
*/
I2CDevice::I2CDevice(uint8_t addr) : _dev_addr(addr) {
};


/*
* Destructor doesn't have any memory to free, but we should tell the bus that we
*   are going away.
*/
I2CDevice::~I2CDevice() {
  if (_bus) {
    _bus->purge_queued_work_by_dev(this);
  }
};


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* This is what we call when this class wants to conduct a transaction on
*   the bus. We simply forward to the bus we are bound to.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t I2CDevice::queue_io_job(BusOp* _op) {
  I2CBusOp* op = (I2CBusOp*) _op;
  if (nullptr == op->callback) {
    op->callback = this;
  }
  return _bus->queue_io_job(op);
}

/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t I2CDevice::io_op_callahead(BusOp* _op) {
  return 0;
}

/**
* When a bus operation completes, it is passed back to its issuing class.
*
* @param  _op  The bus operation that was completed.
* @return 0 on success, or appropriate error code.
*/
int8_t I2CDevice::io_op_callback(BusOp* op) {
  return BUSOP_CALLBACK_NOMINAL;
}


/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/

// Needs to be called by the i2c class during insertion.
bool I2CDevice::assignBusInstance(I2CAdapter *adapter) {
  if (nullptr == _bus) {
    _bus = adapter;
    return true;
  }
  return false;
};


// This is to be called from the adapter's unassignment function.
bool I2CDevice::disassignBusInstance() {
  _bus = nullptr;
  return true;
};


/* This is to become the only interface because of its non-reliance on malloc(). */
bool I2CDevice::writeX(int sub_addr, uint16_t len, uint8_t *buf) {
  if (_bus == nullptr) {
    return false;
  }
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  nu->dev_addr = _dev_addr;
  nu->sub_addr = (int16_t) sub_addr;
  nu->setBuffer(buf, len);
  return (0 == _bus->queue_io_job(nu));
}


bool I2CDevice::readX(int sub_addr, uint8_t len, uint8_t *buf) {
  if (_bus == nullptr) {
    return false;
  }
  I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
  nu->dev_addr = _dev_addr;
  nu->sub_addr = (int16_t) sub_addr;
  nu->setBuffer(buf, len);
  return (0 == _bus->queue_io_job(nu));
}


/*
* Pings the device.
*/
bool I2CDevice::ping_device() {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
  if (nullptr != nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = -1;
    nu->setBuffer(nullptr, 0);
    return (0 == _bus->queue_io_job(nu));
  }
  return false;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CDevice::printDebug(StringBuilder* temp) {
  if (temp) {
    temp->concatf("\n\t+++ I2CDevice  0x%02x ++++ Bus %sassigned +++++\n", _dev_addr, (_bus == nullptr ? "un" : ""));
  }
}
