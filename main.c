#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wolfamdvolt.h"
#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "ir3xxxx.h"
#include "ncp81022.h"
#include "up9505.h"
#include "up1801.h"
#include "rt8894a.h"

/*
void DumpGPUInfo(AMDGPU *GPU)
{
	int ret;
	int8_t off;
	uint8_t byte;
		
	if(IR3XXXXDetect(GPU, NULL))
	{
		uint8_t incmdcode, outlen, out[0x100];
		
		printf("\tDetected IR3XXXX!\n");
		
		printf("\tLoop 1 voltage: %.4fV\n", IR3XXXXGetVoltage(GPU, NULL));
		printf("\tLoop 1 current: %dA\n", IR3XXXXGetCurrent(GPU, NULL));
		printf("\tLoop 1 temp: %dC\n", IR3XXXXGetTemp(GPU, NULL));
		printf("\tLoop 2 voltage: %.4fV\n", AMDI2CReadByte(GPU, IR3XXXX_GET_VOLTAGE_LOOP2_REG, NULL) * 0.0078125);
		
		printf("\tLoop 1 voltage offset: %.4fV\n\n", IR3XXXXGetOffset(GPU, NULL));
	}	
	
	if(NCP81022Detect(GPU, NULL))
	{
		printf("\tDetected NCP81022!\n");
		
		//NCP81022SwitchControls(GPU, true);
		NCP81022SelectRail(GPU, 0);
		
		printf("\tLoop 1 voltage: %.4fV\n", NCP81022GetVoltage(GPU, NULL));
		NCP81022SelectRail(GPU, 1);
		
		printf("\tLoop 2 voltage: %.4fV\n", NCP81022GetVoltage(GPU, NULL));
		NCP81022SelectRail(GPU, 0);
		
		printf("\tIOUT_CAL_GAIN: 0x%04X\n", AMDSMBusReadWord(GPU, NCP81022_PMBUS_IOUT_CAL_GAIN, NULL));
		printf("\tIOUT_OFFSET: 0x%04X\n", AMDSMBusReadWord(GPU, NCP81022_PMBUS_IOUT_OFFSET, NULL));
		printf("\tInput voltage: %.4fV\n", NCP81022GetInputVoltage(GPU, NULL));
		
		printf("\tLoop 1 Output Power: %.4fW\n\n", NCP81022GetOutputPower(GPU, NULL));
		printf("\tLoop 1 Output Current: %.4fA\n", NCP81022GetOutputCurrent(GPU, NULL));
		printf("\tLoop 1 Input Voltage: %.4fV\n", NCP81022GetInputVoltage(GPU, NULL));
		//NCP81022SwitchControls(GPU, false);
	}
	
	if(uP9505Detect(GPU, NULL))
	{
		printf("\tDetected uP9505!\n");
		printf("\tLoop 1 voltage: %.4f\n", uP9505GetVoltage(GPU, NULL));
		printf("\tLoop 2 voltage: %.4f\n", uP9505GetVDDAVoltage(GPU, NULL));
		printf("\tLoop 1 temp: %dC\n\n", uP9505GetTemp(GPU, NULL));
	}
	
	if(RT8894ADetect(GPU, NULL))
	{
		printf("\tDetected RT8894A!\n");
		printf("\tLoop 1 voltage: %.4f\n", RT8894AGetVoltage(GPU, 0, NULL));
		printf("\tLoop 2 voltage: %.4f\n", RT8894AGetVoltage(GPU, 1, NULL));
	}
	
	if(uP1801Detect(GPU, NULL))
	{
		printf("\tDetected uP1801!\n");
		printf("\tVoltage: %.4f\n\n", uP1801GetVoltage(GPU, NULL));
	}
	
}
*/

