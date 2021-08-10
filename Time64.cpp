/*
  Time64 Library:
  (c) Patrick J. Scruggs 2018

  Original Time Library:
  Copyright (c) Michael Margolis 2009-2014

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "TimeLib64.h"

static tmElements_t tm;          // a cache of time elements
static time_t cacheTime;   // the time the cache was updated
static uint32_t syncInterval = 300;  // time sync will be attempted after this many seconds

void refreshCache(time_t t) {
  if (t != cacheTime) {
    breakTime(t, tm);
    cacheTime = t;
  }
}

int hour() { // the hour now
  return hour(now());
}

int hour(time_t t) { // the hour for the given time
  refreshCache(t);
  return tm.Hour;
}

int hourFormat12() { // the hour now in 12 hour format
  return hourFormat12(now());
}

int hourFormat12(time_t t) { // the hour for the given time in 12 hour format
  refreshCache(t);
  if ( tm.Hour == 0 )
    return 12; // 12 midnight
  else if ( tm.Hour  > 12)
    return tm.Hour - 12 ;
  else
    return tm.Hour ;
}

uint8_t isAM() { // returns true if time now is AM
  return !isPM(now());
}

uint8_t isAM(time_t t) { // returns true if given time is AM
  return !isPM(t);
}

uint8_t isPM() { // returns true if PM
  return isPM(now());
}

uint8_t isPM(time_t t) { // returns true if PM
  return (hour(t) >= 12);
}

int minute() {
  return minute(now());
}

int minute(time_t t) { // the minute for the given time
  refreshCache(t);
  return tm.Minute;
}

int second() {
  return second(now());
}

int second(time_t t) {  // the second for the given time
  refreshCache(t);
  return tm.Second;
}

int day() {
  return (day(now()));
}

int day(time_t t) { // the day for the given time (0-6)
  refreshCache(t);
  return tm.Day;
}

int weekday() {   // Sunday is day 1
  return  weekday(now());
}

int weekday(time_t t) {
  refreshCache(t);
  return tm.Wday;
}

int month() {
  return month(now());
}

int month(time_t t) {  // the month for the given time
  refreshCache(t);
  return tm.Month;
}

int year() {  // as in Processing, the full four digit year: (2009, 2010 etc)
  return year(now());
}

int year(time_t t) { // the year for the given time
  refreshCache(t);
  return tmYearToCalendar(tm.Year);
}

/*============================================================================*/
/* functions to convert to and from system time */
/* These are for interfacing with time serivces and are not normally needed in a sketch */

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

static  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void breakTime(time_t timeInput, tmElements_t &tm) {
  // break the given time_t into time components
  // this is a more compact version of the C library localtime function
  // note that year is offset from 1970 !!!

  int64_t time;
  int32_t  month_bin;
  int32_t day_bin;
  int64_t  days;
  int64_t previous_days;
  int32_t  min_bin;
  int64_t  sec_bin;

  time = (int64_t)timeInput;

  if (time >= 0)
  {
    sec_bin = time % 86400;
    tm.Second =  sec_bin % 60;
    min_bin = sec_bin / 60;
    tm.Hour = min_bin / 60;
    tm.Minute = min_bin % 60;
    day_bin = time / 86400;
    tm.Wday = ((day_bin + 4) % 7) + 1;
    tm.Year = ((day_bin * 400) + 2) / 146097;
    const boolean leap = LEAP_YEAR(tm.Year);

    days = -1;

    for (int32_t y = 0; y < tm.Year; y++) {
      if (LEAP_YEAR(y)) {
        days += 366;
      }
      else {
        days += 365;
      }
    }

    day_bin -= days;

    if (day_bin == 0) {

      tm.Month = 1;
      tm.Day = 1;
    }

    else {
      previous_days = 0;
      days = 0;
      month_bin = -1;


      while (days < day_bin) {
        month_bin ++;
        previous_days = days;

        if (month_bin == 1) {
          days += (28 + leap);
        }

        else {
          days += monthDays[month_bin];
        }
      }

      tm.Month = month_bin + 1;
      tm.Day = day_bin - previous_days;
    }
  }

  else {
    sec_bin = time % 86400;
    tm.Second =  sec_bin % 60 + 60;
    min_bin = sec_bin / 60;
    tm.Hour = min_bin / 60 + 23;
    tm.Minute = min_bin % 60 + 59;
    day_bin = time / 86400;
    tm.Wday = 7 - (abs(day_bin - 3) % 7);
    tm.Year = (((day_bin * 400) + 2) / 146097) - 1;
    const boolean leap = LEAP_YEAR(tm.Year);


    for (int32_t y = -1; y > tm.Year; y--) {
      if (LEAP_YEAR(y)) {
        day_bin += 366;
      }
      else {
        day_bin += 365;
      }
    }

    if (day_bin == 0) {
      tm.Month = 12;
      tm.Day = 31;
    }

    else {
      month_bin = 13;

      while (day_bin < 1) {
        month_bin --;
        if (leap) {
          const uint8_t leapMonthDays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
          day_bin  += leapMonthDays[month_bin - 1];
        }
        else {
          day_bin  += monthDays[month_bin - 1];
        }
      }

      tm.Month = month_bin;
      tm.Day = day_bin;
    }
  }
}

