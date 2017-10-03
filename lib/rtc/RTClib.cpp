// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// Modified to suit nixie tap project

#include <Wire.h>
#include "RTClib.h"
#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#elif defined(ARDUINO_ARCH_SAMD)
// nothing special needed
#elif defined(ARDUINO_SAM_DUE)
 #define PROGMEM
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
 #define Wire Wire1
#endif



#if (ARDUINO >= 100)
 #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
 // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
 #define _I2C_WRITE write
 #define _I2C_READ  read
#else
 #include <WProgram.h>
 #define _I2C_WRITE send
 #define _I2C_READ  receive
#endif




static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}


////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;// BQ32000 register addresses:
#define BQ32000_CAL_CFG1        0x07
#define BQ32000_TCH2            0x08
#define BQ32000_CFG2            0x09
#define BQ32000_SFKEY1          0x20
#define BQ32000_SFKEY2          0x21
#define BQ32000_SFR             0x22

// BQ32000 config bits:
#define BQ32000__OUT            0x07 // CAL_CFG1 - IRQ active state// BQ32000 register addresses:
#define BQ32000_CAL_CFG1        0x07
#define BQ32000_TCH2            0x08
#define BQ32000_CFG2            0x09
#define BQ32000_SFKEY1          0x20
#define BQ32000_SFKEY2          0x21
#define BQ32000_SFR             0x22

// BQ32000 config bits:
#define BQ32000__OUT            0x07 // CAL_CFG1 - IRQ active state
#define BQ32000__FT             0x06 // CAL_CFG1 - IRQ square wave enable
#define BQ32000__CAL_S          0x05 // CAL_CFG1 - Calibration sign
#define BQ32000__TCH2_BIT       0x05 // TCH2 - Trickle charger switch 2// BQ32000 register addresses:
#define BQ32000_CAL_CFG1        0x07
#define BQ32000_TCH2            0x08
#define BQ32000_CFG2            0x09
#define BQ32000_SFKEY1          0x20
#define BQ32000_SFKEY2          0x21
#define BQ32000_SFR             0x22

// BQ32000 config bits:
#define BQ32000__OUT            0x07 // CAL_CFG1 - IRQ active state
#define BQ32000__FT             0x06 // CAL_CFG1 - IRQ square wave enable
#define BQ32000__CAL_S          0x05 // CAL_CFG1 - Calibration sign
#define BQ32000__TCH2_BIT       0x05 // TCH2 - Trickle charger switch 2
#define BQ32000__TCFE           0x06 // CFG2 - Trickle FET control
// BQ32000 config values:
#define BQ32000_CHARGE_ENABLE   0x05 // CFG2 - Trickle charger switch 1 enable
#define BQ32000_SFKEY1_VAL      0x5E
#define BQ32000_SFKEY2_VAL      0xC7
#define BQ32000_FTF_1HZ         0x01
#define BQ32000_FTF_512HZ       0x00
#define BQ32000__TCFE           0x06 // CFG2 - Trickle FET control
// BQ32000 config values:
#define BQ32000_CHARGE_ENABLE   0x05 // CFG2 - Trickle charger switch 1 enable
#define BQ32000_SFKEY1_VAL      0x5E
#define BQ32000_SFKEY2_VAL      0xC7
#define BQ32000_FTF_1HZ         0x01
#define BQ32000_FTF_512HZ       0x00
#define BQ32000__FT             0x06 // CAL_CFG1 - IRQ square wave enable
#define BQ32000__CAL_S          0x05 // CAL_CFG1 - Calibration sign
#define BQ32000__TCH2_BIT       0x05 // TCH2 - Trickle charger switch 2
#define BQ32000__TCFE           0x06 // CFG2 - Trickle FET control
// BQ32000 config values:
#define BQ32000_CHARGE_ENABLE   0x05 // CFG2 - Trickle charger switch 1 enable
#define BQ32000_SFKEY1_VAL      0x5E
#define BQ32000_SFKEY2_VAL      0xC7
#define BQ32000_FTF_1HZ         0x01
#define BQ32000_FTF_512HZ       0x00
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

