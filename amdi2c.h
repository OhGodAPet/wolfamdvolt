#ifndef __AMDI2C_H
#define __AMDI2C_H

#include "amdgpu.h"

//#define GPIO_PIN_SCL							0x29 	//0x4D
//#define GPIO_PIN_SDA							0x28	//0x4C

static const I2CPINS GCN3_HWI2C_PINS = { 0x28, 0x29 };
static const I2CPINS GCN3_DDC1_PINS = { 0x00, 0x01 };
static const I2CPINS GCN3_DDC2_PINS = { 0x02, 0x03 };
static const I2CPINS GCN3_DDC3_PINS = { 0x04, 0x05 };
static const I2CPINS GCN3_DDC4_PINS = { 0x41, 0x42 };
static const I2CPINS GCN3_DDC5_PINS = { 0x48, 0x49 };
static const I2CPINS GCN3_DDC6_PINS = { 0x4A, 0x4B };
static const I2CPINS GCN3_DDCVGA_PINS = { 0x4C, 0x4D };

static const I2CPINS GCN3_I2C_BUS_LINES[8] = 
{
	GCN3_HWI2C_PINS,
	GCN3_DDC1_PINS,
	GCN3_DDC2_PINS,
	GCN3_DDC3_PINS,
	GCN3_DDC4_PINS,
	GCN3_DDC5_PINS,
	GCN3_DDC6_PINS,
	GCN3_DDCVGA_PINS
};

/*
#define GCN3_mmGENERIC_I2C_CONTROL				0x16F4
#define GCN3_mmGENERIC_I2C_INTERRUPT_CONTROL	0x16F5
#define GCN3_mmGENERIC_I2C_STATUS				0x16F6
#define GCN3_mmGENERIC_I2C_SPEED				0x16F7
#define GCN3_mmGENERIC_I2C_SETUP				0x16F8
#define GCN3_mmGENERIC_I2C_TRANSACTION			0x16F9
#define GCN3_mmGENERIC_I2C_DATA					0x16FA
#define GCN3_mmGENERIC_I2C_PIN_SELECTION		0x16FB
#define GCN3_mmGENERIC_I2C_PIN_DEBUG			0x16FC

#define GCN2_mmGENERIC_I2C_CONTROL				0x1834
#define GCN2_mmGENERIC_I2C_INTERRUPT_CONTROL	0x1835
#define GCN2_mmGENERIC_I2C_STATUS				0x1836
#define GCN2_mmGENERIC_I2C_SPEED				0x1837
#define GCN2_mmGENERIC_I2C_SETUP				0x1838
#define GCN2_mmGENERIC_I2C_TRANSACTION			0x1839
#define GCN2_mmGENERIC_I2C_DATA					0x183A
#define GCN2_mmGENERIC_I2C_PIN_SELECTION		0x183B
#define GCN2_mmGENERIC_I2C_PIN_DEBUG			0x183C
*/

static const I2CGPIO GCN5_I2C_GPIO_ADDRS = { 0x4A64, 0x4A65, 0x4A66, 0x4A67, 0x4A68, 0x4A69, 0x4A6A, 0x4A6B, 0x4A6C };
static const I2CGPIO GCN3_I2C_GPIO_ADDRS = { 0x16F4, 0x16F5, 0x16F6, 0x16F7, 0x16F8, 0x16F9, 0x16FA, 0x16FB, 0x16FC };
static const I2CGPIO GCN2_I2C_GPIO_ADDRS = { 0x1834, 0x1835, 0x1836, 0x1837, 0x1838, 0x1839, 0x183A, 0x183B, 0x183C };

// The following only applies to wolfamdvolt:
// FIXME: Add an option to make the I2C line selectable
// In order to make all areas requiring a change for this
// to be done, this macro will temporarily serve as a
// static bus number, that being the HWI2C line.
#define STATIC_I2C_LINE_FIXME		((uint8_t)0U)

#define I2C_DONE_INT							0x01
#define I2C_DONE_ACK							(0x01 << 1)
#define I2C_DONE_MASK							(0x01 << 2)

// Speed constants for mmGENERIC_I2C_SPEED
#define I2C_THRESHOLD							0x03
#define I2C DISABLE_FILTER_WHILE_STALLING		0x04
#define I2C_START_STOP_TIMING_CNTL				0x300
#define I2C_PRESCALE							(0xFFFF << 16)

// Control constants for mmGENERIC_I2C_CONTROL
#define I2C_GO									1
#define I2C_SOFT_RESET							(1 << 1)
#define I2C_RESET								(1 << 2)
#define I2C_ENABLE								(1 << 3)

// Status constants for mmGENERIC_I2C_STATUS
#define I2C_STATUS_MASK							(~(0x0F))
#define I2C_DONE								(1 << 4)
#define I2C_ABORTED								(1 << 5)
#define I2C_TIMEOUT								(1 << 6)
#define I2C_STOPPED_ON_NACK						(1 << 9)
#define I2C_NACK								(1 << 10)

// Pin constants for mmGENERIC_I2C_PIN_SELECTION
#define I2C_SCL_PIN_MASK						0x7F
#define I2C_SDA_PIN_MASK						(0x7F << 8)

// Transaction constants for mmGENERIC_I2C_TRANSACTION
#define I2C_RW									1
#define I2C_STOP_ON_NACK						(1 << 8)
#define I2C_ACK_ON_READ							(1 << 9)
#define I2C_START								(1 << 12)
#define I2C_STOP								(1 << 13)

// Data constants for mmGENERIC_I2C_DATA
#define I2C_DATA_RW								1
#define I2C_INDEX(x)							((x) << 16)
#define I2C_INDEX_WRITE							(1 << 31)

extern bool I2CDebugOutput;

void AMDGPUI2CInit(AMDGPU *GPU, uint8_t bus, uint8_t addr);
uint8_t AMDI2CReadByte(AMDGPU *GPU, uint8_t reg, uint32_t *ret);
uint16_t AMDI2CReadWord(AMDGPU *GPU, uint32_t *ret);
uint32_t AMDI2CWriteWord(AMDGPU *GPU, uint16_t data);

int AMDI2CWriteByte(AMDGPU *GPU, uint8_t reg, uint8_t data);
uint32_t AMDI2CWriteRaw(AMDGPU *GPU, uint8_t *data, uint8_t len);
uint8_t AMDI2CReadRaw(AMDGPU *GPU, uint8_t *dout, uint8_t len);
void AMDI2CSoftReset(AMDGPU *GPU);
void I2CExecTX(AMDGPU *GPU);

#endif
