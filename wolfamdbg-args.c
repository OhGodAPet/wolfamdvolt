#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "wolfamdbg.h"

void PrintUsage(char *BinName)
{
	printf("Usage: %s [Selection Options] <Action>\n", BinName);
	printf("Selection Options:\n");
	printf("\t-i <GPUIdx>\n\t-b <I2CBusNum> (0 is HWI2C, 1-6 is DDC1-6, and 7 is DDCVGA - default 0.)\n");
	printf("Action Options (one must be specified, and only one):\n");
	printf("\t--scan\n\t--dump <HexI2CAddress>\n\t--dumpsmbus <HexI2CAddress>\n\t--dumpword <HexI2CAddress>\n\t--dumpwordsmbus <HexI2CAddress>\n");
	printf("\t--read <HexI2CAddress> <HexI2CRegister>\n\t--readsmbus <HexSMBusAddress> <HexSMBusRegister>\n");
	printf("\t--readword <HexI2CAddress> <HexI2CRegister>\n\t--readwordsmbus <HexSMBusAddress> <HexSMBusRegister>\n");
	printf("\t--write <HexI2CAddress> <HexI2CRegister> <HexByteValue>\n\t--writeword <HexI2CAddress> <HexI2CRegister> <HexWordValue>\n");
	printf("\nA valid byte input ranges 0x00 - 0xFF, valid word inputs from 0x0000 - 0xFFFF.\n");
	printf("Valid I2C addresses are 0x08 - 0x77. All ranges specified are inclusive.\n");
}

#define NEXT_ARG_CHECK() do { if(i == (argc - 1)) { printf("Argument \"%s\" requires a parameter.\n", argv[i]); PrintUsage(argv[0]); return(false); } } while(0)
#define NEXT_TWO_ARGS_CHECK() do { if((i == (argc - 1)) || (i == (argc - 2))) { printf("Argument \"%s\" requires two parameters.\n", argv[i]); PrintUsage(argv[0]); return(false); } } while(0)
#define NEXT_THREE_ARGS_CHECK() do { if((i == (argc - 1)) || (i == (argc - 2)) || (i == (argc - 3))) { printf("Argument \"%s\" requires three parameters.\n", argv[i]); PrintUsage(argv[0]); return(false); } } while(0)
#define VALID_ADDR(in)		(((in) > 0x03) && ((in) < 0x78))
#define VALID_BYTE(in)		(((in) >= 0x00) && ((in) < 0x100))
#define VALID_WORD(in)		(((in) >= 0x00) && ((in) < 0x10000))
#define CONVERT(val)		((((val)[0] == '0') && ((val)[1] == 'x')) ? ((uint32_t)strtoul((val) + 2, NULL, 16)) : ((uint32_t)strtoul((val), NULL, 16)))

static bool SetAction(ArgsObj *Args, uint32_t NewAction)
{
	if(Args->Action != ACTION_INVALID)
	{
		printf("Only one action may be used at a time.\n");
		return(false);
	}
	
	Args->Action = NewAction;
	return(true);
}

bool ParseCmdLine(ArgsObj *Args, int argc, char **argv)
{
	memset(Args, 0x00, sizeof(ArgsObj));
	
	if(argc < 2)
	{
		PrintUsage(argv[0]);
		return(false);
	}
	
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
		else if(!strcmp("-b", argv[i]))
		{
			NEXT_ARG_CHECK();
			Args->I2CBusIdx = strtoul(argv[++i], NULL, 10);
			
			if(errno == EINVAL || errno == ERANGE || Args->I2CBusIdx > 7U)
			{
				printf("Invalid I2C bus index specified.\n");
				return(false);
			}
			
			Args->I2CBusIdxProvided = true;
		}
		else if(!strcmp("--scan", argv[i]))
		{
			if(!SetAction(Args, ACTION_SCAN)) return(false);
		}
		else if(!strcmp("--dump", argv[i]))
		{
			NEXT_ARG_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(!VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			if(!SetAction(Args, ACTION_DUMP)) return(false);
		}
		else if(!strcmp("--dumpsmbus", argv[i]))
		{
			NEXT_ARG_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(!VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			if(!SetAction(Args, ACTION_DUMP_SMBUS)) return(false);
		}
		else if(!strcmp("--dumpword", argv[i]))
		{
			NEXT_ARG_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			Args->WordOperation = true;
			if(!SetAction(Args, ACTION_DUMP)) return(false);
		}
		else if(!strcmp("--dumpwordsmbus", argv[i]))
		{
			NEXT_ARG_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(!VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			Args->WordOperation = true;
			if(!SetAction(Args, ACTION_DUMP_SMBUS)) return(false);
		}
		else if(!strcmp("--read", argv[i]))
		{
			NEXT_TWO_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			if(!SetAction(Args, ACTION_READ)) return(false);
		}
		else if(!strcmp("--readsmbus", argv[i]))
		{
			NEXT_TWO_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			if(!SetAction(Args, ACTION_READ_SMBUS)) return(false);
		}
		else if(!strcmp("--readword", argv[i]))
		{
			NEXT_TWO_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			Args->WordOperation = true;
			if(!SetAction(Args, ACTION_READ)) return(false);
		}
		else if(!strcmp("--readwordsmbus", argv[i]))
		{
			NEXT_TWO_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			Args->WordOperation = true;
			if(!SetAction(Args, ACTION_READ_SMBUS)) return(false);
		}
		else if(!strcmp("--write", argv[i]))
		{
			NEXT_THREE_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->WriteValue = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->WriteValue))
			{
				printf("Invalid value to write specified (must be a single byte.)\n");
				return(false);
			}
			
			if(!SetAction(Args, ACTION_WRITE)) return(false);
		}
		else if(!strcmp("--writeword", argv[i]))
		{
			NEXT_THREE_ARGS_CHECK();
			
			// WARNING: Doing this in the macro invocation
			// will cause its duplication!
			++i;
			
			Args->I2CAddress = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_ADDR(Args->I2CAddress))
			{
				printf("Invalid I2C address specified.\n");
				return(false);
			}
			
			// See warning above
			++i;
			
			Args->I2CRegister = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_BYTE(Args->I2CRegister))
			{
				printf("Invalid I2C register specified.\n");
				return(false);
			}
			
			// See warning above.
			++i;
			
			Args->WriteValue = CONVERT(argv[i]);
			
			if(errno == EINVAL || errno == ERANGE || !VALID_WORD(Args->WriteValue))
			{
				printf("Invalid value to write specified (must be a single byte.)\n");
				return(false);
			}
			
			Args->WordOperation = true;
			if(!SetAction(Args, ACTION_WRITE)) return(false);
		}
		else
		{
			PrintUsage(argv[0]);
			printf("\nUnknown option: \"%s\"\n", argv[i]);
			return(false);
		}
	}
	
	// Now ensure there was an action to perform set...
	if(Args->Action == ACTION_INVALID)
	{
		printf("No action option set; nothing to do.\n");
		PrintUsage(argv[0]);
		return(false);
	}
	
	return(true);
}
