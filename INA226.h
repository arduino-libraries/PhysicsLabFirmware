#ifndef _INA226_H_
#define _INA226_H_

#include <Arduino.h>
#include <Wire.h>

class INA226Class {
public:
  INA226Class(TwoWire& wire);
  virtual ~INA226Class();

  int begin(uint8_t address, float shuntResistance = 0.0002);
  void end();

  float readCurrent();
  float readBusVoltage();

private:
  long readRegister(uint8_t address);
  int writeRegister(uint8_t address, uint16_t value);

private:
  TwoWire* _wire;
  uint8_t _i2cAddress;
};

extern INA226Class INA226;

#endif
