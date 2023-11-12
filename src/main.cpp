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

void loop() {
  long time = millis();
  block1->tick(time);
  block2->tick(time);
}