void DumpGPUInfo(AMDGPU *GPU, ArgsObj Config)
{
	VRMController *CurrentVRM;
	uint32_t VRMIdx = 0;
	
	printf("\tNumber of VRMs: %d\n", GPU->VRMCount);
	
	if(!GPU->VRMCount) return;
	
	for(CurrentVRM = GPU->VRMs; CurrentVRM; CurrentVRM = CurrentVRM->next, VRMIdx++)
	{
		if(!Config.Debug) printf("\tVRM %d: %s\n", VRMIdx, GetVRMName(CurrentVRM));
		else printf("\tVRM %d: %s (at 0x%02X)\n", VRMIdx, GetVRMName(CurrentVRM), CurrentVRM->I2CAddressList[0]);
		
		printf("\t\tNumber of outputs: %d\n", CurrentVRM->OutputCount);
		
		for(int OutputIdx = 0; OutputIdx < CurrentVRM->OutputCount; ++OutputIdx)
		{
			float CurVoltage;
			
			printf("\t\tOutput %d:\n", OutputIdx);
			
			CurrentVRM->SetOutputIdx(CurrentVRM, OutputIdx);
			
			if(CurrentVRM->GetVoltage(CurrentVRM, &CurVoltage) != VRM_ERROR_SUCCESS)
			{
				printf("\t\t\tFailure getting voltage from VRM.\n");
				continue;
			}
			
			printf("\t\t\tVoltage: %.4f\n", CurVoltage);
			
			if(CurrentVRM->Capabilities & VRM_CAPABILITY_OFFSET)
			{
				float CurOffset;
				
				if(CurrentVRM->GetVoltageOffset(CurrentVRM, &CurOffset) != VRM_ERROR_SUCCESS)
				{
					printf("\t\t\tFailure getting offset from VRM.\n");
					continue;
				}
				
				printf("\t\t\tOffset: %.4f\n", CurOffset);
			}
			
			if(CurrentVRM->Capabilities & VRM_CAPABILITY_TEMP)
			{
				uint32_t Temp;
				
				if(CurrentVRM->GetTemp(CurrentVRM, &Temp) != VRM_ERROR_SUCCESS)
				{
					printf("\t\t\tFailure getting temperature from VRM.\n");
					continue;
				}
				
				printf("\t\t\tTemp: %dC\n", Temp);
			}
			
			if(CurrentVRM->Capabilities & VRM_CAPABILITY_CURRENT)
			{
				float Amps;
				if(CurrentVRM->GetCurrent(CurrentVRM, &Amps) != VRM_ERROR_SUCCESS)
				{
					printf("\t\t\tFailure getting current from VRM.\n");
					continue;
				}
				
				printf("\t\t\tAmps: %.2fA\n\t\t\tWatts: %.4fW\n", Amps, ((float)Amps) * CurVoltage);
			}
			
			if(CurrentVRM->Capabilities & VRM_CAPABILITY_LOADLINE)
			{
				if(CurrentVRM->VRMType == VRM_CONTROLLER_TYPE_NCP81022 && VRMIdx == 3)
					CurrentVRM->SetLoadLine(CurrentVRM, 0x00);
			}
		}
	}
}

