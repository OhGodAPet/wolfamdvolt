#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "ir3xxxx.h"
#include "ncp81022.h"
#include "up9505.h"
#include "up1801.h"
#include "rt8894a.h"
#include "ir35217.h"
#include "vrm.h"

// Detect functions need to be rewritten to support
// returning multiple VRMs of the same type! 

uint32_t DetectVRMControllers(AMDGPU *GPU)
{
	// Free any old data, if existing.
	if(GPU->VRMs) AMDGPUFreeVRMs(GPU);
	
	GPU->VRMCount += IR356XXDetect(GPU, &GPU->VRMs);
	GPU->VRMCount += IR35217Detect(GPU, &GPU->VRMs);
	GPU->VRMCount += RT8894ADetect(GPU, &GPU->VRMs);
	GPU->VRMCount += NCP81022Detect(GPU, &GPU->VRMs);
	GPU->VRMCount += uP9505Detect(GPU, &GPU->VRMs);
	GPU->VRMCount += uP1801Detect(GPU, &GPU->VRMs);
	
	return(GPU->VRMCount);
}

static const char *VRMNames[8] =
{
	"INVALID",
	"IR356XX",
	"uP6266",
	"uP9505",
	"NCP81022",
	"uP1801",
	"RT8894A",
	"IR35217"
};

const char *GetVRMName(VRMController *VRM)
{
	return(VRMNames[VRM->VRMType]);
}
