#ifndef __WOLFAMDVOLT_H
#define __WOLFAMDVOLT_H

#include <stdint.h>
#include <stdbool.h>

#define WOLFAMDVOLT_VERSION_STR			"v0.95"
#define WOLFAMDVOLT_VERSION				0.95

typedef struct _ArgsObj
{
	bool GPUIdxProvided, VRMIdxProvided, VRMOutputIdxProvided;
	bool SetVoltage, SetVoltageOffset;
	bool Debug;
	
	uint32_t GPUIdx, VRMIdx, VRMOutputIdx;
	float RequestedVoltage, RequestedVoltageOffset;
} ArgsObj;

bool ParseCmdLine(ArgsObj *Args, int argc, char **argv);

#endif
