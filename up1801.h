#ifndef __UP1801_H
#define __UP1801_H

// NOTE: No support for programming initial voltage at this time
// TODO/FIXME: Add current limit and VID1/VID0 readbacks
// TODO/FIXME: MISC reg and its definitions

#define UP1801_CHIP_ID_VALUE			0x12
#define UP1801_ILIM_INIT_SETTING		0x00

#define UP1801_ILIM_DISABLED_OUTPUT		(1 << 5)
#define UP1801_ILIM_PVI_SETTING			(1 << 6)
#define UP1801_ILIM_SVI_SETTING			(1 << 7)

#define UP1801_INI_REG					0x8B
#define UP1801_CHIP_ID_REG				0xD0
#define UP1801_SVI_REG					0xD2
#define UP1801_ILIM_REG					0xD3
#define UP1801_LEVEL1_REG				0xD4
#define UP1801_LEVEL2_REG				0xD5
#define UP1801_LEVEL3_REG				0xD6
#define UP1801_LEVEL4_REG				0xD7


uint32_t uP1801Detect(AMDGPU *GPU, VRMController **VRMs);
uint32_t uP1801GetVoltage(VRMController *VRM, float *VDDC);
uint32_t uP1801SetVoltage(VRMController *VRM, float Voltage);

#endif
