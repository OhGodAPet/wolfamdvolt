#include <stdint.h>
#include <unistd.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdi2cdbg.h"

int AMDSMBusSendByte(AMDGPU *GPU, uint8_t cmd)
{
	AMDI2CSoftReset(GPU);

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP);

	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Select register
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	return(ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
}

int AMDSMBusWriteWord(AMDGPU *GPU, uint8_t cmd, uint16_t data)
{
	uint32_t Status;
	char *DbgStr = NULL;

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP | I2C_STOP_ON_NACK);

	// Select address to write to
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, (cmd << 8));
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, data & 0x0000FF00);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, (data & 0xFF) << 8);

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, data & 0x0000FF00);

	I2CExecTX(GPU);

	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);

		if((!(Status & I2C_STATUS_MASK) & I2C_DONE) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
		{
			AMDI2CDbgGetStatusString(&DbgStr, Status);
			if(DbgStr)
			{
				if(I2CDebugOutput) printf("AMDSMBusWriteWord: First TX returned with %s.\n", DbgStr);
				free(DbgStr);
			}
		}
	} while(!(Status & I2C_DONE) && !(Status & I2C_NACK));

	return(Status);
}

/*
// Parameter ret is for I2C status return; optional
uint8_t AMDSMBusReadByte(AMDGPU *GPU, uint8_t cmd, int *ret)
{
	uint8_t Byte;
	int tmpret;
	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP_ON_NACK);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	tmpret = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	if(ret) *ret = tmpret;

	if(tmpret & I2C_STOPPED_ON_NACK)
	{
		printf("\nError: NACK\n");
		return(0);
	}

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_ACK_ON_READ | I2C_STOP | I2C_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	tmpret |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	if(ret) *ret = tmpret;

	if(tmpret & I2C_STOPPED_ON_NACK)
	{
		printf("\nError: NACK\n");
		return(ret);
	}

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_DATA_RW);

	Byte = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	return(Byte);
}

// According to the spec, this is supposed to send a block length
// before the actual block, but the IR35??B doesn't seem to?
int AMDSMBusReadBlock(AMDGPU *GPU, uint8_t cmd, uint8_t *len, uint8_t *ret)
{
	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_RW | I2C_STOP);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	*len = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	if(*len >= 0xFF) return(-4);

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (*len << 16) | I2C_RW | I2C_ACK_ON_READ | I2C_STOP);

	//I2CExecTX(GPU);

	for(int i = 0; i < *len; ++i)
	{
		WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2 + i) | I2C_DATA_RW);
		ret[i] = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;
	}

	return(ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
}*/

/*
// Parameter ret is for I2C status return; optional
uint16_t AMDSMBusReadWord(AMDGPU *GPU, uint8_t cmd, uint32_t *ret)
{
	uint16_t Word = 0x0000;
	uint32_t Status;
	char *DbgStr = NULL;

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_ACK_ON_READ);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	// Read from address zero.
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1));
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, 0x00UL);

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);


	if((Status & I2C_STATUS_MASK) & (~I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			printf("AMDSMBusReadWord: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(*ret) *ret = Status;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (2 << 16) | I2C_START | I2C_STOP | I2C_RW | I2C_ACK_ON_READ);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));



	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);

	if(!((Status & I2C_STATUS_MASK) == I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			printf("AMDSMBusReadWord: Second TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(ret) *ret |= Status;

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_DATA_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);

	// Read out the data - first low byte, then high.
	Word = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);

	Word |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) & 0x0000FF00;

	return(Word);
}
*/

// Parameter ret is for I2C status return; optional
uint16_t AMDSMBusReadWord(AMDGPU *GPU, uint8_t cmd, uint32_t *ret)
{
	uint16_t Word;
	uint32_t Status;
	char *DbgStr = NULL;

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);

	if((!(Status & I2C_STATUS_MASK) & I2C_DONE) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDSMBusReadWord: First TX returned with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(ret) *ret = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_ACK_ON_READ | I2C_STOP | I2C_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	if((!(Status & I2C_STATUS_MASK) & I2C_DONE) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDSMBusReadWord: Second TX returned with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(ret) *ret |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK;

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_DATA_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	// Read out the data - first low byte, then high.
	Word = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);

	Word |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) & 0x0000FF00;

	return(Word);
}

#if 1

