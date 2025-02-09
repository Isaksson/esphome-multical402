#include "multical402.h"
#include "esphome/core/log.h"

namespace esphome {
namespace multical402 {

static const char *TAG = "multical402";

Multical402::Multical402() : last_update_(0) {}

void Multical402::set_uart(uart::UARTComponent *uart) {
  this->uart_ = uart;
}

void Multical402::add_sensor(unsigned short address, sensor::Sensor *sensor) {
  registers_.push_back({address, sensor});
}

void Multical402::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Multical402...");
  if (this->uart_ == nullptr) {
    ESP_LOGE(TAG, "UART component not set!");
    return;
  }
  this->uart_->set_baud_rate(1200);
}

void Multical402::loop() {
  unsigned long current_time = millis();
  if (current_time - last_update_ >= 5000) {  // 5 seconds
    ESP_LOGD(TAG, "Updating Multical402 registers...");
    update_registers();
    last_update_ = current_time;
  }
}



void Multical402::update_registers() {
  ESP_LOGD(TAG, "Updating registers...");
  for (auto const& reg : registers_) {
    float value = read_register(reg.address);
    if (!isnan(value)) {
      reg.sensor->publish_state(value);
    }
  }
}

float Multical402::read_register(unsigned short kreg) {
  ESP_LOGD(TAG, "Reading register %04x", kreg);
  byte sendmsg[] = {0x3f, 0x10, 0x01, static_cast<byte>(kreg >> 8), static_cast<byte>(kreg & 0xff)};
  send(sendmsg, sizeof(sendmsg));
  std::vector<byte> recvmsg = receive();

  if (recvmsg.size() > 0) {
    return decode(kreg, recvmsg.data());
  } else {
    return NAN; // Return NaN to indicate an error
  }
}

void Multical402::send(const byte* msg, int msgsize) {
  ESP_LOGD(TAG, "Sending message...");
  std::vector<byte> newmsg(msgsize + 2);
  for (int i = 0; i < msgsize; i++) {
    newmsg[i] = msg[i];
  }
  newmsg[msgsize] = 0x00;
  newmsg[msgsize + 1] = 0x00;
  int c = crc_1021(newmsg);
  newmsg[msgsize] = (c >> 8);
  newmsg[msgsize + 1] = c & 0xff;

  std::vector<byte> txmsg = {0x80};
  for (byte b : newmsg) {
    if (b == 0x06 || b == 0x0d || b == 0x1b || b == 0x40 || b == 0x80) {
      txmsg.push_back(0x1b);
      txmsg.push_back(b ^ 0xff);
    } else {
      txmsg.push_back(b);
    }
  }
  txmsg.push_back(0x0d);  // EOF
  ESP_LOGD(TAG, "Sending %d bytes", txmsg.size());
  for (size_t i = 0; i < txmsg.size(); ++i) {
    ESP_LOGD(TAG, "Byte %d: 0x%02X", i, txmsg[i]);
  }
  this->uart_->write_array(txmsg.data(), txmsg.size());
}

std::vector<byte> Multical402::receive() {
  std::vector<byte> rxdata;
  unsigned long starttime = millis();

  uart_->flush();

  byte r = 0;
  while (r != 0x0d) {
    if (millis() - starttime > 300) {
      ESP_LOGW(TAG, "Timed out listening for data");
      return {};
    }

    if (uart_->available()) {
      uint8_t byte;
      if (uart_->read_byte(&byte)) {
        ESP_LOGD(TAG, "Received byte: 0x%02X", byte);
        r = byte;
        if (r != 0xff && r != 0x40) {
          rxdata.push_back(r);
        }
      }
    }
  }

  std::vector<byte> recvmsg;
  for (size_t i = 0; i < rxdata.size() - 1; ++i) {
    if (rxdata[i] == 0x1b) {
      byte v = rxdata[i + 1] ^ 0xff;
      recvmsg.push_back(v);
      i++;
    } else {
      recvmsg.push_back(rxdata[i]);
    }
  }

  if (crc_1021(recvmsg)) {
    ESP_LOGW(TAG, "CRC error");
    return {};
  }
  
  ESP_LOGD(TAG, "Received %d bytes", recvmsg.size());
  return recvmsg;
}

float Multical402::decode(unsigned short kreg, const byte* msg) {
  if (msg[0] != 0x3f || msg[1] != 0x10) {
    return NAN;
  }
  if (msg[2] != (kreg >> 8) || msg[3] != (kreg & 0xff)) {
    return NAN;
  }

  long x = 0;
  for (int i = 0; i < msg[5]; i++) {
    x <<= 8;
    x |= msg[i + 7];
  }

  int i = msg[6] & 0x3f;
  if (msg[6] & 0x40) {
    i = -i;
  }
  float ifl = pow(10, i);
  if (msg[6] & 0x80) {
    ifl = -ifl;
  }

  return (float)(x * ifl);
}

int Multical402::crc_1021(const std::vector<byte>& inmsg) {
  long creg = 0x0000;
  for (unsigned int i = 0; i < inmsg.size(); i++) {
    int mask = 0x80;
    while (mask > 0) {
      creg <<= 1;
      if (inmsg[i] & mask) {
        creg |= 1;
      }
      mask >>= 1;
      if (creg & 0x10000) {
        creg &= 0xffff;
        creg ^= 0x1021;
      }
    }
  }
  return creg;
}

}  // namespace multical402
}  // namespace esphome