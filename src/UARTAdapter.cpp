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
  bus_deinit();
}


int8_t UARTAdapter::init() {
  _adapter_set_flag(UART_FLAG_QUEUE_IDLE);
  return bus_init();
}
