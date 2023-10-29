/*
File:   C3PConsole.h
Author: J. Ian Lindsay
Date:   2020.01.05

Copyright 2020 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef __PARSING_CONSOLE_H__
#define __PARSING_CONSOLE_H__

#include <inttypes.h>
#include <stdint.h>
#include "StringBuilder.h"
#include "CppPotpourri.h"
#include "Pipes/BufferAccepter/BufferAccepter.h"
#include "LightLinkedList.h"
#include "EnumeratedTypeCodes.h"

/* Class flags. */
#define CONSOLE_FLAG_LOCAL_ECHO         0x01  // Should the console echo back?
#define CONSOLE_FLAG_EMIT_PROMPT        0x02  // Emit a prompt when idle?
#define CONSOLE_FLAG_FORCE_RETURN       0x04  // Force a non-empty response to commands.
#define CONSOLE_FLAG_HISTORY_FAIL       0x08  // Do failed commands make it into the history?
#define CONSOLE_FLAG_HAS_ANSI           0x10  // Should we colorize console output?
#define CONSOLE_FLAG_PRINT_HELP_ON_FAIL 0x20  // Should we print the help if the callback returns <0?


/*
* This is the callback fxn signature for commands. It will only be
*   called if a command is parsed successfully.
* Parameters are...
*   Log reference
*   Tokenized list of arguments, as strings.
*/
typedef int (*consoleCallback)(StringBuilder* log, StringBuilder* args);

/* Error conditions that this class might report. */
enum class ConsoleErr : uint8_t {
  NONE          = 0x00,    // Reserved. Denotes end-of-list.
  NO_MEM        = 0x01,    // Class ran out of memory.
  MISSING_ARG   = 0x02,    // Command recognized, but an argument was missing.
  INVALID_ARG   = 0x03,    // Command recognized, but an argument was wrong.
  CMD_NOT_FOUND = 0x04,    // Command not found.
  RESERVED      = 0xFF     // Reserved for custom extension.
};


/*
* This class represents a definition of a command. Many of these will be instanced
*   and stored in the console driver.
*/
class ConsoleCommand {
  public:
    const char*   cmd;          // The string that identifies the command.
    const char    shortcut;     // Single letter shortcut.
    const uint8_t req_count;    // How many of the arguments are required?
    const uint8_t should_free;  // Should this object be freed?
    const consoleCallback ccb;  // Callback for successful parse.
    const char*   help_text;    // One-line help text for this command.
    const char*   param_text;   // Detailed help text for this command.

    ConsoleCommand(const char* c, const char sc, const char* h, const char* p, const uint8_t r, const consoleCallback cb, const bool s_free = false) :
      cmd(c),
      shortcut(sc),
      req_count(r),
      should_free(s_free ? 1 : 0),
      ccb(cb),
      help_text(h),
      param_text(p) {};

    ~ConsoleCommand() {};


    void printDetailedHelp(StringBuilder* output);
    inline const bool shouldFree() {   return (0 != should_free);   };
};


/*
* This is the callback fxn signature for commands. It will only be
*   called if a command is parsed successfully.
* Parameters are...
*   Log reference
*   ConsoleErr (what went wrong)
*   ConsoleCommand, if applicable
*   Original input
*/
typedef int (*consoleErrCallback)(StringBuilder*, const ConsoleErr, const ConsoleCommand*, StringBuilder*);


/*
* This is the base for a console class.
*/
class C3P_Console : public BufferAccepter {
  public:
    /* Implementation of BufferAccepter is done by the child class. */
    virtual int8_t  pushBuffer(StringBuilder*) =0;
    virtual int32_t bufferAvailable()          =0;

    int8_t defineCommand(const char* c, const char* h, const char* p, const uint8_t r, const consoleCallback);
    int8_t defineCommand(const char* c, const char sc, const char* h, const char* p, const uint8_t r, const consoleCallback);
    int8_t defineCommand(const ConsoleCommand* cmd);
    int8_t defineCommands(const ConsoleCommand* cmds, const int cmd_count);

  protected:
    LinkedList<StringBuilder*>  _history;   // Stores a list of prior commands.
    LinkedList<ConsoleCommand*> _cmd_list;  // Stores a list of command definitions.
    uint8_t _max_cmd_len = 0;

    C3P_Console() {};
    ~C3P_Console();

    ConsoleCommand* _cmd_def_lookup(char*);     // Get a command from our list by its name.
};



/*
* This is the console class.
*/
class ParsingConsole : public C3P_Console {
  public:
    ParsingConsole(const uint16_t max_len) : _MAX_LEN(max_len) {};
    ~ParsingConsole();

