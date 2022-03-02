/*
File:   I2CAdapter.h
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

#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>

#include "BusQueue.h"
#include "StringBuilder.h"
#include "LightLinkedList.h"
#include "AbstractPlatform.h"

#ifndef __I2C_ABSTRACTION_LAYER_H__
#define __I2C_ABSTRACTION_LAYER_H__

// Forward declaration. Definition order in this file is very important.
class I2CDevice;
class I2CAdapter;

  /* Compile-time bounds on memory usage. */
  #ifndef I2CADAPTER_MAX_QUEUE_PRINT
    // How many queue items should we print for debug?
    #define I2CADAPTER_MAX_QUEUE_PRINT 3
  #endif
  #ifndef I2CADAPTER_MAX_QUEUE_DEPTH
    // How deep should the queue be allowed to become before rejecting work?
    #define I2CADAPTER_MAX_QUEUE_DEPTH 12
  #endif
  #ifndef I2CADAPTER_PREALLOC_COUNT
    // How many queue items should we have on-tap?
    #define I2CADAPTER_PREALLOC_COUNT 4
  #endif

  /*
  * These state flags are hosted by the superclass. This may change in the future.
  * Might be too much convention surrounding their assignment across inherritence.
  */
  #define I2C_BUS_FLAG_BUS_ERROR      0x0001    // While this is true, don't interact with the RN.
  #define I2C_BUS_FLAG_BUS_ONLINE     0x0002    // Set when the module is verified to be in command mode.
  #define I2C_BUS_FLAG_PING_RUN       0x0004    // Have we run a full bus discovery?
  #define I2C_BUS_FLAG_PINGING        0x0008    // Are we running a full ping?
  #define I2C_BUS_FLAG_PF_ADVANCE_OPS 0x0010    // Optionally set by the platform.
  #define I2C_BUS_FLAG_PF_BEGIN_ASAP  0x0020    // Optionally set by the platform.

  // Flags passed in at construction that become BUS_FLAGS.
  #define I2C_ADAPT_OPT_FLAG_SCL_PU   0x0400    // SCL pullup.
  #define I2C_ADAPT_OPT_FLAG_SDA_PU   0x0800    // SDA pullup.


  enum class I2CPingState : uint8_t {
    NONE = 0,
    NEG  = 1,
    POS  = 2,
    RES  = 3
  };


  class I2CAdapterOptions {
    public:
      I2CAdapterOptions(const I2CAdapterOptions* obj) :
        adapter(obj->adapter),
        sda_pin(obj->sda_pin),
        scl_pin(obj->scl_pin),
        def_flags(obj->def_flags),
        freq(obj->freq)
      {};

      I2CAdapterOptions(uint8_t a, uint8_t d, uint8_t c) :
        adapter(a),
        sda_pin(d),
        scl_pin(c),
        def_flags(I2C_ADAPT_OPT_FLAG_SDA_PU | I2C_ADAPT_OPT_FLAG_SCL_PU),
        freq(100000)
      {};

      I2CAdapterOptions(uint8_t a, uint8_t d, uint8_t c, uint16_t f) :
        adapter(a),
        sda_pin(d),
        scl_pin(c),
        def_flags(f),
        freq(100000)
      {};

      I2CAdapterOptions(uint8_t a, uint8_t d, uint8_t c, uint16_t f, uint32_t fqy) :
        adapter(a),
        sda_pin(d),
        scl_pin(c),
        def_flags(f),
        freq(fqy)
      {};

      /**
      * @return true if either of the pullups are desired.
      */
      inline bool usePullups() const {
        return (def_flags & (I2C_ADAPT_OPT_FLAG_SDA_PU | I2C_ADAPT_OPT_FLAG_SCL_PU));
      };

      /**
      * @return true if SDA pullup is desired.
      */
      inline bool sdaPullup() const {
        return (def_flags & I2C_ADAPT_OPT_FLAG_SDA_PU);
      };

      /**
      * @return true if SCL pullup is desired.
      */
      inline bool sclPullup() const {
        return (def_flags & I2C_ADAPT_OPT_FLAG_SCL_PU);
      };

      const uint8_t  adapter;
      const uint8_t  sda_pin;
      const uint8_t  scl_pin;
      const uint16_t def_flags;
      const uint32_t freq;
  };


  /*
  * This class represents an atomic operation on the i2c bus.
  */
  class I2CBusOp : public BusOp {
    public:
      int16_t sub_addr = -1;
      uint8_t dev_addr =  0;

      I2CBusOp();
      I2CBusOp(BusOpcode, BusOpCallback* requester);
      I2CBusOp(BusOpcode, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint16_t len);
      virtual ~I2CBusOp();

      /* Mandatory overrides from the BusOp interface... */
      XferFault begin();
      void wipe();
      void printDebug(StringBuilder*);

      XferFault advance() {  return advance(0); };
      XferFault advance(uint32_t status_reg);
      void markComplete();

      /**
      * This will mark the bus operation complete with a given error code.
      * Overriden for simplicity. Marks the operation with failure code NO_REASON.
      *
      * @return 0 on success. Non-zero on failure.
      */
      inline int8_t abort() {    return abort(XferFault::NO_REASON); }
      int8_t abort(XferFault);

      inline void setAdapter(I2CAdapter* b) {  device = b;       };

      /**
      * Decide if we need to send a subaddress.
      *
      * @return true if we do. False otherwise.
      */
      inline bool need_to_send_subaddr() {  return (sub_addr != -1);  }


    private:
      I2CAdapter* device = nullptr;
  };


  /*
  * This is the class that represents the actual i2c peripheral (master).
  */
  class I2CAdapter : public BusAdapter<I2CBusOp> {
    public:
      I2CAdapter(const I2CAdapterOptions*);  // Constructor takes a bus ID and pins as arguments.
      ~I2CAdapter();           // Destructor

      /* Overrides from the BusAdapter interface */
      int8_t io_op_callahead(BusOp*);
      int8_t io_op_callback(BusOp*);
      int8_t queue_io_job(BusOp*);

      int8_t init();

      /* Debug aides */
      void printDebug(StringBuilder*);
      void printHardwareState(StringBuilder*);
      void printPingMap(StringBuilder*);

      // Builds a special bus transaction that does nothing but test for the presence or absence of a slave device.
      void ping_slave_addr(uint8_t);

      inline bool busError() {          return (_adapter_flag(I2C_BUS_FLAG_BUS_ERROR));  };
      inline bool busOnline() {         return (_adapter_flag(I2C_BUS_FLAG_BUS_ONLINE)); };

      /* Built-in per-instance console handler. */
      int8_t console_handler(StringBuilder* text_return, StringBuilder* args);


    protected:
      /* Overrides from the BusAdapter interface */
      int8_t advance_work_queue();
      int8_t bus_init();      // This must be provided on a per-platform basis.
      int8_t bus_deinit();    // This must be provided on a per-platform basis.

      inline bool _pf_needs_op_advance() {        return _adapter_flag(I2C_BUS_FLAG_PF_ADVANCE_OPS);  };
      inline void _pf_needs_op_advance(bool x) {  _adapter_set_flag(I2C_BUS_FLAG_PF_ADVANCE_OPS, x);  };
      inline void _bus_error(bool nu) {           _adapter_set_flag(I2C_BUS_FLAG_BUS_ERROR, nu);      };
      inline void _bus_online(bool nu) {          _adapter_set_flag(I2C_BUS_FLAG_BUS_ONLINE, nu);     };


    private:
      const I2CAdapterOptions _bus_opts;
      int8_t  ping_map[32];
      LinkedList<I2CDevice*> dev_list;    // A list of active slaves on this bus.

      int get_slave_dev_by_addr(uint8_t search_addr);

      I2CPingState get_ping_state_by_addr(uint8_t addr);
      void set_ping_state_by_addr(uint8_t addr, I2CPingState nu);


      static const char _ping_state_chr[4];
  };


  /*
  * This class represents a slave device on the bus. It should be extended by any class
  *   representing an i2c device, and should implement the given virtuals.
  *
  * Since the platform we are coding for uses an interrupt-driven i2c implementation,
  *   we will need to have callbacks.
  */
  class I2CDevice : public BusOpCallback {
    public:
      const uint8_t _dev_addr;

      /*
      * Constructor
      */
      I2CDevice(uint8_t); // Takes device address.
      ~I2CDevice();

      // Callback for requested operation completion.
      int8_t io_op_callahead(BusOp*);
      int8_t io_op_callback(BusOp*);
      int8_t queue_io_job(BusOp*);

      bool assignBusInstance(I2CAdapter*);   // Needs to be called by the i2c class during insertion.
      bool disassignBusInstance();           // This is to be called from the adapter's unassignment function.

      inline I2CAdapter* getAdapter() {  return _bus;   };

      /* Debug aides */
      virtual void printDebug(StringBuilder*);


    protected:
      I2CAdapter* _bus = nullptr;

      // Writes <byte_count> bytes from <buf> to the sub-address <sub_addr> of i2c device <dev_addr>.
      // Returns true if the job was accepted. False on error.
      bool writeX(int sub_addr, uint16_t byte_count, uint8_t *buf);
      bool readX(int sub_addr, uint8_t len, uint8_t *buf);
      bool ping_device();   // Pings the device.
  };
#endif  //__I2C_ABSTRACTION_LAYER_H__
