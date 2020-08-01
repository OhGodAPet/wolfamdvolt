#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "amdgpu.h"
#include "vbios-tables.h"

// If this fails, GPU->VBIOS should be NULL, and VBIOSSize should be zero.
void GetVBIOSImage(AMDGPU *GPU, uint32_t ROMSize, uint32_t ROMBase)
{
	void *VBIOSImg;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	
	if(fd == -1) return;
	
	VBIOSImg = (void *)mmap(NULL, ROMSize, PROT_READ, MAP_PRIVATE, fd, ROMBase);
	
	if(VBIOSImg == MAP_FAILED)
	{
		close(fd);
		return;
	}
	
	GPU->VBIOSSize = ROMSize;
	GPU->VBIOS = malloc(ROMSize);
	memcpy(GPU->VBIOS, VBIOSImg, ROMSize);
	
	munmap(VBIOSImg, ROMSize);
	close(fd);
}

// TODO: This structure needs to be adapted for ALL entry types, as well as moved somewhere else.
// TODO: We should also care about the format and content revisions...
typedef struct _VOIEntry
{
	uint8_t VoltageType;
	uint8_t VoltageMode;
	uint16_t Size;
	uint8_t RegulatorID;
	uint8_t I2CLine;
	uint8_t I2CAddress;
	uint8_t ControlOffset;
	uint8_t VoltageControlFlag;
	uint8_t Reserved[3];
} VOIEntry;

// TODO: Extend this to allow for multiple controllers, etc
// The information this provides should stay zero in the case this fails
void GetI2CInfo(AMDGPU *GPU)
{
	uint16_t VOISize, Position = 0;
	
	if(!GPU->VBIOS) return;
	
	ATOM_ROM_HEADER *hdr = GPU->VBIOS + ((uint16_t *)(GPU->VBIOS + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER))[0];
	ATOM_MASTER_LIST_OF_DATA_TABLES *DataTblList = &((ATOM_MASTER_DATA_TABLE *)(GPU->VBIOS + hdr->usMasterDataTableOffset))->ListOfDataTables;
	
	VOISize = *((uint16_t *)(GPU->VBIOS + DataTblList->VoltageObjectInfo));
	VOIEntry *Entry = (VOIEntry *)(GPU->VBIOS + DataTblList->VoltageObjectInfo + 4);
		
	while(Position < VOISize)
	{
		if(Entry->VoltageMode == 3)			// INIT_REGULATOR
		{
			GPU->I2CLine = Entry->I2CLine;
			GPU->I2CAddress = Entry->I2CAddress >> 1;
			GPU->I2CControlOffset = Entry->ControlOffset;
			GPU->I2CHas16BitEntries = (bool)Entry->VoltageControlFlag;
			break;
		}
		
		Position += Entry->Size;
		Entry = (VOIEntry *)(((uint8_t *)Entry) + Entry->Size);
	}
}
	
