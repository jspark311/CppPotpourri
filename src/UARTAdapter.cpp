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
* Destructor.
*/
UARTAdapter::~UARTAdapter() {
  _pf_deinit();
}


int8_t UARTAdapter::init(const UARTOpts* o) {
  _extnd_state = 0;
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
      output->concatf("\tRX: (%u bytes waiting)\n\t------------------------\n", pendingRxBytes());
      output->concatf("\tRing len:\t%u\n", _BUF_LEN_RX);
      output->concatf("\tLast RX: \t%u ms\n", last_byte_rx_time);
      output->concatf("\tTimeout: \t%u ms\n\n", rxTimeout());
    }
    if (txCapable()) {
      output->concatf("\tTX: (%u bytes waiting)\n\t------------------------\n", pendingTxBytes());
      output->concatf("\tRing len:\t%u\n", _BUF_LEN_TX);
      output->concatf("\tFlushed: \t%c\n", (flushed()?'y':'n'));
    }
  }
}
