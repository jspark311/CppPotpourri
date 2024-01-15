#include <inttypes.h>
#include <stdint.h>
#include "BusQueue.h"
#include "../StringBuilder.h"
#include "../AbstractPlatform.h"

#ifndef __SPI_QUEUE_TEMPLATE_H__
#define __SPI_QUEUE_TEMPLATE_H__

class SPIAdapter;

/* Compile-time bounds on memory usage. */
#ifndef CONFIG_SPIADAPTER_MAX_QUEUE_PRINT
  // How many queue items should we print for debug?
  #define CONFIG_SPIADAPTER_MAX_QUEUE_PRINT 3
#endif
#ifndef CONFIG_SPIADAPTER_PREALLOC_COUNT
  // How many queue items should we have on-tap?
  #define CONFIG_SPIADAPTER_PREALLOC_COUNT  4
#endif
#ifndef CONFIG_SPIADAPTER_MAX_QUEUE_DEPTH
  // How deep should the queue be allowed to become before rejecting work?
  #define CONFIG_SPIADAPTER_MAX_QUEUE_DEPTH 6
#endif

/*
* Adapter flag defs. The member that holds these is located in BusAdapter.
*/
#define SPI_FLAG_SPI_READY    0x0001    // Is SPI1 initialized?
#define SPI_FLAG_QUEUE_IDLE   0x0002    // Is the SPI queue idle?
#define SPI_FLAG_QUEUE_GUARD  0x0004    // Prevent bus queue floods?
#define SPI_FLAG_RESERVED_0   0x0008    // Reserved
#define SPI_FLAG_RESERVED_1   0x0010    // Reserved
#define SPI_FLAG_CPOL         0x0020    // Bus configuration details.
#define SPI_FLAG_CPHA         0x0040    // Bus configuration details.
#define SPI_FLAG_MASTER       0x0080    // Bus configuration details.
#define SPI_BUS_FLAG_PF_ADVANCE_OPS 0x4000    // Optionally set by the platform.
#define SPI_BUS_FLAG_PF_BEGIN_ASAP  0x8000    // Optionally set by the platform.

/*
* These flags are hosted by the member in the BusOp class.
* Be careful when scrubing the field between re-use.
*/
#define SPI_XFER_FLAG_FRAME_SIZE_MASK 0x0007   // Holds the enum that dictates frame size.
#define SPI_XFER_FLAG_DEVICE_CS_ASSRT 0x0008   // CS pin is presently asserted.
#define SPI_XFER_FLAG_DEVICE_CS_AH    0x0010   // CS pin for device is active-high.
#define SPI_XFER_FLAG_DEVICE_CPOL     0x0020   // Transfer-specific settings.
#define SPI_XFER_FLAG_DEVICE_CPHA     0x0040   // Transfer-specific settings.
#define SPI_XFER_FLAG_DEVICE_REG_INC  0x0080   // If set, indicates this operation advances addresses in the target device.


enum class SPIFrameSize : uint8_t {
  BITS_8  = 0,  // This is the default.
  BITS_9  = 1,
  BITS_16 = 2,
  BITS_24 = 3,
  BITS_32 = 4,
  BITS_R2 = 5,  // Reserved
  BITS_R1 = 6,  // Reserved
  BITS_R0 = 7   // Reserved
};


/*
* This class represents a single transaction on the SPI bus.
*/
class SPIBusOp : public BusOp {
  public:
    SPIBusOp();
    SPIBusOp(BusOpcode nu_op, BusOpCallback* requester);
    SPIBusOp(BusOpcode nu_op, BusOpCallback* requester, uint8_t cs, bool ah = false);
    virtual ~SPIBusOp();

    /* Mandatory overrides from the BusOp interface... */
    XferFault begin();
    void wipe();
    void printDebug(StringBuilder*);

    int8_t advance_operation(uint32_t status_reg, uint8_t data_reg);

    int8_t abort(XferFault fault = XferFault::NO_REASON);
    int8_t markComplete();

