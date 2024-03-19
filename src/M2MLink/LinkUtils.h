/*
File:   LinkUtils.h
Author: J. Ian Lindsay
Date:   2022.09.03

This header contains classes which sit on top of a link, and add some high-level
  capability.
*/

#ifndef __M2MLINK_UTILS_H_
#define __M2MLINK_UTILS_H_

#include "M2MLink.h"
#include "../Console/C3PConsole.h"
#include "../SensorFilter.h"


// These are specific M2MService classes that have been isolated into their own
//   subdirectories.
#include "LinkUtils/M2MLinkRPC.h"

/*******************************************************************************
* A pair of classes for unidirectional data-sharing.
*******************************************************************************/
/*
* The host is typically an embedded system that maintains time-series data that
*   it wants to annotate and make avaiable to other systems.
*/
class LinkDataHost : public M2MService {
  public:
    LinkDataHost(M2MLink* l);
    ~LinkDataHost();

    int8_t listData(C3PValue*);
    int8_t unlistData(C3PValue*);
    int8_t listData(KeyValuePair*);
    int8_t unlistData(KeyValuePair*);

    /* Implementation of M2MService. Trades messages with a link. */
    int8_t _handle_msg(uint32_t tag, M2MMsg*);
    int8_t _poll_for_link(M2MLink*);


  private:
    uint16_t _seq_mark;

    //int8_t _build_message();
};


/*
* The client is instantiated on a system that wants to read offboard data.
*/
class LinkDataClient : public M2MService {
  public:
    LinkDataClient(M2MLink* l);
    ~LinkDataClient();

    /* Implementation of M2MService. Trades messages with a link. */
    virtual int8_t _handle_msg(uint32_t tag, M2MMsg*) =0;
    virtual int8_t _poll_for_link(M2MLink*) =0;

  private:
    C3PValue* _mirror_val;

    //int8_t _refresh_list();
};



/*******************************************************************************
* A pair of classes for remote console.
*******************************************************************************/
/*
* This class does what ParsingConsole does. But for machines.
*/
class M2MLinkConsoleHost : public C3PConsole, public M2MService {
  public:
    M2MLinkConsoleHost(M2MLink*, ParsingConsole*);
    ~M2MLinkConsoleHost() {};

    /* Implementation of M2MService. Trades messages with a link. */
    virtual int8_t _handle_msg(uint32_t tag, M2MMsg*) =0;
    virtual int8_t _poll_for_link(M2MLink*) =0;


  private:
    ParsingConsole* _console;
};


/*
*/
class M2MLinkConsoleClient {
  public:
    M2MLinkConsoleClient(M2MLink*);
    ~M2MLinkConsoleClient() {};

  private:
};



/*******************************************************************************
* A class to implement a distributed hash table.
*******************************************************************************/


/*******************************************************************************
* A pair of classes to expose device configuration for remote manipulation.
*******************************************************************************/

#endif  // __M2MLINK_UTILS_H_
