#include <ArduinoBLE.h>
#include <MKRIMU.h>

#define SCIENCE_KIT_UUID(val) ("555a0001-" val "-467a-9538-01f0652c74e8")

BLEService                     service                    (SCIENCE_KIT_UUID("0000"));
BLEUnsignedIntCharacteristic   versionCharacteristic      (SCIENCE_KIT_UUID("0001"), BLERead);
BLEByteCharacteristic          ledCharacteristic          (SCIENCE_KIT_UUID("1001"), BLERead | BLEWrite);
BLEUnsignedShortCharacteristic input1Characteristic       (SCIENCE_KIT_UUID("2001"), BLENotify);
BLEUnsignedShortCharacteristic input2Characteristic       (SCIENCE_KIT_UUID("2002"), BLENotify);
BLEUnsignedShortCharacteristic input3Characteristic       (SCIENCE_KIT_UUID("2002"), BLENotify);
BLEByteCharacteristic          ouput1Characteristic       (SCIENCE_KIT_UUID("3001"), BLERead | BLEWrite);
BLEByteCharacteristic          ouput2Characteristic       (SCIENCE_KIT_UUID("3001"), BLERead | BLEWrite);
BLEFloatCharacteristic         voltageCharacteristic      (SCIENCE_KIT_UUID("4001"), BLENotify);
BLEFloatCharacteristic         currentCharacteristic      (SCIENCE_KIT_UUID("4002"), BLENotify);
BLEFloatCharacteristic         resistanceCharacteristic   (SCIENCE_KIT_UUID("4003"), BLENotify);
BLECharacteristic              accelerationCharacteristic (SCIENCE_KIT_UUID("5001"), BLENotify, 3 * sizeof(float));
BLECharacteristic              gyroscopeCharacteristic    (SCIENCE_KIT_UUID("5002"), BLENotify, 3 * sizeof(float));
BLECharacteristic              magneticFieldCharacteristic(SCIENCE_KIT_UUID("5003"), BLENotify, 3 * sizeof(float));

const int LED_PIN     =  0;
const int INPUT1_PIN  = A0;
const int INPUT2_PIN  = A1;
const int INPUT3_PIN  = A3;
const int OUTPUT1_PIN =  1;
const int OUTPUT2_PIN =  5;

String name;
unsigned long lastNotify = 0;

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(INPUT1_PIN, INPUT);
  pinMode(INPUT2_PIN, INPUT);
  pinMode(INPUT3_PIN, INPUT);
  pinMode(OUTPUT1_PIN, OUTPUT);
  pinMode(OUTPUT2_PIN, OUTPUT);

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

    if (abs(now - lastNotify) >= 100) {
      // every 100ms update subscribed characteristics
      updateSubscribedCharacteristics();
    }

    updateSubscribedIMUCharacteristics();
  }
}

void updateSubscribedCharacteristics() {
  if (input1Characteristic.subscribed()) {
    input1Characteristic.writeValue(analogRead(INPUT1_PIN));
  }

  if (input2Characteristic.subscribed()) {
    input2Characteristic.writeValue(analogRead(INPUT2_PIN));
  }

  if (input3Characteristic.subscribed()) {
    input3Characteristic.writeValue(analogRead(INPUT3_PIN));
  }

  if (voltageCharacteristic.subscribed()) {
    // TODO: read from sensor
    voltageCharacteristic.writeValue(voltageCharacteristic.value() + 1);
  }

  if (currentCharacteristic.subscribed()) {
    // TODO: read from sensor
    currentCharacteristic.writeValue(currentCharacteristic.value() + 1);
  }

  if (resistanceCharacteristic.subscribed()) {
    // TODO: read from sensor
    resistanceCharacteristic.writeValue(resistanceCharacteristic.value() + 1);
  }
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
