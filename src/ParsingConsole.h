/*
File:   ParsingConsole.h
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

#include <inttypes.h>
#include <stdint.h>
#include "StringBuilder.h"
#include "CppPotpourri.h"
#include "LightLinkedList.h"
#include "EnumeratedTypeCodes.h"

#ifndef __PARSING_CONSOLE_H__
#define __PARSING_CONSOLE_H__

/*
* Class flags.
*/
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
enum class LineTerm : uint8_t {
  ZEROBYTE = 0x00,
  CR       = 0x01,
  LF       = 0x02,
  CRLF     = 0x03
};


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
    const TCode*  fmt;          // A null-terminated string of TCodes.
    const consoleCallback ccb;  // Callback for successful parse.
    const char*   help_text;    // One-line help text for this command.
    const char*   param_text;   // Detailed help text for this command.

    ConsoleCommand(const char* c, const char sc, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback cb) :
      cmd(c),
      shortcut(sc),
      req_count(r),
      should_free(0),
      fmt(f),
      ccb(cb),
      help_text(h),
      param_text(p) {};

    ConsoleCommand(const char* c, const char sc, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback cb, const bool s_free) :
      cmd(c),
      shortcut(sc),
      req_count(r),
      should_free(1),
      fmt(f),
      ccb(cb),
      help_text(h),
      param_text(p) {};


    ~ConsoleCommand() {};

    void printDetailedHelp(StringBuilder* output);
    int maxArgumentCount();

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
* This is the console class.
*/
class ParsingConsole : public BufferAccepter {
  public:
    ParsingConsole(const uint16_t max_len);
    ~ParsingConsole();

    int8_t init();

    /* Implementation of BufferAccepter. */
    int8_t provideBuffer(StringBuilder* buf);

    void   fetchLog(StringBuilder*);
    void   printToLog(StringBuilder*);
    void   printHelp(StringBuilder*);
    void   printHelp(StringBuilder*, char*);
    void   printHistory(StringBuilder*);
    inline void setTXTerminator(LineTerm x) {  _tx_terminator = x; };
    inline void setRXTerminator(LineTerm x) {  _rx_terminator = x; };
    inline LineTerm getTXTerminator() {   return _tx_terminator;   };
    inline LineTerm getRXTerminator() {   return _rx_terminator;   };

    int8_t defineCommand(const char* c, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback);
    int8_t defineCommand(const char* c, const char sc, const TCode* f, const char* h, const char* p, const uint8_t r, const consoleCallback);
    int8_t defineCommand(const ConsoleCommand* cmd);
    int8_t defineCommands(const ConsoleCommand* cmds, const int cmd_count);
    inline void errorCallback(consoleErrCallback ecb) {  errCB = ecb;           };
    inline void setOutputTarget(BufferAccepter* obj) {   _output_target = obj;  };

    // History management...
    void clearHistory();
    void maxHistoryDepth(uint8_t);
    inline uint8_t maxHistoryDepth() {    return _max_history;       };
    inline uint8_t historyDepth() {       return _history.size();    };
    inline uint8_t logLength() {          return _log.length();      };


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
    static const char* const typecodeToStr(TCode);
    static void styleHeader1(StringBuilder*, const char*);
    //static void styleHeader2(StringBuilder*, const char*);

    /* Common static TCode strings. */
    static const TCode tcodes_0[];
    static const TCode tcodes_uint_1[];
    static const TCode tcodes_uint_2[];
    static const TCode tcodes_uint_3[];
    static const TCode tcodes_uint_4[];
    static const TCode tcodes_str_1[];
    static const TCode tcodes_str_2[];
    static const TCode tcodes_str_3[];
    static const TCode tcodes_str_4[];
    static const TCode tcodes_float_1[];


  private:
    const uint16_t _MAX_LEN;   // Maximum buffer length before we reject input.
    uint8_t _max_history = 8;
    uint8_t _max_cmd_len = 0;
    uint8_t _history_idx = 0;
    uint8_t _flags       = 0;
    LineTerm _tx_terminator  = LineTerm::CRLF;
    LineTerm _rx_terminator  = LineTerm::CRLF;
    char*    _prompt_string  = nullptr;    // Pointer to the optional prompt string.
    consoleErrCallback errCB = nullptr;    // Optional function pointer for failed commands.
    StringBuilder _buffer;     // Unused input is accumulated here.
    StringBuilder _log;        // Stores a log for retreival.
    BufferAccepter* _output_target = nullptr;
    LinkedList<StringBuilder*>  _history;   // Stores a list of prior commands.
    LinkedList<ConsoleCommand*> _cmd_list;  // Stores a list of command definitions.

    int8_t _relay_to_output_target();

    int8_t _exec_line(StringBuilder*);          // Executes a single entered line.
    void   _append_to_history(StringBuilder*);  // Appends a command to the history.
    void   _cull_history();                     // Trims the history list back to bounds.
    ConsoleCommand* _cmd_def_lookup(char*);     // Get a command from our list by its name.

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

    static const char* const _get_terminator(LineTerm);
};

#endif  // __PARSING_CONSOLE_H__
