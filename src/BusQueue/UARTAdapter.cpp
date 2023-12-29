/*
File:   UARTAdapter.cpp
Author: J. Ian Lindsay
Date:   2020.06.10
*/

#include "AbstractPlatform.h"
#include "UARTAdapter.h"


UARTAdapter::UARTAdapter(
  const uint8_t adapter,
  const uint8_t txd_pin,
  const uint8_t rxd_pin,
  const uint8_t cts_pin,
  const uint8_t rts_pin,
  const uint16_t tx_buf_len,
  const uint16_t rx_buf_len
) : ADAPTER_NUM(adapter), _TXD_PIN(txd_pin), _RXD_PIN(rxd_pin), _CTS_PIN(cts_pin), _RTS_PIN(rts_pin),
  _tx_buffer(tx_buf_len), _rx_buffer(rx_buf_len) {}


/**
* Destructor.
*/
UARTAdapter::~UARTAdapter() {
  _pf_deinit();
}


int8_t UARTAdapter::init(const UARTOpts* o) {
  _extnd_state = 0;
  _tx_buffer.allocated();    // Enforce ring allocation.
  _rx_buffer.allocated();    // Enforce ring allocation.
  _adapter_set_flag(UART_FLAG_PENDING_CONF);
  for (uint32_t i = 0; i < sizeof(UARTOpts); i++) {
    *((uint8_t*) &_opts + i) = *((uint8_t*) o + i);
  }
  return _pf_init();
}


void UARTAdapter::printDebug(StringBuilder* output) {
  StringBuilder temp("UART");
  temp.concatf("%u (%sinitialized", ADAPTER_NUM, (initialized() ? "":"un"));
  if (initialized()) {
    temp.concatf(", %u bps)", _opts.bitrate);
  }
  else {
    temp.concat(")");
  }
  StringBuilder::styleHeader1(output, (char*) temp.string());

  if (initialized()) {
    output->concatf("\tPending reset:\t%c\n", _adapter_flag(UART_FLAG_PENDING_RESET) ? 'y':'n');
    output->concatf("\tPending conf:\t%c\n", _adapter_flag(UART_FLAG_PENDING_CONF) ? 'y':'n');
  }

  char* str_par = (char*) "<INVALID>";
  char* str_flw = str_par;
  char* str_stp = str_par;
  switch (_opts.parity) {
    case UARTParityBit::NONE:       str_par = (char*) "NONE";     break;
    case UARTParityBit::EVEN:       str_par = (char*) "EVEN";     break;
    case UARTParityBit::ODD:        str_par = (char*) "ODD";      break;
    case UARTParityBit::FORCE_0:    str_par = (char*) "FORCE_0";  break;
    case UARTParityBit::FORCE_1:    str_par = (char*) "FORCE_1";  break;
    default: break;
  }
  switch (_opts.flow_control) {
    case UARTFlowControl::NONE:     str_flw = (char*) "NONE";     break;
    case UARTFlowControl::RTS:      str_flw = (char*) "RTS";      break;
    case UARTFlowControl::CTS:      str_flw = (char*) "CTS";      break;
    case UARTFlowControl::RTS_CTS:  str_flw = (char*) "RTS_CTS";  break;
    default: break;
  }
  switch (_opts.stop_bits) {
    case UARTStopBit::STOP_1:       str_stp = (char*) "1";        break;
    case UARTStopBit::STOP_1_5:     str_stp = (char*) "1.5";      break;
    case UARTStopBit::STOP_2:       str_stp = (char*) "2";        break;
    default: break;
  }

  output->concat("\tPins:\n\t------------------------\n");
  if (255 != _TXD_PIN) output->concatf("\tTXD:  %d (%s)\n", _TXD_PIN, (readPin(_TXD_PIN) ? "high" : "low"));
  if (255 != _RXD_PIN) output->concatf("\tRXD:  %d (%s)\n", _RXD_PIN, (readPin(_RXD_PIN) ? "high" : "low"));
  if (255 != _CTS_PIN) output->concatf("\tCTS:  %d (%s)\n", _CTS_PIN, (readPin(_CTS_PIN) ? "high" : "low"));
  if (255 != _RTS_PIN) output->concatf("\tRTS:  %d (%s)\n", _RTS_PIN, (readPin(_RTS_PIN) ? "high" : "low"));

  output->concat("\tOpts:\n\t------------------------\n");
  output->concatf("\tChar size:\t%u bits\n", _opts.bit_per_word);
  output->concatf("\tStart bits:\t%u\n",     _opts.start_bits);
  output->concatf("\tStop bits:\t%s\n",      str_stp);
  output->concatf("\tParity:\t\t%s\n",       str_par);
  output->concatf("\tFlow CTRL:\t%s\n\n",    str_flw);

  if (initialized()) {
    if (rxCapable()) {
      output->concatf("\tRX ring: %u bytes waiting (max %u)\n\t------------------------\n", pendingRxBytes(), _rx_buffer.capacity());
      output->concatf("\tLast RX: \t%u ms\n", last_byte_rx_time);
      output->concatf("\tTimeout: \t%u ms\n\n", rxTimeout());
    }
    if (txCapable()) {
      output->concatf("\tTX ring: %u bytes waiting (max %u)\n\t------------------------\n", pendingTxBytes(), _tx_buffer.capacity());
      output->concatf("\tFlushed: \t%c\n", (flushed()?'y':'n'));
    }
  }
}


