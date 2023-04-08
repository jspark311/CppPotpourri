/*
File:   SPIAdapter.cpp
Author: J. Ian Lindsay
Date:   2016.12.17
*/

#include "AbstractPlatform.h"
#include "SPIAdapter.h"


/**
* Constructor
*/
SPIAdapter::SPIAdapter(
  const uint8_t adapter,
  const uint8_t clk_pin,
  const uint8_t mosi_pin,
  const uint8_t miso_pin,
  const uint8_t max_queue
) : BusAdapter(adapter, max_queue), _CLK_PIN(clk_pin), _MOSI_PIN(mosi_pin), _MISO_PIN(miso_pin) {
}


/**
* Destructor. Should never be called.
*/
SPIAdapter::~SPIAdapter() {
  purge_queued_work();
  bus_deinit();
}


int8_t SPIAdapter::init() {
  _memory_init();
  _adapter_set_flag(SPI_FLAG_QUEUE_IDLE);
  return bus_init();
}



/*******************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄   Members related to the work queue
* ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌  and SPI bus I/O
* ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌ ▀▀▀▀█░█▀▀▀▀
* ▐░▌          ▐░▌       ▐░▌     ▐░▌       SPI transactions have two phases:
* ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌       1) ADDR (Addressing) Max of 4 bytes.
* ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌       2) IO_WAIT (Transfer)
*  ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀      ▐░▌
*           ▐░▌▐░▌               ▐░▌       Jobs having NULL buffers don't have
*  ▄▄▄▄▄▄▄▄▄█░▌▐░▌           ▄▄▄▄█░█▄▄▄▄     transfer phases. Jobs without ADDR
* ▐░░░░░░░░░░░▌▐░▌          ▐░░░░░░░░░░░▌    parameters don't have address
*  ▀▀▀▀▀▀▀▀▀▀▀  ▀            ▀▀▀▀▀▀▀▀▀▀▀     phases.
*******************************************************************************/

/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

/**
* This is what is called when the class wants to conduct a transaction on the bus.
* Note that this is different from other class implementations, in that it checks for
*   callback population before clobbering it. This is because this class is also the
*   SPI driver. This might end up being reworked later.
*
* @param  _op  The bus operation to execute.
* @return Zero on success, or appropriate error code.
*/
int8_t SPIAdapter::queue_io_job(BusOp* _op){
  return SPIAdapter::queue_io_job(_op, 0);
}

int8_t SPIAdapter::queue_io_job(BusOp* _op, int priority) {
  SPIBusOp* op = (SPIBusOp*) _op;
  int8_t ret = -5;

  if (op) {
    ret = -4;
    op->setAdapter(this);
    if (!op->hasCallback()) {
      op->callback = (BusOpCallback*) this;
    }

    if (op->get_state() == XferState::IDLE) {
      if (_adapter_flag(SPI_FLAG_QUEUE_GUARD) && !roomInQueue()) {
        ret = -1;
        if (getVerbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "SPI%u:\t Bus queue at max size. Dropping transaction.", ADAPTER_NUM);
        op->abort(XferFault::QUEUE_FLUSH);
        callback_queue.insertIfAbsent(op, priority);
      }
      else if (0 > work_queue.insertIfAbsent(op, priority)) {
        ret = -3;
        if (getVerbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "SPI%u:\t Double-insertion. Dropping transaction with no status change.\n", ADAPTER_NUM);
      }
      else {
        ret = 0;
        op->set_state(XferState::QUEUED);
      }
    }
    else {
      if (getVerbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "SPI%u:\t Tried to fire a bus op that is not in IDLE state.", ADAPTER_NUM);
    }
  }
  return ret;
}


/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

// These are platform-specific. They should be supplied in another file.
// int8_t SPIAdapter::bus_init() {
//   return 0;
// }
//
// int8_t SPIAdapter::bus_deinit() {
//   return 0;
// }


/**
* Calling this function will advance the work queue after performing cleanup
*   operations on the present or pending operation.
*
* @return the number of bus operations proc'd.
*/
int8_t SPIAdapter::advance_work_queue() {
  int8_t return_value = 0;

  if (nullptr == current_job) {
    current_job = work_queue.dequeue();
    // Begin the bus operation.
    if (current_job) {
      return_value++;
    }
  }

  if (current_job) {
    switch (current_job->get_state()) {
      case XferState::TX_WAIT:
      case XferState::RX_WAIT:
        if (_pf_needs_op_advance()) {
          // Platforms that need explicit advancement of BusOps should do so.
          current_job->advance_operation(0, 0);
        }
        if (current_job->hasFault()) {
          if (getVerbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "SPI%u\t Failed at IO_WAIT.\n", ADAPTER_NUM);
        }
        break;

      case XferState::COMPLETE:
      case XferState::FAULT:
        callback_queue.insert(current_job);
        current_job = nullptr;
        break;

      case XferState::IDLE:
        current_job->set_state(XferState::QUEUED);
      case XferState::QUEUED:
        switch (current_job->begin()) {
          case XferFault::NONE:     // Nominal outcome. Transfer started with no problens...
            break;
          case XferFault::BUS_BUSY:    // Bus appears to be in-use. State did not change.
            // Re-throw queue_ready event and try again later.
            current_job->set_state(XferState::QUEUED);
            break;
          default:    // Began the transfer, and it barffed... was aborted.
            if (getVerbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "SPI%u:\t Failed to begin transfer after starting. %s\n", ADAPTER_NUM, BusOp::getErrorString(current_job->getFault()));
            callback_queue.insert(current_job);
            current_job = nullptr;
            break;
        }
        break;

      /* Cases below ought to be handled by ISR flow... */
      case XferState::INITIATE:
        break;
      case XferState::ADDR:
        break;
      case XferState::STOP:
      default:
        if (getVerbosity() >= LOG_LEV_INFO) c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "SPI%u: BusOp state at poll(): %s", ADAPTER_NUM, BusOp::getStateString(current_job->get_state()));
        break;
    }
  }

  return return_value;
}


