/* 
 * Filename: cat-curfew.ino
 * FileType: Arduino source file
 * Author: Thomas Mitchell
 * Last modified: 21/11/2023
 * Description: Uses RTC alarms to toggle servo twice a day
 */

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>      // https://github.com/janelia-arduino/Streaming
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Servo.h>
 
/*** CHANGE THESE ***/
// servo open and close angles
#define OPEN  90
#define CLOSE 270

constexpr uint8_t SQW_PIN {2};  // RTC provides an alarm signal on this pin
const int BUTTON_PIN = 3;
const int SERVO_PIN = 12;
volatile Servo myServo; 
DS3232RTC myRTC;

long debouncing_time = 500; // Debouncing Time in Milliseconds
volatile unsigned long last_micros;
volatile bool closed = false;

void setup()
{
    // enable serial monitor
    Serial.begin(115200);
    Serial.println("Starting...");

    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    myRTC.begin();
    myRTC.setAlarm(DS3232RTC::ALM1_MATCH_DATE, 0, 0, 0, 1);
    myRTC.setAlarm(DS3232RTC::ALM2_MATCH_DATE, 0, 0, 0, 1);
    myRTC.alarm(DS3232RTC::ALARM_1);
    myRTC.alarm(DS3232RTC::ALARM_2);
    myRTC.alarmInterrupt(DS3232RTC::ALARM_1, false);
    myRTC.alarmInterrupt(DS3232RTC::ALARM_2, false);
    myRTC.squareWave(DS3232RTC::SQWAVE_NONE);

    /*** CHANGE THESE ***/
    // set the RTC time
    tmElements_t tm;
    tm.Hour = 19;              
    tm.Minute = 59;
    tm.Second = 45;
    tm.Day = 1;
    tm.Month = 1;
    tm.Year = 2023 - 1970;      // tmElements_t.Year is the offset from 1970
    myRTC.write(tm);            // set the RTC from the tm structure
  
    // configure an interrupt on button push
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), debounceHandler, RISING);

    /*** CHANGE THESE ***/
    // set alarms for 05:30:00 and 19:00:00
    myRTC.setAlarm(DS3232RTC::ALM1_MATCH_HOURS, 0, 7, 1);
    myRTC.setAlarm(DS3232RTC::ALM2_MATCH_HOURS, 0, 20, 1);

    // clear the alarm flags
    myRTC.alarm(DS3232RTC::ALARM_1);
    myRTC.alarm(DS3232RTC::ALARM_2);

    // enable interrupt output for alarms
    myRTC.alarmInterrupt(DS3232RTC::ALARM_1, true);
    myRTC.alarmInterrupt(DS3232RTC::ALARM_2, true);

    // move servo to 'open' position
    myServo.attach(SERVO_PIN);
    openServo();
}

void loop() {
  // check to see if the INT/SQW pin is low, i.e. an alarm has occurred
  if (!digitalRead(SQW_PIN)) {
    if (myRTC.alarm(DS3232RTC::ALARM_1)) {  // resets the alarm flag if set
      printDateTime( myRTC.get() );
      Serial << "  Alarm 1\n";
      openServo();
    }
    else if (myRTC.alarm(DS3232RTC::ALARM_2)) {
      printDateTime( myRTC.get() );
      Serial << "  Alarm 2\n";
      closeServo();
    }
  }
}

void debounceHandler()
{
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    last_micros = micros();
    if (closed) {
      openServo();
    } else {
      closeServo();
    } 
  }
}

void closeServo()
{
  myServo.write(CLOSE);
  closed = true;
  Serial.println("CLOSED");
}

void openServo()
{
  myServo.write(OPEN);
  closed = false;
  Serial.println("OPENED");
}

void printDateTime(time_t t)
{
    Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t));
    Serial << monthShortStr(month(t)) << _DEC(year(t)) << ' ';
    Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
    Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
    Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}
