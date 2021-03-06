#include "Wire.h"
#include <Adafruit_INA219.h>

struct cathodeData {
  byte curr;
  byte comma;
};

Adafruit_INA219 ina219;

const byte A = 2, B = 3, C = 4, D = 5, commaL = 13, commaR = 7, modePin = 6, Vdd = 8,
           N[] = {12, 11, 10, 9}, clean[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, cleanSymbolic[] = {1, 2, 3, 4, 6, 0, 9}, measureDelay = 100;

unsigned long lastTest, lastCPP, timePressed, ohms, lastMeasurement;
float currV, currI;
int mV, mA, mW, mS;
cathodeData curr1, curr2, curr3, curr4;
byte mode = 0;
boolean holding, currVdd, flip;

void setup() {
  Wire.begin();
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);
  pinMode(commaL, OUTPUT);
  pinMode(commaR, OUTPUT);
  pinMode(Vdd, OUTPUT);
  for (int a = 0; a < 4; a++)
    pinMode(N[a], OUTPUT);
  pinMode(modePin, INPUT_PULLUP);
  ina219.begin();
  test();
}
void loop() {
  multPlex();
  if (millis() - lastTest >= (mode < 2 ? 25 : 25)) {
    //blank();
    test();
  }
  if (millis() - timePressed >= 50)
    checkButton();
  if (millis() - lastCPP >= 180000)
    cathodePoisoningPrevention();
}

void binOut(cathodeData data, byte stage) {
  digitalWrite(N[stage - 1], LOW);
  blank();
  digitalWrite(A, bitRead(data.curr, 0));
  digitalWrite(B, bitRead(data.curr, 1));
  digitalWrite(C, bitRead(data.curr, 2));
  digitalWrite(D, bitRead(data.curr, 3));
  digitalWrite(N[stage], HIGH);
  switch (data.comma) {
    case 1: digitalWrite(commaL, HIGH); break;
    case 2: digitalWrite(commaR, HIGH); break;
  }
  delayMicroseconds(700);
}

void multPlex() {
  binOut(curr1, 0);
  binOut(curr2, 1);
  binOut(curr3, 2);
  binOut(curr4, 3);
  digitalWrite(N[3], LOW);
}

void refresh(byte number, byte comma, byte stage) {
  switch (stage) {
    case 1: curr1.curr = number; curr1.comma = comma; break;
    case 2: curr2.curr = number; curr2.comma = comma; break;
    case 3: curr3.curr = number; curr3.comma = comma; break;
    case 4: curr4.curr = number; curr4.comma = comma; break;
  }
}

void test() {
  lastTest = millis();
  if (mode == 0) {
    deliverSourceVoltage(false);
    if (millis() - lastMeasurement >= measureDelay) {
      lastMeasurement = millis();
      mV = 1000 * readVoltmeter();
    }
    else
      readVoltmeter();
    if (mV < 1000) {
      refresh(mV / 100, 1, 1);
      refresh(mV / 10 % 10, 0, 2);
      refresh(mV % 10, 0, 3);
    }
    else if (mV >= 1000 && mV < 10000) {
      refresh(mV / 1000, 2, 1);
      refresh(mV / 100 % 10, 0, 2);
      refresh(mV / 10 % 10, 0 , 3);
    }
    else {
      refresh(mV / 10000, 0, 1);
      refresh(mV / 1000 % 10, 2, 2);
      refresh(mV / 100 % 10, 0, 3);
    }
    refresh(-1, 2, 4);
  }
  else if (mode == 1) {
    deliverSourceVoltage(false);
    if (millis() - lastMeasurement >= measureDelay) {
      lastMeasurement = millis();
      mA = abs(floor(readAmmeter() - 0.25));
    }
    else
      abs(floor(readAmmeter() - 0.25));
    if (mA < 1000) {
      refresh(mA / 100, 1, 1);
      refresh(mA / 10 % 10, 0, 2);
      refresh(mA % 10, 0, 3);
    }
    else {
      refresh(mA / 1000, 2, 1);
      refresh(mA / 100 % 10, 0, 2);
      refresh(mA / 10 % 10, 0, 3);
    }
    refresh(2, 0, 4);
  }
  else if (mode == 2) {
    deliverSourceVoltage(false);
    if (millis() - lastMeasurement >= measureDelay) {
      lastMeasurement = millis();
      mW = readPower();
    }
    else
      readPower();
    if (mW < 1000) {
      refresh(mW / 100, 1, 1);
      refresh(mW / 10 % 10, 0, 2);
      refresh(mW % 10, 0, 3);
    }
    else if (mW >= 1000 && mW < 10000) {
      refresh(mW / 1000, 2, 1);
      refresh(mW / 100 % 10, 0, 2);
      refresh(mW / 10 % 10, 0, 3);
    }
    else {
      refresh(mW / 10000, 0, 1);
      refresh(mW / 1000 % 10, 2, 2);
      refresh(mW / 100 % 10, 0, 3);
    }
    refresh(2, 2, 4);
  }
  else if (mode == 3) {
    deliverSourceVoltage(true);
    if (millis() - lastMeasurement >= measureDelay) {
      lastMeasurement = millis();
      ohms = readOhmmeter() * 1000000;
    }
    else
      readOhmmeter() * 1000000;
    if (ohms < 1000) {
      refresh(ohms / 100, 1, 1);
      refresh(ohms / 10 % 10, 0, 2);
      refresh(ohms % 10, 0, 3);
    }
    else if (ohms >= 1000 && ohms < 10000) {
      refresh(ohms / 1000, 2, 1);
      refresh(ohms / 100 % 10, 0, 2);
      refresh(ohms / 10 % 10, 0, 3);
    }
    else {
      refresh(ohms / 10000, 0, 1);
      refresh(ohms / 1000 % 10, 2, 2);
      refresh(ohms / 100 % 10, 0, 3);
    }
    refresh(3, 0, 4);
  } else {
    deliverSourceVoltage(true);
    if (millis() - lastMeasurement >= measureDelay) {
      lastMeasurement = millis();
      mS = 1 / readOhmmeter();
    }
    else
      1 / readOhmmeter();
    if (mS < 1000) {
      refresh(mS / 100, 1, 1);
      refresh(mS / 10 % 10, 0, 2);
      refresh(mS % 10, 0, 3);
    }
    else if (mS >= 1000 && mS < 10000) {
      refresh(mS / 1000, 2, 1);
      refresh(mS / 100 % 10, 0, 2);
      refresh(mS / 10 % 10, 0, 3);
    }
    else {
      refresh(mS / 10000, 0, 1);
      refresh(mS / 1000 % 10, 2, 2);
      refresh(mS / 100 % 10, 0, 3);
    }
    refresh(6, 0, 4);
  }
}

