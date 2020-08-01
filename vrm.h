#ifndef __VRM_H
#define __VRM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct _VRMController VRMController;

#define VRM_CONTROLLER_TYPE_INVALID			0x00
#define VRM_CONTROLLER_TYPE_IR356XX			0x01
#define VRM_CONTROLLER_TYPE_UP6266			0x02

// This (probably) covers the uP9503P, uP9504, and uP9505
#define VRM_CONTROLLER_TYPE_UP9505			0x03

// NCP81022
#define VRM_CONTROLLER_TYPE_NCP81022		0x04

// uP1801, unlikely to be found alone, but usually a secondary controller
#define VRM_CONTROLLER_TYPE_UP1801			0x05

#define VRM_CONTROLLER_TYPE_RT8894A			0x06

// The pain-in-the-ass IR35217, found on current Vega (codename Vega10) GPUs.
#define VRM_CONTROLLER_TYPE_IR35217			0x07

// VRM Capabilities - these are masks, of course

// If offsets are supported, max/min offset is required.
#define VRM_CAPABILITY_SET_VOLTAGE			0x01
#define VRM_CAPABILITY_OFFSET				0x02
#define VRM_CAPABILITY_TEMP					0x04
#define VRM_CAPABILITY_CURRENT				0x08
#define VRM_CAPABILITY_LOADLINE				0x10

// VRM Errors - if an error occurs, the VRM controller driver code should
// return a non-success (nonzero) value
#define VRM_ERROR_SUCCESS					0x00
#define VRM_ERROR_RANGE						0x01			// Input value out of range
#define VRM_ERROR_I2C						0x02			// I2C failure
#define VRM_ERROR_UNSUPPORTED				0x03			// Unsupported feature
#define VRM_ERROR_UNKNOWN					0xFF

struct _VRMController
{
	// Filled in by caller of VRM init controller function,
	// that is, controller driver code
	AMDGPU *ParentGPU;
	
	uint8_t VRMType;
	uint32_t Capabilities;
	
	uint8_t OutputCount;
	uint8_t SelectedOutput;
	
	// TODO/FIXME: Min/max voltage entries
	float MaxOffset;
	float MinOffset;
	
	uint8_t I2CAddressList[8];		// This had better be enough, I don't want dynamic allocation in here.
	uint8_t Model;
	
	// Required.
	uint32_t (*GetVoltage)(struct _VRMController *VRM, float *VDDC);
	
	// MUST not be NULL if VRM_CAPABILITY_SET_VOLTAGE is supported.
	uint32_t (*SetVoltage)(struct _VRMController *VRM, float VDDC);

	// Required to support - even if they are no-ops.
	uint32_t (*GetOutputIdx)(struct _VRMController *VRM, uint32_t *idx);
	uint32_t (*SetOutputIdx)(struct _VRMController *VRM, uint32_t idx);
	
	// These two MUST be supported if offset and/or negative offset
	// support is signaled in the capabilities.
	
	// They MUST get the offset of the currently selected output.
	// If the output selected does not support it (but another does)
	// these functions must return VRM_ERROR_UNSUPPORTED.
	uint32_t (*GetVoltageOffset)(struct _VRMController *VRM, float *VDDCOff);
	uint32_t (*SetVoltageOffset)(struct _VRMController *VRM, float VDDCOff);
	
	// This MUST be supported if temperature support is signaled
	// in the capabilities dword.
	uint32_t (*GetTemp)(struct _VRMController *VRM, uint32_t *Temp);
	
	// MUST be supported if VRM_CAPABILITY_CURRENT is signaled
	uint32_t (*GetCurrent)(struct _VRMController *VRM, float *Current);
	
	// MUST be supported if VRM_CAPABILITY_LOADLINE is signaled
	uint32_t (*SetLoadLine)(struct _VRMController *VRM, uint8_t LoadLine);
	
	// Pointer to next in list
	struct _VRMController *next;
};

uint32_t DetectVRMControllers(AMDGPU *GPU);
const char *GetVRMName(VRMController *VRM);

#endif
