#ifndef __AMDI2CDBG_H
#define __AMDI2CDBG_H

#include <stdint.h>
#include <stdbool.h>

#include "amdi2c.h"

void AMDI2CDbgGetStatusString(char **DbgStrIn, uint32_t I2CStatusVal);

#define	AMDI2CDBG_STATUS_STR_MAX_LEN		0x80

#define I2CTXERRORCHK(I2CStatus)			((((I2CStatus) & I2C_STATUS_MASK) & (I2C_ABORTED | I2C_TIMEOUT | I2C_NACK)) || (!(((I2CStatus) & I2C_STATUS_MASK) & I2C_DONE)))

#endif
