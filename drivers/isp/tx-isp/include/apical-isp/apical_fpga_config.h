#ifndef __APICAL_FPGA_CONFIG_H__
#define __APICAL_FPGA_CONFIG_H__


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
// Instance 'fpga' of module 'ip_config_fpga'
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_BASE_ADDR (0x20000L)
#define APICAL_FPGA_SIZE (0x1000)

// ------------------------------------------------------------------------------ //
// Group: Fpga Top
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Active Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_TOP_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_FPGA_FPGA_TOP_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_fpga_fpga_top_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20010L);
	APICAL_WRITE_32(0x20010L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_fpga_fpga_top_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20010L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Active Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_TOP_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_FPGA_FPGA_TOP_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_fpga_fpga_top_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20014L);
	APICAL_WRITE_32(0x20014L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_fpga_fpga_top_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20014L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Output sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Output selector: 0-ISP debug output 1-frame reader
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_TOP_OUTPUT_SEL_DEFAULT (1)
#define APICAL_FPGA_FPGA_TOP_OUTPUT_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_fpga_top_output_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20020L);
	APICAL_WRITE_32(0x20020L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_fpga_fpga_top_output_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20020L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Fpga Sync
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frame size and sync timing
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Offset X
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset in pixels from v-sync reference to start of active video
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_OFFSET_X_DEFAULT (0x000)
#define APICAL_FPGA_FPGA_SYNC_OFFSET_X_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_offset_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20040L);
	APICAL_WRITE_32(0x20040L, (((uint32_t) (data & 0x1fff)) << 0) | (curr & 0xffffe000));
}
static __inline uint16_t apical_fpga_fpga_sync_offset_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20040L) & 0x1fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Offset Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset in lines from v-sync reference to start of active video
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_OFFSET_Y_DEFAULT (0x000)
#define APICAL_FPGA_FPGA_SYNC_OFFSET_Y_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_offset_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20040L);
	APICAL_WRITE_32(0x20040L, (((uint32_t) (data & 0x1fff)) << 16) | (curr & 0xe000ffff));
}
static __inline uint16_t apical_fpga_fpga_sync_offset_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20040L) & 0x1fff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Total Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Total frame width including horizontal blanking
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_TOTAL_WIDTH_DEFAULT (0x672)
#define APICAL_FPGA_FPGA_SYNC_TOTAL_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_fpga_fpga_sync_total_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20044L);
	APICAL_WRITE_32(0x20044L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_fpga_fpga_sync_total_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20044L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Total Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Total frame height including vertical blanking
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_TOTAL_HEIGHT_DEFAULT (0x2EE)
#define APICAL_FPGA_FPGA_SYNC_TOTAL_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_fpga_fpga_sync_total_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20044L);
	APICAL_WRITE_32(0x20044L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_fpga_fpga_sync_total_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20044L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: H Front Porch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync front porch (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_H_FRONT_PORCH_DEFAULT (0x048)
#define APICAL_FPGA_FPGA_SYNC_H_FRONT_PORCH_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_h_front_porch_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2004cL);
	APICAL_WRITE_32(0x2004cL, (((uint32_t) (data & 0x1fff)) << 0) | (curr & 0xffffe000));
}
static __inline uint16_t apical_fpga_fpga_sync_h_front_porch_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2004cL) & 0x1fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: V Front Porch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync front porch (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_V_FRONT_PORCH_DEFAULT (0x003)
#define APICAL_FPGA_FPGA_SYNC_V_FRONT_PORCH_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_v_front_porch_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2004cL);
	APICAL_WRITE_32(0x2004cL, (((uint32_t) (data & 0x1fff)) << 16) | (curr & 0xe000ffff));
}
static __inline uint16_t apical_fpga_fpga_sync_v_front_porch_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2004cL) & 0x1fff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: H-Sync Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync width (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_H_SYNC_WIDTH_DEFAULT (0x50)
#define APICAL_FPGA_FPGA_SYNC_H_SYNC_WIDTH_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_h_sync_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20050L);
	APICAL_WRITE_32(0x20050L, (((uint32_t) (data & 0x1fff)) << 0) | (curr & 0xffffe000));
}
static __inline uint16_t apical_fpga_fpga_sync_h_sync_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20050L) & 0x1fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: V-Sync Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync width (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_V_SYNC_WIDTH_DEFAULT (0x5)
#define APICAL_FPGA_FPGA_SYNC_V_SYNC_WIDTH_DATASIZE (13)

// args: data (13-bit)
static __inline void apical_fpga_fpga_sync_v_sync_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20050L);
	APICAL_WRITE_32(0x20050L, (((uint32_t) (data & 0x1fff)) << 16) | (curr & 0xe000ffff));
}
static __inline uint16_t apical_fpga_fpga_sync_v_sync_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20050L) & 0x1fff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: H Sync Pol
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync out polarity (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_H_SYNC_POL_DEFAULT (1)
#define APICAL_FPGA_FPGA_SYNC_H_SYNC_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_fpga_sync_h_sync_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20054L);
	APICAL_WRITE_32(0x20054L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_fpga_fpga_sync_h_sync_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20054L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: V Sync Pol
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync out polarity (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FPGA_SYNC_V_SYNC_POL_DEFAULT (1)
#define APICAL_FPGA_FPGA_SYNC_V_SYNC_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_fpga_sync_v_sync_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20054L);
	APICAL_WRITE_32(0x20054L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_fpga_fpga_sync_v_sync_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20054L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Group: Frame Reader
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: format
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_FORMAT_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_READER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_fpga_frame_reader_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20100L);
	APICAL_WRITE_32(0x20100L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_fpga_frame_reader_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20100L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rbase load
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_RBASE_LOAD_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_READER_RBASE_LOAD_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_frame_reader_rbase_load_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2010cL);
	APICAL_WRITE_32(0x2010cL, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_fpga_frame_reader_rbase_load_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2010cL) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rbase load sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selector for rbase_load strobe: 0-field, 1-configuration bit rbase_load
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_RBASE_LOAD_SEL_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_READER_RBASE_LOAD_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_frame_reader_rbase_load_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2010cL);
	APICAL_WRITE_32(0x2010cL, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_fpga_frame_reader_rbase_load_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2010cL) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: rbase
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_RBASE_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_READER_RBASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_fpga_frame_reader_rbase_write(uint32_t data) {
	APICAL_WRITE_32(0x20110L, data);
}
static __inline uint32_t apical_fpga_frame_reader_rbase_read(void) {
	return APICAL_READ_32(0x20110L);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Indicates offset in bytes from the start of one line to the next line. Should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_FPGA_FRAME_READER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_fpga_frame_reader_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0x20114L, data);
}
static __inline uint32_t apical_fpga_frame_reader_line_offset_read(void) {
	return APICAL_READ_32(0x20114L);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_AXI_PORT_ENABLE_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_READER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_frame_reader_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20104L);
	APICAL_WRITE_32(0x20104L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_fpga_frame_reader_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20104L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: config
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_CONFIG_DEFAULT (0x0000)
#define APICAL_FPGA_FRAME_READER_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_fpga_frame_reader_config_write(uint32_t data) {
	APICAL_WRITE_32(0x20120L, data);
}
static __inline uint32_t apical_fpga_frame_reader_config_read(void) {
	return APICAL_READ_32(0x20120L);
}
// ------------------------------------------------------------------------------ //
// Register: status
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_READER_STATUS_DEFAULT (0x0000)
#define APICAL_FPGA_FRAME_READER_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_reader_status_read(void) {
	return APICAL_READ_32(0x20124L);
}
// ------------------------------------------------------------------------------ //
// Group: Frame Stats
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: stats reset
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_STATS_RESET_DEFAULT (0)
#define APICAL_FPGA_FRAME_STATS_STATS_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_frame_stats_stats_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20180L);
	APICAL_WRITE_32(0x20180L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_fpga_frame_stats_stats_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20180L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: stats hold
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_STATS_HOLD_DEFAULT (0)
#define APICAL_FPGA_FRAME_STATS_STATS_HOLD_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_frame_stats_stats_hold_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20184L);
	APICAL_WRITE_32(0x20184L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_fpga_frame_stats_stats_hold_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20184L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active width min
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_MIN_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_width_min_read(void) {
	return APICAL_READ_32(0x20190L);
}
// ------------------------------------------------------------------------------ //
// Register: active width max
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_MAX_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_width_max_read(void) {
	return APICAL_READ_32(0x20194L);
}
// ------------------------------------------------------------------------------ //
// Register: active width sum
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_SUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_width_sum_read(void) {
	return APICAL_READ_32(0x20198L);
}
// ------------------------------------------------------------------------------ //
// Register: active width num
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_NUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_WIDTH_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_width_num_read(void) {
	return APICAL_READ_32(0x2019cL);
}
// ------------------------------------------------------------------------------ //
// Register: active height min
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_MIN_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_height_min_read(void) {
	return APICAL_READ_32(0x201a0L);
}
// ------------------------------------------------------------------------------ //
// Register: active height max
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_MAX_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_height_max_read(void) {
	return APICAL_READ_32(0x201a4L);
}
// ------------------------------------------------------------------------------ //
// Register: active height sum
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_SUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_height_sum_read(void) {
	return APICAL_READ_32(0x201a8L);
}
// ------------------------------------------------------------------------------ //
// Register: active height num
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_NUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_ACTIVE_HEIGHT_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_active_height_num_read(void) {
	return APICAL_READ_32(0x201acL);
}
// ------------------------------------------------------------------------------ //
// Register: hblank min
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_HBLANK_MIN_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_HBLANK_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_hblank_min_read(void) {
	return APICAL_READ_32(0x201b0L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank max
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_HBLANK_MAX_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_HBLANK_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_hblank_max_read(void) {
	return APICAL_READ_32(0x201b4L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank sum
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_HBLANK_SUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_HBLANK_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_hblank_sum_read(void) {
	return APICAL_READ_32(0x201b8L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank num
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_HBLANK_NUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_HBLANK_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_hblank_num_read(void) {
	return APICAL_READ_32(0x201bcL);
}
// ------------------------------------------------------------------------------ //
// Register: vblank min
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_VBLANK_MIN_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_VBLANK_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_vblank_min_read(void) {
	return APICAL_READ_32(0x201c0L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank max
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_VBLANK_MAX_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_VBLANK_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_vblank_max_read(void) {
	return APICAL_READ_32(0x201c4L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank sum
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_VBLANK_SUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_VBLANK_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_vblank_sum_read(void) {
	return APICAL_READ_32(0x201c8L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank num
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_FRAME_STATS_VBLANK_NUM_DEFAULT (0x0)
#define APICAL_FPGA_FRAME_STATS_VBLANK_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_fpga_frame_stats_vblank_num_read(void) {
	return APICAL_READ_32(0x201ccL);
}
// ------------------------------------------------------------------------------ //
// Group: Horizontal Shift
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// :Pixel resolution shift offset
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_HORIZONTAL_SHIFT_OFFSET_DEFAULT (0x0)
#define APICAL_FPGA_HORIZONTAL_SHIFT_OFFSET_DATASIZE (5)

// args: data (5-bit)
static __inline void apical_fpga_horizontal_shift_offset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20170L);
	APICAL_WRITE_32(0x20170L, (((uint32_t) (data & 0x1f)) << 0) | (curr & 0xffffffe0));
}
static __inline uint8_t apical_fpga_horizontal_shift_offset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20170L) & 0x1f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

#define APICAL_FPGA_HORIZONTAL_SHIFT_ENABLE_DEFAULT (0x0)
#define APICAL_FPGA_HORIZONTAL_SHIFT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_fpga_horizontal_shift_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x20170L);
	APICAL_WRITE_32(0x20170L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_fpga_horizontal_shift_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x20170L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
#endif //__APICAL_FPGA_CONFIG_H__
