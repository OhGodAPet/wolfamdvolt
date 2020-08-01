#ifndef __AMDSMBUS_H
#define __AMDSMBUS_H

#include <stdint.h>
#include "amdgpu.h"

#define SMBUS_BLOCK_TRANSFER_MAX	32

int AMDSMBusSendByte(AMDGPU *GPU, uint8_t cmd);
uint8_t AMDSMBusReadByte(AMDGPU *GPU, uint8_t cmd, uint32_t *ret);
uint16_t AMDSMBusReadWord(AMDGPU *GPU, uint8_t cmd, uint32_t *ret);
int AMDSMBusWriteWord(AMDGPU *GPU, uint8_t cmd, uint16_t data);
int AMDSMBusReadBlock(AMDGPU *GPU, uint8_t cmd, uint8_t *len, uint8_t *ret);
int AMDSMBusBlockProcessCall(AMDGPU *GPU, uint8_t cmd, uint8_t inlen, uint8_t *indata, uint8_t *outlen, uint8_t *ret);

#endif
