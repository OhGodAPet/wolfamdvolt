#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __linux__
#include <pci/pci.h>
#include <sys/mman.h>
#endif

#include "amdgpu.h"
#include "vrm.h"

void InsertAfter(AMDGPU **List, AMDGPU *Node, AMDGPU *Prev)
{
	if(Prev)
	{
		Node->next = Prev->next;
		Prev->next = Node;
	}
	else
	{
		Node->next = *List;
		(*List)->prev = Node;
		*List = Node;
	}
	
	Node->prev = Prev;
	if(Node->next) Node->next->prev = Node;
}

void RemoveFromList(AMDGPU **List, AMDGPU *Node)
{
	if(Node->prev) Node->prev->next = Node->next;
	else *List = Node->next;
	
	if(Node->next) Node->next->prev = Node->prev;
	else Node->prev->next = NULL;
	
	Node->prev = Node->next = NULL;
}

void ListSwapEntries(AMDGPU **GPUList, AMDGPU *CurGPU, AMDGPU *TestGPU)
{
	AMDGPU *CurPrev = CurGPU->prev, *TestPrev = TestGPU->prev;
	
	if(CurPrev == TestGPU)
	{
		RemoveFromList(GPUList, CurGPU);
		InsertAfter(GPUList, CurGPU, TestPrev);
	}
	else if(TestPrev == CurGPU)
	{
		RemoveFromList(GPUList, TestGPU);
		InsertAfter(GPUList, TestGPU, CurPrev);
	}
	else
	{
		RemoveFromList(GPUList, CurGPU);
		RemoveFromList(GPUList, TestGPU);
		
		InsertAfter(GPUList, CurGPU, TestPrev);
		InsertAfter(GPUList, TestGPU, CurPrev);
	}
}

#ifdef __linux__

// Returns number indicating number of GPUs in list
// Returns number of GPUs, or negative on failure
int FindAMDGPUs(AMDGPU **GPUList, HANDLE DrvHandle)
{
	struct pci_access *PCI;
	struct pci_dev *Device;
	int GPUCount = 0;
	uint32_t tmp;
	AMDGPU *CurGPU;
	
	PCI = pci_alloc();
	pci_init(PCI);
	
	PCI->method = PCI_ACCESS_SYS_BUS_PCI;
	
	*GPUList = NULL;
	
	pci_scan_bus(PCI);
	
	for(Device = PCI->devices; Device; Device = Device->next)
	{
		// We only care about display adapters with AMD's vendor ID (0x1002)
		if(((Device->device_class & 0xFF00) >> 8) == PCI_BASE_CLASS_DISPLAY && Device->vendor_id == 0x1002)
		{
			if(!(*GPUList))
			{
				CurGPU = *GPUList = (AMDGPU *)calloc(1, sizeof(AMDGPU));
				CurGPU->prev = NULL;
			}
			else
			{
				
				CurGPU->next = (AMDGPU *)calloc(1, sizeof(AMDGPU));
				CurGPU->next->prev = CurGPU;
				CurGPU = CurGPU->next;
			}
			
			CurGPU->SubVendor = pci_read_word(Device, PCI_SUBSYSTEM_VENDOR_ID);
			CurGPU->SubDevice = pci_read_word(Device, PCI_SUBSYSTEM_ID);
			CurGPU->Revision = pci_read_byte(Device, PCI_REVISION_ID);
			CurGPU->MMIOBaseAddr = Device->base_addr[5] & 0xFFFFFFF0;
			CurGPU->MMIOSize = Device->size[5];
			CurGPU->VendorID = pci_read_word(Device, PCI_VENDOR_ID);
			CurGPU->DeviceID = pci_read_word(Device, PCI_DEVICE_ID);
			CurGPU->PCIDomain = Device->domain;
			CurGPU->PCIBus = Device->bus;
			CurGPU->PCIDevice = Device->dev;
			CurGPU->PCIFunction = Device->func;
			CurGPU->MMIOBasePtr = NULL;
			CurGPU->fd = -1;
									
			/*
			tmp = pci_read_long(Device, PCI_ROM_ADDRESS);
			tmp &= ~PCI_ROM_ADDRESS_MASK;
			tmp |= Device->rom_base_addr | PCI_ROM_ADDRESS_ENABLE;
			pci_write_long(Device, PCI_ROM_ADDRESS, tmp);
			
			GetVBIOSImage(CurGPU, Device->rom_size, Device->rom_base_addr & PCI_ROM_ADDRESS_MASK);
			
			tmp = pci_read_long(Device, PCI_ROM_ADDRESS);
			tmp &= ~PCI_ROM_ADDRESS_ENABLE;
			pci_write_long(Device, PCI_ROM_ADDRESS, Device->rom_base_addr);
			
			GetI2CInfo(CurGPU);
			*/
			
			GPUCount++;
		}
	}
	
	CurGPU->next = NULL;
	
	//ReorderListByHash(GPUList);
	//ReorderListByBus(GPUList);
	
	pci_cleanup(PCI);
	
	return(GPUCount);
}

