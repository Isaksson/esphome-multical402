#pragma once

#include <Arduino.h>
#include <vector>
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace multical402 {

struct KamstrupRegister {
  unsigned short address;
  sensor::Sensor *sensor;
};

class Multical402 : public Component, public uart::UARTDevice {
 public:
  void set_uart(uart::UARTComponent *uart);
  void add_sensor(unsigned short address, sensor::Sensor *sensor);

  Multical402();

  void setup() override;
  void loop() override;

 private:
  void update_registers();
  float read_register(unsigned short kreg);
  void send(const byte* msg, int msgsize);
  std::vector<byte> receive();
  float decode(unsigned short kreg, const byte* msg);
  int crc_1021(const std::vector<byte>& inmsg);

  uart::UARTComponent *uart_;
  std::vector<KamstrupRegister> registers_;
  unsigned long last_update_; 
};

}  // namespace multical402
}  // namespace esphome