// Parameter ret is for I2C status return; optional
uint8_t AMDSMBusReadByte(AMDGPU *GPU, uint8_t cmd, uint32_t *ret)
{
	uint8_t Byte;
	uint32_t Status;
	char *DbgStr = NULL;

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	// That is, number of bytes to read, plus one.
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);


	if(!((Status & I2C_STATUS_MASK) == I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDSMBusReadByte: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(ret) *ret = Status;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP | I2C_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);

	if(!((Status & I2C_STATUS_MASK) == I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDSMBusReadByte: Second TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}


	if(ret) *ret |= Status;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	Byte = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	return(Byte);
}

#elif 0

// Parameter ret is for I2C status return; optional
uint8_t AMDSMBusReadByte(AMDGPU *GPU, uint8_t cmd, uint32_t *ret)
{
	uint8_t Byte;
	uint32_t Status;
	char *DbgStr = NULL;

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, I2C_INDEX(1) | I2C_START);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((uint32_t)cmd) << 8));


	I2CExecTX(GPU);

	if(!((Status & I2C_STATUS_MASK) == I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			printf("AMDSMBusReadByte: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(*ret) *ret = Status;


	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, I2C_INDEX(2) | I2C_START | I2C_RW | I2C_STOP);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);

	if(!((Status & I2C_STATUS_MASK) == I2C_DONE))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			printf("AMDSMBusReadByte: Second TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	if(ret) *ret |= Status;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_DATA_RW);

	// Read out the data
	Byte = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	return(Byte);
}

#else

uint8_t AMDSMBusReadByte(AMDGPU *GPU, uint8_t cmd, uint32_t *ret)
{
	return(AMDI2CReadByte(GPU, cmd, ret));
}

#endif

// According to the spec, this is supposed to send a block length
// before the actual block, but the IR35??B doesn't seem to?
int AMDSMBusReadBlock(AMDGPU *GPU, uint8_t cmd, uint8_t *len, uint8_t *ret)
{
	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP_ON_NACK);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	I2CExecTX(GPU);

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_ACK_ON_READ | I2C_RW | I2C_STOP);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	*len = 0;
	*len = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	if(*len >= 0xFF) return(-4);

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (*len << 16) | I2C_RW | I2C_ACK_ON_READ | I2C_STOP);

	//I2CExecTX(GPU);

	for(int i = 0; i < *len; ++i)
	{
		WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2 + i) | I2C_DATA_RW);
		ret[i] = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;
	}

	return(ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
}


int AMDSMBusBlockWriteBlockReadProcessCall(AMDGPU *GPU, uint8_t cmd, uint8_t inlen, uint8_t *indata, uint8_t *outlen, uint8_t *ret)
{
	if(inlen < 1 || inlen > 0xFF) return(-4);

	AMDI2CSoftReset(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, ((3 + inlen) << 16) | I2C_START | I2C_STOP);

	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));

	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (cmd << 8));

	// Write byte count of data we're sending
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | (inlen << 8));

	// Write input data
	for(int i = 0; i < inlen; ++i) WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(3 + i) |  (indata[i] << 8));

	I2CExecTX(GPU);

	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_RW | I2C_STOP);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));

	I2CExecTX(GPU);

	char *DbgStr;
	uint32_t Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	if((!(Status & I2C_STATUS_MASK) & I2C_DONE) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			printf("AMDSMBusReadWord: Second TX returned with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	/*
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_DATA_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);

	// Read out the data - first low byte, then high.
	Word = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);

	Word |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) & 0x0000FF00;
	*/

	*outlen = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;
	printf("outlen = %d\n", *outlen);
	if((*outlen < 1) || (*outlen + inlen) > 0xFF) return(-4);

	for(int i = 0; i < *outlen; ++i) ret[i] = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	return(ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
}

static uint8_t CalcCRC8(uint8_t Fcs, uint8_t in)
{
	for(int k = 7; k >= 0; k--)
	{
		int j = Fcs >> 7 ^ in >> k & 1;
		Fcs = Fcs << 1 & 0xff;

		Fcs ^= j; j <<= 1;
		Fcs ^= j; j <<= 1;
		Fcs ^= j; j <<= 1;
	}
}

uint32_t PECVal(const uint8_t *Packet, uint32_t PacketLen)
{
	uint8_t PEC = 0x00;

	for(int i = 0; i < PacketLen; ++i) PEC = CalcCRC8(PEC, Packet[i]);

	return(PEC);
}