#elif defined(_WIN32)

int FindAMDGPUs(AMDGPU **GPUList, HANDLE DrvHandle)
{
	WritePCIConfigData wrargs;
	ReadPCIConfigData args;
	int GPUCount = 0;
	
	args.Padding = 0x00;
	args.ReadSize = 4;
	
	for(int bus = 0; bus < 256; ++bus)
	{
		args.BusNumber = bus;
		
		for(int slot = 0; slot < 32; ++slot)
		{
			uint32_t BAR, BARSize;
			AMDGPU *CurrentGPU;
			uint16_t IDs[2];
			DWORD RetSize;
			BOOL ret;
			
			args.Offset = 0x00;
			args.SlotNumber = slot;
			
			ret = DeviceIoControl(DrvHandle, 0x9C40253C, (LPVOID)&args, (DWORD)sizeof(args), (LPVOID)IDs, (DWORD)sizeof(uint32_t), &RetSize, NULL);

			if(!ret)
			{
				char ErrorMsg[256];
				printf("DeviceIoControl returned error.\n");
				
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
				
				printf("Details: %s\n", ErrorMsg);
				continue;
			}
			else if((IDs[0] != 0x1002) && (IDs[0] != 0xFFFF))
			{
				//printf("Device with vendor ID 0x%02X at bus %d, slot %d.\n", IDs[0], bus, slot);
				continue;
			}
			else if(IDs[0] == 0xFFFF) continue;
			else if((IDs[1] >= 0xAAF0) && (IDs[1] <= 0xAAFF)) continue;
			
			//printf("Found device with ID 0x1002:0x%04X (AMD) at bus %d, slot %d.\n", IDs[1], bus, slot);

			if(!(*GPUList))
				*GPUList = CurrentGPU = (AMDGPU *)calloc(sizeof(AMDGPU), 1);
			else
				CurrentGPU = CurrentGPU->next = (AMDGPU *)calloc(sizeof(AMDGPU), 1);
			
			CurrentGPU->PCIBus = bus;
			CurrentGPU->PCISlot = slot;
			CurrentGPU->VendorID = IDs[0];
			CurrentGPU->DeviceID = IDs[1];

			args.Offset = 0x2C;
			DeviceIoControl(DrvHandle, 0x9C40253C, (LPVOID)&args, (DWORD)sizeof(args), (LPVOID)IDs, (DWORD)sizeof(uint32_t), &RetSize, NULL);
			
			CurrentGPU->SubVendor = IDs[0];
			CurrentGPU->SubDevice = IDs[1];

			args.Offset = 0x24;
			DeviceIoControl(DrvHandle, 0x9C40253C, (LPVOID)&args, (DWORD)sizeof(args), (LPVOID)&BAR, (DWORD)sizeof(uint32_t), &RetSize, NULL);
			
			CurrentGPU->MMIOBaseAddr = BAR & 0xFFFFFFF0UL;
			CurrentGPU->MMIOHandle = DrvHandle;
			CurrentGPU->next = NULL;
			
			wrargs.Offset = 0x24;
			wrargs.SlotNumber = slot;
			wrargs.BusNumber = bus;
			wrargs.Padding = 0x00;
			wrargs.WriteSize = 4;
			wrargs.Value = 0xFFFFFFFFUL;
			GPUCount++;
			
			ret = DeviceIoControl(DrvHandle, 0x9C402540, (LPVOID)&wrargs, (DWORD)sizeof(wrargs), (LPVOID)&BARSize, (DWORD)sizeof(uint32_t), &RetSize, NULL);

			if(!ret)
			{
				char ErrorMsg[256];
				printf("DeviceIoControl returned error.\n");
				
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
				
				printf("Details: %s\n", ErrorMsg);
				continue;
			}

			DeviceIoControl(DrvHandle, 0x9C40253C, (LPVOID)&args, (DWORD)sizeof(args), (LPVOID)&BARSize, (DWORD)sizeof(uint32_t), &RetSize, NULL);

			BARSize &= 0xFFFFFFF8;
			CurrentGPU->MMIOSize = (~BARSize) + 1;
			
			wrargs.Value = BAR;
			ret = DeviceIoControl(DrvHandle, 0x9C402540, (LPVOID)&wrargs, (DWORD)sizeof(wrargs), (LPVOID)&BARSize, (DWORD)sizeof(uint32_t), &RetSize, NULL);
		}
	}

	return(GPUCount);
}

