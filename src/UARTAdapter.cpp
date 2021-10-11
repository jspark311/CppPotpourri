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
) : _BUF_LEN_TX(tx_buf_len), _BUF_LEN_RX(rx_buf_len), ADAPTER_NUM(adapter), _TXD_PIN(txd_pin), _RXD_PIN(rxd_pin), _CTS_PIN(cts_pin), _RTS_PIN(rts_pin) {
}


/**
* Destructor. Should never be called.
*/
UARTAdapter::~UARTAdapter() {
  _pf_deinit();
}


int8_t UARTAdapter::init(const UARTOpts* o) {
  _extnd_state = 0;
  _adapter_set_flag(UART_FLAG_PENDING_CONF);
  for (uint i = 0; i < sizeof(UARTOpts); i++) {
    *((uint8_t*) &_opts + i) = *((uint8_t*) o + i);
  }
  return _pf_init();
}


void UARTAdapter::printDebug(StringBuilder* output) {
  StringBuilder temp("UART");
  temp.concatf("%u (%sinitialized)", ADAPTER_NUM, (initialized() ? "":"un"));
  StringBuilder::styleHeader1(output, (char*) temp.string());
  output->concatf("\tLast RX: \t%u\n", last_byte_rx_time);
  output->concatf("\tFlushed: \t%c\n", (flushed()?'y':'n'));
  output->concatf("\tRX bytes:\t%u\n", pendingRxBytes());
  output->concatf("\tTX bytes:\t%u\n", pendingTxBytes());

  output->concatf("\tbitrate: \t%u\n", _opts.bitrate);
}
