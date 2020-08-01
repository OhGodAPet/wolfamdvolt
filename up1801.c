#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "up1801.h"

uint32_t uP1801GetVoltage(VRMController *VRM, float *VDDC)
{
	uint8_t Mode, VID;
	
	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	// First - check what mode we're in
	
	Mode = AMDI2CReadByte(VRM->ParentGPU, UP1801_ILIM_REG, NULL);

	if(Mode & UP1801_ILIM_DISABLED_OUTPUT)
	{
		*VDDC = 0.0;
		return(VRM_ERROR_SUCCESS);
	}
	
	if(Mode & UP1801_ILIM_SVI_SETTING)
		VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_SVI_REG, NULL);
	else if(Mode & UP1801_ILIM_INIT_SETTING)
		VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_INI_REG, NULL);
	else if(Mode & UP1801_ILIM_PVI_SETTING)				// This is safe because we know SVI is not set
	{
		// I know since the regs are contiguous, this could
		// be a LOT simpler, but doing it this way for readability
		switch(Mode & 0x03)
		{
			case 0:
				VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_LEVEL1_REG, NULL);
				break;
			case 1:
				VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_LEVEL2_REG, NULL);
				break;
			case 2:
				VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_LEVEL3_REG, NULL);
				break;
			case 3:
				VID = AMDI2CReadByte(VRM->ParentGPU, UP1801_LEVEL4_REG, NULL);
				break;
		}
	}
	
	*VDDC = 0.6 + (VID * 0.005);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP1801SetVoltage(VRMController *VRM, float Voltage)
{
	uint8_t Mode, VID;
		
	if(Voltage > 1.875 || Voltage < 0.6) return(VRM_ERROR_RANGE);

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);
	
	// Program the voltage first, so that we don't have the
	// possibility of dropping it to 0.6V accidentally.
	
	VID = ((Voltage - 0.6) / 0.005) + 0.5;
		
	AMDI2CWriteByte(VRM->ParentGPU, UP1801_SVI_REG, VID);
	
	// Ensure the controller is in SVI mode
	Mode = AMDI2CReadByte(VRM->ParentGPU, UP1801_ILIM_REG, NULL);
	
	if(!(Mode & UP1801_ILIM_SVI_SETTING))
		AMDI2CWriteByte(VRM->ParentGPU, UP1801_ILIM_REG, Mode | UP1801_ILIM_SVI_SETTING);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP1801GetOutputIdx(struct _VRMController *VRM, uint32_t *idx)
{	
	*idx = VRM->SelectedOutput;
	return(VRM_ERROR_SUCCESS);
}

uint32_t uP1801SetOutputIdx(struct _VRMController *VRM, uint32_t idx)
{
	if(idx >= VRM->OutputCount) return(VRM_ERROR_RANGE);
	
	VRM->SelectedOutput = idx;
	return(VRM_ERROR_SUCCESS);
}

// uP1801 can only reside at addresses 0x51, 0x52, or 0x53
uint32_t uP1801Detect(AMDGPU *GPU, VRMController **VRMs)
{
	uint8_t Addresses[3] = { 0x51, 0x52, 0x53 };
	uint32_t DevicesFound = 0;
	VRMController *CurrentVRM;
	
	for(int i = 0; i < 3; ++i)
	{
		AMDGPUI2CInit(GPU, STATIC_I2C_LINE_FIXME, Addresses[i]);
			
		if(AMDI2CReadByte(GPU, UP1801_CHIP_ID_REG, NULL) == UP1801_CHIP_ID_VALUE)
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
			CurrentVRM->VRMType = VRM_CONTROLLER_TYPE_UP1801;
			CurrentVRM->Capabilities = 0;
			
			CurrentVRM->OutputCount = 1;
			CurrentVRM->SelectedOutput = 0;
			
			CurrentVRM->I2CAddressList[0] = Addresses[i];
			
			CurrentVRM->GetVoltage = uP1801GetVoltage;
			CurrentVRM->SetVoltage = uP1801SetVoltage;
			CurrentVRM->GetOutputIdx = uP1801GetOutputIdx;
			CurrentVRM->SetOutputIdx = uP1801SetOutputIdx;
			CurrentVRM->next = NULL;
			
			DevicesFound++;
		}
	}
		
	return(DevicesFound);
}
