/*
File:   CppPotpourri.cpp
Author: J. Ian Lindsay
Date:   2020.09.10

Copyright 2016 Manuvr, Inc

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

#include "CppPotpourri.h"
#include "StringBuilder.h"
#include <time.h>

void timestampToString(StringBuilder* output, uint64_t ts) {
  const struct tm* ptm = localtime((const time_t*) &ts);
  char buf[64] = "";
  if (0 != strftime(buf, sizeof(buf), "%T %F", ptm)) {
    output->concat(buf);
  }
}


/**
* Takes an ISO-8601 datatime string in Zulu time, and returns the corresponding epoch time.
* Does no bounds checking on the input string. Assumes it is at least 20 bytes.
*
* NOTE: 1972 was the first leap-year of the epoch.
* NOTE: We don't handle dates prior to the Unix epoch (since we return an epoch timestamp).
* NOTE: Leap years that are divisible by 100 are NOT leap years unless they are also
*         divisible by 400. I have no idea why. Year 2000 meets this criteria, but 2100
*         does not. 2100 is not a leap year. In any event, this code will give bad results
*         past the year 2100. So fix it before then.
* NOTE: We can't handle dates prior to 1583 because some king ripped 10 days out of the
*         calandar in October of the year prior.
*
* Format: 2016-11-16T21:44:07Z
*
* TODO: Time is a *really* complicated idea. Validate this function's correctness.
* This code will never support timezone offsets.
*
* @param str The string containing the string to convert. Not bounds-checked.
* @return 0 on failure, or epoch timestamp otherwise.
*/
uint64_t stringToTimestamp(const char* str) {
  char tmp_buf[20];
  memcpy(tmp_buf, str, 20);
  tmp_buf[4]  = '\0';   // Year boundary.
  tmp_buf[7]  = '\0';   // Month
  tmp_buf[10] = '\0';   // Days
  tmp_buf[13] = '\0';   // Hours
  tmp_buf[16] = '\0';   // Minutes
  tmp_buf[19] = '\0';   // Seconds
  int year    = atoi(&tmp_buf[0]);
  int month   = atoi(&tmp_buf[5]);
  int hour    = atoi(&tmp_buf[11]);
  int minute  = atoi(&tmp_buf[14]);
  unsigned int days = atoi(&tmp_buf[8]);
  uint64_t seconds  = atoi(&tmp_buf[17]);
  //printf("%d-%d-%u %d:%d:%u\n",year, month, days, hour, minute, seconds);

  // Boundary-checks
  if ((days > 31) || (month > 12) || (year < 1970) || (seconds > 59) || (minute > 59) || (hour > 23)) {
    // All of these conditions are bad.
    // NOTE: we ignore the possibility of some other system calling midnight "24:00".
    return 0;
  }

  if (year >= 1972) {
    // Check for leap year.
    days += ((year - 1972) >> 2);  // Extra days caused by leap-years.
    if ((((year - 1972) % 4) == 0) && (month > 2)) {
      // If it is at-least March of this year, and this year is a leap-year, add another 86400.
      days++;
    }
  }

  // This much is easy, since ISO-8601 disregards leap-seconds, and we are assuming
  //   Zulu time (no DST brain damage).
  seconds += (minute * 60) + (hour * 3600);

  // Add the Y-M-D without leaps...
  days += (year - 1970) * 365; // Days in a year.
  switch (month-1) {           // Days in each month.
    case 11:  days += 30;
    case 10:  days += 31;
    case 9:   days += 30;
    case 8:   days += 31;
    case 7:   days += 31;
    case 6:   days += 30;
    case 5:   days += 31;
    case 4:   days += 30;
    case 3:   days += 31;
    case 2:   days += 28;
    case 1:   days += 31;
    case 0:
      break;
    default:
      // Not valid.
      return 0;
  }
  // Finally, add the days...
  seconds += (days - 1) * 86400;
  return seconds;
}


/*
* randomArt() taken from github user nirenjan and bent to fit this project.
* No original license was found. Assuming attribution-only.
* https://gist.github.com/nirenjan/4450419
*                                                    ---J. Ian Lindsay
*
* "Hash Visualization: a New Technique to improve Real-World Security",
* Perrig A. and Song D., 1999, International Workshop on Cryptographic
* Techniques and E-Commerce (CrypTEC '99)
* sparrow.ece.cmu.edu/~adrian/projects/validation/validation.pdf
*/
#define XLIM 17
#define YLIM 9
#define ARSZ (XLIM * YLIM)

static const char* ra_symbols = " .-:+=R^v<>*&#XoO";

uint8_t ra_new_position(uint8_t *pos, uint8_t direction) {
  uint8_t newpos;
  uint8_t upd = 1;
  int8_t x0 = *pos % XLIM;
  int8_t y0 = *pos / XLIM;
  int8_t x1 = (direction & 0x01) ? (x0 + 1) : (x0 - 1);
  int8_t y1 = (direction & 0x02) ? (y0 + 1) : (y0 - 1);

  // Limit the range of x1 & y1
  if (x1 < 0) {
    x1 = 0;
  }
  else if (x1 >= XLIM) {
    x1 = XLIM - 1;
  }

  if (y1 < 0) {
    y1 = 0;
  }
  else if (y1 >= YLIM) {
    y1 = YLIM - 1;
  }

  newpos = y1 * XLIM + x1;

  if (newpos == *pos) {
    upd = 0;
  }
  else {
    *pos = newpos;
  }
  return upd;
}


int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* title, StringBuilder* output) {
  if (0 >= dgst_raw_len) {
    return -1;
  }
  uint8_t array[ARSZ];
  bzero(&array, ARSZ);
  uint8_t pos = 76;

  for (uint16_t idx = 0; idx < dgst_raw_len; idx++) {
    uint8_t temp = *(dgst_raw + idx);
    for (uint8_t i = 0; i < 4; i++) {
      if (ra_new_position(&pos, (temp & 0x03))) array[pos]++;
      temp >>= 2;
    }
  }

  array[pos] = 16; // End
  array[76]  = 15; // Start

  StringBuilder temp;
  temp.concatf("+--[%10s ]--+\n", title);
  char line_buf[21];
  line_buf[0]  = '|';
  line_buf[18] = '|';
  line_buf[19] = '\n';
  line_buf[20] = '\0';
  for (uint8_t i = 0; i < YLIM; i++) {
    for (uint8_t j = 0; j < XLIM; j++) {
      *(line_buf + j + 1) = *(ra_symbols + array[j + (XLIM * i)] % 18);
    }
    temp.concatf(line_buf);
  }
  temp.concat("+-----------------+\n");
  temp.string();
  output->concatHandoff(&temp);
  return 0;
}
