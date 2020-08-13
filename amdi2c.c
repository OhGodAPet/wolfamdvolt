#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdi2cdbg.h"

void AMDI2CSoftReset(AMDGPU *GPU)
{
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_CONTROL, I2C_ENABLE | I2C_SOFT_RESET);
	usleep(1000);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_CONTROL, I2C_ENABLE);
	usleep(1000);
}

void AMDI2CReset(AMDGPU *GPU)
{
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_CONTROL, I2C_ENABLE | I2C_RESET | I2C_GO);
	usleep(500);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_CONTROL, I2C_ENABLE);
	usleep(500);
}

//	SetI2CSpeedValues sets the parameters for the
//	speed-related I2C bus AMD GPU access.
//		Params:
//			GPU:
//				This MUST have its GPIO addresses and pins set.
//				It is only used to write the settings to the AMD
//				GPU control register.
//			Threshold:
//				This is a two-bit field which is interpreted as so:
//					0x00 - Greater than zero
//					0x01 - 1/4 of total samples
//					0x02 - 1/2 of total samples
//					0x03 - 3/4 of total samples
//			DisableFilterWhileStalling:
//				Boolean which does as it says on the tin:
//					false: Disable
//					true: Enable
//			StartAndStopTiming:
//				Field two bits wide that is interpreted as follows:
//					0x00: 1/4 I2C data bit period
//					0x01: 1/2 I2C data bit period
//					0x02: 3/4 I2C data bit period
//					0x03: Reserved
//			Prescale:
//				Full 16 bit prescale value.
static void SetI2CSpeedValues(AMDGPU *GPU, uint8_t Threshold, bool DisableFilterWhileStalling, uint8_t StartAndStopTiming, uint16_t Prescale)
{
	uint32_t I2CSpeedCfgDword = 0UL;
	
	I2CSpeedCfgDword |= (Threshold & 0x03);
	I2CSpeedCfgDword |= ((DisableFilterWhileStalling) ? (1UL << 4UL) : 0UL);
	I2CSpeedCfgDword |= (((uint32_t)(StartAndStopTiming & 0x03)) << 8UL);
	I2CSpeedCfgDword |= (((uint32_t)Prescale) << 16UL);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SPEED, I2CSpeedCfgDword);
}

//	SetI2CSetupParams configures the basic options of the line to access.
//		Params:
//			GPU:
//				This MUST have its GPIO addresses and pins set.
//				It is only used to write the settings to the AMD
//				GPU control register.
//			DataDriveEn:
//				false: SDA line is pulled up by an external resistor.
//				true: SDA line is driven by I2C pads.
//			DataDriveSel:
//				false: Drive for 10 memory clocks
//				true: Drive for 20 memory clocks.
//			ClkDriveEn:
//				false: SCL line is pulled up by an external resistor.
//				true: I2C pads drive SCL
//			IntraByteDelay:
//				Does what it says on the tin - 8 bit field.
//			TimeoutLimit:
//				Ditto - timeout limit. 8 bit field.
static void SetI2CSetupParams(AMDGPU *GPU, bool DataDriveEn, bool DataDriveSel, bool ClkDriveEn, uint8_t IntraByteDelay, uint8_t TimeoutLimit)
{
	uint32_t SetupDword = 0UL;
	
	SetupDword |= ((DataDriveEn) ? 1UL : 0UL);
	SetupDword |= ((DataDriveSel) ? (1UL << 1UL) : 0);
	SetupDword |= ((ClkDriveEn) ? (1UL << 7UL) : 0);
	SetupDword |= (((uint32_t)IntraByteDelay) << 8UL);
	SetupDword |= (((uint32_t)TimeoutLimit) << 24UL);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SETUP, SetupDword);
}


