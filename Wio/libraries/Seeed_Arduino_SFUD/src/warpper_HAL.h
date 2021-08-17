#ifndef WARPPER_SPI_H
#define WARPPER_SPI_H
#include "sfud_cfg.h"
#include "Arduino.h"
#ifdef __cplusplus
extern "C" {
#endif
void Print(const char *str);
#ifdef SFUD_USING_QSPI
#define QSPIDEV QSPIdev
void QSPIBegin();
void QSPISetClockSpeed(uint32_t clock_hz);

bool QSPIReadMemory(uint32_t addr, uint8_t *data, uint32_t len);
bool QSPIWriteMemory(uint32_t addr, uint8_t *data, uint32_t len);
bool QSPIEraseCommand(uint8_t command,uint32_t address);

bool QSPIRunCommand(uint8_t command);
bool QSPIReadCommand(uint8_t command, uint8_t *response, uint32_t len);
bool QSPIWriteCommand(uint8_t command, uint8_t const *data, uint32_t len);
bool QSPIReadSFDP(uint8_t command, uint8_t *data, uint32_t write_data, uint8_t *response, uint32_t len);
#else
#define SPIDEV SPI
void SPIBegin();
uint8_t SPITransfer(uint8_t data);
void SPICsInit(uint8_t pin, uint8_t mode);
void SPICsControl(uint8_t pin, uint8_t val);
void SPISetClock(uint64_t hz);
#endif

#ifdef __cplusplus
}
#endif

#endif