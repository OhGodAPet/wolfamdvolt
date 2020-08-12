#pragma once

#include <windows.h>
#include <stdint.h>

#pragma pack(push, 1)

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef struct _AMDIOPortReadArgs
{
	uint32_t SrcPort;
	uint32_t ReadSize;
} AMDIOPortReadArgs;

typedef struct _AMDMemRWArgs
{
	LARGE_INTEGER PhysMemAddr;
	uint64_t OperationSize;			// Only 1, 2, or 4 allowed
	uint64_t DestAddr;
} AMDMemRWArgs;

typedef struct _AMDIOPortWriteArgs
{
	uint32_t DestPort;
	uint32_t WriteSize;
	uint32_t WriteValue;
} AMDIOPortWriteArgs;

typedef struct _AMDMemRegionRWArgs
{
	LARGE_INTEGER PhysMemAddr;
	uint64_t MemRegionSize;
	uint64_t DestAddr;
	uint64_t CachingType;
} AMDMemRegionRWArgs;

typedef struct _AMDMSRAccessArgs
{
	uint32_t TargetMSR;
	uint64_t WriteValue;
} AMDMSRAccessArgs;

typedef struct _AMDMDLIOCTLRetVal
{
	void *MDLHandle;
	uint64_t MmMapLockedPagesRetVal;
} AMDMDLIOCTLRetVal;

typedef struct _AMDMapMDLIOCTLArgs
{
	LARGE_INTEGER PhysMemAddr;
	uint64_t MemRegionSize;
	uint64_t CachingType;
} AMDMapMDLIOCTLArgs;

typedef struct _AMDFreeMDLIOCTLArgs
{
	LARGE_INTEGER PhysMemAddr;
	uint64_t MemRegionSize;
	uint64_t MDLAddr;
} AMDFreeMDLIOCTLArgs;

typedef struct _AMDReadMemRegionWithCustomCachingType
{
	LARGE_INTEGER PhysMemAddr;
	uint64_t MemRegionSize;
	uint64_t CachingType;
} AMDReadMemRegionWithCustomCachingType;

union _AMDRetVal
{
	uint8_t ByteReturn;
	uint16_t WordReturn;
	DWORD DwordReturn;
	uint64_t QwordReturn;
} AMDRetVal;

typedef struct _AMDPCIConfigSpaceReadArgs
{
	uint8_t Offset;
	uint8_t SlotNum;
	uint8_t BusNum;
	uint8_t Padding;
	uint32_t AccessSize;
} AMDPCIConfigSpaceReadArgs;

typedef struct _AMDPCIConfigSpaceWriteArgs
{
	uint8_t Offset;
	uint8_t SlotNum;
	uint8_t BusNum;
	uint8_t Padding;
	uint32_t AccessSize;
	uint32_t WriteValue;
} AMDPCIConfigSpaceWriteArgs;

enum AMD_ATILLK64_IOCTL_CODES
{
	ATILLK64_IOCTL_READ_MEM_REGION_WITH_CUSTOM_CACHING_ARGS = 0x9C402568,
	ATILLK64_IOCTL_WRITE_MEM_REGION_WITH_CUSTOM_CACHING_ARGS = 0x9C40256C,
	ATILLK64_IOCTL_X86_IN_FROM_PORT_INSTRUCTION = 0x9C40252C,
	ATILLK64_IOCTL_X86_OUT_TO_PORT_INSTRUCTION = 0x9C402530,
	ATILLK64_IOCTL_READ_PHYS_MEM = 0x9C402534,
	ATILLK64_IOCTL_WRITE_PHYS_MEM = 0x9C402538,
	ATILLK64_IOCTL_READ_PCI_CONFIG_SPACE = 0x9C40253C,
	ATILLK64_IOCTL_WRITE_PCI_CONFIG_SPACE = 0x9C402540,
	ATILLK64_IOCTL_READ_MEM_REGION = 0x9C402544,
	ATILLK64_IOCTL_WRITE_MEM_REGION = 0x9C402548,
	ATILLK64_IOCTL_MAP_FROM_NON_PAGED_POOL = 0x9C40254C,
	ATILLK64_IOCTL_RDMSR = 0x9C402550,
	ATILLK64_IOCTL_WRMSR = 0x9C402554,
	ATILLK64_IOCTL_MDL_FROM_NON_PAGED_POOL_MAPPING = 0x9C402558,
	ATILLK64_IOCTL_FREE_MDL = 0x9C40255C,
	ATILLK64_IOCTL_MAP_FROM_NON_PAGED_POOL_WITH_CUSTOM_CACHING_ARGS = 0x9C402560,
	ATILLK64_IOCTL_LOCK_REGION_TO_NON_PAGED_POOL = 0x9C402564
};


#pragma pack(pop)