void AMDGPUI2CInit(AMDGPU *GPU, uint8_t bus, uint8_t addr)
{
	GPU->I2CAddress = addr;
	
	if(AMDGPUIsHawaii(GPU) || AMDGPUIsBonaire(GPU))
	{
		GPU->I2CGPIOAddrs = GCN2_I2C_GPIO_ADDRS;
	}
	else if(AMDGPUIsVega(GPU))
	{
		GPU->I2CGPIOAddrs = GCN5_I2C_GPIO_ADDRS;
	}
	else
	{
		GPU->I2CGPIOAddrs = GCN3_I2C_GPIO_ADDRS;
	}
	
	if(bus > 0x07) GPU->I2CPins = GCN3_I2C_BUS_LINES[0];		// HWI2C
	else GPU->I2CPins = GCN3_I2C_BUS_LINES[bus];
	
	AMDI2CReset(GPU);
	
	bool VegaGPU = AMDGPUIsVega(GPU);
	
	uint32_t tmp = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_INTERRUPT_CONTROL);
	if(!(tmp & 0x02)) WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_INTERRUPT_CONTROL, tmp | 0x02);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_INTERRUPT_CONTROL, 0x00);
	
	// Select pins
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_PIN_SELECTION, (GPU->I2CPins->SDA << 8) | GPU->I2CPins->SCL);
	
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SPEED, ((0x021C << 16) | 0x02));
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SPEED, ((0x010E << 16) | 0x02));
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SPEED, ((0x87 << 16) | 0x02));
	SetI2CSpeedValues(GPU, 0x02, false, 0x00, 0x01F4);			// 0x01F4 == 50Khz; 0x01E8 == 100Khz
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SPEED, ((0x43 << 16) | 0x03));
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SETUP, 0x32000107);
	//WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_SETUP, 0x28010007);
	if(VegaGPU) SetI2CSetupParams(GPU, false, false, false, 1, 0xFF);
	else SetI2CSetupParams(GPU, true, true, true, 1, 0xFF);
	//SetI2CSetupParams(GPU, false, false, false, 1, 0xFF);
	//AMDI2CReset(GPU);
	
	
}

void I2CExecTX(AMDGPU *GPU)
{
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_CONTROL, I2C_ENABLE | I2C_GO);
	usleep(2500);
}

// Parameter ret is for I2C status return; optional
uint8_t AMDI2CReadByte(AMDGPU *GPU, uint8_t reg, uint32_t *ret)
{
	uint8_t Byte;
	uint32_t Status;
	char *DbgStr = NULL;
	
	AMDI2CSoftReset(GPU);
	
	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP);
	
	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	// Indicate register to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (reg << 8));
	
	I2CExecTX(GPU);
	
	/*
	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	*/
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CReadByte: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	if(ret) *ret = Status;
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (3 << 16) | I2C_START | I2C_STOP | I2C_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));
	
	I2CExecTX(GPU);
	
	/*
	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	*/
	
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CReadByte: Second TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	if(ret) *ret |= Status;
		
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);
	
	Byte = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;
	
	return(Byte);
}

// Parameter ret is for I2C status return; optional
uint16_t AMDI2CReadWord(AMDGPU *GPU, uint32_t *ret)
{
	uint16_t Word;
	uint32_t Status;
	char *DbgStr = NULL;
	
	AMDI2CSoftReset(GPU);
	
	// Number of transactions is shifted up by 16
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP | I2C_ACK_ON_READ | I2C_STOP_ON_NACK | I2C_RW);
	
	// Select address to read from
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));
	
	I2CExecTX(GPU);
	
	/*
	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	*/
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CReadByte: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	if(ret) *ret = Status;
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);
	
	Word = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;
	Word = (Word << 8) | ((ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF);
	
	return(Word);
}

#if 1

int AMDI2CWriteByte(AMDGPU *GPU, uint8_t reg, uint8_t data)
{
	uint32_t Status;
	char *DbgStr = NULL;
	
	AMDI2CSoftReset(GPU);
	
	// Length of write is 2 - register, then data
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (2 << 16) | I2C_START | I2C_STOP);
	
	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	// Select register
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (reg << 8));
	
	// Now write in data byte
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | (data << 8));
	
	I2CExecTX(GPU);

	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CWriteByte: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
		
	return(ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
}

