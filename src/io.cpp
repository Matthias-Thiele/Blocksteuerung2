#include "io.hpp"
#include <Wire.h>

Io::Io() {

}

/**
 * @brief Liest ein Byte vom IO Port ein
 * 
 * @param portNo 
 * @return uint8_t 
 */
uint8_t Io::pioRead(uint8_t portNo) {
  Wire.requestFrom((uint8_t)(pioBaseAddress + portNo), (uint8_t)1);
  uint8_t wert = Wire.read();
  return wert;
}

/**
 * @brief Schreibt ein Byte zum IO Port
 * 
 * @param portNo 
 * @param value 
 */
void Io::pioWrite(uint8_t portNo, uint8_t value) {
  //static uint8_t oldValue;
  //if (portNo == 1 && value != oldValue) {Serial.print("PIO write port "); Serial.print(portNo); Serial.print(", value "); Serial.println(value, HEX);
  //oldValue = value;}

  Wire.beginTransmission( pioBaseAddress + portNo );
  Wire.write(value | 7);
  Wire.endTransmission();  
}

/**
 * @brief Schreibt ein Byte in das EEPROM
 * 
 * @param dataAddress 
 * @param dataVal 
 */
void Io::eeWrite(uint16_t dataAddress, uint8_t dataVal)
{
  Wire.beginTransmission(eeBaseAddress);

  Wire.write(dataAddress >> 8);
  Wire.write(dataAddress & 0xff);
  Wire.write(dataVal);
  Wire.endTransmission();

  delay(5);
}

/**
 * @brief Liest ein Byte aus dem EEPROM aus
 * 
 * @param dataAddress 
 * @return uint8_t 
 */
uint8_t Io::eeRead(uint16_t dataAddress)
{
  byte readData = 0;
  Wire.beginTransmission(eeBaseAddress);
  Wire.write(dataAddress >> 8);
  Wire.write(dataAddress & 0xff);
  Wire.endTransmission();

  delay(5);
  Wire.requestFrom(eeBaseAddress, (uint8_t)1);
 
  if(Wire.available())
  {
    readData =  Wire.read();
  }

  return readData;
}