time_t makeTime(const tmElements_t &tm) {
  // assemble time elements into time_t
  // note year argument is offset from 1970 (see macros in time.h to convert to other formats)
  // previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9

  int64_t i;
  int64_t seconds = 0;
  const boolean leap = LEAP_YEAR(tm.Year);

  if (tm.Year >= 0) {

    seconds = tm.Year * (SECS_PER_DAY * 365);
    for (i = 0; i < tm.Year; i++) {
      if (LEAP_YEAR(i)) {
        seconds +=  SECS_PER_DAY;   // add extra days for leap  years
      }
    }

    // add days for this year, months start from 1
    for (i = 1; i < tm.Month; i++) {
      if ( (i == 2) && leap) {
        seconds += SECS_PER_DAY * 29;
      }
      else {
        seconds += SECS_PER_DAY * monthDays[i - 1]; //monthDay array  starts from 0
      }
    }
    seconds += (tm.Day - 1) * SECS_PER_DAY;
    seconds += tm.Hour * SECS_PER_HOUR;
    seconds += tm.Minute * SECS_PER_MIN;
    seconds += tm.Second;
    return (time_t)seconds;
  }

  else {

    seconds = (tm.Year + 1) * (SECS_PER_DAY * 365);

    for (i = -1; i > tm.Year; i--) {
      if (LEAP_YEAR(i)) {
        seconds -=  SECS_PER_DAY;   // add extra days for leap  years
      }
    }

    if (leap) {
      const uint8_t leapMonthDays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      for (i = 12; i > tm.Month; i--) {
        seconds -= leapMonthDays[i - 1] * SECS_PER_DAY;
      }
      seconds -= (leapMonthDays[tm.Month - 1] - tm.Day) * SECS_PER_DAY;
    }
    else {
      for (i = 12; i > tm.Month; i--) {
        seconds -= SECS_PER_DAY * (monthDays[i - 1]);
      }

      seconds -= (monthDays[tm.Month - 1] - tm.Day) * SECS_PER_DAY;
    }

    seconds -= (23 - tm.Hour) * SECS_PER_HOUR;
    seconds -= (59 - tm.Minute) * SECS_PER_MIN;
    seconds -= (60 - tm.Second);
    return (time_t)seconds;

  }
}



/*=====================================================*/
/* Low level system time functions  */

static int64_t sysTime = 0;
static uint32_t prevMillis = 0;
static int64_t nextSyncTime = 0;
static timeStatus_t Status = timeNotSet;

getExternalTime getTimePtr;  // pointer to external sync function
//setExternalTime setTimePtr; // not used in this version

#ifdef TIME_DRIFT_INFO   // define this to get drift data
time_t sysUnsyncedTime = 0; // the time sysTime unadjusted by sync
#endif


time_t now() {
  // calculate number of seconds passed since last call to now()
  while (millis() - prevMillis >= 1000) {
    // millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
    sysTime++;
    prevMillis += 1000;
#ifdef TIME_DRIFT_INFO
    sysUnsyncedTime++; // this can be compared to the synced time to measure long term drift
#endif
  }
  if (nextSyncTime <= sysTime) {
    if (getTimePtr != 0) {
      time_t t = getTimePtr();
      if (t != 0) {
        setTime(t);
      } else {
        nextSyncTime = sysTime + syncInterval;
        Status = (Status == timeNotSet) ?  timeNotSet : timeNeedsSync;
      }
    }
  }
  return (time_t)sysTime;
}

void setTime(time_t t) {
#ifdef TIME_DRIFT_INFO
  if (sysUnsyncedTime == 0)
    sysUnsyncedTime = t;   // store the time of the first call to set a valid Time
#endif

  sysTime = (int64_t)t;
  nextSyncTime = (int64_t)t + syncInterval;
  Status = timeSet;
  prevMillis = millis();  // restart counting from now (thanks to Korman for this fix)
}

void setTime(int hr, int min, int sec, int dy, int mnth, int yr)
{
  tm.Year = yr - 1970;
  tm.Month = mnth;
  tm.Day = dy;
  tm.Hour = hr;
  tm.Minute = min;
  tm.Second = sec;
  setTime(makeTime(tm));
}

void adjustTime(long adjustment) {
  sysTime += adjustment;
}

// indicates if time has been set and recently synchronized
timeStatus_t timeStatus() {
  now(); // required to actually update the status
  return Status;
}

void setSyncProvider( getExternalTime getTimeFunction) {
  getTimePtr = getTimeFunction;
  nextSyncTime = sysTime;
  now(); // this will sync the clock
}

void setSyncInterval(time_t interval) { // set the number of seconds between re-sync
  syncInterval = (uint32_t)interval;
  nextSyncTime = sysTime + syncInterval;
}