#endif

#ifdef __linux__

// Returns 0 on success, negative on failure
int InitAMDGPUMMIO(AMDGPU *GPU)
{
	struct pci_access *PCI = NULL;
	struct pci_dev *Device = NULL;

	PCI = pci_alloc();
	pci_init(PCI);

	PCI->method = PCI_ACCESS_SYS_BUS_PCI;

	Device = pci_get_dev(PCI, GPU->PCIDomain, GPU->PCIBus, GPU->PCIDevice, GPU->PCIFunction);
	if (Device)
	{
		uint16_t cmd_state = pci_read_word(Device, PCI_COMMAND);
		if (0 == (cmd_state & PCI_COMMAND_MEMORY))
		{
			//No application or driver enabled PCI memory space for this device yet. Enable it now.
			pci_write_word(Device, PCI_COMMAND, cmd_state | PCI_COMMAND_MEMORY);
		}
		pci_free_dev(Device);
	}
	pci_cleanup(PCI);

	if(GPU->MMIOBasePtr) return(0);		// Check if already initialized
	
	GPU->fd = open("/dev/mem", O_RDWR | O_SYNC);
	
	if(GPU->fd == -1) return(-1);
	
	GPU->MMIOBasePtr = (void *)mmap(NULL, GPU->MMIOSize, PROT_READ | PROT_WRITE, MAP_SHARED, GPU->fd, GPU->MMIOBaseAddr);
	
	if(GPU->MMIOBasePtr == MAP_FAILED)
	{
		close(GPU->fd);
		GPU->fd = -1;
		GPU->MMIOBasePtr = NULL;
		return(-2);
	}
	
	return(0);
}

#elif defined(_WIN32)

int InitAMDGPUMMIO(AMDGPU *GPU)
{
	return(0);
}

#endif

bool AMDGPUIsHawaii(AMDGPU *GPU)
{
	for(int i = 0; i < (sizeof(HAWAII_PCI_DEVICE_IDS) / sizeof(HAWAII_PCI_DEVICE_IDS[0])); ++i)
	{
		if(HAWAII_PCI_DEVICE_IDS[i] == GPU->DeviceID) return(true);
	}
	
	return(false);
}

