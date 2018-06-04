/* Project name: Arduino - Air Quality Monitor
   Project URI: https://www.studiopieters.nl/arduino-air-quality-monitor
   Description: Arduino - Air Quality Monitor
   Version: 3.1.0
   License: MIT
*/

#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

#define DHTTYPE DHT22

// select the pins used on the LCD panel
LiquidCrystal_I2C lcd(0x27, 16, 2);

DHT dht(A1, DHTTYPE);
const int chipSelect = 4;
int measurePin = A0;
int ledPower = 7;
int thermistorPin = A1;

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

float avgDust = 0;
int avgDustCnt = 0;

byte termometru[8] = //icon for termometer
{
  4, 10, 10, 10, 14, 31, 31, 14
};

byte picatura[8] = //icon for water droplet
{
  4, 4, 14, 14, 31, 31, 31, 14
};

byte threeIco[8] = //raise to power 3 character
{
  24, 4, 24, 4, 24, 0, 0, 0
};

byte miliIco[8] = //micrograms character
{
  0, 17, 17, 17, 19, 29, 16, 16
};


boolean backlit = true;
RTC_DS3231 rtc;

void setup()
{
  lcd.begin ();
  lcd.createChar(1, termometru);
  lcd.createChar(2, picatura);
  lcd.createChar(3, threeIco);
  lcd.createChar(4, miliIco);
  lcd.setBacklight(HIGH);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect.
  }
  Wire.begin();
  rtc.begin();


  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


  
  pinMode(A3, OUTPUT);
  lcd.home();
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    lcd.print("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  lcd.print("Card Initialized");
}

void loop()
{
  DateTime now = rtc.now();
  digitalWrite(ledPower, LOW); // power on the dust sensor LED
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin); // read the dust value

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH); // turn the dust sensor LED off
  delayMicroseconds(sleepTime);

  // 0 - 3.3V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (3.3 / 1024);

  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = (0.172 * calcVoltage - 0.0999) * 1000;

  avgDust += dustDensity;
  avgDustCnt++;
  if (avgDustCnt == 1000)
  {
    // make a string for assembling the data to log:
    String dataString = "";
    dataString += "Dst" + String(avgDust / avgDustCnt) + ",";
    dataString += "Tmp" + String(dht.readTemperature()) + ",";
    dataString += "Hum" + String(dht.readHumidity()) + ",";
    dataString += String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " ";
    dataString += String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    lcd.clear();
    lcd.home();
    lcd.write(1);
    lcd.print(dht.readTemperature(), 0);
    lcd.print((char)223);
    lcd.print("C ");
    lcd.write(2);
    lcd.print(dht.readHumidity(), 0);
    lcd.print("%");
    lcd.print(" ");
    if (now.hour() < 10)
    {
      lcd.print('0');
    }
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.setCursor(0, 1);
    lcd.print("Dust: ");
    lcd.print(avgDust / avgDustCnt, 1);
    lcd.print(" ");
    lcd.write(4);
    lcd.print("g/m");
    lcd.write(3);
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    } else {
      Serial.println("error opening datalog.txt");
    }
    avgDust = 0;
    avgDustCnt = 0;
  }
}
