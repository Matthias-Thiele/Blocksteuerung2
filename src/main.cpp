#include <Arduino.h>
#include <Wire.h>
#include "io.hpp"
#include "block.hpp"
#include "input.hpp"

#define EEPROM_I2C_ADDRESS 0x50

Block *block1, *block2;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Started.");

  Io *io = new Io();

  block1 = new Block(io, 0, true);
  block2 = new Block(io, 1, false);

  block1->setTickerBlock(block2);
  block1->activateBlock();
}

/**
 * @brief Sammelt die Zustände aller Blöcke und gibt sie als ein Zeichen zurück
 * 
 * Bit 0: Block 1 ist rot wenn gesetzt, sonst weiß
 * Bit 1: Block 2 ist rot wenn gesetzt, sonst weiß
 * 
 * @return uint8_t 
 */
uint8_t collectStatus() {
  uint8_t result = 0x40;
  if (block1->getState()) {
    result |= 1;
  }
  if (block2->getState()) {
    result |= 2;
  }

  return result;
}

void loop() {
  long time = millis();
  block1->tick(time);
  block2->tick(time);

  if (Serial.available()) {
    uint8_t c = Serial.read();
    switch (c) {
      case 't':
        block2->startTicker();
        break;

      case 'i':
        block1->doBlock();
        break;

      case 'o':
        block2->doBlock();
        break;

      case 's':
        Serial.write(collectStatus());
        break;
    }
  }
}

