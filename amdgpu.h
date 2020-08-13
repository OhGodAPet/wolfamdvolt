#ifndef __AMDGPU_H
#define __AMDGPU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
typedef int HANDLE;
#define INVALID_HANDLE_VALUE		-1
#endif

typedef struct _AMDGPU AMDGPU;

#include "vrm.h"

// No support for mobile GPUs
static const uint16_t HAWAII_PCI_DEVICE_IDS[] = { 	0x67A0, 0x67A1, 0x67A2, 0x67A8,
													0x67A9, 0x67AA, 0x67B0, 0x67B1,
													0x67B8, 0x67B9, 0x67BA, 0x67BE };

static const uint16_t BONAIRE_PCI_DEVICE_IDS[] = {  0x6649, 0x6650, 0x6651, 0x6658,
													0x665c, 0x665d, 0x665f };

static const uint16_t VEGA_PCI_DEVICE_IDS[] =	{	0x6863, 0x687F };

typedef struct _I2CPINS
{
	uint16_t SDA;
	uint16_t SCL;
} I2CPINS;

typedef struct _I2CGPIO
{
	uint16_t mmGENERIC_I2C_CONTROL;
	uint16_t mmGENERIC_I2C_INTERRUPT_CONTROL;
	uint16_t mmGENERIC_I2C_STATUS;
	uint16_t mmGENERIC_I2C_SPEED;
	uint16_t mmGENERIC_I2C_SETUP;
	uint16_t mmGENERIC_I2C_TRANSACTION;
	uint16_t mmGENERIC_I2C_DATA;
	uint16_t mmGENERIC_I2C_PIN_SELECTION;
	uint16_t mmGENERIC_I2C_PIN_DEBUG;
} I2CGPIO;

struct _AMDGPU
{
	int fd;
	bool I2CHas16BitEntries;
	void *MMIOBasePtr;

	#ifdef _WIN32
	HANDLE MMIOHandle;
	#endif
	
	I2CPINS *I2CPins;
	I2CGPIO I2CGPIOAddrs;
	VRMController *VRMs;
	
	uint32_t MMIOBaseAddr, MMIOSize, VRMCount;
	uint8_t Revision, I2CAddress, I2CLine, I2CControlOffset;
	uint16_t VendorID, DeviceID, SubVendor, SubDevice, PCIBus, PCIDevice, PCISlot, PCIFunction;
	struct _AMDGPU *next, *prev;
};

#ifdef _WIN32

#pragma pack(push, 1)

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef struct _ReadPCIConfigData
{
	uint8_t Offset;
	uint8_t SlotNumber;
	uint8_t BusNumber;
	uint8_t Padding;
	uint32_t ReadSize;
} ReadPCIConfigData;

typedef struct _WritePCIConfigData
{
	uint8_t Offset;
	uint8_t SlotNumber;
	uint8_t BusNumber;
	uint8_t Padding;
	uint32_t WriteSize;
	uint32_t Value;
} WritePCIConfigData;

typedef struct _ReadPhysMemData
{
	PHYSICAL_ADDRESS PhysMemAddr;
	uint64_t ReadSize;			// Only 1, 2 or 4 allowed.
} ReadPhysMemData;

typedef struct _WritePhysMemData
{
	PHYSICAL_ADDRESS PhysMemAddr;
	uint64_t WriteSize;			// Only 1, 2, or 4 allowed.
	uint64_t Value;
} WritePhysMemData;

#pragma pack(pop)

#endif

int FindAMDGPUs(AMDGPU **GPUList, HANDLE DrvHandle);
int InitAMDGPUMMIO(AMDGPU *GPU);
void AMDGPUFreeVRMs(AMDGPU *GPU);
void CloseAMDGPUMMIO(AMDGPU *GPU);
void ReleaseAMDGPUs(AMDGPU *GPUList);

uint32_t ReadMMIOReg(AMDGPU *GPU, uint32_t reg);
void WriteMMIOReg(AMDGPU *GPU, uint32_t reg, uint32_t value);

bool AMDGPUIsHawaii(AMDGPU *GPU);
bool AMDGPUIsBonaire(AMDGPU *GPU);
bool AMDGPUIsVega(AMDGPU *GPU);

#endif