/*******************************************************************************
* Implementation of BufferAccepter.
* This is the self-managed interface to a UART, and is usually preferable to the
*   direct read/write API (given below).
*******************************************************************************/

/**
* This function is the basis of all write operations to the UART.
*
* NOTE: The abstraction will not allow excursions past its declared buffer limit.
*   In the event that it is requested, the UART driver will take all that it can,
*   free that memory from the arguemnt, and return 0 to inform the caller that
*   not all memory was claimed.
*
* @param is the pointer to the managed container for the content.
* @return -1 to reject buffer, 0 to accept with partial claim, 1 to accept with full claim.
*/
int8_t UARTAdapter::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if ((nullptr != buf) & txCapable()) {
    const int32_t FULL_BUFFER_LEN = (int32_t) buf->length();
    const int32_t TXBUF_AVAILBLE = bufferAvailable();
    // For this call to make sense, there must be at least some input data,
    //   and some free buffer to accept it.
    const int32_t BYTES_TO_TAKE = strict_min(TXBUF_AVAILBLE, FULL_BUFFER_LEN);
    if (0 < BYTES_TO_TAKE) {
      // Iterate through each fragment in the StringBuilder and bulk-insert each
      //   into the RingBuffer, freeing them as we go.
      // If we were to call StringBuilder::string(), there is an excellent
      //   chance of needlessly forcing reallocation in StringBuilder. So this
      //   is not only safer, but faster.
      int32_t bytes_taken  = 0;
      bool    bail_on_loop = false;
      while (!bail_on_loop & (bytes_taken < BYTES_TO_TAKE)) {
        uint32_t frag_len = 0;
        uint8_t* frag_ptr = buf->position(0, &frag_len);
        bail_on_loop = (nullptr == frag_ptr) & (frag_len > 0);
        if (!bail_on_loop) {
          const int32_t BYTES_REMAINING = (BYTES_TO_TAKE - bytes_taken);
          const int32_t BYTES_TO_INSERT = strict_min((int32_t) frag_len, BYTES_REMAINING);
          const uint32_t BYTES_INSERTED = _tx_buffer.insert(frag_ptr, BYTES_TO_INSERT);
          bytes_taken += BYTES_INSERTED;
          // Drop the entire fragment (if possible) or cull the bytes we took.
          if (BYTES_INSERTED == frag_len) {   buf->drop_position(0);      }
          else {                              buf->cull(BYTES_INSERTED);  }
        }
      }
      ret = (FULL_BUFFER_LEN > bytes_taken) ? 0 : 1;
    }
  }
  return ret;
}


