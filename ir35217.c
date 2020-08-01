#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "ir35217.h"
#include "vrm.h"

#define IR35217_VID_STEP_MV		5

/*
// PMBus method
uint32_t IR35217GetVoltage(VRMController *VRM, float *VDDC)
{
	int16_t EncodedVolts, VOUTMode;
	uint8_t Addr = VRM->I2CAddressList[VRM->SelectedOutput + 1];

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, Addr);

	// VID Mode
	VOUTMode = AMDSMBusReadByte(VRM->ParentGPU, PMBUS_VOUT_MODE, NULL);
	//printf("VOUTMode = %d\n", VOUTMode);
	// If it's not in linear format, it should be.
	if(VOUTMode >> 5) return(VRM_ERROR_UNKNOWN);

	EncodedVolts = AMDSMBusReadWord(VRM->ParentGPU, PMBUS_READ_VOUT, NULL);
	*VDDC = PMBusDecodeLinearValueWithExponent(EncodedVolts, VOUTMode);

	//*VDDC = EncodedVolts * 0.0005;
	return(VRM_ERROR_SUCCESS);
}
*/

uint32_t IR35217GetVoltage(VRMController *VRM, float *VDDC)
{
	uint8_t VID;

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);

	VID = AMDI2CReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? IR35217_GET_VOLTAGE_LOOP1_REG : IR35217_GET_VOLTAGE_LOOP2_REG, NULL);

	*VDDC = 1.55 - (VID * 0.00625);

	return(VRM_ERROR_SUCCESS);
}

/*
uint32_t IR35217SetVoltage(VRMController *VRM, float Voltage)
{
	uint8_t VID = (uint8_t)((Voltage / 0.00625) + 0.5);

	AMDGPUI2CInit(VRM->ParentGPU, VRM->I2CAddressList[0], NULL);
	AMDI2CWriteByte(VRM->ParentGPU, IR35217_SET_VOLTAGE_LOOP1_REG, VID);

	return(VRM_ERROR_SUCCESS);
}*/


uint32_t IR35217SetVoltage(VRMController *VRM, float Voltage)
{
	int16_t EncodedVolts, VOUTMode;
	uint8_t Addr = VRM->I2CAddressList[VRM->SelectedOutput + 1];

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, Addr);

	// VID Mode
	VOUTMode = AMDSMBusReadByte(VRM->ParentGPU, PMBUS_VOUT_MODE, NULL);

	// If it's not in linear format, it should be.
	if(VOUTMode >> 5) return(VRM_ERROR_UNKNOWN);
	uint16_t RawVolts = PMBusEncodeValueToLinearWithExponent(VOUTMode, Voltage);

	//uint16_t RawVolts = (uint16_t)((Voltage * 0.005) + 0.5);

	AMDSMBusWriteWord(VRM->ParentGPU, PMBUS_VOUT_COMMAND, 0x0083);


	return(VRM_ERROR_SUCCESS);
}

uint32_t IR35217GetOutputIdx(VRMController *VRM, uint32_t *Idx)
{
	*Idx = VRM->SelectedOutput;
	return(VRM_ERROR_SUCCESS);
}

uint32_t IR35217SetOutputIdx(VRMController *VRM, uint32_t Idx)
{
	if(Idx >= VRM->OutputCount) return(VRM_ERROR_RANGE);

	VRM->SelectedOutput = Idx;
	return(VRM_ERROR_SUCCESS);
}

uint32_t IR35217GetOffset(VRMController *VRM, float *VoltOffset)
{
	int8_t VID;

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);

	VID = AMDI2CReadByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? IR35217_SET_VOLTAGE_OFFSET_LOOP1_REG : IR35217_SET_VOLTAGE_OFFSET_LOOP2_REG, NULL);

	*VoltOffset = VID * 0.00625;
	return(VRM_ERROR_SUCCESS);
}

uint32_t IR35217SetOffset(VRMController *VRM, float Voltage)
{
	if((Voltage >= VRM->MaxOffset) || (Voltage <= VRM->MinOffset)) return(VRM_ERROR_RANGE);

	uint8_t VOffset = (Voltage > 0) ? (Voltage / 0.00625) + 0.5 : (Voltage / 0.00625) - 0.5;

	AMDGPUI2CInit(VRM->ParentGPU, STATIC_I2C_LINE_FIXME, VRM->I2CAddressList[0]);

	AMDI2CWriteByte(VRM->ParentGPU, (!VRM->SelectedOutput) ? IR35217_SET_VOLTAGE_OFFSET_LOOP1_REG : IR35217_SET_VOLTAGE_OFFSET_LOOP2_REG, VOffset);

	return(VRM_ERROR_SUCCESS);
}

