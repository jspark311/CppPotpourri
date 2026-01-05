/*
File:   UARTAdapter.h
Author: J. Ian Lindsay

*/

#ifndef __ABSTRACT_UART_QUEUE_TEMPLATE_H__
#define __ABSTRACT_UART_QUEUE_TEMPLATE_H__

#include <inttypes.h>
#include <stdint.h>
#include "../Pipes/BufferAccepter/BufferAccepter.h"
#include "../StringBuilder.h"
#include "../RingBuffer.h"
#include "../AbstractPlatform.h"

/*
* Adapter flag defs.
*/
#define UART_FLAG_UART_READY    0x01    // Is the UART initialized?
#define UART_FLAG_PENDING_RESET 0x04    // Peripheral is waiting on a reset.
#define UART_FLAG_PENDING_CONF  0x08    // Peripheral is waiting on a reconf.
#define UART_FLAG_HAS_TX        0x10    // Bus configuration details.
#define UART_FLAG_HAS_RX        0x20    // Bus configuration details.


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
* TODO: This should be a base class with pure-virtuals, rather than a
*   hard-linked function provided by platform.
*/
class UARTAdapter : public BufferAccepter, public C3PPollable {
  public:
    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    int8_t init(const UARTOpts*);
    PollResult poll();

    int8_t deinit();

    // Basic management and init,
    int8_t reset();
    void printDebug(StringBuilder*);

    // On-the-fly conf accessors...
    inline UARTOpts* uartOpts() {   return &_opts;         };
    inline uint32_t bitrate() {     return _bitrate_real;  };

    inline bool initialized() {  return _adapter_flag(UART_FLAG_UART_READY); };
    inline bool flushed() {      return _flushed;                            };
    inline bool txCapable() {    return _adapter_flag(UART_FLAG_HAS_TX);     };
    inline bool rxCapable() {    return _adapter_flag(UART_FLAG_HAS_RX);     };

    uint32_t write(StringBuilder*);
    uint32_t write(uint8_t* buf, uint32_t len);
    uint32_t write(char c);
    uint32_t read(uint8_t* buf, uint32_t len);
    uint32_t read(StringBuilder*);
    virtual void irq_handler() =0;

    inline uint32_t write(const char* str) {  return write((uint8_t*) str, strlen(str));  }
    inline void readCallback(BufferAccepter* cb) {   _read_cb_obj = cb;   };

    inline int pendingRxBytes() {     return _rx_buffer.count();    };
    inline int pendingTxBytes() {     return _tx_buffer.count();    };
    inline uint32_t rxTimeout() {     return bus_timeout_millis;    };
    inline uint32_t lastRXTime() {    return last_byte_rx_time;     };
    inline bool rxTimeoutExpired() {  return (millis_since(last_byte_rx_time) > bus_timeout_millis);  };

    inline const uint8_t txdPin() {   return _TXD_PIN;   };
    inline const uint8_t rxdPin() {   return _RXD_PIN;   };
    inline const uint8_t ctsPin() {   return _CTS_PIN;   };
    inline const uint8_t rtsPin() {   return _RTS_PIN;   };

    int uart_console_handler(StringBuilder* text_return, StringBuilder* args);


    static const char* const flowCtrlStr(const UARTFlowControl);


  protected:
    const uint8_t   ADAPTER_NUM;              // The platform-relatable index of the adapter.
    const uint8_t   _TXD_PIN;                 // Pin assignment for TXD.
    const uint8_t   _RXD_PIN;                 // Pin assignment for RXD.
    const uint8_t   _CTS_PIN;                 // Pin assignment for CTS.
    const uint8_t   _RTS_PIN;                 // Pin assignment for RTS.
    uint8_t         _extnd_state       = 0;   // Flags for the concrete class to use.
    bool            _flushed           = true;   // Assumed to be set by ISR.
    UARTOpts        _opts;
    uint32_t        bus_timeout_millis = 50;  // How long is a temporal break;
    uint32_t        last_byte_rx_time  = 0;   // Time of last character RX.
    uint32_t        _bitrate_real      = 0;   // The hardware's report of the true bitrate.
    BufferAccepter* _read_cb_obj       = nullptr;
    RingBuffer<uint8_t> _tx_buffer;
    RingBuffer<uint8_t> _rx_buffer;

    UARTAdapter(
      const uint8_t adapter,
      const uint8_t txd_pin, const uint8_t rxd_pin,
      const uint8_t cts_pin, const uint8_t rts_pin,
      const uint16_t tx_buf_len, const uint16_t rx_buf_len
    );

    /* Destructor. */
    virtual ~UARTAdapter();


    int _handle_rx_push();

    inline uint8_t _adapter_flags() {                 return _extnd_state;            };
    inline bool _adapter_flag(uint8_t _flag) {        return (_extnd_state & _flag);  };
    inline void _adapter_clear_flag(uint8_t _flag) {  _extnd_state &= ~_flag;         };
    inline void _adapter_set_flag(uint8_t _flag) {    _extnd_state |= _flag;          };
    inline void _adapter_set_flag(uint8_t _flag, bool nu) {
      if (nu) _extnd_state |= _flag;
      else    _extnd_state &= ~_flag;
    };

    /* These functions are provided by the platform-specific code. */
    virtual int8_t   _pf_init()   =0;
    virtual int8_t   _pf_poll()   =0;
    virtual int8_t   _pf_deinit() =0;
};

#endif  // __ABSTRACT_UART_QUEUE_TEMPLATE_H__
