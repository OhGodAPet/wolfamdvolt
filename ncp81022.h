#ifndef __NCP81022_H
#define __NCP81022_H

#include "vrm.h"

#define NCP81022_PMBUS_IOUT_CAL_GAIN				0x38
#define NCP81022_PMBUS_IOUT_OFFSET					0x39
#define NCP81022_PMBUS_VRCONFIG1_REG				0xD2
#define NCP81022_PMBUS_VRCONFIG2_REG				0xD3
#define NCP81022_PMBUS_LOADLINE_REG					0xE4
#define NCP81022_PMBUS_SPOFFSET_REG					0xE6
#define NCP81022_READ_VOUT_LINEAR_REG				0xD4

uint32_t NCP81022SwitchControls(VRMController *VRM, bool SMBusControl);
uint32_t NCP81022Detect(AMDGPU *GPU, VRMController **VRMs);
uint32_t NCP81022GetOffset(VRMController *VRM, float *VoltOffset);
uint32_t NCP81022SetOffset(VRMController *VRM, float Voltage);
uint32_t NCP81022GetOutputIdx(VRMController *VRM, uint32_t *Idx);
uint32_t NCP81022SetOutputIdx(VRMController *VRM, uint32_t Idx);
uint32_t NCP81022GetVoltage(VRMController *VRM, float *VDDC);
uint32_t NCP81022SetVoltage(VRMController *VRM, float Voltage);
float NCP81022GetInputVoltage(AMDGPU *GPU, int *ret);
float NCP81022GetOutputPower(AMDGPU *GPU, int *ret);
uint32_t NCP81022GetOutputCurrent(VRMController *VRM, float *Current);
uint32_t NCP81022SetLoadLine(VRMController *VRM, uint8_t Setting);

#endif
