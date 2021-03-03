#include <inttypes.h>
#include <stdint.h>
#include "BusQueue.h"
#include "StringBuilder.h"
#include "AbstractPlatform.h"

#ifndef __ABSTRACT_UART_QUEUE_TEMPLATE_H__
#define __ABSTRACT_UART_QUEUE_TEMPLATE_H__

/*
* Adapter flag defs.
*/
#define UART_FLAG_UART_READY   0x0001    // Is the UART initialized?
#define UART_FLAG_QUEUE_IDLE   0x0002    // Is the queue idle?
#define UART_FLAG_QUEUE_GUARD  0x0004    // Prevent bus queue floods?
#define UART_FLAG_9_BIT_CHAR   0x0008    // Bus configuration details.
#define UART_FLAG_RESERVED_0   0x0010    // Reserved
#define UART_FLAG_RESERVED_1   0x0020    // Reserved
#define UART_FLAG_HAS_TX       0x0040    // Bus configuration details.
#define UART_FLAG_HAS_RX       0x0080    // Bus configuration details.


/* Flow-control strategies. */
enum class UARTFlowControl : uint8_t {
  NONE        = 0,
  XON_XOFF_R  = 1,
  XON_XOFF_T  = 2,
  XON_XOFF_RT = 3,
  RTS         = 4,
  CTS         = 5,
  RTS_CTS     = 6
};

/* Parity bit strategies. */
enum class UARTParityBit : uint8_t {
  NONE    = 0,
  EVEN    = 1,
  ODD     = 2,
  FORCE_0 = 3,
  FORCE_1 = 4
};

/* Stop bit definitions. */
enum class UARTStopBit : uint8_t {
  STOP_1    = 0,
  STOP_1_5  = 1,
  STOP_2    = 2
};


/* Conf representation for a port. */
typedef struct {
  uint32_t        bitrate;
  uint8_t         start_bits;
  uint8_t         bit_per_word;
  UARTStopBit     stop_bits;
  UARTParityBit   parity;
  UARTFlowControl flow_control;
  uint8_t         xoff_char;
  uint8_t         xon_char;
  uint8_t         padding;
} UARTOpts;



/*
* The UART driver class.
*/
class UARTAdapter : public BufferAccepter {
  public:
    UARTAdapter(const uint8_t adapter, const uint8_t txd_pin, const uint8_t rxd_pin, const uint8_t cts_pin, const uint8_t rts_pin, const uint16_t tx_buf_len, const uint16_t rx_buf_len);
    ~UARTAdapter();

    /* Implementation of BufferAccepter. */
    int8_t provideBuffer(StringBuilder* buf) {  _tx_buffer.concatHandoff(buf); return 1;   };

    int8_t init(const UARTOpts*);
    int8_t poll();

    // Basic management and init,
    void reset();
    void printDebug(StringBuilder*);

    // On-the-fly conf accessors...
    inline UARTOpts* uartOpts() {   return &_opts;         };
    inline uint32_t bitrate() {     return _opts.bitrate;  };

    inline bool ready() {    return _port_ready;    };
    inline bool flushed() {  return _port_flushed;  };
    inline bool txCapable() {    return _adapter_flag(UART_FLAG_HAS_TX);     };
    inline bool rxCapable() {    return _adapter_flag(UART_FLAG_HAS_RX);     };

    uint write(uint8_t* buf, uint len);
    uint write(char c);
    inline uint write(const char* str) {  return write((uint8_t*) str, strlen(str));  }
    uint read(uint8_t* buf, uint len);
    uint read(StringBuilder*);

    inline void readCallback(BufferAccepter* cb) {   _read_cb_obj = cb;   };

    inline int pendingRxBytes() {   return _rx_buffer.length();   };
    inline int pendingTxBytes() {   return _tx_buffer.length();   };

    void irq_handler();


  private:
    const uint16_t _BUF_LEN_TX;
    const uint16_t _BUF_LEN_RX;
    const uint8_t  ADAPTER_NUM;   // The platform-relatable index of the adapter.
    const uint8_t  _TXD_PIN;      // Pin assignment for TXD.
    const uint8_t  _RXD_PIN;      // Pin assignment for RXD.
    const uint8_t  _CTS_PIN;      // Pin assignment for CTS.
    const uint8_t  _RTS_PIN;      // Pin assignment for RTS.
    UARTOpts _opts;

    uint32_t   bus_timeout_millis = 50;  // How long is a temporal break;
    uint32_t   last_byte_rx_time  = 0;   // Time of last character RX.
    BufferAccepter* _read_cb_obj  = nullptr;
    StringBuilder  _tx_buffer;
    StringBuilder  _rx_buffer;
    uint16_t   _extnd_state       = 0;   // Flags for the concrete class to use.

    // These values manipulated by driver.
    bool     _port_ready          = false;
    bool     _pending_conf_change = false;  // UART sets true. Driver sets false.
    bool     _pending_reset       = false;  // UART sets true. Driver sets false.
    bool     _port_flushed        = true;

	/* These functions are provided by the platform-specific code. */
    int8_t _pf_init();
    int8_t _pf_deinit();
    int8_t _pf_write();
    int8_t _pf_read();

    inline uint16_t _adapter_flags() {                 return _extnd_state;            };
    inline bool _adapter_flag(uint16_t _flag) {        return (_extnd_state & _flag);  };
    inline void _adapter_flip_flag(uint16_t _flag) {   _extnd_state ^= _flag;          };
    inline void _adapter_clear_flag(uint16_t _flag) {  _extnd_state &= ~_flag;         };
    inline void _adapter_set_flag(uint16_t _flag) {    _extnd_state |= _flag;          };
    inline void _adapter_set_flag(uint16_t _flag, bool nu) {
      if (nu) _extnd_state |= _flag;
      else    _extnd_state &= ~_flag;
    };
};

#endif  // __ABSTRACT_UART_QUEUE_TEMPLATE_H__
