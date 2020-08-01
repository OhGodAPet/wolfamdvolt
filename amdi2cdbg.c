#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdi2cdbg.h"

#ifdef DEBUG
bool I2CDebugOutput = true;
#else
bool I2CDebugOutput = false;
#endif

static const char DoneStr[] = "I2C_DONE";
static const char AbortedStr[] = "I2C_ABORTED";
static const char TimeoutStr[] = "I2C_TIMEOUT";
static const char StopNACKStr[] = "I2C_STOPPED_ON_NACK";
static const char NACKStr[] = "I2C_NACK";

// Caller is responsible for freeing returned pointer!
// If NULL is returned - none of the status flags were set.
void AMDI2CDbgGetStatusString(char **DbgStrIn, uint32_t I2CStatusVal)
{
	char *DbgStrBuf;
	if(!DbgStrIn) return;
	
	*DbgStrIn = (char *)malloc(AMDI2CDBG_STATUS_STR_MAX_LEN);
	DbgStrBuf = *DbgStrIn;
	
	bool AppendSuffix = false;
	
	DbgStrBuf[0] = 0x00;
	/*
	if(I2CStatusVal & I2C_DONE)
	{
		strcat(DbgStrBuf, DoneStr);
		AppendSuffix = true;
	}
	*/
	
	if(I2CStatusVal & I2C_ABORTED)
	{
		if(AppendSuffix) strcat(DbgStrBuf, ", ");
		strcat(DbgStrBuf, AbortedStr);
		AppendSuffix = true;
	}
	
	if(I2CStatusVal & I2C_TIMEOUT)
	{
		if(AppendSuffix) strcat(DbgStrBuf, ", ");
		strcat(DbgStrBuf, TimeoutStr);
		AppendSuffix = true;
	}
	
	if(I2CStatusVal & I2C_STOPPED_ON_NACK)
	{
		if(AppendSuffix) strcat(DbgStrBuf, ", ");
		strcat(DbgStrBuf, StopNACKStr);
		AppendSuffix = true;
	}
	
	if(I2CStatusVal & I2C_NACK)
	{
		if(AppendSuffix) strcat(DbgStrBuf, ", ");
		strcat(DbgStrBuf, NACKStr);
	}
	
	if(DbgStrBuf[0] == 0x00)
	{
		free(*DbgStrIn);
		*DbgStrIn = NULL;
	}
}
