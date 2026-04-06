/*
 * fixed_point.h
 *
 *  Created on: Apr 6, 2026
 *      Author: H.Dani
 */

#ifndef INC_FIXED_POINT_H_
#define INC_FIXED_POINT_H_

#include "stdint.h"
#include "main.h"

// Q16.16 format
typedef int32_t fix16_t;

#define FIX_SHIFT 16
#define FIX_ONE   (1 << FIX_SHIFT)

// Conversion
#define FLOAT_TO_FIX(x) ((fix16_t)((x) * FIX_ONE))
#define FIX_TO_FLOAT(x) ((float)(x) / FIX_ONE)

// Multiply
static inline fix16_t fix_mul(fix16_t a, fix16_t b)
{
    return (fix16_t)(((int64_t)a * b) >> FIX_SHIFT);
}

// Divide
static inline fix16_t fix_div(fix16_t a, fix16_t b)
{
    return (fix16_t)(((int64_t)a << FIX_SHIFT) / b);
}


#endif /* INC_FIXED_POINT_H_ */