bool AMDGPUIsBonaire(AMDGPU *GPU)
{
	for(int i = 0; i < (sizeof(BONAIRE_PCI_DEVICE_IDS) / sizeof(BONAIRE_PCI_DEVICE_IDS[0])); ++i)
	{
		if(BONAIRE_PCI_DEVICE_IDS[i] == GPU->DeviceID) return(true);
	}
	
	return(false);
}

bool AMDGPUIsVega(AMDGPU *GPU)
{
	for(int i = 0; i < (sizeof(VEGA_PCI_DEVICE_IDS) / sizeof(VEGA_PCI_DEVICE_IDS[0])); ++i)
	{
		if(VEGA_PCI_DEVICE_IDS[i] == GPU->DeviceID) return(true);
	}
	
	return(false);
}

#ifdef __linux__

void CloseAMDGPUMMIO(AMDGPU *GPU)
{
	munmap(GPU->MMIOBasePtr, GPU->MMIOSize);
	GPU->MMIOBasePtr = NULL;
	GPU->MMIOSize = 0;
	
	close(GPU->fd);
	GPU->fd = -1;
}

#elif defined(_WIN32)

void CloseAMDGPUMMIO(AMDGPU *GPU)
{
	if(GPU->MMIOHandle != INVALID_HANDLE_VALUE) CloseHandle(GPU->MMIOHandle);
	GPU->MMIOHandle = INVALID_HANDLE_VALUE;
}

#endif

void ReleaseAMDGPUs(AMDGPU *GPUList)
{
	AMDGPU *NextGPU;
	
	for(AMDGPU *GPU = GPUList; GPU; GPU = NextGPU)
	{
		#ifdef __linux__
		if(GPU->MMIOBasePtr) CloseAMDGPUMMIO(GPU);
		#elif defined(_WIN32)
		if(GPU->MMIOHandle != INVALID_HANDLE_VALUE) CloseAMDGPUMMIO(GPU);
		#endif
		
		if(GPU->VRMs)
		{
			VRMController *NextVRM;
			for(VRMController *VRM = GPU->VRMs; VRM; VRM = NextVRM)
			{
				NextVRM = VRM->next;
				free(VRM);
			}
			
			GPU->VRMs = NULL;
			GPU->VRMCount = 0;
		}
		
		NextGPU = GPU->next;
		free(GPU);
	}
}

void AMDGPUFreeVRMs(AMDGPU *GPU)
{
	VRMController *NextVRM;
	
	if(!GPU->VRMs) return;
	
	for(VRMController *VRM = GPU->VRMs; VRM; VRM = NextVRM)
	{
		NextVRM = VRM->next;
		free(VRM);
	}
	
	GPU->VRMs = NULL;
	GPU->VRMCount = 0;
}

#ifdef __linux__

uint32_t _ReadMMIOReg(AMDGPU *GPU, uint32_t reg)
{
	// MMIO not initialized
	if(!GPU->MMIOBasePtr) return(0);
	
	// Register index is fucked
	if((reg << 2) >= GPU->MMIOSize) return(0);
	
	return(((uint32_t *)GPU->MMIOBasePtr)[reg]);
}

uint32_t ReadMMIOReg(AMDGPU *GPU, uint32_t reg)
{
	// MMIO not initialized
	if(!GPU->MMIOBasePtr) return(0);
	
	// Register index is fucked
	if((reg << 2) >= GPU->MMIOSize)
	{
		WriteMMIOReg(GPU, 0x00 * 4, reg);
		return(_ReadMMIOReg(GPU, 1));
	}
	else
	{
		return(_ReadMMIOReg(GPU, reg));
	}
}

void _WriteMMIOReg(AMDGPU *GPU, uint32_t reg, uint32_t value)
{
	// MMIO not initialized
	if(!GPU->MMIOBasePtr) return;
	
	// Register index is fucked
	if((reg << 2) >= GPU->MMIOSize) return;
	
	((uint32_t *)GPU->MMIOBasePtr)[reg] = value;
}

