#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "amdgpu.h"
#include "amdi2c.h"
#include "amdi2cdbg.h"
#include "amdsmbus.h"
#include "amdpmbus.h"
#include "wolfamdbg.h"

#define CONVERT(val)		((val[0] == '0' && val[1] == 'x') ? strtoul(val + 2, NULL, 16) : strtoul(val, NULL, 16))

#define AD527XMAKEWORD(cmd, data)	((uint16_t)((((uint16_t)(cmd)) << 10)) | (((uint16_t)(data)) & 0x3FF))
#define AD527XWORD2CMD(val)			(((val) >> 10) & 0xF)
#define AD527XWORD2DATA(val)		((val) & 0x3FF)

// Parameter len is the size in bytes of asciistr, meaning rawstr
// must have (len >> 1) bytes allocated
// Maybe asciistr just NULL terminated?
// Returns length of rawstr in bytes
int ASCIIHexToBinary(void *restrict rawstr, const char *restrict asciistr, size_t len)
{
	for(int i = 0, j = 0; i < len; ++i)
	{
		char tmp = asciistr[i];
		if(tmp < 'A') tmp -= '0';
		else if(tmp < 'a') tmp = (tmp - 'A') + 10;
		else tmp = (tmp - 'a') + 10;

		if(i & 1) ((uint8_t *)rawstr)[j++] |= tmp & 0x0F;
		else ((uint8_t *)rawstr)[j] = tmp << 4;
	}

	return(len >> 1);
}

uint32_t IR3XXXMFR_READ_REG(AMDGPU *GPU, uint8_t I2CAddr, int *ret);

