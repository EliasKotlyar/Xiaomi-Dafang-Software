#ifndef __APICAL_FLASH_TIMER_CONFIG_H__
#define __APICAL_FLASH_TIMER_CONFIG_H__


/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2014 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#include "apical_isp_io.h"

// ------------------------------------------------------------------------------ //
// Instance 'flash_timer' of module 'flash_timer'
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_BASE_ADDR (0x2000L)
#define APICAL_FLASH_TIMER_SIZE (0x200)

// ------------------------------------------------------------------------------ //
// Group: Flash Timer
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Number of channels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of channels instantiated
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_NUMBER_OF_CHANNELS_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_NUMBER_OF_CHANNELS_DATASIZE (8)

// args: data (8-bit)
static __inline uint8_t apical_flash_timer_flash_timer_number_of_channels_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2000L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: API revision
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// API revision
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_API_REVISION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_API_REVISION_DATASIZE (8)

// args: data (8-bit)
static __inline uint8_t apical_flash_timer_flash_timer_api_revision_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2000L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Input status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Status of inputs
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_INPUT_STATUS_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_INPUT_STATUS_DATASIZE (8)

// args: data (8-bit)
static __inline uint8_t apical_flash_timer_flash_timer_input_status_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2008L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Start flag0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Firmware start flag 0
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_START_FLAG0_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_START_FLAG0_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_start_flag0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2008L);
	APICAL_WRITE_32(0x2008L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_flash_timer_flash_timer_start_flag0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2008L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Start flag1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Firmware start flag 1
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_START_FLAG1_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_START_FLAG1_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_start_flag1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2008L);
	APICAL_WRITE_32(0x2008L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_flash_timer_flash_timer_start_flag1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2008L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: Output0 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output0 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output0_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2020L);
	APICAL_WRITE_32(0x2020L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output0_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2020L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output0 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output0
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output0_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2020L);
	APICAL_WRITE_32(0x2020L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output0_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2020L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output0 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output0 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT0_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output0_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2020L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output1 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output1 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output1_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2024L);
	APICAL_WRITE_32(0x2024L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output1_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2024L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output1 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output1
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output1_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2024L);
	APICAL_WRITE_32(0x2024L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output1_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2024L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output1 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output1 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT1_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output1_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2024L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output2 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output2 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output2_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2028L);
	APICAL_WRITE_32(0x2028L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output2_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2028L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output2 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output2
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output2_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2028L);
	APICAL_WRITE_32(0x2028L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output2_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2028L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output2 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output2 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT2_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output2_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2028L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output3 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output3 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output3_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x202cL);
	APICAL_WRITE_32(0x202cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output3_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x202cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output3 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output3
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output3_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x202cL);
	APICAL_WRITE_32(0x202cL, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output3_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x202cL) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output3 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output3 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT3_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output3_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x202cL) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output4 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output4 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output4_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2030L);
	APICAL_WRITE_32(0x2030L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output4_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2030L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output4 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output4
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output4_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2030L);
	APICAL_WRITE_32(0x2030L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output4_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2030L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output4 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output4 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT4_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output4_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2030L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output5 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output5 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output5_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2034L);
	APICAL_WRITE_32(0x2034L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output5_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2034L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output5 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output5
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output5_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2034L);
	APICAL_WRITE_32(0x2034L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output5_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2034L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output5 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output5 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT5_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output5_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2034L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output6 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output6 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output6_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2038L);
	APICAL_WRITE_32(0x2038L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output6_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2038L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output6 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output6
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output6_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2038L);
	APICAL_WRITE_32(0x2038L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output6_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2038L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output6 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output6 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT6_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output6_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2038L) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Output7 sources
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source mask for output7 (bits 0-7 inputs, bits 8-15 timers)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_SOURCES_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_SOURCES_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_flash_timer_flash_timer_output7_sources_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x203cL);
	APICAL_WRITE_32(0x203cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_flash_timer_flash_timer_output7_sources_read(void) {
	return (uint16_t)((APICAL_READ_32(0x203cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output7 polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Inversion on output7
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_POLARITY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_output7_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x203cL);
	APICAL_WRITE_32(0x203cL, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_output7_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x203cL) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Output7 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// State of output7 pin
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_OUTPUT7_STATE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_flash_timer_flash_timer_output7_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x203cL) & 0x80000000) >> 31);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer0_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2040L);
	APICAL_WRITE_32(0x2040L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2040L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer0_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2040L);
	APICAL_WRITE_32(0x2040L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2040L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer0_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2040L);
	APICAL_WRITE_32(0x2040L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2040L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer0_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2040L);
	APICAL_WRITE_32(0x2040L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2040L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer0_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2040L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer0_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2044L);
	APICAL_WRITE_32(0x2044L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2044L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer0_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2044L);
	APICAL_WRITE_32(0x2044L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2044L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer0_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2044L);
	APICAL_WRITE_32(0x2044L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2044L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer0_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2044L);
	APICAL_WRITE_32(0x2044L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer0_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2044L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer0_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x2048L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer0_delay_read(void) {
	return APICAL_READ_32(0x2048L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer0_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x204cL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer0_duration_read(void) {
	return APICAL_READ_32(0x204cL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer0 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer0 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER0_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer0_counter_read(void) {
	return APICAL_READ_32(0x2050L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer1_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2060L);
	APICAL_WRITE_32(0x2060L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2060L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer1_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2060L);
	APICAL_WRITE_32(0x2060L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2060L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer1_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2060L);
	APICAL_WRITE_32(0x2060L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2060L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer1_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2060L);
	APICAL_WRITE_32(0x2060L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2060L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer1_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2060L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer1_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2064L);
	APICAL_WRITE_32(0x2064L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2064L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer1_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2064L);
	APICAL_WRITE_32(0x2064L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2064L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer1_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2064L);
	APICAL_WRITE_32(0x2064L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2064L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer1_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2064L);
	APICAL_WRITE_32(0x2064L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer1_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2064L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer1_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x2068L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer1_delay_read(void) {
	return APICAL_READ_32(0x2068L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer1_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x206cL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer1_duration_read(void) {
	return APICAL_READ_32(0x206cL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer1 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer1 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER1_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer1_counter_read(void) {
	return APICAL_READ_32(0x2070L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer2_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2080L);
	APICAL_WRITE_32(0x2080L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2080L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer2_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2080L);
	APICAL_WRITE_32(0x2080L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2080L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer2_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2080L);
	APICAL_WRITE_32(0x2080L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2080L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer2_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2080L);
	APICAL_WRITE_32(0x2080L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2080L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer2_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2080L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer2_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2084L);
	APICAL_WRITE_32(0x2084L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2084L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer2_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2084L);
	APICAL_WRITE_32(0x2084L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2084L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer2_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2084L);
	APICAL_WRITE_32(0x2084L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2084L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer2_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2084L);
	APICAL_WRITE_32(0x2084L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer2_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2084L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer2_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x2088L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer2_delay_read(void) {
	return APICAL_READ_32(0x2088L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer2_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x208cL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer2_duration_read(void) {
	return APICAL_READ_32(0x208cL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer2 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer2 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER2_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer2_counter_read(void) {
	return APICAL_READ_32(0x2090L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer3_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a0L);
	APICAL_WRITE_32(0x20a0L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a0L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer3_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a0L);
	APICAL_WRITE_32(0x20a0L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a0L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer3_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a0L);
	APICAL_WRITE_32(0x20a0L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a0L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer3_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a0L);
	APICAL_WRITE_32(0x20a0L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a0L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer3_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a0L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer3_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a4L);
	APICAL_WRITE_32(0x20a4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer3_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a4L);
	APICAL_WRITE_32(0x20a4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer3_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a4L);
	APICAL_WRITE_32(0x20a4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer3_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20a4L);
	APICAL_WRITE_32(0x20a4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer3_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20a4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer3_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x20a8L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer3_delay_read(void) {
	return APICAL_READ_32(0x20a8L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer3_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x20acL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer3_duration_read(void) {
	return APICAL_READ_32(0x20acL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer3 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer3 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER3_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer3_counter_read(void) {
	return APICAL_READ_32(0x20b0L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer4_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c0L);
	APICAL_WRITE_32(0x20c0L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c0L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer4_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c0L);
	APICAL_WRITE_32(0x20c0L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c0L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer4_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c0L);
	APICAL_WRITE_32(0x20c0L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c0L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer4_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c0L);
	APICAL_WRITE_32(0x20c0L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c0L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer4_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c0L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer4_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c4L);
	APICAL_WRITE_32(0x20c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer4_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c4L);
	APICAL_WRITE_32(0x20c4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer4_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c4L);
	APICAL_WRITE_32(0x20c4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer4_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20c4L);
	APICAL_WRITE_32(0x20c4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer4_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20c4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer4_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x20c8L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer4_delay_read(void) {
	return APICAL_READ_32(0x20c8L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer4_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x20ccL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer4_duration_read(void) {
	return APICAL_READ_32(0x20ccL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer4 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer4 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER4_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer4_counter_read(void) {
	return APICAL_READ_32(0x20d0L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer5_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e0L);
	APICAL_WRITE_32(0x20e0L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e0L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer5_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e0L);
	APICAL_WRITE_32(0x20e0L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e0L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer5_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e0L);
	APICAL_WRITE_32(0x20e0L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e0L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer5_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e0L);
	APICAL_WRITE_32(0x20e0L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e0L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer5_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e0L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer5_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e4L);
	APICAL_WRITE_32(0x20e4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer5_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e4L);
	APICAL_WRITE_32(0x20e4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer5_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e4L);
	APICAL_WRITE_32(0x20e4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer5_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20e4L);
	APICAL_WRITE_32(0x20e4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer5_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20e4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer5_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x20e8L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer5_delay_read(void) {
	return APICAL_READ_32(0x20e8L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer5_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x20ecL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer5_duration_read(void) {
	return APICAL_READ_32(0x20ecL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer5 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer5 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER5_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer5_counter_read(void) {
	return APICAL_READ_32(0x20f0L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer6_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2100L);
	APICAL_WRITE_32(0x2100L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2100L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer6_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2100L);
	APICAL_WRITE_32(0x2100L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2100L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer6_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2100L);
	APICAL_WRITE_32(0x2100L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2100L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer6_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2100L);
	APICAL_WRITE_32(0x2100L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2100L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer6_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2100L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer6_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2104L);
	APICAL_WRITE_32(0x2104L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2104L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer6_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2104L);
	APICAL_WRITE_32(0x2104L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2104L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer6_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2104L);
	APICAL_WRITE_32(0x2104L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2104L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer6_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2104L);
	APICAL_WRITE_32(0x2104L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer6_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2104L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer6_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x2108L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer6_delay_read(void) {
	return APICAL_READ_32(0x2108L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer6_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x210cL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer6_duration_read(void) {
	return APICAL_READ_32(0x210cL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer6 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer6 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER6_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer6_counter_read(void) {
	return APICAL_READ_32(0x2110L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 trigger software
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 trigger source on firmware flags
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_SOFTWARE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_SOFTWARE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer7_trigger_software_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2120L);
	APICAL_WRITE_32(0x2120L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_trigger_software_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2120L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 mode: 0-new event restarts, 1-new event ignored until timer finishes, 2-new event is stored and cause retrigger at the end of interval, 3-pulse count mode
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_MODE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_flash_timer_flash_timer_timer7_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2120L);
	APICAL_WRITE_32(0x2120L, (((uint32_t) (data & 0x3)) << 16) | (curr & 0xfffcffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2120L) & 0x30000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 software reset: 1-timer is held at 0, triggers are reset
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_RESET_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer7_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2120L);
	APICAL_WRITE_32(0x2120L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2120L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 pause
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 counter pause: 1-counting is paused, triggers are still working
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_PAUSE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_PAUSE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_flash_timer_flash_timer_timer7_pause_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2120L);
	APICAL_WRITE_32(0x2120L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_pause_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2120L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 state
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 status: 0-inactive, 1-initial delay, 2-active
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_STATE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_STATE_DATASIZE (2)

// args: data (2-bit)
static __inline uint8_t apical_flash_timer_flash_timer_timer7_state_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2120L) & 0xc0000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 trigger input rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 trigger source on rising edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_INPUT_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_INPUT_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer7_trigger_input_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2124L);
	APICAL_WRITE_32(0x2124L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_trigger_input_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2124L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 trigger input falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 trigger source on falling edge of input channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_INPUT_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_INPUT_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer7_trigger_input_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2124L);
	APICAL_WRITE_32(0x2124L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_trigger_input_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2124L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 trigger timer rising edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 trigger source on rising edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_TIMER_RISING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_TIMER_RISING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer7_trigger_timer_rising_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2124L);
	APICAL_WRITE_32(0x2124L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_trigger_timer_rising_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2124L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 trigger timer falling edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 trigger source on falling edge of timer channels
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_TIMER_FALLING_EDGE_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_TRIGGER_TIMER_FALLING_EDGE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_flash_timer_flash_timer_timer7_trigger_timer_falling_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2124L);
	APICAL_WRITE_32(0x2124L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_flash_timer_flash_timer_timer7_trigger_timer_falling_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2124L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 delay between trigger and output activation
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_DELAY_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_DELAY_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer7_delay_write(uint32_t data) {
	APICAL_WRITE_32(0x2128L, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer7_delay_read(void) {
	return APICAL_READ_32(0x2128L);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 duration
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 duration of output pulse (FFFFFFF is infinite, reset only by software or rettrigger)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_DURATION_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_DURATION_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_flash_timer_flash_timer_timer7_duration_write(uint32_t data) {
	APICAL_WRITE_32(0x212cL, data);
}
static __inline uint32_t apical_flash_timer_flash_timer_timer7_duration_read(void) {
	return APICAL_READ_32(0x212cL);
}
// ------------------------------------------------------------------------------ //
// Register: Timer7 counter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Timer7 counter status (internal 33 bit counter state saturated to 32 bits)
// ------------------------------------------------------------------------------ //

#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_COUNTER_DEFAULT (0x0)
#define APICAL_FLASH_TIMER_FLASH_TIMER_TIMER7_COUNTER_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_flash_timer_flash_timer_timer7_counter_read(void) {
	return APICAL_READ_32(0x2130L);
}
// ------------------------------------------------------------------------------ //
#endif //__APICAL_FLASH_TIMER_CONFIG_H__
