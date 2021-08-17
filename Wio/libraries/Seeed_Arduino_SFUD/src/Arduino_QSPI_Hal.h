/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 hathach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef ARDUINO_QSPI_HAL_H_
#define ARDUINO_QSPI_HAL_H_

#include <stdbool.h>
#include <stdint.h>
#ifdef __SAMD51__
#include <WVariant.h>
#endif 
enum {
  SFLASH_CMD_READ = 0x03,      // Single Read
  SFLASH_CMD_QUAD_READ = 0x6B, // 1 line address, 4 line data

  SFLASH_CMD_READ_JEDEC_ID = 0x9f,

  SFLASH_CMD_PAGE_PROGRAM = 0x02,
  SFLASH_CMD_QUAD_PAGE_PROGRAM = 0x32, // 1 line address, 4 line data

  SFLASH_CMD_READ_STATUS = 0x05,
  SFLASH_CMD_READ_STATUS2 = 0x35,

  SFLASH_CMD_WRITE_STATUS = 0x01,
  SFLASH_CMD_WRITE_STATUS2 = 0x31,

  SFLASH_CMD_ENABLE_RESET = 0x66,
  SFLASH_CMD_RESET = 0x99,

  SFLASH_CMD_WRITE_ENABLE = 0x06,
  SFLASH_CMD_WRITE_DISABLE = 0x04,

  SFLASH_CMD_ERASE_SECTOR = 0x20,
  SFLASH_CMD_ERASE_BLOCK = 0xD8,
  SFLASH_CMD_ERASE_CHIP = 0xC7,
};
class Adafruit_FlashTransport_QSPI{
private:
  int8_t _sck, _cs;
  int8_t _io0, _io1, _io2, _io3;

public:
  Adafruit_FlashTransport_QSPI(int8_t pinSCK, int8_t pinCS, int8_t pinIO0,
                               int8_t pinIO1, int8_t pinIO2, int8_t pinIO3);
  Adafruit_FlashTransport_QSPI(void);

  virtual void begin(void);

  virtual bool supportQuadMode(void) { return true; }

  virtual void setClockSpeed(uint32_t clock_hz);

  virtual bool runCommand(uint8_t command);
  virtual bool readCommand(uint8_t command, uint8_t *response, uint32_t len);
  virtual bool writeCommand(uint8_t command, uint8_t const *data, uint32_t len);

  virtual bool eraseCommand(uint8_t command, uint32_t address);
  virtual bool readMemory(uint32_t addr, uint8_t *data, uint32_t len);
  virtual bool writeMemory(uint32_t addr, uint8_t const *data, uint32_t len);
  virtual bool readSFDP(uint8_t command, uint8_t *data, uint32_t data_len, uint8_t *response, uint32_t len);
};

#endif /* ARDUINO_QSPI_HAL_H_ */
