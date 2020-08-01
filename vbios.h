#ifndef __VBIOS_H
#define __VBIOS_H

void GetVBIOSImage(AMDGPU *GPU, uint32_t ROMSize, uint32_t ROMBase);
void GetI2CInfo(AMDGPU *GPU);

#endif