// Return value is how many devices were found.
// IR35217 can be found ANYWHERE in the I2C address space
// BUT - the PMBus interface can only be found in certain
// I2C locations - addresses 0x40 - 0x47, 0x70 - 0x77,
// and 0x0D. From the PMBus interface, we can read the
// I2C interface's location.

uint32_t IR35217Detect(AMDGPU *GPU, VRMController **VRMs)
{
	uint8_t Addresses[17] = { 	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
								0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x0D };

	uint32_t DevicesFound = 0;
	for(int i = 0; i < 17; ++i)
	{
		uint8_t len, blk[0x100], Model;
		AMDGPUI2CInit(GPU, STATIC_I2C_LINE_FIXME, Addresses[i]);

		// First things first - a block read could confuse some
		// devices that may be on the bus. Let's verify it's a
		// PMBus device before proceeding.
		uint32_t myret;
		blk[0] = AMDSMBusReadByte(GPU, PMBUS_REVISION, &myret);

		// IR35217 will support at LEAST PMBus
		// Part I rev. 1.1 and PMBus Part II rev. 1.2.
		if(((blk[0] & 0x0F) < 0x02) && ((blk[0] >> 4) < 0x01)) continue;

		// PMBUS_MFR_ID isn't set on all IR35217-compatibles.
		// So, we'll do a (hopefully non-intrusive) poke -
		// doing a block read on PMBUS_MFR_MODEL.

		AMDSMBusReadBlock(GPU, PMBUS_MFR_MODEL, &len, blk);

		if(len == 1)
		{
			int idx = 0;
			Model = blk[0];

			// Compare it to the IR35217 model...
			if(Model != MODEL_IR35217) continue;

			// Highest bit is a toggle for enabling/disabling the I2C bus
			// so it needs to be masked to get the address.
			uint8_t I2CAddr = AMDSMBusReadByte(GPU, IR35217_PMBUS_SET_I2C, NULL) & 0x7F;
			VRMController *CurrentVRM;

			// If the address is 0x00, then it's tied to the PMBus address
			if(!I2CAddr)
			{
				switch(Addresses[i])
				{
					case 0x40:
					case 0x41:
					case 0x42:
					case 0x43:
					case 0x44:
					case 0x45:
					case 0x46:
					case 0x47:
						I2CAddr = Addresses[i] - 24;
						break;
					case 0x70:
					case 0x71:
					case 0x72:
					case 0x73:
					case 0x74:
					case 0x75:
					case 0x76:
					case 0x77:
						I2CAddr = Addresses[i] - 64;
						break;
					case 0x0D:
						I2CAddr = 0x0A;
						break;
				}
			}

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
			CurrentVRM->VRMType = VRM_CONTROLLER_TYPE_IR35217;
			CurrentVRM->Capabilities = 0;

			CurrentVRM->MinOffset = -0.8;
			CurrentVRM->MaxOffset = 0.8;

			CurrentVRM->OutputCount = 2;
			CurrentVRM->SelectedOutput = 0;

			CurrentVRM->Model = Model;

			// I2C one first, then PMBus ones
			CurrentVRM->I2CAddressList[0] = I2CAddr;
			CurrentVRM->I2CAddressList[1] = Addresses[i];
			CurrentVRM->I2CAddressList[2] = Addresses[i] + 1;

			// Skip past our secondary output.
			++i;

			if(I2CDebugOutput)
				printf("IR35217 found with I2C at 0x%02X and PMBus at 0x%02X and 0x%02X.\n", I2CAddr, Addresses[i], Addresses[i] + 1);

			CurrentVRM->GetVoltage = IR35217GetVoltage;
			CurrentVRM->SetVoltage = IR35217SetVoltage;
			CurrentVRM->GetOutputIdx = IR35217GetOutputIdx;
			CurrentVRM->SetOutputIdx = IR35217SetOutputIdx;

			CurrentVRM->next = NULL;
			DevicesFound++;
		}
	}

	return(DevicesFound);
}
