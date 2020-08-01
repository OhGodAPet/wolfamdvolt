#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "ncp81022.h"

/*
// Returns true if an NCP81022 was found, false if not
bool NCP81022Detect(AMDGPU *GPU, int *ret)
{
	uint16_t out;
	
	// NCP81022 SMBus address can only be at 0x20 - 0x27
	for(int addr = 0x20; addr < 0x28; ++addr)
	{
		AMDGPUI2CInit(GPU, addr);
		out = AMDSMBusReadWord(GPU, PMBUS_MFR_ID, ret);
		
		if(out == 0x001A)
		{
			out = AMDSMBusReadWord(GPU, PMBUS_MFR_MODEL, ret);
			if(out == 0x1022) return(true);
		}
	}
	
	return(false);
}
*/

uint32_t NCP81022GetOutputCurrent(VRMController *VRM, float *Current)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	uint16_t Iout = AMDSMBusReadWord(VRM->ParentGPU, PMBUS_READ_IOUT, NULL);
	uint16_t Offset = AMDSMBusReadWord(VRM->ParentGPU, NCP81022_PMBUS_IOUT_OFFSET, NULL);
	uint16_t Gain = AMDSMBusReadWord(VRM->ParentGPU, NCP81022_PMBUS_IOUT_CAL_GAIN, NULL);
	//float IOUTGain = PMBusDecodeLinearValue(Gain);
	//float IOUTOffset = PMBusDecodeLinearValue(Offset);
	float IOUT = PMBusDecodeLinearValue(Iout);
	
	*Current = (float)IOUT;
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022GetVoltage(VRMController *VRM, float *VDDC)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	//uint16_t VID = AMDSMBusReadWord(VRM->ParentGPU, PMBUS_READ_VOUT, NULL);
	//*VDDC = ((VID & 0xFF) >= 0xF7) ? 0.0 : 1.55 - ((VID & 0xFF) * 0.00625);
	
	 int16_t val = AMDSMBusReadWord(VRM->ParentGPU, NCP81022_READ_VOUT_LINEAR_REG, NULL);
	 *VDDC = PMBusDecodeLinearValueWithExponent(val, -9);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022SetVoltage(VRMController *VRM, float Voltage)
{
	uint16_t VID;
	
	// Ensure voltage is in range
	if(Voltage < 0.25 || Voltage > 1.55) return(VRM_ERROR_RANGE);
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	Voltage = (1.55 - Voltage) / 0.00625;
	
	// Get VID from GPU
	VID = AMDSMBusReadWord(VRM->ParentGPU, PMBUS_READ_VOUT, NULL) & 0xFF00;
	
	// Round to nearest VID and save
	VID |= (uint8_t)(Voltage + 0.5);
	
	AMDSMBusWriteWord(VRM->ParentGPU, PMBUS_VOUT_COMMAND, VID);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022GetOutputIdx(VRMController *VRM, uint32_t *Idx)
{
	*Idx = VRM->SelectedOutput;
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022SetOutputIdx(VRMController *VRM, uint32_t Idx)
{
	if(Idx >= VRM->OutputCount) return(VRM_ERROR_RANGE);
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	uint8_t reg = AMDSMBusReadByte(VRM->ParentGPU, NCP81022_PMBUS_VRCONFIG1_REG, NULL);
	
	if(Idx) reg |= 0x04;
	else reg &= 0xFB;
	
	AMDI2CWriteByte(VRM->ParentGPU, NCP81022_PMBUS_VRCONFIG1_REG, reg);
	
	VRM->SelectedOutput = Idx;
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022GetOffset(VRMController *VRM, float *VoltOffset)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	int8_t VID = AMDSMBusReadByte(VRM->ParentGPU, NCP81022_PMBUS_SPOFFSET_REG, NULL);
	*VoltOffset = VID * 0.00625;
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022SetOffset(VRMController *VRM, float Voltage)
{
	if((Voltage >= VRM->MaxOffset) || (Voltage <= VRM->MinOffset)) return(VRM_ERROR_RANGE);
			
	uint8_t VOffset = (Voltage > 0) ? (Voltage / 0.00625) + 0.5 : (Voltage / 0.00625) - 0.5;
	
	AMDI2CWriteByte(VRM->ParentGPU, NCP81022_PMBUS_SPOFFSET_REG, VOffset);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t NCP81022SetLoadLine(VRMController *VRM, uint8_t Setting)
{
	return(AMDI2CWriteByte(VRM->ParentGPU, NCP81022_PMBUS_LOADLINE_REG, Setting));
}

// Return value is how many devices were found.
uint32_t NCP81022Detect(AMDGPU *GPU, VRMController **VRMs)
{
	uint16_t out;
	uint32_t DevicesFound = 0;
	VRMController *CurrentVRM;
	
	// NCP81022 SMBus address can only be at 0x20 - 0x27
	for(int addr = 0x20; addr < 0x28; ++addr)
	{
		uint32_t tmp;
		AMDGPUI2CInit(GPU, STATIC_I2C_LINE_FIXME, addr);
		//tmp = AMDSMBusReadByte(GPU, PMBUS_REVISION, NULL);

		// NCP81022 will support at LEAST PMBus Part I
		// rev. 1.1 and PMBus Part II rev. 1.2.
		//if(((tmp & 0x0F) < 0x02) && ((tmp >> 4) < 0x01)) continue;
		
		//tmp = AMDSMBusReadByte(GPU, PMBUS_CAPABILITY, NULL);

		// Lowest 5 bits should be zero.
		//if(tmp & 0x1F) continue;
		//else if(!(tmp & 0x80)) continue;
		//else if(!(tmp & 0x20)) continue;
		
		out = AMDSMBusReadWord(GPU, PMBUS_MFR_ID, NULL);
		
		if(out == 0x001A)
		{
			out = AMDSMBusReadWord(GPU, PMBUS_MFR_MODEL, NULL);
			if(out == 0x1022)
			{				
				if(!*VRMs)
				{
					CurrentVRM = *VRMs = (VRMController *)calloc(1, sizeof(VRMController));
				}
				else
				{
					for(CurrentVRM = *VRMs; CurrentVRM->next; CurrentVRM = CurrentVRM->next);
					CurrentVRM = CurrentVRM->next = (VRMController *)calloc(1, sizeof(VRMController));
				}
				DevicesFound++;
						
				CurrentVRM->ParentGPU = GPU;
				CurrentVRM->VRMType = VRM_CONTROLLER_TYPE_NCP81022;
				CurrentVRM->Capabilities = VRM_CAPABILITY_OFFSET | VRM_CAPABILITY_LOADLINE;
				
				CurrentVRM->MinOffset = -793.75;
				CurrentVRM->MaxOffset = 793.75;
				
				CurrentVRM->OutputCount = 2;
				CurrentVRM->SelectedOutput = 0;
				
				CurrentVRM->I2CAddressList[0] = addr;
				
				CurrentVRM->GetVoltage = NCP81022GetVoltage;
				CurrentVRM->SetVoltage = NCP81022SetVoltage;
				CurrentVRM->GetOutputIdx = NCP81022GetOutputIdx;
				CurrentVRM->SetOutputIdx = NCP81022SetOutputIdx;
				CurrentVRM->GetVoltageOffset = NCP81022GetOffset;
				CurrentVRM->SetVoltageOffset = NCP81022SetOffset;
				CurrentVRM->SetLoadLine	= NCP81022SetLoadLine;
				//CurrentVRM->GetCurrent = NCP81022GetOutputCurrent;
				CurrentVRM->next = NULL;
			}
		}
	}
	
	return(DevicesFound);
}
// Register format:
//		Bit 0:
//			0 - SVI2 controls shit
//			1 - SMBus controls shit
//		Bit 1: Reserved
//		Bit 2: Rail select
//			0 - Main rail
//			1 - Other rail
//		Bits 3 - 5: Reserved
//		Bit 6:
//			0 - CLIM latch-off enabled
//			1 - CLIM latch-off disabled
//		Bit 7: Reserved
uint32_t NCP81022SwitchControls(VRMController *VRM, bool SMBusControl)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	uint8_t reg = AMDSMBusReadByte(VRM->ParentGPU, NCP81022_PMBUS_VRCONFIG1_REG, NULL);
	
	if(SMBusControl) reg |= 0x01;
	else reg &= 0xFE;
	
	AMDI2CWriteByte(VRM->ParentGPU, NCP81022_PMBUS_VRCONFIG1_REG, reg);
	
	return(VRM_ERROR_SUCCESS);
}

float NCP81022GetInputVoltage(AMDGPU *GPU, int *ret)
{
	uint16_t EncodedVolts = AMDSMBusReadWord(GPU, PMBUS_READ_VIN, ret);
	return(PMBusDecodeLinearValue(EncodedVolts));
}

// Broke atm
float NCP81022GetOutputPower(AMDGPU *GPU, int *ret)
{
	uint16_t EncodedWatts = AMDSMBusReadWord(GPU, PMBUS_READ_POUT, ret);
	return(PMBusDecodeLinearValue(EncodedWatts));
}
