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
int8_t SPIAdapter::queue_io_job(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;
  int8_t ret = -5;

  if (op) {
    ret = -4;
    op->setAdapter(this);
    if (!op->hasCallback()) {
      op->callback = (BusOpCallback*) this;
    }

    if (op->get_state() == XferState::IDLE) {
      ret = 0;
      //if ((nullptr == current_job) && (work_queue.size() == 0)){
      //  // If the queue is empty, fire the operation now.
      //  current_job = op;
      //  advance_work_queue();
      //}
      //else {    // If there is something already in progress, queue up.
        if (_adapter_flag(SPI_FLAG_QUEUE_GUARD) && !roomInQueue()) {
          ret = -1;
          _local_log.concatf("SPI%u:\t Bus queue at max size. Dropping transaction.\n", ADAPTER_NUM);
          op->abort(XferFault::QUEUE_FLUSH);
          callback_queue.insertIfAbsent(op);
        }
        else if (0 > work_queue.insertIfAbsent(op)) {
          ret = -3;
          //if (getVerbosity() > 2) {
          //  _local_log.concatf("SPI%u:\t Double-insertion. Dropping transaction with no status change.\n", ADAPTER_NUM);
          //  op->printDebug(&_local_log);
          //}
        }
        else {
          op->set_state(XferState::QUEUED);
        }
      //}
    }
    else {
      if (getVerbosity() > 3) _local_log.concat("Tried to fire a bus op that is not in IDLE state.\n");
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

  if (current_job) {
    switch (current_job->get_state()) {
      case XferState::TX_WAIT:
      case XferState::RX_WAIT:
        if (current_job->hasFault()) {
          if (getVerbosity() > 3) _local_log.concatf("SPI%u::advance_work_queue():\t Failed at IO_WAIT.\n", ADAPTER_NUM);
        }
        else {
          current_job->markComplete();
        }
        // NOTE: No break on purpose.
      case XferState::COMPLETE:
        callback_queue.insert(current_job);
        current_job = nullptr;
        // TODO: Raise an event for service_callback_queue() if polling becomes burdensome.
        break;

      case XferState::IDLE:
      case XferState::INITIATE:
        switch (current_job->begin()) {
          case XferFault::NONE:     // Nominal outcome. Transfer started with no problens...
            break;
          case XferFault::BUS_BUSY:    // Bus appears to be in-use. State did not change.
            // Re-throw queue_ready event and try again later.
            if (getVerbosity() > 2) _local_log.concat("advance_work_queue() tried to clobber an existing transfer on chain.\n");
            current_job->set_state(XferState::INITIATE);
            break;
          default:    // Began the transfer, and it barffed... was aborted.
            if (getVerbosity() > 3) _local_log.concatf("advance_work_queue():\t Failed to begin transfer after starting. %s\n", BusOp::getErrorString(current_job->getFault()));
            callback_queue.insert(current_job);
            current_job = nullptr;
            // TODO: Raise an event for service_callback_queue() if polling becomes burdensome.
            break;
        }
        break;

      /* Cases below ought to be handled by ISR flow... */
      case XferState::ADDR:
        //current_job->advance_operation(0, 0);
      case XferState::STOP:
        if (getVerbosity() > 5) _local_log.concat("State might be corrupted if we tried to advance_queue(). \n");
        break;
      default:
        if (getVerbosity() > 6) _local_log.concat("advance_work_queue() default state \n");
        break;
    }
  }

  if (nullptr == current_job) {
    current_job = work_queue.dequeue();
    // Begin the bus operation.
    if (current_job) {
      XferFault f = current_job->begin();
      if (XferFault::NONE != f) {
        if (getVerbosity() > 2) _local_log.concatf("advance_work_queue() tried to clobber an existing transfer on the pick-up. %s\n", BusOp::getErrorString(f));
        // TODO: Raise an event for advance_work_queue() if polling becomes burdensome.
      }
      return_value++;
    }
    else {
      // No Queue! Relax...
    }
  }

  //flushLocalLog();
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
    if (getVerbosity() > 6) temp_op->printDebug(&_local_log);
    if (nullptr != temp_op->callback) {
      int8_t cb_code = temp_op->callback->io_op_callback(temp_op);
      switch (cb_code) {
        case BUSOP_CALLBACK_RECYCLE:
          temp_op->markForRequeue();
          queue_io_job(temp_op);
          break;

        case BUSOP_CALLBACK_ERROR:
        case BUSOP_CALLBACK_NOMINAL:
          reclaim_queue_item(temp_op);
          break;
        default:
          if (getVerbosity() > 1) _local_log.concatf("Unsure about BUSOP_CALLBACK_CODE %d.\n", cb_code);
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

  //flushLocalLog();
  return return_value;
}
