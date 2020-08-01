#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "up9505.h"

uint32_t uP9505GetVoltage(VRMController *VRM, float *VDDC)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	uint8_t VID = AMDI2CReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? UP9505_VFB_REG : UP9505_VFBA_REG, NULL);
	*VDDC = (VID >= 0xF7) ? 0.0 : 1.55 - ((float)VID * 0.00625);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP9505SetVoltage(VRMController *VRM, float Voltage)
{
	uint8_t VID;
	
	if(Voltage > 1.55 || Voltage < 0.0) return(VRM_ERROR_RANGE);
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	VID = (uint8_t)(((1.55 - Voltage) / 0.00625) + 0.5);
		
	AMDI2CWriteByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? UP9505_LCHVID_REG : UP9505_ALCHVID, VID);
	
	return(VRM_ERROR_SUCCESS);
}

// In the offset, it is NOT two's complement, but instead,
// bit 7 is sign, and the rest are as if it was signed.
// 6.25mV steps - 10000000b is -0mV, 11111000 is -750mV,
// 01111000 is +750mV, 00000001 is +6.25mV, and so on.
// No matter what you put, it won't go higher than +750mV,
// or lower than -750mV - 11111111b is still -750mV.

// Right now - just set them all.
uint32_t uP9505SetOffset(VRMController *VRM, float Voltage)
{
	if((Voltage >= VRM->MaxOffset) || (Voltage <= VRM->MinOffset)) return(VRM_ERROR_RANGE);
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
			
	uint8_t VDDOffset = (Voltage > 0) ? (Voltage / 0.00625) + 0.5 : (Voltage / 0.00625) - 0.5;
	
	if(VDDOffset & 0x80)
	{
		VDDOffset = -VDDOffset;
		VDDOffset |= 0x80;
	}
	
	if(!VRM->SelectedOutput)
	{
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS0_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS1_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS2_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS3_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS4_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_VOFS5_REG, VDDOffset);
	}
	else
	{
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_AVOFS0_REG, VDDOffset);
		AMDI2CWriteByte(VRM->ParentGPU, UP9505_AVOFS1_REG, VDDOffset);
	}
	
	return(VRM_ERROR_SUCCESS);
}

// Since we set them all... just get the first.
uint32_t uP9505GetOffset(VRMController *VRM, float *VDDCOff)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	uint8_t VID = AMDI2CReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? UP9505_VOFS0_REG : UP9505_AVOFS0_REG, NULL);
	
	*VDDCOff = (VID & 0x80) ? -((VID & 0x7F) * 0.00625) : VID * 0.00625;
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP9505GetOutputIdx(VRMController *VRM, uint32_t *Idx)
{
	*Idx = VRM->SelectedOutput;
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP9505SetOutputIdx(VRMController *VRM, uint32_t Idx)
{
	if(Idx >= VRM->OutputCount) return(VRM_ERROR_RANGE);
	
	VRM->SelectedOutput = Idx;
	return(VRM_ERROR_SUCCESS);
}

// TODO/FIXME: Replace magic numbers with macros, and some
// comments on the register layouts for MISC1 & MISC2
uint32_t uP9505Acquire(VRMController *VRM)
{
	uint8_t MISC1, MISC2;
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	// Clear OFS control 2, so SVI has ZERO say
	MISC1 = AMDI2CReadByte(VRM->ParentGPU, UP9505_MISC1_REG, NULL) & 0x7F;
	MISC2 = AMDI2CReadByte(VRM->ParentGPU, UP9505_MISC2_REG, NULL);
	
	AMDI2CWriteByte(VRM->ParentGPU, UP9505_MISC1_REG, MISC1 | 0x70);
	AMDI2CWriteByte(VRM->ParentGPU, UP9505_MISC2_REG, MISC2 | 0x80);
	
	return(VRM_ERROR_SUCCESS);
}

// TODO/FIXME: Replace magic numbers with macros, and some
// comments on the register layouts for MISC1 & MISC2

// WARNING: NOT IMPLEMENTED RIGHT.
uint32_t uP9505Release(VRMController *VRM)
{
	uint8_t MISC1, MISC2;
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	MISC1 = AMDI2CReadByte(VRM->ParentGPU, UP9505_MISC1_REG, NULL);
	MISC2 = AMDI2CReadByte(VRM->ParentGPU, UP9505_MISC2_REG, NULL);
	
	AMDI2CWriteByte(VRM->ParentGPU, UP9505_MISC1_REG, MISC1 & 0x8F);
	AMDI2CWriteByte(VRM->ParentGPU, UP9505_MISC2_REG, MISC2 & 0x7F);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP9505GetTemp(VRMController *VRM, uint32_t *Temp)
{
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	*Temp = (uint32_t)AMDSMBusReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? UP9505_TM_REG : UP9505_ATM_REG, NULL);
	
	return(VRM_ERROR_SUCCESS);
}

// uP9505 can only reside at addresses 0x44 or 0x46
// Returns number of devices found.
uint32_t uP9505Detect(AMDGPU *GPU, VRMController **VRMs)
{
	uint8_t byte;
	uint32_t DevicesFound = 0;
	VRMController *CurrentVRM;
	
	for(int i = 0; i < 2; ++i)
	{
		AMDGPUI2CInit(GPU, STATIC_I2C_LINE_FIXME, (i) ? 0x46 : 0x44);
		
		if(AMDI2CReadByte(GPU, UP9505_CHIP_ID_REG, NULL) == UP9505_CHIP_ID_VALUE)
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
			
			CurrentVRM->ParentGPU = GPU;
			CurrentVRM->VRMType = VRM_CONTROLLER_TYPE_UP9505;
			CurrentVRM->Capabilities = VRM_CAPABILITY_OFFSET | VRM_CAPABILITY_TEMP;
			
			CurrentVRM->MinOffset = -0.75;
			CurrentVRM->MaxOffset = 0.75;
			
			CurrentVRM->OutputCount = 2;
			CurrentVRM->SelectedOutput = 0;
			
			CurrentVRM->I2CAddressList[0] = (i) ? 0x46 : 0x44;
			
			CurrentVRM->GetVoltage = uP9505GetVoltage;
			CurrentVRM->SetVoltage = uP9505SetVoltage;
			CurrentVRM->GetVoltageOffset = uP9505GetOffset;
			CurrentVRM->SetVoltageOffset = uP9505SetOffset;
			CurrentVRM->GetOutputIdx = uP9505GetOutputIdx;
			CurrentVRM->SetOutputIdx = uP9505SetOutputIdx;
			CurrentVRM->GetTemp = uP9505GetTemp;
			
			CurrentVRM->next = NULL;
			DevicesFound++;
		}
	}
	
	return(DevicesFound);
	
}

int uP9505SetVDDLoadLineAllPhases(AMDGPU *GPU, uint8_t Setting)
{
	int ret;
	
	// LL setting is a 4-bit value, each register controls
	// two phases - duplicate the low nibble.
	Setting &= 0x0F;
	Setting = (Setting << 4) | Setting;
		
	ret = AMDI2CWriteByte(GPU, UP9505_IICLL01_REG, Setting);
	ret |= AMDI2CWriteByte(GPU, UP9505_IICLL23_REG, Setting);
	ret |= AMDI2CWriteByte(GPU, UP9505_IICLL45_REG, Setting);
	
	return(ret);
}

