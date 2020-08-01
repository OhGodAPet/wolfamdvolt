#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "wolfamdvolt.h"

void PrintUsage(char *BinName)
{
	printf("Usage: %s [Global Options] [Selection Options] [Modification Options]\n", BinName);
	printf("Global Options:\n");
	printf("\t--debug\n");
	printf("Selection Options:\n");
	printf("\t-i GPUIdx\n\t-v VRMIdx\n\t-o VRMOutputIdx\n");
	printf("Modification options (require a GPU index to be specified):\n");
	printf("\t--vddc <Volts>\n\t--offset <Volts>\n");
	printf("If the selection options are used without modification options, then info is displayed.\n");
}

#define NEXT_ARG_CHECK() do { if(i == (argc - 1)) { printf("Argument \"%s\" requires a parameter.\n"); return(false); } } while(0)

bool ParseCmdLine(ArgsObj *Args, int argc, char **argv)
{
	memset(Args, 0x00, sizeof(ArgsObj));
	
	if(argc < 2) return(true);
	
	for(int i = 1; i < argc; ++i)
	{
		if(!strcmp("-i", argv[i]))
		{
			NEXT_ARG_CHECK();
			Args->GPUIdx = strtoul(argv[++i], NULL, 10);
			
			if(errno == EINVAL || errno == ERANGE || Args->GPUIdx > 15)
			{
				printf("Invalid GPU index specified.\n");
				return(false);
			}
			
			Args->GPUIdxProvided = true;
		}
		else if(!strcmp("-v", argv[i]))
		{
			NEXT_ARG_CHECK();
			Args->VRMIdx = strtoul(argv[++i], NULL, 10);
			
			if(errno == EINVAL || errno == ERANGE)
			{
				printf("Invalid VRM index specified.\n");
				return(false);
			}
			
			Args->VRMIdxProvided = true;
		}
		else if(!strcmp("-o", argv[i]))
		{
			NEXT_ARG_CHECK();
			Args->VRMOutputIdx = strtoul(argv[++i], NULL, 10);
			
			if(errno == EINVAL || errno == ERANGE)
			{
				printf("Invalid VRM output index specified.\n");
				return(false);
			}
			
			Args->VRMOutputIdxProvided = true;
		}
		else if(!strcmp("--vddc", argv[i]))
		{
			NEXT_ARG_CHECK();
			sscanf(argv[++i], "%f", &Args->RequestedVoltage);
			
			Args->SetVoltage = true;
		}
		else if(!strcmp("--offset", argv[i]))
		{
			NEXT_ARG_CHECK();
			sscanf(argv[++i], "%f", &Args->RequestedVoltageOffset);
			
			Args->SetVoltageOffset = true;
		}
		else if(!strcmp("--debug", argv[i]))
		{
			Args->Debug = true;
		}
		else
		{
			PrintUsage(argv[0]);
			printf("\nUnknown option: \"%s\"\n", argv[i]);
			return(false);
		}
	}
	
	return(true);
}