DateTime::DateTime (const DateTime& copy):
  yOff(copy.yOff),
  m(copy.m),
  d(copy.d),
  hh(copy.hh),
  mm(copy.mm),
  ss(copy.ss)
{}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (date[0]) {
        case 'J': m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7); break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

// A convenient constructor for using "the compiler's time":
// This version will save RAM by using PROGMEM to store it by using the F macro.
//   DateTime now (F(__DATE__), F(__TIME__));
DateTime::DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    char buff[11];
    memcpy_P(buff, date, 11);
    yOff = conv2d(buff + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (buff[0]) {
        case 'J': m = (buff[1] == 'a') ? 1 : ((buff[2] == 'n') ? 6 : 7); break;
        case 'F': m = 2; break;
        case 'A': m = buff[2] == 'r' ? 4 : 8; break;
        case 'M': m = buff[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(buff + 4);
    memcpy_P(buff, time, 8);
    hh = conv2d(buff);
    mm = conv2d(buff + 3);
    ss = conv2d(buff + 6);
}

uint8_t DateTime::dayOfTheWeek() const {
    uint16_t day = date2days(yOff, m, d);
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const {
  long t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  return t;
}


////////////////////////////////////////////////////////////////////////////////
// RTC_BQ32000 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

boolean RTC_BQ32000::begin(void) {
    Wire.pins(RTC_SDA_PIN, RTC_SCL_PIN);
    Wire.begin();
    // enable charger
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire._I2C_WRITE((byte)9);
    Wire._I2C_WRITE((byte)0x05);
    Wire._I2C_WRITE((byte)8);
    Wire._I2C_WRITE((byte)0x20);
    Wire._I2C_WRITE((byte)9);
    Wire._I2C_WRITE((byte)0x45);
    Wire.endTransmission();
    // enable freq output
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire.write(BQ32000_SFKEY1);
    Wire.write(BQ32000_SFKEY1_VAL);
    Wire.write(BQ32000_SFKEY2_VAL);
    Wire.write(BQ32000_FTF_1HZ);
    Wire.endTransmission();


    return true;
}

uint8_t RTC_BQ32000::isrunning(void) {
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(BQ32000_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return !(ss>>7);
}

void RTC_BQ32000::adjust(const DateTime& dt) {
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();
}


void RTC_BQ32000::configure_interrrupts() {
    pinMode(RTC_IRQ_PIN, INPUT_PULLUP);

    // configure RTC to output a 512 Hz signal as output
    Wire.beginTransmission(BQ32000_ADDRESS);
    Wire._I2C_WRITE((byte)0x7);
    Wire._I2C_WRITE((byte)0x40);
    Wire.endTransmission();


}

DateTime RTC_BQ32000::now() {
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(BQ32000_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;

  return DateTime (y, m, d, hh, mm, ss);
}

BQ32000SqwPinMode RTC_BQ32000::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE(BQ32000_CONTROL);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)BQ32000_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<BQ32000SqwPinMode>(mode);
}

void RTC_BQ32000::writeSqwPinMode(BQ32000SqwPinMode mode) {
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE(BQ32000_CONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}

void RTC_BQ32000::readnvram(uint8_t* buf, uint8_t size, uint8_t address) {
  int addrByte = BQ32000_NVRAM + address;
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t) BQ32000_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = Wire._I2C_READ();
  }
}

void RTC_BQ32000::writenvram(uint8_t address, uint8_t* buf, uint8_t size) {
  int addrByte = BQ32000_NVRAM + address;
  Wire.beginTransmission(BQ32000_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    Wire._I2C_WRITE(buf[pos]);
  }
  Wire.endTransmission();
}

uint8_t RTC_BQ32000::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

void RTC_BQ32000::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}