    int8_t  bitsPerFrame(SPIFrameSize);
    uint8_t bitsPerFrame();

    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6, uint8_t p7);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2);
    void setParams(uint8_t p0, uint8_t p1);
    void setParams(uint8_t p0);
    inline uint8_t getTransferParam(int x) {  return xfer_params[x]; };
    inline uint8_t transferParamLength() {    return _param_len;     };

    inline void setCSPin(uint8_t pin) {      _cs_pin = pin;  };
    inline uint8_t getCSPin() {              return _cs_pin; };
    inline void setAdapter(SPIAdapter* b) {  _bus = b;       };
    inline SPIAdapter* getAdapter() {        return _bus;    };

    /**
    * Is the chip select pin presently asserted?
    *
    * @return true if the CS pin is active.
    */
    inline bool csAsserted() {         return _busop_flag(SPI_XFER_FLAG_DEVICE_CS_ASSRT);   };
    inline void csAsserted(bool x) {   _busop_set_flag(SPI_XFER_FLAG_DEVICE_CS_ASSRT, x);   };

    /**
    * CPOL/CPHA settings for this specific transfer.
    *
    * @return The state of the given option bit.
    */
    inline bool cpol() {         return _busop_flag(SPI_XFER_FLAG_DEVICE_CPOL);   };
    inline void cpol(bool x) {   _busop_set_flag(SPI_XFER_FLAG_DEVICE_CPOL, x);   };
    inline bool cpha() {         return _busop_flag(SPI_XFER_FLAG_DEVICE_CPHA);   };
    inline void cpha(bool x) {   _busop_set_flag(SPI_XFER_FLAG_DEVICE_CPHA, x);   };

    /**
    * Is the chip select pin supposed to be active high?
    *
    * @return true if the CS pin is active.
    */
    inline bool csActiveHigh() {          return _busop_flag(SPI_XFER_FLAG_DEVICE_CS_AH);   };
    inline void csActiveHigh(bool x) {    _busop_set_flag(SPI_XFER_FLAG_DEVICE_CS_AH, x);   };

    /**
    * Accessors for the optional maximum frequency of the bus for this transaction.
    *
    * @return true if the CS pin is active.
    */
    inline uint32_t maxFreq() {          return _max_freq;   };
    inline void maxFreq(uint32_t x) {    _max_freq = x;      };


  private:
    SPIAdapter* _bus;
    uint32_t    _max_freq;
    uint8_t     _param_len;
    uint8_t     _cs_pin;
    uint8_t     xfer_params[8];

    int8_t _assert_cs(bool);
};



/*
* The SPI driver class.
*/
class SPIAdapter : public BusAdapter<SPIBusOp> {
  public:
    StopWatch profiler_cb;    // Profiler for bureaucracy within SPIBusOpCallback.

    SPIAdapter(const uint8_t adapter, const uint8_t clk_pin, const uint8_t mosi_pin, const uint8_t miso_pin, const uint8_t max_queue);
    ~SPIAdapter();

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);
    int8_t queue_io_job(BusOp*, int);

    int8_t advance_work_queue();
    int8_t service_callback_queue();

    int8_t setMode(const uint8_t);
    int8_t frequency(const uint32_t);
    inline uint32_t frequency() {   return _current_freq;   };

    int8_t init();
    void printDebug(StringBuilder*);
    void printHardwareState(StringBuilder*);

    inline bool allQueuesClear() {   return (busIdle() && (0 == callback_queue.size()));  };

    /* Built-in per-instance console handler. */
    int8_t console_handler(StringBuilder* text_return, StringBuilder* args);


  private:
    const uint8_t _CLK_PIN;      // Pin assignments for SPI.
    const uint8_t _MOSI_PIN;     // Pin assignments for SPI.
    const uint8_t _MISO_PIN;     // Pin assignments for SPI.
    uint8_t _cb_per_event  = 3;  // Limit the number of callbacks processed per event.
    uint32_t _current_freq = 0;  // The current frequency of the adapter.
    PriorityQueue<SPIBusOp*> callback_queue;  // List of pending callbacks for bus transactions.

    /* Overrides from the BusAdapter interface */
    int8_t bus_init();
    int8_t bus_deinit();

    inline bool _pf_needs_op_advance() {        return _adapter_flag(SPI_BUS_FLAG_PF_ADVANCE_OPS);  };
    inline void _pf_needs_op_advance(bool x) {  _adapter_set_flag(SPI_BUS_FLAG_PF_ADVANCE_OPS, x);  };
};

#endif  // __SPI_QUEUE_TEMPLATE_H__