uint32_t AMDI2CWriteWord(AMDGPU *GPU, uint16_t data)
{
	uint32_t Status;
	char *DbgStr = NULL;
	
	AMDI2CSoftReset(GPU);
	
	// Length of write is 2 - register, then data
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP | I2C_STOP_ON_NACK);
	
	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	// Write MSB
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (data & 0xFF00));
	
	// Write LSB
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | (data << 8));
	
	I2CExecTX(GPU);

	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CWriteWord: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
		
	return(Status);
}

#else

// Length is IGNORING the fact the reg is written!
uint8_t AMDI2CWrite(AMDGPU *GPU, uint8_t reg, uint8_t *data, uint8_t len)
{
	uint32_t Status;
	char *DbgStr = NULL;
	
 	AMDI2CSoftReset(GPU);
 	
	// Length of write, add one because we must write the register to access
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, ((len) << 16) | I2C_START | I2C_STOP | I2C_STOP_ON_NACK);
 	
 	// Write in address
 	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, ((GPU->I2CAddress << 1) << 8));

	// Select register
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (reg << 8));
 	
	// Now put in the data
 	for(int i = 0; i < len; ++i)
 		WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(i + 1) | (data[i] << 8));
 	
 	I2CExecTX(GPU);
 	
 	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
 	
 	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CWrite: TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	return(Status);
}

int AMDI2CWriteByte(AMDGPU *GPU, uint8_t reg, uint8_t data)
{
	return(AMDI2CWrite(GPU, reg, &data, 2));
}

#endif

/*
uint8_t AMDI2CWriteRaw(AMDGPU *GPU, uint8_t *data, uint8_t len)
{
	uint8_t ret;
	
	AMDI2CSoftReset(GPU);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, I2C_START | I2C_STOP_ON_NACK);
	
	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	I2CExecTX(GPU);
	
	ret = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
	
	if(ret & I2C_STOPPED_ON_NACK)
	{
		printf("\nError: NACK\n");
		return(ret);
	}
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (len << 16) | I2C_STOP_ON_NACK | I2C_STOP);
	
	// Now put in the data
	for(int i = 0; i < len; ++i)
		WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, (data[i] << 8));
	
	I2CExecTX(GPU);
	
	ret |= (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS) & I2C_STATUS_MASK);
	
	if(ret & I2C_STOPPED_ON_NACK)
	{
		printf("\nError: NACK\n");
		return(ret);
	}
	
	return(ret);
}
*/

uint32_t AMDI2CWriteRaw(AMDGPU *GPU, uint8_t *data, uint8_t len)
{
	uint32_t Status;
	char *DbgStr = NULL;
		
	AMDI2CSoftReset(GPU);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, I2C_INDEX(len + 1) | I2C_START | I2C_STOP_ON_NACK | I2C_STOP);
	
	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	for(int i = 0; i < len; ++i)
	{
		WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, data[i] << 8);
	}
	
	I2CExecTX(GPU);

	do
	{
		Status = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
		if((Status & I2C_STATUS_MASK) & I2C_DONE) break;
		usleep(5);
		
	} while(!(((Status) & I2C_STATUS_MASK) & I2C_TIMEOUT));
	
	if((((Status & I2C_STATUS_MASK) & I2C_ABORTED)) || ((Status & I2C_STATUS_MASK) & I2C_NACK))
	{
		AMDI2CDbgGetStatusString(&DbgStr, Status);
		if(DbgStr)
		{
			if(I2CDebugOutput) printf("AMDI2CWriteWord: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	return(Status);
}

uint8_t AMDI2CReadRaw(AMDGPU *GPU, uint8_t *dout, uint8_t len)
{
	uint8_t ret;
	
	AMDI2CSoftReset(GPU);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, I2C_INDEX(len + 1) | I2C_ACK_ON_READ | I2C_START | I2C_STOP | I2C_RW);
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));
	
	I2CExecTX(GPU);
	
	ret = ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	
	if(ret & I2C_STOPPED_ON_NACK)
	{
		if(I2CDebugOutput) printf("\nError: NACK\n");
		return(ret);
	}
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);
	
	for(int i = 0; i < len; ++i)
	{
		dout[i] = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;	
	}
	
	return(ret);
}