#ifdef __linux__
HANDLE OpenDriverHandle(void)
{
	return(INVALID_HANDLE_VALUE);
}
#elif defined(_WIN32)
HANDLE OpenDriverHandle(void)
{
	return(CreateFileA("\\\\.\\atillk64", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
}
#endif

int main(int argc, char **argv)
{
	uint8_t byte;
	int ret, i, GPUCount, VRMIdx, VRMOutputIdx;
	ArgsObj Config;
	AMDGPU *GPUList, *CurGPU;
	HANDLE DrvHandle = INVALID_HANDLE_VALUE;
	
	printf("wolfamdvolt %s\n", WOLFAMDVOLT_VERSION_STR);
	
	memset(&Config, 0x00, sizeof(ArgsObj));
	if(!ParseCmdLine(&Config, argc, argv)) return(-1);
	
	VRMIdx = Config.VRMIdxProvided ? Config.VRMIdx : 0;
	VRMOutputIdx = Config.VRMOutputIdxProvided ? Config.VRMOutputIdx : 0;
	
	I2CDebugOutput = Config.Debug;
	DrvHandle = OpenDriverHandle();
	GPUCount = FindAMDGPUs(&GPUList, DrvHandle);
	
	if(!Config.GPUIdxProvided)
	{	
		for(CurGPU = GPUList, i = 0; CurGPU; CurGPU = CurGPU->next, ++i)
		{
			ret = InitAMDGPUMMIO(CurGPU);
			
			if(ret < 0)
			{
				printf("Unable to initialize MMIO for GPU - failed with %d.\n", ret);
				return(-1);
			}
			
			DetectVRMControllers(CurGPU);
			
			if(!Config.Debug) printf("GPU %d:\n", i);
			else printf("GPU %d - (%02X:%02X:%02X - DID: 0x%04X - SSVID: 0x%04X - SSDID: 0x%04X):\n", i, CurGPU->PCIBus, CurGPU->PCIDevice, CurGPU->PCIFunction, CurGPU->DeviceID, CurGPU->SubVendor, CurGPU->SubDevice);
			DumpGPUInfo(CurGPU, Config);			
		}
	}
	
	if(Config.GPUIdxProvided && (!Config.SetVoltage && !Config.SetVoltageOffset))
	{
		uint32_t idx = Config.GPUIdx;
				
		if(idx >= GPUCount)
		{
			printf("GPU index out of range.\n");
			ReleaseAMDGPUs(GPUList);
			return(-2);
		}
		
		CurGPU = GPUList;
		for(int i = 0; i < idx; ++i) CurGPU = CurGPU->next;
		
		ret = InitAMDGPUMMIO(CurGPU);
		
		if(ret < 0)
		{
			printf("Unable to initialize MMIO for GPU - failed with %d.\n", ret);
			return(-1);
		}
		
		DetectVRMControllers(CurGPU);
		
		if(!Config.Debug) printf("GPU %d:\n", idx);
		else printf("GPU %d - (%02X:%02X:%02X - DID: 0x%04X - SSVID: 0x%04X - SSDID: 0x%04X):\n", idx, CurGPU->PCIBus, CurGPU->PCIDevice, CurGPU->PCIFunction, CurGPU->DeviceID, CurGPU->SubVendor, CurGPU->SubDevice);
		
		DumpGPUInfo(CurGPU, Config);
	}
	
	if(Config.GPUIdxProvided && Config.SetVoltage)
	{
		uint32_t idx = Config.GPUIdx;
		float CurVoltage;
		
		if(idx >= GPUCount)
		{
			printf("GPU index out of range.\n");
			ReleaseAMDGPUs(GPUList);
			return(-2);
		}
		
		CurGPU = GPUList;
		
		for(int i = 0; i < idx; ++i) CurGPU = CurGPU->next;
		
		ret = InitAMDGPUMMIO(CurGPU);
		
		if(ret < 0)
		{
			printf("Unable to initialize MMIO for GPU - failed with %d.\n", ret);
			return(-1);
		}
		
		DetectVRMControllers(CurGPU);
		
		if(!CurGPU->VRMCount)
		{
			printf("No recognized/supported VRMs found.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		if(VRMIdx >= CurGPU->VRMCount)
		{
			printf("Selected VRM index does not exist.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		VRMController *CurVRM = CurGPU->VRMs;
		
		for(int i = 0; i < VRMIdx && CurVRM; ++i) CurVRM = CurVRM->next;
		
		if(CurVRM->VRMType == VRM_CONTROLLER_TYPE_NCP81022)
			NCP81022SwitchControls(CurVRM, true);
		else if(CurVRM->VRMType == VRM_CONTROLLER_TYPE_UP9505)
			uP9505Acquire(CurVRM);
		
		if(VRMOutputIdx >= CurVRM->OutputCount)
		{
			printf("Selected VRM output does not exist on selected VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		if(!(CurVRM->Capabilities & VRM_CAPABILITY_SET_VOLTAGE))
		{
			printf("Selected VRM does not support direct voltage setting. Try using an offset instead.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		CurVRM->SetOutputIdx(CurVRM, VRMOutputIdx);
		
		if(CurVRM->GetVoltage(CurVRM, &CurVoltage) != VRM_ERROR_SUCCESS)
		{
			printf("Failure getting voltage from VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		printf("Voltage is %.4f - attempting to set %.4f.\n", CurVoltage, Config.RequestedVoltage);
		
		if(CurVRM->SetVoltage(CurVRM, Config.RequestedVoltage) != VRM_ERROR_SUCCESS)
		{
			printf("Failure setting voltage.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		if(CurVRM->GetVoltage(CurVRM, &CurVoltage) != VRM_ERROR_SUCCESS)
		{
			printf("Failure getting voltage from VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		printf("Voltage is now %.4f.\n", CurVoltage);
		
		CloseAMDGPUMMIO(CurGPU);
	}
	
	if(Config.GPUIdxProvided && Config.SetVoltageOffset)
	{
		uint32_t idx = Config.GPUIdx;
		float CurVoltage;
				
		if(idx >= GPUCount)
		{
			printf("GPU index out of range.\n");
			ReleaseAMDGPUs(GPUList);
			return(-2);
		}
		
		CurGPU = GPUList;
		
		for(int i = 0; i < idx; ++i) CurGPU = CurGPU->next;
		
		ret = InitAMDGPUMMIO(CurGPU);
		
		if(ret < 0)
		{
			printf("Unable to initialize MMIO for GPU - failed with %d.\n", ret);
			return(-1);
		}
		
		DetectVRMControllers(CurGPU);
		
		if(!CurGPU->VRMCount)
		{
			printf("No recognized/supported VRMs found.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		if(VRMIdx >= CurGPU->VRMCount)
		{
			printf("Selected VRM index does not exist.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		VRMController *CurVRM = CurGPU->VRMs;
		
		for(int i = 0; i < VRMIdx && CurVRM; ++i) CurVRM = CurVRM->next;
		
		if(CurGPU->VRMs->VRMType == VRM_CONTROLLER_TYPE_UP9505)
			uP9505Acquire(CurGPU->VRMs);
		
		if(VRMOutputIdx >= CurVRM->OutputCount)
		{
			printf("Selected VRM output does not exist on selected VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(0);
		}
		
		CurVRM->SetOutputIdx(CurVRM, VRMOutputIdx);
		
		if(CurVRM->GetVoltageOffset(CurVRM, &CurVoltage) != VRM_ERROR_SUCCESS)
		{
			printf("Failure getting voltage offset from VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		printf("Voltage offset is %.4f. Attempting to set %.4f...\n", CurVoltage, Config.RequestedVoltageOffset);
		
		if(CurVRM->SetVoltageOffset(CurVRM, Config.RequestedVoltageOffset) != VRM_ERROR_SUCCESS)
		{
			printf("Failure setting voltage.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		if(CurVRM->GetVoltageOffset(CurVRM, &CurVoltage) != VRM_ERROR_SUCCESS)
		{
			printf("Failure getting voltage offset from VRM.\n");
			ReleaseAMDGPUs(GPUList);
			return(-1);
		}
		
		printf("Voltage offset is %.4f.\n", CurVoltage);
		CloseAMDGPUMMIO(CurGPU);
	}
	
	ReleaseAMDGPUs(GPUList);
	
	return(0);
}
		
