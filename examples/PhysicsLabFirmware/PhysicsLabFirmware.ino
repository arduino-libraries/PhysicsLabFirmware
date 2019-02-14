/*
  This file is part of the PhysicsLabFirmware library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <ArduinoBLE.h>
#include <MKRIMU.h>

#include "INA226.h"

#define SCIENCE_KIT_UUID(val) ("555a0001-" val "-467a-9538-01f0652c74e8")

BLEService                     service                    (SCIENCE_KIT_UUID("0000"));
BLEUnsignedIntCharacteristic   versionCharacteristic      (SCIENCE_KIT_UUID("0001"), BLERead);
BLEByteCharacteristic          ledCharacteristic          (SCIENCE_KIT_UUID("1001"), BLERead | BLEWrite);
BLEUnsignedShortCharacteristic input1Characteristic       (SCIENCE_KIT_UUID("2001"), BLENotify);
BLEUnsignedShortCharacteristic input2Characteristic       (SCIENCE_KIT_UUID("2002"), BLENotify);
BLEUnsignedShortCharacteristic input3Characteristic       (SCIENCE_KIT_UUID("2003"), BLENotify);
BLEByteCharacteristic          ouput1Characteristic       (SCIENCE_KIT_UUID("3001"), BLERead | BLEWrite);
BLEByteCharacteristic          ouput2Characteristic       (SCIENCE_KIT_UUID("3002"), BLERead | BLEWrite);
BLEFloatCharacteristic         voltageCharacteristic      (SCIENCE_KIT_UUID("4001"), BLENotify);
BLEFloatCharacteristic         currentCharacteristic      (SCIENCE_KIT_UUID("4002"), BLENotify);
BLEFloatCharacteristic         resistanceCharacteristic   (SCIENCE_KIT_UUID("4003"), BLENotify);
BLECharacteristic              accelerationCharacteristic (SCIENCE_KIT_UUID("5001"), BLENotify, 3 * sizeof(float));
BLECharacteristic              gyroscopeCharacteristic    (SCIENCE_KIT_UUID("5002"), BLENotify, 3 * sizeof(float));
BLECharacteristic              magneticFieldCharacteristic(SCIENCE_KIT_UUID("5003"), BLENotify, 3 * sizeof(float));

const int LED_PIN            =  0;
const int INPUT1_PIN         = A3;
const int INPUT2_PIN         = A1;
const int INPUT3_PIN         = A0;
const int OUTPUT1_PIN        =  5;
const int OUTPUT2_PIN        =  1;
const int RESISTANCE_PIN     = A2;
const int RESISTANCE_AUX_PIN = 13;

String name;
unsigned long lastNotify = 0;

#define RESISTOR_AUX_LOW  47000.0
#define RESISTOR_AUX_HIGH 979.16 // 47k in parallel with 1k = 979.16 Ohm

//#define DEBUG //uncomment to debug the code :)

void setup() {
  Serial.begin(9600);
#ifdef DEBUG
  while (!Serial);
  Serial.println("Started");
#endif

  pinMode(LED_PIN, OUTPUT);
  pinMode(INPUT1_PIN, INPUT);
  pinMode(INPUT2_PIN, INPUT);
  pinMode(INPUT3_PIN, INPUT);
  pinMode(OUTPUT1_PIN, OUTPUT);
  pinMode(OUTPUT2_PIN, OUTPUT);
  pinMode(RESISTANCE_AUX_PIN, OUTPUT);

  digitalWrite(RESISTANCE_AUX_PIN, LOW);

  if (!INA226.begin(0x45)) {
    Serial.println("Failled to initialized INA226!");

    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failled to initialized IMU!");

    while (1);
  }

  if (!BLE.begin()) {
    Serial.println("Failled to initialized BLE!");

    while (1);
  }

  String address = BLE.address();

  address.toUpperCase();

  name = "MKRSci";
  name += address[address.length() - 5];
  name += address[address.length() - 4];
  name += address[address.length() - 2];
  name += address[address.length() - 1];

  BLE.setLocalName(name.c_str());
  BLE.setDeviceName(name.c_str());
  BLE.setAdvertisedService(service);

  service.addCharacteristic(versionCharacteristic);
  service.addCharacteristic(ledCharacteristic);
  service.addCharacteristic(input1Characteristic);
  service.addCharacteristic(input2Characteristic);
  service.addCharacteristic(input3Characteristic);
  service.addCharacteristic(ouput1Characteristic);
  service.addCharacteristic(ouput2Characteristic);
  service.addCharacteristic(voltageCharacteristic);
  service.addCharacteristic(currentCharacteristic);
  service.addCharacteristic(resistanceCharacteristic);
  service.addCharacteristic(accelerationCharacteristic);
  service.addCharacteristic(gyroscopeCharacteristic);
  service.addCharacteristic(magneticFieldCharacteristic);

  BLE.addService(service);

  BLE.advertise();
}

void loop() {
  lastNotify = 0;

  while (BLE.connected()) {
    if (ledCharacteristic.written()) {
      analogWrite(LED_PIN, ledCharacteristic.value());
    }

    if (ouput1Characteristic.written()) {
      analogWrite(OUTPUT1_PIN, ouput1Characteristic.value());
    }

    if (ouput2Characteristic.written()) {
      analogWrite(OUTPUT2_PIN, ouput2Characteristic.value());
    }

    unsigned long now = millis();

    if (abs((long)now - (long)lastNotify) >= 100) {
      lastNotify = now;

      // every 100ms update subscribed characteristics
      updateSubscribedCharacteristics();
    }

    updateSubscribedIMUCharacteristics();
  }
}

void updateSubscribedCharacteristics() {
  if (input1Characteristic.subscribed()) {
    input1Characteristic.writeValue(analogReadAverage(INPUT1_PIN, 30));
  }

  if (input2Characteristic.subscribed()) {
    input2Characteristic.writeValue(analogReadAverage(INPUT2_PIN, 30));
  }

  if (input3Characteristic.subscribed()) {
    input3Characteristic.writeValue(analogReadAverage(INPUT3_PIN, 30));
  }

  if (voltageCharacteristic.subscribed()) {
    float voltage = INA226.readBusVoltage();

    voltageCharacteristic.writeValue(voltage);
  }

  if (currentCharacteristic.subscribed()) {
    float current = INA226.readCurrent();

    currentCharacteristic.writeValue(current);
  }

  if (resistanceCharacteristic.subscribed()) {
    float Vout = 0;
    float resistanceAuxLow = INFINITY;
    float resistanceAuxHigh = INFINITY;
    float resistanceAvg = INFINITY; //open circuit as default

    digitalWrite(RESISTANCE_AUX_PIN, LOW);
    Vout = getVoutAverage();
    if (Vout >= 0.1) {
      resistanceAuxLow = RESISTOR_AUX_LOW * ((3.3 / Vout) - 1);
    }

    digitalWrite(RESISTANCE_AUX_PIN, HIGH);
    Vout = getVoutAverage();
    if (Vout >= 0.1) {
      resistanceAuxHigh = RESISTOR_AUX_HIGH * ((3.3 / Vout) - 1);
    }

#ifdef DEBUG
    Serial.print("Resistance (HIGH): ");
    Serial.print(resistanceAuxHigh);
    Serial.println(" Ohm");

    Serial.print("Resistance (LOW): ");
    Serial.print(resistanceAuxLow);
    Serial.println(" Ohm");
#endif

    if ((resistanceAuxHigh != INFINITY) && (resistanceAuxLow != INFINITY)) {
      resistanceAvg = (resistanceAuxHigh + resistanceAuxLow) / 2;
    } else if ((resistanceAuxHigh != INFINITY) && (resistanceAuxLow == INFINITY)) {
      resistanceAvg = resistanceAuxHigh;
    } else if ((resistanceAuxHigh == INFINITY) && (resistanceAuxLow != INFINITY)) {
      resistanceAvg = resistanceAuxLow;
    }
    
#ifdef DEBUG
    Serial.print("Resistance (AVG): ");
    Serial.print(resistanceAvg);
    Serial.println(" Ohm");
#endif
    resistanceCharacteristic.writeValue(resistanceAvg);
  }
}

float getVoutAverage() {
  float Vout = 0;
  for (int i = 0; i < 30; i++) {
    Vout += (analogRead(RESISTANCE_PIN) * 3.30) / 1023.0;
  }
  Vout /= 30;

#ifdef DEBUG
  Serial.print("Vout: ");
  Serial.print(Vout);
  Serial.println("V");
#endif
  return Vout;
}

int analogReadAverage(int pin, int numberOfSamples) {
  int averageValue = 0;
  for (int i = 0; i < numberOfSamples; i++) {
    averageValue += analogRead(pin);
  }

  return (averageValue / numberOfSamples);
}

void updateSubscribedIMUCharacteristics() {
  if (accelerationCharacteristic.subscribed()) {
    float acceleration[3];

    if (IMU.accelerationAvailable() && IMU.readAcceleration(acceleration[0], acceleration[1], acceleration[2])) {
      accelerationCharacteristic.writeValue((byte*)acceleration, sizeof(acceleration));
    }
  }

  if (gyroscopeCharacteristic.subscribed()) {
    float gyroscope[3];

    if (IMU.gyroscopeAvailable() && IMU.readGyroscope(gyroscope[0], gyroscope[1], gyroscope[2])) {
      gyroscopeCharacteristic.writeValue((byte*)gyroscope, sizeof(gyroscope));
    }
  }

  if (magneticFieldCharacteristic.subscribed()) {
    float magneticField[3];

    if (IMU.magneticFieldAvailable() && IMU.readMagneticField(magneticField[0], magneticField[1], magneticField[2])) {
      magneticFieldCharacteristic.writeValue((byte*)magneticField, sizeof(magneticField));
    }
  }
}
