#ifndef __UP9505_H
#define __UP9505_H

#define UP9505_CHIP_ID_VALUE			0x29

#define UP9505_VOFS0_REG				0x0C
#define UP9505_VOFS1_REG				0x0D
#define UP9505_VOFS2_REG				0x0E
#define UP9505_VOFS3_REG				0x0F
#define UP9505_VOFS4_REG				0x10
#define UP9505_VOFS5_REG				0x11
#define UP9505_IICLL01_REG				0x15
#define UP9505_IICLL23_REG				0x16
#define UP9505_IICLL45_REG				0x17
#define UP9505_LCHVID_REG				0x20
#define UP9505_IMON_REG					0x21
#define UP9505_VFB_REG					0x22
#define UP9505_MISC1_REG				0x27
#define UP9505_MISC2_REG				0x28
#define UP9505_TM_REG					0x2F
#define UP9505_AVOFS0_REG				0x31
#define UP9505_AVOFS1_REG				0x32
#define UP9505_IMONA_REG				0x3A
#define UP9505_VFBA_REG					0x3B
#define UP9505_ALCHVID					0x3F
#define UP9505_ATM_REG					0x45
#define UP9505_VERSION_ID_REG			0x49
#define UP9505_CHIP_ID_REG				0x4A

uint32_t uP9505Acquire(VRMController *VRM);
uint32_t uP9505Detect(AMDGPU *GPU, VRMController **VRMs);
int uP9505SetVDDLoadLineAllPhases(AMDGPU *GPU, uint8_t Setting);
#endif