#ifdef __linux__
HANDLE OpenDriverHandle(void)
{
	return(INVALID_HANDLE_VALUE);
}
#elif defined(_WIN32)
HANDLE OpenDriverHandle(void)
{
	return(CreateFileA("\\\\.\\atillk64", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
}
#endif

int main(int argc, char **argv)
{
	int Status;
	ArgsObj Config;
	uint32_t ret, outval;
	AMDGPU *GPUList, *CurGPU;
	HANDLE DrvHandle = INVALID_HANDLE_VALUE;

	printf("wolfamdbg %s\n", WOLFAMDBG_VERSION_STR);
	memset(&Config, 0x00, sizeof(Config));

	if(!ParseCmdLine(&Config, argc, argv)) return(1);

	DrvHandle = OpenDriverHandle();

	int GPUCount = FindAMDGPUs(&GPUList, DrvHandle);

	// Remember that we're counting from one, but they
	// are indexing from zero in this case. Adjusting
	// the value keeps things simpler.

	if(!GPUCount)
	{
		printf("Found no AMD GPUs at all.\n");
		return(0);
	}

	GPUCount--;

	if(Config.GPUIdx > GPUCount)
	{
		printf("GPU index does not exist.\n");
		ReleaseAMDGPUs(GPUList);
		return(2);
	}

	CurGPU = GPUList;
	for(int i = 0; i < Config.GPUIdx; ++i) CurGPU = CurGPU->next;

	Status = InitAMDGPUMMIO(CurGPU);

	if(Status < 0)
	{
		printf("Unable to initialize MMIO for GPU - failed with %d.\n", Status);
		return(-1);
	}

	switch(Config.Action)
	{
		case ACTION_SCAN:
		{
			for(uint8_t addr = 0x03; addr < 0x78; ++addr)
			{
				bool bad = true;
				uint8_t reg = 0x00;

				AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, addr);
				printf("Scanning address 0x%02X.\n", addr);
				Status = 0;

				do
				{
					outval = (uint8_t)AMDI2CReadByte(CurGPU, reg, &Status);
					//if((byte != off) && (ret && (!(I2CTXERRORCHK(ret)))))
					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
					{
						bad = false;
						break;
					}
				} while(reg++ < 0xFF);

				if(bad) printf("Appears nothing interesting at 0x%02X. (Status: 0x%04X)\n", addr, Status);
				else printf("Address 0x%02X looks like it has something!\nOffset 0x%02X returned 0x%02X! (Status: 0x%04X)\n", addr, reg, (uint8_t)outval, Status);
			}
			break;
		}
		case ACTION_DUMP:
		{
			AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, Config.I2CAddress);

			for(uint16_t reg = 0x00; reg < 0x100; reg += 0x02)
			{
				if(Config.WordOperation)
				{
					if(!(reg & 0x0F)) putchar('\n');

					outval = (uint16_t)AMDSMBusReadWord(CurGPU, reg, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%04X ", (uint16_t)outval);
					else
						printf("0xXXXX ");
				}
				else
				{
					if(!(reg & 0x0F)) putchar('\n');

					outval = (uint8_t)AMDI2CReadByte(CurGPU, reg, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%02X ", (uint8_t)outval);
					else
						printf("0xXX (0x%04X)", Status);

					outval = (uint8_t)AMDI2CReadByte(CurGPU, reg + 0x01, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%02X ", (uint8_t)outval);
					else
						printf("0xXX (0x%04X)", Status);
				}

			}

			putchar('\n');
			break;
		}
		case ACTION_DUMP_SMBUS:
		{
			AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, Config.I2CAddress);

			for(uint16_t reg = 0x00; reg < 0x100; reg += 0x02)
			{
				if(Config.WordOperation)
				{
					if(!(reg & 0x0F)) putchar('\n');

					outval = (uint16_t)AMDSMBusReadWord(CurGPU, reg, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%04X ", (uint16_t)outval);
					else
						printf("0xXXXX ");
				}
				else
				{
					if(!(reg & 0x0F)) putchar('\n');

					outval = (uint8_t)AMDSMBusReadByte(CurGPU, reg, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%02X ", (uint8_t)outval);
					else
						printf("0xXX ");

					// Second read in the same loop iter - keeping pace with the version for word-sized reads
					outval = (uint8_t)AMDSMBusReadByte(CurGPU, reg + 0x01, &Status);

					if(((Status & I2C_STATUS_MASK) & I2C_DONE) && (!((Status & I2C_STATUS_MASK) & I2C_NACK)))
						printf("0x%02X ", (uint8_t)outval);
					else
						printf("0xXX ");
				}

			}

			putchar('\n');
			break;
		}
		case ACTION_READ:
		{
			uint8_t reg = Config.I2CRegister;
			AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, Config.I2CAddress);

			/*if(Config.WordOperation)
			{
				outval = (uint16_t)AMDI2CReadWord(CurGPU, reg, &Status);
				printf("Returned 0x%04X (status 0x%04X).\n", (uint16_t)outval, Status);
			}
			else
			{
				outval = (uint8_t)AMDI2CReadByte(CurGPU, reg, &Status);
				printf("Returned 0x%02X (status 0x%04X).\n", (uint8_t)outval, Status);
			}*/
			break;
		}

		case ACTION_READ_SMBUS:
		{
			uint8_t reg = Config.I2CRegister;
			AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, Config.I2CAddress);

			if(Config.WordOperation)
			{
				outval = (uint16_t)AMDSMBusReadWord(CurGPU, reg, &Status);
				printf("Returned 0x%04X (status 0x%04X).\n", (uint16_t)outval, Status);
			}
			else
			{
				outval = (uint8_t)AMDSMBusReadByte(CurGPU, reg, &Status);
				printf("Returned 0x%02X (status 0x%04X).\n", (uint8_t)outval, Status);
			}
			break;
		}
		case ACTION_WRITE:
		{
			uint8_t reg = Config.I2CRegister;
			AMDGPUI2CInit(CurGPU, Config.I2CBusIdx, Config.I2CAddress);

			if(Config.WordOperation) Status = AMDSMBusWriteWord(CurGPU, reg, (uint16_t)Config.WriteValue);
			else Status = AMDI2CWriteByte(CurGPU, reg, (uint8_t)Config.WriteValue);
			printf("Write done (status 0x%04X)\n", Status);
			break;
		}
	}

	CloseAMDGPUMMIO(CurGPU);
	ReleaseAMDGPUs(GPUList);
	return(0);
}