void WriteMMIOReg(AMDGPU *GPU, uint32_t reg, uint32_t value)
{
	// MMIO not initialized
	if(!GPU->MMIOBasePtr) return;
	
	// Register index is fucked
	if((reg << 2) >= GPU->MMIOSize)
	{
		_WriteMMIOReg(GPU, 0x00 * 4, reg);
		_WriteMMIOReg(GPU, 1, value);
	}
	else
	{
		_WriteMMIOReg(GPU, reg, value);
	}
	usleep(1);
}

#elif defined(_WIN32)

void WriteMMIOReg(AMDGPU *GPU, uint32_t reg, uint32_t value)
{
	WritePhysMemData wrargs;
	BOOL ret;
	
	if((GPU->MMIOHandle == INVALID_HANDLE_VALUE) || (!GPU->MMIOBaseAddr)) return;

	wrargs.PhysMemAddr.QuadPart = (uint64_t)GPU->MMIOBaseAddr;			// MM_INDEX
	wrargs.WriteSize = 4;
	wrargs.Value = reg << 2;

	ret = DeviceIoControl(GPU->MMIOHandle, 0x9C402538, (LPVOID)&wrargs, (DWORD)sizeof(wrargs), NULL, 0UL, NULL, NULL);
	
	if(!ret)
	{
		char ErrorMsg[256];
		printf("DeviceIoControl returned error.\n");
		
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
		
		printf("Details: %s\n", ErrorMsg);
	}

	wrargs.PhysMemAddr.QuadPart = (uint64_t)(GPU->MMIOBaseAddr + 4);
	wrargs.Value = value;
	
	ret = DeviceIoControl(GPU->MMIOHandle, 0x9C402538, (LPVOID)&wrargs, (DWORD)sizeof(wrargs), NULL, 0UL, NULL, NULL);
	
	if(!ret)
	{
		char ErrorMsg[256];
		printf("DeviceIoControl returned error.\n");
		
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
		
		printf("Details: %s\n", ErrorMsg);
	}

	Sleep(2);
}

uint32_t ReadMMIOReg(AMDGPU *GPU, uint32_t reg)
{
	WritePhysMemData wrargs;
	ReadPhysMemData rdargs;
	uint32_t retval;
	BOOL ret;
	
	if((GPU->MMIOHandle == INVALID_HANDLE_VALUE) || (!GPU->MMIOBaseAddr)) return(0);

	wrargs.PhysMemAddr.QuadPart = (uint64_t)GPU->MMIOBaseAddr;			// MM_INDEX
	wrargs.WriteSize = 4;
	wrargs.Value = reg << 2;

	ret = DeviceIoControl(GPU->MMIOHandle, 0x9C402538, (LPVOID)&wrargs, (DWORD)sizeof(wrargs), NULL, 0UL, NULL, NULL);
	
	if(!ret)
	{
		char ErrorMsg[256];
		printf("DeviceIoControl returned error.\n");
		
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
		
		printf("Details: %s\n", ErrorMsg);
	}

	Sleep(2);
	
	rdargs.PhysMemAddr.QuadPart = GPU->MMIOBaseAddr + 4;							// MM_DATA
	rdargs.ReadSize = 4;
	
	ret = DeviceIoControl(GPU->MMIOHandle, 0x9C402534, (LPVOID)&rdargs, (DWORD)sizeof(rdargs), (LPVOID)&retval, (DWORD)sizeof(uint32_t), NULL, NULL);
	
	if(!ret)
	{
		char ErrorMsg[256];
		printf("DeviceIoControl returned error.\n");
		
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, sizeof(ErrorMsg), NULL);
		
		printf("Details: %s\n", ErrorMsg);
	}

	return(retval);
}

#endif
