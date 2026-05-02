#include <Chrono.h>
#include <Streaming.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/wdt.h>


#define outputA 5 
#define outputB 6
#define button 7

#define motor 11
#define valve 10
#define spare1 8
#define spare2 9
#define rainSensor A3
#define trigPin 4
#define echoPin 3


int aState;
int aLastState;

int mode = 0;
int prevMode = 0;
int tankFull = 80;
int distance;

LiquidCrystal_I2C lcd(0x27, 16, 2);

Chrono myChrono;
Chrono ultrasonicChrono;
Chrono displayChrono;

int counter = 0;


void setup() {
  pinMode(outputA, INPUT);
  pinMode(outputB, INPUT);
  pinMode(rainSensor, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(motor, OUTPUT), digitalWrite(motor, HIGH);
  pinMode(valve, OUTPUT), digitalWrite(valve, HIGH);
  pinMode(spare1, OUTPUT), digitalWrite(spare1, HIGH);
  pinMode(spare2, OUTPUT), digitalWrite(spare2, HIGH);

  lcd.init();
  lcd.backlight();

  // lcd.setCursor(0, 0), lcd << F("    SOLAR AND   ");
  // lcd.setCursor(0, 1), lcd << F("  WATER TURBINE ");
  // delay(2000);
  // lcd.clear();
  Serial.begin(9600);
  Serial.setTimeout(100);
  Serial << "BEGIN!" << endl;

  aLastState = digitalRead(outputA);

  int tankSave = EEPROM.read(0);
  if (tankSave >= 10 && tankSave <= 100) {
    tankFull = tankSave;
  }

  int modeSave = EEPROM.read(10);
  if (modeSave >= 0 && modeSave <= 2) {
    mode = modeSave;
    prevMode = mode;
  }
  wdt_enable(WDTO_2S);
}

bool onButtonPressed() {
  return !digitalRead(button);
}

void loop() {
  // testingProccess();
  normalProccess();
  wdt_reset();
  // distanceReading();
}

void normalProccess() {
  aState = digitalRead(outputA);

  if (aState != aLastState) {
    if (digitalRead(outputB) != aState) {
      tankFull++;
    } else {
      tankFull--;
    }
    displayChrono.restart();
    if (mode != 3) {
     
      mode = 3;
      lcd.clear();
       lcd.setCursor(0, 0), lcd << F(" DISPENSE SETUP ");
    }
    tankFull = constrain(tankFull, 10, 100);
    lcd.setCursor(0, 1), lcd << F("   ") << tankFull << F("% ");
    aLastState = aState;
  }



  switch (mode) {
    case 0:
      lcd.setCursor(0, 0), lcd << F("   SOLAR PANEL  ");
      lcd.setCursor(0, 1), lcd << F("                ");
      onSolar();
      if (!rainDetect()) {
        myChrono.restart();
      }
      if (myChrono.hasPassed(5000)) {
        lcd.clear();
        mode = 1;
        prevMode = mode;
        EEPROM.write(10, mode);
      }
      break;
    case 1:
      lcd.setCursor(0, 0), lcd << F(" WATER TURBINE  ");
      lcd.setCursor(0, 1), lcd << F("Water Lvl: ") << distance << F("%");
      onHydro();

      if (rainDetect()) {
        myChrono.restart();
      }
      if (myChrono.hasPassed(20000)) {
        lcd.clear();
        mode = 0;
        prevMode = mode;
        EEPROM.write(10, mode);
      }
      if (isTankFull()) {
        myChrono.restart();
        lcd.clear();
        mode = 2;
        prevMode = mode;
        EEPROM.write(10, mode);
      }
      break;

    case 2:
      lcd.setCursor(0, 0), lcd << F("  DISPENSING    ");
      lcd.setCursor(0, 1), lcd << F("Level: ") << distance << F("%");
      valveOpen();
      if (isTankEmpty() || myChrono.hasPassed(600000)) {
        valveClose();
        mode = 1;
        prevMode = mode;
        EEPROM.write(10, mode);
      }
      break;

    case 3:

      if (displayChrono.hasPassed(5000) || onButtonPressed()) {
        mode = prevMode;
        EEPROM.write(0, tankFull);
      }
      break;
  }
}

void testingProccess() {
  if (Serial.available()) {
    String reading = Serial.readString();
    Serial << reading << endl;
    reading.trim();
    if (reading == "1") {
      Serial << "On Solar" << endl;
      onSolar();
    } else if (reading == "2") {
      Serial << "On Hydro" << endl;
      onHydro();
    } else if (reading == "3") {
      Serial << "Valve Open" << endl;
      valveOpen();
    } else if (reading == "4") {
      Serial << "Valve Close" << endl;
      valveClose();
    }
  }
}

bool rainDetect() {
  return !digitalRead(rainSensor);
}
int distanceReading() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  int duration = pulseIn(echoPin, HIGH);
  delay(100);
  distance = map(duration, 5300, 1200, -10, 110);
  distance = constrain(distance, 0, 100);
  // Serial.println(distance);


  return distance;
}
bool isTankFull() {
  return distanceReading() >= tankFull;
}

bool isTankEmpty() {
  if (distanceReading() == 0) {
    if (ultrasonicChrono.hasPassed(30000)) {
      return true;
    }
  } else {
    ultrasonicChrono.restart();
  }
  return false;
}

void onSolar() {
  digitalWrite(motor, HIGH);
}

void onHydro() {
  digitalWrite(motor, LOW);
}

void valveOpen() {
  digitalWrite(valve, LOW);
}

void valveClose() {
  digitalWrite(valve, HIGH);
}