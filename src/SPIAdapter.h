#include <inttypes.h>
#include <stdint.h>
#include "BusQueue.h"
#include "StringBuilder.h"
#include "AbstractPlatform.h"

#ifndef __SPI_QUEUE_TEMPLATE_H__
#define __SPI_QUEUE_TEMPLATE_H__

class SPIAdapter;

/*
* These flags are hosted by the member in the BusOp class.
* Be careful when scrubing the field between re-use.
*/
#define SPI_XFER_FLAG_FRAME_SIZE_MASK 0x0007   // Holds the enum that dictates frame size.
#define SPI_XFER_FLAG_PROFILE         0x0008   // If set, this bus operation shall be profiled.
#define SPI_XFER_FLAG_DEVICE_CS_ASSRT 0x0010   // CS pin is presently asserted.
#define SPI_XFER_FLAG_DEVICE_CS_AH    0x0020   // CS pin for device is active-high.
#define SPI_XFER_FLAG_DEVICE_REG_INC  0x0040   // If set, indicates this operation advances addresses in the target device.
// 0x80 is used by the superclass.


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
    int8_t markComplete();

    /**
    * This will mark the bus operation complete with a given error code.
    * Overriden for simplicity. Marks the operation with failure code NO_REASON.
    *
    * @return 0 on success. Non-zero on failure.
    */
    inline int8_t abort() {    return abort(XferFault::NO_REASON); }
    int8_t abort(XferFault);

    int8_t  bitsPerFrame(SPIFrameSize);
    uint8_t bitsPerFrame();

    void setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3);
    void setParams(uint8_t p0, uint8_t p1, uint8_t p2);
    void setParams(uint8_t p0, uint8_t p1);
    void setParams(uint8_t p0);
    inline uint8_t getTransferParam(int x) {  return xfer_params[x]; };
    inline uint8_t transferParamLength() {    return _param_len;     };

    inline void setCSPin(uint8_t pin) {      _cs_pin = pin;  };
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
    * Is the chip select pin supposed to be active high?
    *
    * @return true if the CS pin is active.
    */
    inline bool csActiveHigh() {          return _busop_flag(SPI_XFER_FLAG_DEVICE_CS_AH);   };
    inline void csActiveHigh(bool x) {    _busop_set_flag(SPI_XFER_FLAG_DEVICE_CS_AH, x);   };

    static uint16_t  spi_wait_timeout;   // In microseconds. Per-byte.


  private:
    SPIAdapter* _bus       = nullptr;
    uint8_t xfer_params[4] = {0, 0, 0, 0};
    uint8_t  _param_len    = 0;
    uint8_t  _cs_pin       = 255;  // Chip-select pin.

    int8_t _assert_cs(bool);
};


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
#define SPI_FLAG_SPI_READY    0x01    // Is SPI1 initialized?
#define SPI_FLAG_QUEUE_IDLE   0x02    // Is the SPI queue idle?
#define SPI_FLAG_QUEUE_GUARD  0x04    // Prevent bus queue floods?
#define SPI_FLAG_RESERVED_0   0x08    // Reserved
#define SPI_FLAG_RESERVED_1   0x10    // Reserved
#define SPI_FLAG_CPOL         0x20    // Bus configuration details.
#define SPI_FLAG_CPHA         0x40    // Bus configuration details.
#define SPI_FLAG_MASTER       0x80    // Bus configuration details.



/*
* The SPI driver class.
*/
class SPIAdapter : public BusAdapter<SPIBusOp> {
  public:
    SPIAdapter(const uint8_t adapter, const uint8_t clk_pin, const uint8_t mosi_pin, const uint8_t miso_pin, const uint8_t max_queue);
    ~SPIAdapter();

    /* Overrides from the BusAdapter interface */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);
    int8_t advance_work_queue();

    int8_t service_callback_queue();

    int8_t init();
    void printDebug(StringBuilder*);
    void printHardwareState(StringBuilder*);


  private:
    const uint8_t  _CLK_PIN;      // Pin assignments for SPI.
    const uint8_t  _MOSI_PIN;     // Pin assignments for SPI.
    const uint8_t  _MISO_PIN;     // Pin assignments for SPI.

    uint8_t   spi_cb_per_event   = 3;  // Limit the number of callbacks processed per event.
    uint32_t  bus_timeout_millis = 5;  // How long to spend in IO_WAIT?
    PriorityQueue<SPIBusOp*> callback_queue;  // List of pending callbacks for bus transactions.

    /* Overrides from the BusAdapter interface */
    int8_t bus_init();
    int8_t bus_deinit();
};

#endif  // __SPI_QUEUE_TEMPLATE_H__