/**
* Execute any I/O callbacks that are pending. The function is present because
*   this class contains the bus implementation.
*
* @return the number of callbacks proc'd.
*/
int8_t SPIAdapter::service_callback_queue() {
  int8_t return_value = 0;

  while ((return_value < _cb_per_event) && (0 < callback_queue.size())) {
    SPIBusOp* temp_op = callback_queue.dequeue();
    if (nullptr != temp_op->callback) {
      int8_t cb_code = temp_op->callback->io_op_callback(temp_op);
      switch (cb_code) {
        case BUSOP_CALLBACK_RECYCLE:
          temp_op->markForRequeue();
          queue_io_job(temp_op);
          break;

        case BUSOP_CALLBACK_ERROR:
          if (temp_op->hasFault()) {
            if (getVerbosity() >= LOG_LEV_ERROR) {    // Print failures.
              StringBuilder tmp_str;
              temp_op->printDebug(&tmp_str);
              c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, &tmp_str);
            }
          }
        case BUSOP_CALLBACK_NOMINAL:
        default:
          reclaim_queue_item(temp_op);
          break;
      }
    }
    else {
      // We are the responsible party.
      reclaim_queue_item(temp_op);
    }
    return_value++;
  }

  return return_value;
}



/*******************************************************************************
* Console callback
* These are built-in handlers for using this instance via a console.
*******************************************************************************/

/**
* @page console-handlers
* @section spi-tools SPI tools
*
* This is the console handler for debugging the operation of `SPIAdapter`'s.
*
* @subsection arguments Arguments
* Argument | Purpose | Required
* -------- | ------- | --------
* 1        | BusID   | Yes
* 2        | Action  | No (Prints debugging info if omitted).
* 3        | Action-Specific | No
*
* @subsection cmd-actions Actions
* Action      | Description | Additional arguments
* ----------- | ----------- | --------------------
* `purge`     | Flush the current job. | None
* `ragepurge` | Flush the current job, as well as anything in the queue. | None
* `init`      | Manually calls the bus init function. | None
* `deinit`    | Manually calls the bus deinit function. | None
* `poll`      | Manually invoke the driver's `poll()` function. | None
* `queue`     | Render the current job queue to the console. | None
* `verbosity` | Print or limit how chatty the driver is. | [log-level]
*/
int8_t SPIAdapter::console_handler(StringBuilder* text_return, StringBuilder* args) {
  int ret = 0;
  if (0 < args->count()) {
    char* cmd = args->position_trimmed(0);
    if (0 == StringBuilder::strcasecmp(cmd, "poll")) {
      text_return->concatf("SP%u advance_work_queue() returns: %d\n", adapterNumber(), advance_work_queue());
      text_return->concatf("SP%u service_callback_queue() returns: %d\n", adapterNumber(), service_callback_queue());
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "init")) {
      text_return->concatf("SPI%u init() returns %d\n", adapterNumber(), init());
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "deinit")) {
      text_return->concatf("SPI%u deinit() returns %d\n", adapterNumber(), bus_deinit());
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "queue")) {
      uint8_t arg1 = (uint8_t) args->position_as_int(1);
      printWorkQueue(text_return, strict_max((uint8_t) 3, arg1));
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "purge")) {
      text_return->concatf("SPI%u purge_current_job()\n", adapterNumber());
      purge_current_job();
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "ragepurge")) {
      text_return->concatf("SPI%u purge_queued_work()\n", adapterNumber());
      text_return->concatf("SPI%u purge_current_job()\n", adapterNumber());
      purge_queued_work();
      purge_current_job();
    }
    else if (0 == StringBuilder::strcasecmp(cmd, "verbosity")) {
      if (1 < args->count()) {
        uint8_t arg1 = (uint8_t) args->position_as_int(1);
        setVerbosity(arg1);
      }
      text_return->concatf("Verbosity for SPI%u is %d\n", adapterNumber(), getVerbosity());
    }
    else {
      ret = -1;
    }
  }
  else printAdapter(text_return);

  return ret;
}