void deliverSourceVoltage(boolean on) {
  if (currVdd != on) {
    currVdd = on;
    digitalWrite(Vdd, on);
  }
}

float readVoltmeter() {
  float shuntVoltage = 0;
  float busVoltage = 0;
  float loadVoltage = 0;
  shuntVoltage = ina219.getShuntVoltage_mV();
  busVoltage = ina219.getBusVoltage_V();
  loadVoltage = busVoltage + (shuntVoltage / 1000);
  return loadVoltage;
}

float readAmmeter() {
  float mA = 0;
  mA = ina219.getCurrent_mA();
  return mA;
}

float readPower() {
  float mW = 0;
  mW = ina219.getPower_mW();
  return mW;
}

float readOhmmeter() {
  float ohms = 0;
  if (flip)
    currV = readVoltmeter();
  else
    currI = readAmmeter();
  flip = !flip;
  //ohms = (readVoltmeter() * 1000000.0) / readAmmeter();
  // ohms = (ina219.getPower_mW() ) / (readAmmeter() * readAmmeter() );
  return currV / currI;
}

void cathodePoisoningPrevention() {
  lastCPP = millis();
  for (byte a = 0; a < 4; a++)
    digitalWrite(N[a], LOW);
  digitalWrite(commaL, HIGH);
  digitalWrite(commaR, HIGH);
  for (byte c = 0; c < 3; c++) {
    digitalWrite(N[c], HIGH);
    for (byte d = 0; d < 2; d++)
      for (byte n = 0; n <= 9; n++) {
        digitalWrite(A, bitRead(clean[n], 0));
        digitalWrite(B, bitRead(clean[n], 1));
        digitalWrite(C, bitRead(clean[n], 2));
        digitalWrite(D, bitRead(clean[n], 3));
        delay(300);
      }
    digitalWrite(N[c], LOW);
  }
  digitalWrite(commaL, LOW);
  digitalWrite(commaR, LOW);
  digitalWrite(N[3], HIGH);
  for (byte d = 0; d < 2; d++) {
    for (byte n = 0; n < 7; n++) {
      digitalWrite(A, bitRead(cleanSymbolic[n], 0));
      digitalWrite(B, bitRead(cleanSymbolic[n], 1));
      digitalWrite(C, bitRead(cleanSymbolic[n], 2));
      digitalWrite(D, bitRead(cleanSymbolic[n], 3));
      delay(300);
    }
    blank();
    digitalWrite(commaR, HIGH);
    delay(300);
    digitalWrite(commaR, LOW);
  }
  digitalWrite(N[3], LOW);
  blank();
  for (byte a = 0; a < 4; a++)
    digitalWrite(N[a], LOW);
}

void blank() {
  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);
  digitalWrite(C, HIGH);
  digitalWrite(D, HIGH);
  digitalWrite(commaL, LOW);
  digitalWrite(commaR, LOW);
}

void checkButton() {
  if (!digitalRead(modePin) && !holding) {
    timePressed = millis();
    holding = true;
    mode = mode == 4 ? 0 : mode + 1;
  }
  else if (digitalRead(modePin))
    holding = false;
}
