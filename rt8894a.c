#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "rt8894a.h"

// It is important that before we update any register at runtime
// that we write 0x5A to RT8894A_I2C_LOCK_IND, and that we write
// 0xFF back to RT8894A_I2C_LOCK_IND when our work is complete.

void RT8894AUnlockI2C(AMDGPU *GPU)
{
	AMDI2CWriteByte(GPU, RT8894A_I2C_LOCK_IND, RT8894A_UNLOCK_VALUE);
}

void RT8894ALockI2C(AMDGPU *GPU)
{
	AMDI2CWriteByte(GPU, RT8894A_I2C_LOCK_IND, RT8894A_LOCK_VALUE);
}

uint32_t RT8894AGetVoltage(VRMController *VRM, float *VDDC)
{
	uint8_t VID = AMDSMBusReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? RT8894A_I2C_READ_VDD : RT8894A_I2C_READ_VDDNB, NULL) << 1;
	
	*VDDC = (VID > 0xF8) ? 0.0 : 1.55 - ((float)VID * 0.00625);
	
	return(VRM_ERROR_SUCCESS);
}

uint32_t RT8894AGetOutputIdx(VRMController *VRM, uint32_t *Idx)
{
	*Idx = VRM->SelectedOutput;
	return(VRM_ERROR_SUCCESS);
}

uint32_t RT8894ASetOutputIdx(VRMController *VRM, uint32_t Idx)
{
	if(Idx >= VRM->OutputCount) return(VRM_ERROR_RANGE);
	
	VRM->SelectedOutput = Idx;
	return(VRM_ERROR_SUCCESS);
}

// TODO/FIXME: Macro away the magic constants in here.
uint32_t RT8894ASetVoltage(VRMController *VRM, float Voltage)
{
	uint8_t VID, reg;
	
	if(Voltage > 1.55 || Voltage <= 0.0) return(VRM_ERROR_RANGE);
	
	VID = (uint8_t)(((1.55 - Voltage) / 0.00625) + 0.5);
	
	
	
	RT8894AUnlockI2C(VRM->ParentGPU);
	
	AMDI2CWriteByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? RT8894A_I2C_FIX_VDD : RT8894A_I2C_FIX_NB, VID);
	
	reg = AMDSMBusReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? RT8894A_I2C_AUTO_ZLL_VDD : RT8894A_I2C_AUTO_ZLL_NB, NULL);
	// If VFIX mode for the chosen rail isn't enabled, we need to enable it
	// 0x2E means set VDD to VID + RT8894A_I2C_OFS_VDD (ignoring all SVI2 offset trim commands)
	// Enable VDD rail auto down phase mode, enable VDD rail zero load-line, disable VDD power
	// scenario load-line, and enable VDD rail VFIX mode. Only set it if we haven't yet.
	if(reg != 0x26) AMDI2CWriteByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? RT8894A_I2C_AUTO_ZLL_VDD : RT8894A_I2C_AUTO_ZLL_NB, 0x26);
	
		
	RT8894ALockI2C(VRM->ParentGPU);
	
	return(VRM_ERROR_SUCCESS);
}


// RT8894A can only reside at a single address: 0x20
// TODO/FIXME: This could use more checks...

// Return value is how many devices were found.
uint32_t RT8894ADetect(AMDGPU *GPU, VRMController **VRMs)
{
	VRMController *CurrentVRM;
	
	AMDGPUI2CInit(GPU, STATIC_I2C_LINE_FIXME, 0x20);
	
	if(AMDSMBusReadByte(GPU, RT8894A_I2C_READ_PROD_ID, NULL) == 0x01)
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
		CurrentVRM->VRMType = VRM_CONTROLLER_TYPE_RT8894A;
		CurrentVRM->Capabilities = 0;
		
		CurrentVRM->OutputCount = 2;
		CurrentVRM->SelectedOutput = 0;
		
		CurrentVRM->I2CAddressList[0] = 0x20;
		
		CurrentVRM->GetVoltage = RT8894AGetVoltage;
		CurrentVRM->SetVoltage = RT8894ASetVoltage;
		CurrentVRM->GetOutputIdx = RT8894AGetOutputIdx;
		CurrentVRM->SetOutputIdx = RT8894ASetOutputIdx;
		
		CurrentVRM->next = NULL;
		return(1);
	}
	
	return(0);
}

