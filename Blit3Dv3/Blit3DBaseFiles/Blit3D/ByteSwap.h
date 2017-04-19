#pragma once

#include <stdint.h>

//! Byte swap unsigned short
uint16_t swap_uint16(uint16_t val);

//! Byte swap short
int16_t swap_int16(int16_t val);

//! Byte swap unsigned int
uint32_t swap_uint32(uint32_t val);

//! Byte swap int
int32_t swap_int32(int32_t val);

//64 bit
int64_t swap_int64(int64_t val);
uint64_t swap_uint64(uint64_t val);