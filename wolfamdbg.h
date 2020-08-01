#ifndef __WOLFAMDBG_H
#define __WOLFAMDBG_H

#include <stdint.h>
#include <stdbool.h>

#define WOLFAMDBG_VERSION_STR			"v0.91"
#define WOLFAMDBG_VERSION				0.91

#define ACTION_INVALID 		0x00UL
#define ACTION_SCAN			0x01UL
#define ACTION_DUMP			0x02UL
#define ACTION_READ			0x03UL
#define ACTION_WRITE		0x04UL
#define ACTION_DUMP_SMBUS	0x05UL
#define ACTION_READ_SMBUS	0x06UL

typedef struct _ArgsObj
{
	bool GPUIdxProvided, I2CBusIdxProvided;
	
	uint32_t Action;
	
	bool WordOperation;
		
	uint32_t GPUIdx, I2CBusIdx, I2CAddress, I2CRegister, WriteValue;
} ArgsObj;

bool ParseCmdLine(ArgsObj *Args, int argc, char **argv);

#endif