    int8_t init();

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    void   fetchLog(StringBuilder*);
    void   printToLog(StringBuilder*);
    void   printHelp(StringBuilder*);
    void   printHelp(StringBuilder*, char*);
    void   printHistory(StringBuilder*);
    void   printPrompt();

    inline void setTXTerminator(LineTerm x) {  _tx_terminator = x; };
    inline void setRXTerminator(LineTerm x) {  _rx_terminator = x; };
    inline LineTerm getTXTerminator() {   return _tx_terminator;   };
    inline LineTerm getRXTerminator() {   return _rx_terminator;   };

    inline void errorCallback(consoleErrCallback ecb) {  errCB = ecb;           };
    inline void setOutputTarget(BufferAccepter* obj) {   _output_target = obj;  };

    // History management...
    void clearHistory();
    void maxHistoryDepth(uint8_t);
    inline uint8_t maxHistoryDepth() {    return _max_history;       };
    inline uint8_t historyDepth() {       return _history.size();    };
    inline uint8_t logLength() {          return _log.length();      };

    // Console features...
    inline bool localEcho() {          return _console_flag(CONSOLE_FLAG_LOCAL_ECHO);           };
    inline void localEcho(bool x) {    return _console_set_flag(CONSOLE_FLAG_LOCAL_ECHO, x);    };
    inline bool forceReturn() {        return _console_flag(CONSOLE_FLAG_FORCE_RETURN);         };
    inline void forceReturn(bool x) {  return _console_set_flag(CONSOLE_FLAG_FORCE_RETURN, x);  };
    inline bool emitPrompt() {         return _console_flag(CONSOLE_FLAG_EMIT_PROMPT);          };
    inline void emitPrompt(bool x) {   return _console_set_flag(CONSOLE_FLAG_EMIT_PROMPT, x);   };
    inline void setPromptString(const char* str) {    _prompt_string = (char*) str;   };

    inline bool historyFail() {            return _console_flag(CONSOLE_FLAG_HISTORY_FAIL);               };
    inline void historyFail(bool x) {      return _console_set_flag(CONSOLE_FLAG_HISTORY_FAIL, x);        };
    inline bool hasColor() {               return _console_flag(CONSOLE_FLAG_HAS_ANSI);                   };
    inline void hasColor(bool x) {         return _console_set_flag(CONSOLE_FLAG_HAS_ANSI, x);            };
    inline bool printHelpOnFail() {        return _console_flag(CONSOLE_FLAG_PRINT_HELP_ON_FAIL);         };
    inline void printHelpOnFail(bool x) {  return _console_set_flag(CONSOLE_FLAG_PRINT_HELP_ON_FAIL, x);  };

    static const char* const errToStr(ConsoleErr);

    /* Built-in per-instance console handlers. */
    int8_t console_handler_help(StringBuilder* text_return, StringBuilder* args);
    int8_t console_handler_conf(StringBuilder* text_return, StringBuilder* args);


  private:
    const uint16_t _MAX_LEN;   // Maximum buffer length before we reject input.
    uint8_t _max_history = 8;
    uint8_t _history_idx = 0;
    uint8_t _flags       = 0;
    LineTerm _tx_terminator  = LineTerm::CRLF;
    LineTerm _rx_terminator  = LineTerm::LF;   // Default should also support the CRLF case.
    char*    _prompt_string  = nullptr;    // Pointer to the optional prompt string.
    consoleErrCallback errCB = nullptr;    // Optional function pointer for failed commands.
    StringBuilder _buffer;     // Unused input is accumulated here.
    StringBuilder _log;        // Stores a log for retreival.
    BufferAccepter* _output_target = nullptr;

    int8_t _relay_to_output_target();

    int8_t _exec_line(StringBuilder*);          // Executes a single entered line.
    void   _append_to_history(StringBuilder*);  // Appends a command to the history.
    void   _cull_history();                     // Trims the history list back to bounds.

    bool _line_ending_rxd();
    int8_t _process_buffer();

    /* Flag manipulation inlines */
    inline uint8_t _console_flags() {                return _flags;           };
    inline bool _console_flag(uint8_t _flag) {       return (_flags & _flag); };
    inline void _console_clear_flag(uint8_t _flag) { _flags &= ~_flag;        };
    inline void _console_set_flag(uint8_t _flag) {   _flags |= _flag;         };
    inline void _console_set_flag(uint8_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};

#endif  // __PARSING_CONSOLE_H__