/**
* TODO: For minimum confusion, we need a bi-directional analog of BufferAccepter.
* This function is called by a client class trying to send data over the UART.
*   Thus we consider the TX ring.
*
* @return the number of bytes available in the TX ring.
*/
int32_t UARTAdapter::bufferAvailable() {
  int32_t ret = -1;
  if (_tx_buffer.allocated()) {
    ret = (int32_t) _tx_buffer.vacancy();
  }
  return ret;
}


/*******************************************************************************
* Basic read//write API.
* These are a simpler alternative to BufferAccepter, and are usually only used
*   by classes that extend the UARTAdapter class, or manage a UARTAdapter
*   directly.
*******************************************************************************/

uint32_t UARTAdapter::write(StringBuilder* buf) {
  const uint32_t STARTING_LENGTH = (uint32_t) buf->length();
  pushBuffer(buf);
  const uint32_t ENDING_LENGTH   = (uint32_t) buf->length();
  return (STARTING_LENGTH >= ENDING_LENGTH) ? (STARTING_LENGTH - ENDING_LENGTH) : 0;
}


/*
* Write to the UART.
*/
uint32_t UARTAdapter::write(uint8_t* buf, uint32_t len) {
  uint32_t ret = 0;
  if (txCapable()) {
    ret = _tx_buffer.insert(buf, len);
  }
  return ret;
}


/*
* Write to the UART.
*/
uint32_t UARTAdapter::write(char c) {
  uint32_t ret = 0;
  if (txCapable()) {
    ret = ((0 == _tx_buffer.insert(c)) ? 1 : 0);
  }
  return ret;
}


/*
* Read from the class buffer.
*/
uint32_t UARTAdapter::read(uint8_t* buf, uint32_t len) {
  uint32_t XFER_LEN = strict_min(len, (uint32_t) _rx_buffer.count());
  if (0 < XFER_LEN) {
    for (uint32_t i = 0; i < XFER_LEN; i++) {
      *(buf + i) = _rx_buffer.get();
    }
  }
  return XFER_LEN;
}


/*
* Read from the class buffer.
*/
uint32_t UARTAdapter::read(StringBuilder* buf) {
  const uint32_t RX_COUNT = _rx_buffer.count();
  uint32_t ret = 0;
  if (0 < RX_COUNT) {
    uint8_t* _temp_buf = (uint8_t*) malloc(RX_COUNT);
    if (nullptr != _temp_buf) {
      ret = _rx_buffer.get(_temp_buf, RX_COUNT);
      buf->concatHandoff(_temp_buf, ret);
    }
  }
  return ret;
}


/*******************************************************************************
* Private implementations details.
*******************************************************************************/

/**
* Read from the class buffer.
*
* @return the number of bytes from the RX buffer that were pushed downstream.
*/
int UARTAdapter::_handle_rx_push() {
  int ret = 0;
  const uint32_t RX_COUNT = _rx_buffer.count();
  if (0 < RX_COUNT) {
    if (nullptr != _read_cb_obj) {
      const uint32_t BUF_AVAILABLE = _read_cb_obj->bufferAvailable();
      const uint32_t SAFE_RX_COUNT = strict_min(BUF_AVAILABLE, RX_COUNT);
      if (SAFE_RX_COUNT) {
        StringBuilder  unpushed_rx;
        uint8_t* temp_buf = (uint8_t*) malloc(SAFE_RX_COUNT);
        if (nullptr != temp_buf) {
          const uint32_t RX_BYTES_FROM_RB = _rx_buffer.get(temp_buf, SAFE_RX_COUNT);
          unpushed_rx.concatHandoff(temp_buf, RX_BYTES_FROM_RB);
          if (1 != _read_cb_obj->pushBuffer(&unpushed_rx)) {
            // TODO: There is a contract violation here. The pushed buffer might
            //   be at least partially rejected, in which case, the data will
            //   be dropped.
          }
          ret = (RX_BYTES_FROM_RB - unpushed_rx.length());
        }
      }
    }
  }
  return ret;
}
