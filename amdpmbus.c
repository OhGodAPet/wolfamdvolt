#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdi2cdbg.h"

float PMBusDecodeLinearValueWithExponent(int16_t mantissa, int8_t exp)
{
	float Exponent;
	
	// Extract sign bit and convert if needed
	//Exponent = (exp & 0x10) ? -((int)((~exp) & 0x0F) + 1) : (exp & 0xF);
	//Exponent = (exp & 0x10) ? -(~((exp & 0x0F) + 1) & 0x0F) : (exp & 0x0F);
	//float Value = (mantissa & 0x400) ? -(~((mantissa & 0x3FF) + 1) & 0x3FF) : (mantissa & 0x3FF);
	
	exp &= 0x1F;
	mantissa &= 0x7FF;
	if(exp > 0x0F) exp |= 0xE0;
	if(mantissa > 0x3FF) mantissa |= 0xF800;
	
	Exponent = (float)exp;
	float Value = (float)mantissa;
	
	//printf("Exponent: 0x%02X (%f)\n", exp, Exponent);
	//printf("Value: 0x%02X (%f)\n", mantissa, Value);
	
	// TODO: Maybe clean this so we don't need math libs?
	return(Value * powf(2.0, Exponent));
}

float PMBusDecodeLinearValue(uint16_t Input)
{
	float Exponent, Value;
	int8_t exp = Input >> 11;
	int16_t mantissa = Input & 0x7FF;
	
	if(exp > 0x0F) exp |= 0xE0;
	if(mantissa > 0x3FF) mantissa |= 0xF800;
		
	Exponent = (float)exp;
	Value = (float)mantissa;
	
	// TODO: Maybe clean this so we don't need math libs?
	return(Value * powf(2.0, Exponent));
}

// TODO: I feel like this needs more sanity checking.
uint16_t PMBusEncodeValueToLinearWithExponent(uint8_t RawExp, float Value)
{
	float Exponent = (float)((int8_t)((RawExp & 0x10) ? (RawExp | 0xE0) : RawExp));
	float ScaledVoltage = Value / powf(2.0, Exponent);
	//printf("Scaled voltage: %f\n", ScaledVoltage);
	uint16_t EncodedVoltage = (ScaledVoltage < 0) ? ((((uint16_t)ScaledVoltage) & 0x3FF) | 0x400) : (((uint16_t)ScaledVoltage) & 0x3FF);
	//printf("Encoded exponent: 0x%02X\n", RawExp);
	//printf("Encoded value: 0x%04X\n", EncodedVoltage);
	return(((uint16_t)EncodedVoltage) | (((uint16_t)RawExp) << 11));
	//return(EncodedVoltage);
}

// TODO: I feel like this needs more sanity checking.
uint16_t PMBusEncodeValueToLinear(float Value, uint8_t RawExp)
{
	float Exponent = (float)((int8_t)((RawExp & 0x10) ? (RawExp | 0xE0) : RawExp));
	float ScaledVoltage = Value / powf(2.0, Exponent);
	//printf("Scaled voltage: %f\n", ScaledVoltage);
	uint16_t EncodedVoltage = (ScaledVoltage < 0) ? ((((uint16_t)ScaledVoltage) & 0x3FF) | 0x400) : (((uint16_t)ScaledVoltage) & 0x3FF);
	//printf("Encoded exponent: 0x%02X\n", RawExp);
	//printf("Encoded value: 0x%04X\n", EncodedVoltage);
	return(((uint16_t)EncodedVoltage));
	//return(EncodedVoltage);
}

uint32_t IR3XXXMFR_READ_REG(AMDGPU *GPU, uint8_t I2CAddr, int *ret)
{
	uint32_t Dword;
	int Status;
	char *DbgStr;
	AMDI2CSoftReset(GPU);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (1 << 16) | I2C_START | I2C_STOP_ON_NACK);
	
	// Write in address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | ((GPU->I2CAddress << 1) << 8));
	
	// Select PMBus address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | (0xD0 << 8));

	// Select I2C address
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | (I2CAddr << 8));
	
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
			if(I2CDebugOutput) printf("IR3XXXMFR_READ_REG: First TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_TRANSACTION, (4 << 16) | I2C_START | I2C_ACK_ON_READ | I2C_STOP | I2C_RW);
	
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | (((GPU->I2CAddress << 1) | 0x01) << 8));
	
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
			if(I2CDebugOutput) printf("IR3XXXMFR_READ_REG: Second TX errored with %s.\n", DbgStr);
			free(DbgStr);
		}
	}
	
	if(ret) *ret |= Status;
		
	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(1) | I2C_DATA_RW);
	
	Dword = (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) >> 8) & 0xFF;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(2) | I2C_DATA_RW);
	
	Dword |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) & 0xFF;

	WriteMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA, I2C_INDEX_WRITE | I2C_INDEX(3) | I2C_DATA_RW);
	
	Dword |= (ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_DATA) << 8) & 0xFF;
	
	if(*ret) *ret |= ReadMMIOReg(GPU, GPU->I2CGPIOAddrs.mmGENERIC_I2C_STATUS);
	
	return(Dword);
}
