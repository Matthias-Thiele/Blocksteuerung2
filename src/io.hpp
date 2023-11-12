#ifndef IO_HPP
#define IO_HPP

#include <Arduino.h>


class Io {
  public:
    Io();

    // liest ein Byte vom Parallelport ein
    uint8_t pioRead(uint8_t portNo);

    // schreibt ein Byte zum Parallelport
    void pioWrite(uint8_t portNo, uint8_t value);

    // liest ein Byte vom EEPROM Speicher ein
    uint8_t eeRead(uint16_t dataAddress);

    // schreibt ein Byte zum EEPROM Speicher
    void eeWrite(uint16_t dataAddress, uint8_t dataVal);

  private:
    const uint8_t pioBaseAddress = 0x20;
    const uint8_t eeBaseAddress = 0x50;
};

#endif