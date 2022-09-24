/*
File:   LinkUtils.h
Author: J. Ian Lindsay
Date:   2022.09.03

This header contains classes which sit on top of a link, and add some high-level
  capability.
*/

#include "M2MLink.h"
#include "../ParsingConsole.h"
#include "../SensorFilter.h"


/*******************************************************************************
* A pair of classes for unidirectional data-sharing.
*******************************************************************************/
/*
* The host is typically an embedded system that maintains time-series data that
*   it wants to annotate and make avaiable to other systems.
*/
class LinkDataHost {
  public:
    LinkDataHost(M2MLink* l);
    ~LinkDataHost() {};

  private:
    M2MLink* _link;
};


/*
* The client is instantiated on a system that wants to replicate offboard data.
*/
class LinkDataClient {
  public:
    LinkDataClient(M2MLink* l);
    ~LinkDataClient() {};

  private:
    M2MLink* _link;
};



/*******************************************************************************
* A pair of classes for remote console.
*******************************************************************************/
/*
*/
class LinkConsoleHost {
  public:
    LinkConsoleHost(M2MLink* l, ParsingConsole*);
    ~LinkConsoleHost() {};

  private:
    M2MLink*     _link;
    ParsingConsole* _console;
};


/*
*/
class LinkConsoleClient {
  public:
    LinkConsoleClient(M2MLink* l);
    ~LinkConsoleClient() {};

  private:
    M2MLink* _link;
};



/*******************************************************************************
* A class to implement a distributed hash table.
*******************************************************************************/


/*******************************************************************************
* A pair of classes to expose device configuration for remote manipulation.
*******************************************************************************/
