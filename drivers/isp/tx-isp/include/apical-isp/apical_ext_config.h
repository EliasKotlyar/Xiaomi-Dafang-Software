#ifndef __APICAL_EXT_CONFIG_H__
#define __APICAL_EXT_CONFIG_H__


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
// Instance 'ext' of module 'ext_config'
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_BASE_ADDR (0x40000L)
#define APICAL_EXT_SIZE (0x200)

// ------------------------------------------------------------------------------ //
// Group: Sync
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

#define APICAL_EXT_SYNC_OFFSET_X_DEFAULT (0x000)
#define APICAL_EXT_SYNC_OFFSET_X_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_offset_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40000L);
	APICAL_WRITE_32(0x40000L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_ext_sync_offset_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40000L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Offset Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset in lines from v-sync reference to start of active video
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_OFFSET_Y_DEFAULT (0x000)
#define APICAL_EXT_SYNC_OFFSET_Y_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_offset_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40000L);
	APICAL_WRITE_32(0x40000L, (((uint32_t) (data & 0xfff)) << 16) | (curr & 0xf000ffff));
}
static __inline uint16_t apical_ext_sync_offset_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40000L) & 0xfff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Total Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Total frame width including horizontal blanking
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TOTAL_WIDTH_DEFAULT (0x672)
#define APICAL_EXT_SYNC_TOTAL_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_total_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40004L);
	APICAL_WRITE_32(0x40004L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sync_total_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40004L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Total Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Total frame height including vertical blanking
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TOTAL_HEIGHT_DEFAULT (0x2EE)
#define APICAL_EXT_SYNC_TOTAL_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_total_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40004L);
	APICAL_WRITE_32(0x40004L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_total_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40004L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Active Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_ACTIVE_WIDTH_DEFAULT (0x500)
#define APICAL_EXT_SYNC_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40008L);
	APICAL_WRITE_32(0x40008L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sync_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40008L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Active Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_ACTIVE_HEIGHT_DEFAULT (0x2D0)
#define APICAL_EXT_SYNC_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40008L);
	APICAL_WRITE_32(0x40008L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40008L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: H Front Porch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync front porch (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_H_FRONT_PORCH_DEFAULT (0x048)
#define APICAL_EXT_SYNC_H_FRONT_PORCH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_h_front_porch_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4000cL);
	APICAL_WRITE_32(0x4000cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_ext_sync_h_front_porch_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4000cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: V Front Porch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync front porch (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_V_FRONT_PORCH_DEFAULT (0x003)
#define APICAL_EXT_SYNC_V_FRONT_PORCH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_v_front_porch_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4000cL);
	APICAL_WRITE_32(0x4000cL, (((uint32_t) (data & 0xfff)) << 16) | (curr & 0xf000ffff));
}
static __inline uint16_t apical_ext_sync_v_front_porch_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4000cL) & 0xfff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: H-Sync Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync width (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_H_SYNC_WIDTH_DEFAULT (0x50)
#define APICAL_EXT_SYNC_H_SYNC_WIDTH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_h_sync_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40010L);
	APICAL_WRITE_32(0x40010L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_ext_sync_h_sync_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40010L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: V-Sync Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync width (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_V_SYNC_WIDTH_DEFAULT (0x5)
#define APICAL_EXT_SYNC_V_SYNC_WIDTH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_v_sync_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40010L);
	APICAL_WRITE_32(0x40010L, (((uint32_t) (data & 0xfff)) << 16) | (curr & 0xf000ffff));
}
static __inline uint16_t apical_ext_sync_v_sync_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40010L) & 0xfff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Control
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_CONTROL_DEFAULT (0x8D)
#define APICAL_EXT_SYNC_CONTROL_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_sync_control_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40014L);
	APICAL_WRITE_32(0x40014L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_sync_control_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40014L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: H Sync Out Pol
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal sync out polarity (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_H_SYNC_OUT_POL_DEFAULT (0x0)
#define APICAL_EXT_SYNC_H_SYNC_OUT_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_h_sync_out_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40014L);
	APICAL_WRITE_32(0x40014L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_ext_sync_h_sync_out_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40014L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: V Sync Out Pol
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical sync out polarity (for DVI out)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_V_SYNC_OUT_POL_DEFAULT (0x0)
#define APICAL_EXT_SYNC_V_SYNC_OUT_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_v_sync_out_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40014L);
	APICAL_WRITE_32(0x40014L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_ext_sync_v_sync_out_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40014L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: DVI Tx Clock Edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// DVI output clock edge: 0=falling 1=rising
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_DVI_TX_CLOCK_EDGE_DEFAULT (0)
#define APICAL_EXT_SYNC_DVI_TX_CLOCK_EDGE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_dvi_tx_clock_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_ext_sync_dvi_tx_clock_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: DVI Rx Clock Edge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Video input clock phase: 0=0 1=90 2=180 3=270
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_DVI_RX_CLOCK_EDGE_DEFAULT (2)
#define APICAL_EXT_SYNC_DVI_RX_CLOCK_EDGE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_ext_sync_dvi_rx_clock_edge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x3)) << 1) | (curr & 0xfffffff9));
}
static __inline uint8_t apical_ext_sync_dvi_rx_clock_edge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x6) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: H Sync In Pol
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_H_SYNC_IN_POL_DEFAULT (0)
#define APICAL_EXT_SYNC_H_SYNC_IN_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_h_sync_in_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_ext_sync_h_sync_in_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: V Sync In Pol
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_V_SYNC_IN_POL_DEFAULT (0)
#define APICAL_EXT_SYNC_V_SYNC_IN_POL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_v_sync_in_pol_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_ext_sync_v_sync_in_pol_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: Field Mode
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_FIELD_MODE_DEFAULT (0)
#define APICAL_EXT_SYNC_FIELD_MODE_DATASIZE (2)
#define APICAL_EXT_SYNC_FIELD_MODE_SAME_AS_EXTERNAL (0)
#define APICAL_EXT_SYNC_FIELD_MODE_ADJUST_WITH_ACTIVELINE (1)
#define APICAL_EXT_SYNC_FIELD_MODE_ADJUST_WITH_VSYNC (2)
#define APICAL_EXT_SYNC_FIELD_MODE_RESERVED (3)

// args: data (2-bit)
static __inline void apical_ext_sync_field_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x3)) << 12) | (curr & 0xffffcfff));
}
static __inline uint8_t apical_ext_sync_field_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x3000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: Auto Pos
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_AUTO_POS_DEFAULT (0)
#define APICAL_EXT_SYNC_AUTO_POS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_auto_pos_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x1)) << 14) | (curr & 0xffffbfff));
}
static __inline uint8_t apical_ext_sync_auto_pos_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x4000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: Auto Size
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_AUTO_SIZE_DEFAULT (0)
#define APICAL_EXT_SYNC_AUTO_SIZE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_auto_size_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40020L);
	APICAL_WRITE_32(0x40020L, (((uint32_t) (data & 0x1)) << 15) | (curr & 0xffff7fff));
}
static __inline uint8_t apical_ext_sync_auto_size_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40020L) & 0x8000) >> 15);
}
// ------------------------------------------------------------------------------ //
// Register: max line length
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MAX_LINE_LENGTH_DEFAULT (0xfff)
#define APICAL_EXT_SYNC_MAX_LINE_LENGTH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_max_line_length_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_ext_sync_max_line_length_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40030L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max line length dis y
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MAX_LINE_LENGTH_DIS_Y_DEFAULT (0xfff)
#define APICAL_EXT_SYNC_MAX_LINE_LENGTH_DIS_Y_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_ext_sync_max_line_length_dis_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4003cL);
	APICAL_WRITE_32(0x4003cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_ext_sync_max_line_length_dis_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4003cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Input field toggle
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_INPUT_FIELD_TOGGLE_DEFAULT (0)
#define APICAL_EXT_SYNC_INPUT_FIELD_TOGGLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_input_field_toggle_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_ext_sync_input_field_toggle_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Input field polarity
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_INPUT_FIELD_POLARITY_DEFAULT (0)
#define APICAL_EXT_SYNC_INPUT_FIELD_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_input_field_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_ext_sync_input_field_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Output field manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_OUTPUT_FIELD_MANUAL_DEFAULT (0)
#define APICAL_EXT_SYNC_OUTPUT_FIELD_MANUAL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_output_field_manual_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_ext_sync_output_field_manual_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Output field value
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_OUTPUT_FIELD_VALUE_DEFAULT (0)
#define APICAL_EXT_SYNC_OUTPUT_FIELD_VALUE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_output_field_value_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_ext_sync_output_field_value_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Jumbo Frame mode
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_JUMBO_FRAME_MODE_DEFAULT (0)
#define APICAL_EXT_SYNC_JUMBO_FRAME_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_jumbo_frame_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 19) | (curr & 0xfff7ffff));
}
static __inline uint8_t apical_ext_sync_jumbo_frame_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x80000) >> 19);
}
// ------------------------------------------------------------------------------ //
// Register: Mask enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enable masking of frame edges based on the values of Mask x and Mask y
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MASK_ENABLE_DEFAULT (0)
#define APICAL_EXT_SYNC_MASK_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_mask_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 22) | (curr & 0xffbfffff));
}
static __inline uint8_t apical_ext_sync_mask_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x400000) >> 22);
}
// ------------------------------------------------------------------------------ //
// Register: Mask auto
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Calculate mask size based on offsets
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MASK_AUTO_DEFAULT (0)
#define APICAL_EXT_SYNC_MASK_AUTO_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_sync_mask_auto_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40030L);
	APICAL_WRITE_32(0x40030L, (((uint32_t) (data & 0x1)) << 23) | (curr & 0xff7fffff));
}
static __inline uint8_t apical_ext_sync_mask_auto_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40030L) & 0x800000) >> 23);
}
// ------------------------------------------------------------------------------ //
// Register: Dis offset x
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_DIS_OFFSET_X_DEFAULT (0)
#define APICAL_EXT_SYNC_DIS_OFFSET_X_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_sync_dis_offset_x_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40034L);
	APICAL_WRITE_32(0x40034L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_sync_dis_offset_x_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40034L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dis offset y
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_DIS_OFFSET_Y_DEFAULT (0)
#define APICAL_EXT_SYNC_DIS_OFFSET_Y_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_sync_dis_offset_y_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40034L);
	APICAL_WRITE_32(0x40034L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_sync_dis_offset_y_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40034L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Scale y
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_SCALE_Y_DEFAULT (0)
#define APICAL_EXT_SYNC_SCALE_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_scale_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40034L);
	APICAL_WRITE_32(0x40034L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_scale_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40034L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Skew x
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_SKEW_X_DEFAULT (0)
#define APICAL_EXT_SYNC_SKEW_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_skew_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40038L);
	APICAL_WRITE_32(0x40038L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sync_skew_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40038L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mask x
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MASK_X_DEFAULT (0)
#define APICAL_EXT_SYNC_MASK_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_mask_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40038L);
	APICAL_WRITE_32(0x40038L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_mask_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40038L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Mask y
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_MASK_Y_DEFAULT (0)
#define APICAL_EXT_SYNC_MASK_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_mask_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4003cL);
	APICAL_WRITE_32(0x4003cL, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_mask_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4003cL) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: TG Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// sensor TG full line length
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TG_WIDTH_DEFAULT (2401)
#define APICAL_EXT_SYNC_TG_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_tg_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40040L);
	APICAL_WRITE_32(0x40040L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sync_tg_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40040L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: TG Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// sensor TG full frame height
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TG_HEIGHT_DEFAULT (1125)
#define APICAL_EXT_SYNC_TG_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_tg_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40040L);
	APICAL_WRITE_32(0x40040L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_tg_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40040L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Temper Active Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TEMPER_ACTIVE_WIDTH_DEFAULT (0x500)
#define APICAL_EXT_SYNC_TEMPER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_temper_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40048L);
	APICAL_WRITE_32(0x40048L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sync_temper_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40048L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Temper Active Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYNC_TEMPER_ACTIVE_HEIGHT_DEFAULT (0x2D0)
#define APICAL_EXT_SYNC_TEMPER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sync_temper_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40048L);
	APICAL_WRITE_32(0x40048L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sync_temper_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40048L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Group: System CCM
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// System reference CCM
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Coefft R-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_R_DEFAULT (0x01D0)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_r_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40080L);
	APICAL_WRITE_32(0x40080L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_ccm_coefft_r_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40080L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft R-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_G_DEFAULT (0x8150)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_r_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40080L);
	APICAL_WRITE_32(0x40080L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_system_ccm_coefft_r_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40080L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft R-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_B_DEFAULT (0x0080)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_R_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_r_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40084L);
	APICAL_WRITE_32(0x40084L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_ccm_coefft_r_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40084L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_R_DEFAULT (0x8050)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_g_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40084L);
	APICAL_WRITE_32(0x40084L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_system_ccm_coefft_g_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40084L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_G_DEFAULT (0x01A0)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_g_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40088L);
	APICAL_WRITE_32(0x40088L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_ccm_coefft_g_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40088L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_B_DEFAULT (0x8050)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_G_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_g_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40088L);
	APICAL_WRITE_32(0x40088L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_system_ccm_coefft_g_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40088L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_R_DEFAULT (0x8030)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_b_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4008cL);
	APICAL_WRITE_32(0x4008cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_ccm_coefft_b_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4008cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_G_DEFAULT (0x80C0)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_b_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4008cL);
	APICAL_WRITE_32(0x4008cL, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_system_ccm_coefft_b_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4008cL) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_B_DEFAULT (0x01F0)
#define APICAL_EXT_SYSTEM_CCM_COEFFT_B_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_ccm_coefft_b_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40090L);
	APICAL_WRITE_32(0x40090L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_ccm_coefft_b_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40090L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Exposure Status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Exposure status registers
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Analog gain status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Analog gain status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_ANALOG_GAIN_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_ANALOG_GAIN_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_analog_gain_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400a8L);
	APICAL_WRITE_32(0x400a8L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_exposure_status_analog_gain_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400a8L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Coarse digital gain status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Coarse digital gain status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_COARSE_DIGITAL_GAIN_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_COARSE_DIGITAL_GAIN_STATUS_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_exposure_status_coarse_digital_gain_status_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_exposure_status_coarse_digital_gain_status_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Fine digital gain status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Fine digital gain status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_FINE_DIGITAL_GAIN_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_FINE_DIGITAL_GAIN_STATUS_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_ext_exposure_status_fine_digital_gain_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0x3ff)) << 16) | (curr & 0xfc00ffff));
}
static __inline uint16_t apical_ext_exposure_status_fine_digital_gain_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400a0L) & 0x3ff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Long integration time status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Long integration time status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_LONG_INTEGRATION_TIME_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_LONG_INTEGRATION_TIME_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_long_integration_time_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400a4L);
	APICAL_WRITE_32(0x400a4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_exposure_status_long_integration_time_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400a4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Short integration time status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Short integration time status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_SHORT_INTEGRATION_TIME_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_SHORT_INTEGRATION_TIME_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_short_integration_time_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400a4L);
	APICAL_WRITE_32(0x400a4L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_exposure_status_short_integration_time_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400a4L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Average brightness status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Average brightness status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_AVERAGE_BRIGHTNESS_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_AVERAGE_BRIGHTNESS_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_average_brightness_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400a8L);
	APICAL_WRITE_32(0x400a8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_exposure_status_average_brightness_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400a8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Illumination status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor Illumination status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_ILLUMINATION_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_ILLUMINATION_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_illumination_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400acL);
	APICAL_WRITE_32(0x400acL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_exposure_status_illumination_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400acL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Exposure Ratio status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor Illumination status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_EXPOSURE_RATIO_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_EXPOSURE_RATIO_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_exposure_status_exposure_ratio_status_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400acL);
	APICAL_WRITE_32(0x400acL, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_exposure_status_exposure_ratio_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400acL) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Anti-flicker status
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_ANTI_FLICKER_STATUS_DEFAULT (0x0)
#define APICAL_EXT_EXPOSURE_STATUS_ANTI_FLICKER_STATUS_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_exposure_status_anti_flicker_status_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_exposure_status_anti_flicker_status_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Short overexposed
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Short overexposed
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_SHORT_OVEREXPOSED_DEFAULT (0)
#define APICAL_EXT_EXPOSURE_STATUS_SHORT_OVEREXPOSED_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_exposure_status_short_overexposed_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_ext_exposure_status_short_overexposed_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Short underexposed
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Short underexposed
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_SHORT_UNDEREXPOSED_DEFAULT (0)
#define APICAL_EXT_EXPOSURE_STATUS_SHORT_UNDEREXPOSED_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_exposure_status_short_underexposed_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_ext_exposure_status_short_underexposed_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Long overexposed
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Long overexposed
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_LONG_OVEREXPOSED_DEFAULT (0)
#define APICAL_EXT_EXPOSURE_STATUS_LONG_OVEREXPOSED_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_exposure_status_long_overexposed_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_ext_exposure_status_long_overexposed_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Long underexposed
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Long underexposed
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_EXPOSURE_STATUS_LONG_UNDEREXPOSED_DEFAULT (0)
#define APICAL_EXT_EXPOSURE_STATUS_LONG_UNDEREXPOSED_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_exposure_status_long_underexposed_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400a0L);
	APICAL_WRITE_32(0x400a0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_ext_exposure_status_long_underexposed_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400a0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Group: Sensor Access
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Indirect access to sensor registers
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Custom sensor addr0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor address register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR0_DEFAULT (0xFFFF)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR0_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_addr0_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b0L);
	APICAL_WRITE_32(0x400b0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_addr0_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Custom sensor data0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor data register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA0_DEFAULT (0x0)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA0_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_data0_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b0L);
	APICAL_WRITE_32(0x400b0L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_data0_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b0L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Custom sensor addr1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor address register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR1_DEFAULT (0xFFFF)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR1_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_addr1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b4L);
	APICAL_WRITE_32(0x400b4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_addr1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Custom sensor data1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor data register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA1_DEFAULT (0x0)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA1_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_data1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b4L);
	APICAL_WRITE_32(0x400b4L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_data1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b4L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Custom sensor addr2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor address register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR2_DEFAULT (0xFFFF)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_ADDR2_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_addr2_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b8L);
	APICAL_WRITE_32(0x400b8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_addr2_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Custom sensor data2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor data register 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA2_DEFAULT (0x0)
#define APICAL_EXT_SENSOR_ACCESS_CUSTOM_SENSOR_DATA2_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_sensor_access_custom_sensor_data2_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x400b8L);
	APICAL_WRITE_32(0x400b8L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_sensor_access_custom_sensor_data2_read(void) {
	return (uint16_t)((APICAL_READ_32(0x400b8L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Group: General Purpose
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Miscellaneous registers
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: gpi
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Status of push buttons
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_GPI_DEFAULT (0x0)
#define APICAL_EXT_GENERAL_PURPOSE_GPI_DATASIZE (4)

// args: data (4-bit)
static __inline uint8_t apical_ext_general_purpose_gpi_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c0L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: illumination target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 1
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_ILLUMINATION_TARGET_DEFAULT (0xFF)
#define APICAL_EXT_GENERAL_PURPOSE_ILLUMINATION_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_illumination_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c4L);
	APICAL_WRITE_32(0x400c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_general_purpose_illumination_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: equilibrium point
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 2
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_EQUILIBRIUM_POINT_DEFAULT (0x0)
#define APICAL_EXT_GENERAL_PURPOSE_EQUILIBRIUM_POINT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_equilibrium_point_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c4L);
	APICAL_WRITE_32(0x400c4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_general_purpose_equilibrium_point_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: iris rate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 3
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_IRIS_RATE_DEFAULT (0xFF)
#define APICAL_EXT_GENERAL_PURPOSE_IRIS_RATE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_iris_rate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c8L);
	APICAL_WRITE_32(0x400c8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_general_purpose_iris_rate_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: debug 4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 4
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_4_DEFAULT (0xFF)
#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_4_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_debug_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c8L);
	APICAL_WRITE_32(0x400c8L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_general_purpose_debug_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c8L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: debug 5
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 5
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_5_DEFAULT (0xFF)
#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_5_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_debug_5_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c8L);
	APICAL_WRITE_32(0x400c8L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_general_purpose_debug_5_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c8L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: debug 6
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// debug register 6
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_6_DEFAULT (0xFF)
#define APICAL_EXT_GENERAL_PURPOSE_DEBUG_6_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_general_purpose_debug_6_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400c8L);
	APICAL_WRITE_32(0x400c8L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_general_purpose_debug_6_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400c8L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: misc control
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 32 bit control output
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_GENERAL_PURPOSE_MISC_CONTROL_DEFAULT (0x00000001)
#define APICAL_EXT_GENERAL_PURPOSE_MISC_CONTROL_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_ext_general_purpose_misc_control_write(uint32_t data) {
	APICAL_WRITE_32(0x400ccL, data);
}
static __inline uint32_t apical_ext_general_purpose_misc_control_read(void) {
	return APICAL_READ_32(0x400ccL);
}
// ------------------------------------------------------------------------------ //
// Group: Flash interface
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Flash controller
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Trigger Select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Xenon flash trigger mode: 0 - Manual, 1 - from sensor, 2 - from timing generator, 3 - reserved
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_TRIGGER_SELECT_DEFAULT (00)
#define APICAL_EXT_FLASH_INTERFACE_TRIGGER_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_ext_flash_interface_trigger_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400d0L);
	APICAL_WRITE_32(0x400d0L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_ext_flash_interface_trigger_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d0L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Sensor strobe polarity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 - invert sensor strobe
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_SENSOR_STROBE_POLARITY_DEFAULT (0)
#define APICAL_EXT_FLASH_INTERFACE_SENSOR_STROBE_POLARITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_flash_interface_sensor_strobe_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400d0L);
	APICAL_WRITE_32(0x400d0L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_ext_flash_interface_sensor_strobe_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d0L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: AF LED Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Turn on AF LED
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_AF_LED_ENABLE_DEFAULT (00)
#define APICAL_EXT_FLASH_INTERFACE_AF_LED_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_flash_interface_af_led_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400d0L);
	APICAL_WRITE_32(0x400d0L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_ext_flash_interface_af_led_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d0L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Charge
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Charge the flash module
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_CHARGE_DEFAULT (00)
#define APICAL_EXT_FLASH_INTERFACE_CHARGE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_flash_interface_charge_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400d0L);
	APICAL_WRITE_32(0x400d0L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_ext_flash_interface_charge_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d0L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Flash trigger
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 - invert sensor strobe
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_FLASH_TRIGGER_DEFAULT (00)
#define APICAL_EXT_FLASH_INTERFACE_FLASH_TRIGGER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_flash_interface_flash_trigger_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400d0L);
	APICAL_WRITE_32(0x400d0L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_ext_flash_interface_flash_trigger_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d0L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Flash ready
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 - flash is charged
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_FLASH_READY_DEFAULT (0x0)
#define APICAL_EXT_FLASH_INTERFACE_FLASH_READY_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_ext_flash_interface_flash_ready_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Flash strobe
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// strobe signal from sensor module
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_FLASH_INTERFACE_FLASH_STROBE_DEFAULT (0x0)
#define APICAL_EXT_FLASH_INTERFACE_FLASH_STROBE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_ext_flash_interface_flash_strobe_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400d4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Group: System
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_ADDR_MIN 0x40100L
#define APICAL_EXT_SYSTEM_ADDR_MAX 0x401f7L
// ------------------------------------------------------------------------------ //
// High-level interface to the ISP
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Flags1
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_FLAGS1_DEFAULT (0x0)
#define APICAL_EXT_SYSTEM_FLAGS1_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_flags1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_flags1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Freeze Firmware
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Disables firmware and stops updating the ISP
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_FREEZE_FIRMWARE_DEFAULT (0)
#define APICAL_EXT_SYSTEM_FREEZE_FIRMWARE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_freeze_firmware_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_ext_system_freeze_firmware_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Exposure
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual exposure: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_exposure_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_ext_system_manual_exposure_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Exposure Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual exposure ratio control
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_RATIO_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_RATIO_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_exposure_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_ext_system_manual_exposure_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Integration Time
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual Integration time control: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_INTEGRATION_TIME_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_INTEGRATION_TIME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_integration_time_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_ext_system_manual_integration_time_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Sensor Analog Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual control of sensor analog gain: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_SENSOR_ANALOG_GAIN_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_SENSOR_ANALOG_GAIN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_sensor_analog_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_ext_system_manual_sensor_analog_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Sensor Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual control of sensor digital gain: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_SENSOR_DIGITAL_GAIN_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_SENSOR_DIGITAL_GAIN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_sensor_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_ext_system_manual_sensor_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Manual ISP Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual control of ISP digital gain: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_ISP_DIGITAL_GAIN_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_ISP_DIGITAL_GAIN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_isp_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_ext_system_manual_isp_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual control of directional sharpening strength: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_DIRECTIONAL_SHARPENING_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_DIRECTIONAL_SHARPENING_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_ext_system_manual_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: Flags2
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_FLAGS2_DEFAULT (0x0)
#define APICAL_EXT_SYSTEM_FLAGS2_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_flags2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_flags2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Un-Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual control of un-directional sharpening strength: 0=auto 1=manual
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_UN_DIRECTIONAL_SHARPENING_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_UN_DIRECTIONAL_SHARPENING_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_un_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_ext_system_manual_un_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Iridix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual iridix control - iridix strength target controls iridix strength directly
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_IRIDIX_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_IRIDIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_iridix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_ext_system_manual_iridix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Sinter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual sinter control - sinter strength target controls sinter strength directly
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_SINTER_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_SINTER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_sinter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_ext_system_manual_sinter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Temper
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual temper control - temper strength target controls temper strength directly
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_TEMPER_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_TEMPER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_temper_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_ext_system_manual_temper_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: Manual AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual AWB control
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_AWB_DEFAULT (0)
#define APICAL_EXT_SYSTEM_MANUAL_AWB_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_awb_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 12) | (curr & 0xffffefff));
}
static __inline uint8_t apical_ext_system_manual_awb_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x1000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: Antiflicker Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables the Anti Flicker function
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_ANTIFLICKER_ENABLE_DEFAULT (0)
#define APICAL_EXT_SYSTEM_ANTIFLICKER_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_antiflicker_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 13) | (curr & 0xffffdfff));
}
static __inline uint8_t apical_ext_system_antiflicker_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x2000) >> 13);
}
// ------------------------------------------------------------------------------ //
// Register: Slow Frame Rate Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables slow frame rates.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SLOW_FRAME_RATE_ENABLE_DEFAULT (1)
#define APICAL_EXT_SYSTEM_SLOW_FRAME_RATE_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_slow_frame_rate_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 14) | (curr & 0xffffbfff));
}
static __inline uint8_t apical_ext_system_slow_frame_rate_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x4000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Saturation
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables manual saturation control
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_SATURATION_DEFAULT (1)
#define APICAL_EXT_SYSTEM_MANUAL_SATURATION_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_manual_saturation_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40100L);
	APICAL_WRITE_32(0x40100L, (((uint32_t) (data & 0x1)) << 15) | (curr & 0xffff7fff));
}
static __inline uint8_t apical_ext_system_manual_saturation_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40100L) & 0x8000) >> 15);
}
// ------------------------------------------------------------------------------ //
// Register: Frame rates
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_FRAME_RATES_DEFAULT (0x0)
#define APICAL_EXT_SYSTEM_FRAME_RATES_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_frame_rates_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40104L);
	APICAL_WRITE_32(0x40104L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_frame_rates_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40104L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Set fps Base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0000 Force the system to use 60fps as the base frame rate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0001 Force the system to use 50fps as the base frame rate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0010 Force the system to use 30fps as the base frame rate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0011 Force the system to use 25fps as the base frame rate
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SET_FPS_BASE_DEFAULT (0)
#define APICAL_EXT_SYSTEM_SET_FPS_BASE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_ext_system_set_fps_base_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40104L);
	APICAL_WRITE_32(0x40104L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_ext_system_set_fps_base_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40104L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Set 1001 rate divider
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Set 1/1.001 rate multiplier to achieve 29.97 FPS
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SET_1001_RATE_DIVIDER_DEFAULT (0)
#define APICAL_EXT_SYSTEM_SET_1001_RATE_DIVIDER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_set_1001_rate_divider_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40104L);
	APICAL_WRITE_32(0x40104L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_ext_system_set_1001_rate_divider_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40104L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Half pixel clock
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Corrects internal interface dividers to match the case when pixel clock is slowed down
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_HALF_PIXEL_CLOCK_DEFAULT (0)
#define APICAL_EXT_SYSTEM_HALF_PIXEL_CLOCK_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_half_pixel_clock_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40104L);
	APICAL_WRITE_32(0x40104L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_ext_system_half_pixel_clock_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40104L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Calibrate flag
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CALIBRATE_FLAG_DEFAULT (0x0)
#define APICAL_EXT_SYSTEM_CALIBRATE_FLAG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_calibrate_flag_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40108L);
	APICAL_WRITE_32(0x40108L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_calibrate_flag_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40108L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Calibrate Bad Pixels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Start calibration algorithm for defect pixel correction, bit will be cleared by FW when finished
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_CALIBRATE_BAD_PIXELS_DEFAULT (0)
#define APICAL_EXT_SYSTEM_CALIBRATE_BAD_PIXELS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_ext_system_calibrate_bad_pixels_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40108L);
	APICAL_WRITE_32(0x40108L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_ext_system_calibrate_bad_pixels_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40108L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Manual Exposure Time
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sensor integration time (in lines) for manual exposure
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_TIME_DEFAULT (0x00000000)
#define APICAL_EXT_SYSTEM_MANUAL_EXPOSURE_TIME_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_ext_system_manual_exposure_time_write(uint32_t data) {
	APICAL_WRITE_32(0x4010cL, data);
}
static __inline uint32_t apical_ext_system_manual_exposure_time_read(void) {
	return APICAL_READ_32(0x4010cL);
}
// ------------------------------------------------------------------------------ //
// Register: Exposure Dark Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets dark histogram target.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_EXPOSURE_DARK_TARGET_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_EXPOSURE_DARK_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_exposure_dark_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40110L);
	APICAL_WRITE_32(0x40110L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_exposure_dark_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40110L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Exposure Bright Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets bright histogram target.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_EXPOSURE_BRIGHT_TARGET_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_EXPOSURE_BRIGHT_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_exposure_bright_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40110L);
	APICAL_WRITE_32(0x40110L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_exposure_bright_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40110L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Exposure Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Ratio between short and long exposure. A ratio of one gives standard non-wdr mode
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_EXPOSURE_RATIO_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_EXPOSURE_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_exposure_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40110L);
	APICAL_WRITE_32(0x40110L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_exposure_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40110L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Max Exposure Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum Ratio between short and long exposure in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAX_EXPOSURE_RATIO_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MAX_EXPOSURE_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_max_exposure_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40110L);
	APICAL_WRITE_32(0x40110L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_max_exposure_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40110L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Integration Time
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual sensor integration time (in lines)
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_INTEGRATION_TIME_DEFAULT (0x0002)
#define APICAL_EXT_SYSTEM_INTEGRATION_TIME_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_integration_time_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40114L);
	APICAL_WRITE_32(0x40114L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_ext_system_integration_time_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40114L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Max Integration Time
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum sensor integration time (in lines) for both manual and auto exposure
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAX_INTEGRATION_TIME_DEFAULT (0xFFFF)
#define APICAL_EXT_SYSTEM_MAX_INTEGRATION_TIME_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_ext_system_max_integration_time_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x40114L);
	APICAL_WRITE_32(0x40114L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_ext_system_max_integration_time_read(void) {
	return (uint16_t)((APICAL_READ_32(0x40114L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Sensor Analog Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual sensor analog gain.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SENSOR_ANALOG_GAIN_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_SENSOR_ANALOG_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_sensor_analog_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40118L);
	APICAL_WRITE_32(0x40118L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_sensor_analog_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40118L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Max Sensor Analog Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum sensor analogue gain that can be applied.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAX_SENSOR_ANALOG_GAIN_DEFAULT (0x35)
#define APICAL_EXT_SYSTEM_MAX_SENSOR_ANALOG_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_max_sensor_analog_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40118L);
	APICAL_WRITE_32(0x40118L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_max_sensor_analog_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40118L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Sensor Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual sensor digital gain.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SENSOR_DIGITAL_GAIN_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_SENSOR_DIGITAL_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_sensor_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4011cL);
	APICAL_WRITE_32(0x4011cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_sensor_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4011cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Max Sensor Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum sensor digital gain that can be applied.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAX_SENSOR_DIGITAL_GAIN_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MAX_SENSOR_DIGITAL_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_max_sensor_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4011cL);
	APICAL_WRITE_32(0x4011cL, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_max_sensor_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4011cL) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: ISP Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual ISP digital gain.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_ISP_DIGITAL_GAIN_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_ISP_DIGITAL_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_isp_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40120L);
	APICAL_WRITE_32(0x40120L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_isp_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40120L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Max ISP Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum ISP digital gain that can be applied.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAX_ISP_DIGITAL_GAIN_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MAX_ISP_DIGITAL_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_max_isp_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40120L);
	APICAL_WRITE_32(0x40120L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_max_isp_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40120L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Directional Sharpening Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Directional sharpening target. In manual mode this directly sets the directional sharpening strength.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DIRECTIONAL_SHARPENING_TARGET_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_DIRECTIONAL_SHARPENING_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_directional_sharpening_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40124L);
	APICAL_WRITE_32(0x40124L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_directional_sharpening_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40124L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Maximum Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum directional sharpening in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAXIMUM_DIRECTIONAL_SHARPENING_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MAXIMUM_DIRECTIONAL_SHARPENING_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_maximum_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40124L);
	APICAL_WRITE_32(0x40124L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_maximum_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40124L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Minimum Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum directional sharpening in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MINIMUM_DIRECTIONAL_SHARPENING_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MINIMUM_DIRECTIONAL_SHARPENING_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_minimum_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40124L);
	APICAL_WRITE_32(0x40124L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_minimum_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40124L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Un-Directional Sharpening Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Un-directional sharpening target. In manual mode this directly sets the un-directional sharpening strength.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_UN_DIRECTIONAL_SHARPENING_TARGET_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_UN_DIRECTIONAL_SHARPENING_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_un_directional_sharpening_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40128L);
	APICAL_WRITE_32(0x40128L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_un_directional_sharpening_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40128L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Maximum Un-Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum un-directional sharpening in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAXIMUM_UN_DIRECTIONAL_SHARPENING_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MAXIMUM_UN_DIRECTIONAL_SHARPENING_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_maximum_un_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40128L);
	APICAL_WRITE_32(0x40128L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_maximum_un_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40128L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Minimum Un-Directional Sharpening
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum un-directional sharpening in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MINIMUM_UN_DIRECTIONAL_SHARPENING_DEFAULT (0x08)
#define APICAL_EXT_SYSTEM_MINIMUM_UN_DIRECTIONAL_SHARPENING_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_minimum_un_directional_sharpening_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40128L);
	APICAL_WRITE_32(0x40128L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_minimum_un_directional_sharpening_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40128L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Iridix Strength Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual setting for Iridix strength
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_IRIDIX_STRENGTH_TARGET_DEFAULT (0x80)
#define APICAL_EXT_SYSTEM_IRIDIX_STRENGTH_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_iridix_strength_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4012cL);
	APICAL_WRITE_32(0x4012cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_iridix_strength_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4012cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Maximum Iridix Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum iridix strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAXIMUM_IRIDIX_STRENGTH_DEFAULT (0x80)
#define APICAL_EXT_SYSTEM_MAXIMUM_IRIDIX_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_maximum_iridix_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4012cL);
	APICAL_WRITE_32(0x4012cL, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_maximum_iridix_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4012cL) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Minimum Iridix Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum iridix strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MINIMUM_IRIDIX_STRENGTH_DEFAULT (0x80)
#define APICAL_EXT_SYSTEM_MINIMUM_IRIDIX_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_minimum_iridix_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4012cL);
	APICAL_WRITE_32(0x4012cL, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_minimum_iridix_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4012cL) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Sinter Threshold Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Overall strength of Sinter noise-reduction effect
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SINTER_THRESHOLD_TARGET_DEFAULT (0x18)
#define APICAL_EXT_SYSTEM_SINTER_THRESHOLD_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_sinter_threshold_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40130L);
	APICAL_WRITE_32(0x40130L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_sinter_threshold_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40130L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Maximum Sinter Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum sinter strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAXIMUM_SINTER_STRENGTH_DEFAULT (0x18)
#define APICAL_EXT_SYSTEM_MAXIMUM_SINTER_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_maximum_sinter_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40130L);
	APICAL_WRITE_32(0x40130L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_maximum_sinter_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40130L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Minimum Sinter Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum sinter strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MINIMUM_SINTER_STRENGTH_DEFAULT (0x18)
#define APICAL_EXT_SYSTEM_MINIMUM_SINTER_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_minimum_sinter_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40130L);
	APICAL_WRITE_32(0x40130L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_minimum_sinter_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40130L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Temper Threshold Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Overall strength of Temper noise-reduction effect
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_TEMPER_THRESHOLD_TARGET_DEFAULT (0x24)
#define APICAL_EXT_SYSTEM_TEMPER_THRESHOLD_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_temper_threshold_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40134L);
	APICAL_WRITE_32(0x40134L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_temper_threshold_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40134L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Maximum Temper Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum temper strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MAXIMUM_TEMPER_STRENGTH_DEFAULT (0x24)
#define APICAL_EXT_SYSTEM_MAXIMUM_TEMPER_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_maximum_temper_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40134L);
	APICAL_WRITE_32(0x40134L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_maximum_temper_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40134L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Minimum Temper Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum temper strength in all modes.
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_MINIMUM_TEMPER_STRENGTH_DEFAULT (0x24)
#define APICAL_EXT_SYSTEM_MINIMUM_TEMPER_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_minimum_temper_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40134L);
	APICAL_WRITE_32(0x40134L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_minimum_temper_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40134L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: AWB Red Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Red channel strength multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_AWB_RED_GAIN_DEFAULT (0xA0)
#define APICAL_EXT_SYSTEM_AWB_RED_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_awb_red_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40138L);
	APICAL_WRITE_32(0x40138L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_awb_red_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40138L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB Blue Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Blue channel strength multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_AWB_BLUE_GAIN_DEFAULT (0xA0)
#define APICAL_EXT_SYSTEM_AWB_BLUE_GAIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_awb_blue_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40138L);
	APICAL_WRITE_32(0x40138L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_awb_blue_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40138L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Anti-flicker frequency
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Anti-flicker frequency, in integer format.
//A value of 0  means no Anti-flicker, this is the default setting as anti-flicker reduces the choice of integration times.
//A value of 50 should be used for 50Hz lamp flicker (exposure ratio MUST be set to 1).
//A value of 60 should be used for 60Hz lamp flicker (exposure ratio MUST be set to 1).
//Any other values are also allowed, but not practical and not being tested.
//
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_ANTI_FLICKER_FREQUENCY_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_ANTI_FLICKER_FREQUENCY_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_anti_flicker_frequency_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40138L);
	APICAL_WRITE_32(0x40138L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_anti_flicker_frequency_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40138L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Slow Frame Rate Divider
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Slow frame rate limit multiplier, in 4.4 fixed-point format.
//A value of 1.0 (0x10) means no slow frame rate allowed.
//A value of 2.0 (0x20) means frame rate can be slowed down by 2x maximum.
//
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SLOW_FRAME_RATE_DIVIDER_DEFAULT (0x10)
#define APICAL_EXT_SYSTEM_SLOW_FRAME_RATE_DIVIDER_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_slow_frame_rate_divider_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4013cL);
	APICAL_WRITE_32(0x4013cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_slow_frame_rate_divider_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4013cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AE compensation
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Shifts AE target
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_AE_COMPENSATION_DEFAULT (0x80)
#define APICAL_EXT_SYSTEM_AE_COMPENSATION_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_ae_compensation_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40140L);
	APICAL_WRITE_32(0x40140L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_ae_compensation_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40140L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Saturation Target
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Saturation target, this value used directly when Manual Saturation is used
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_SATURATION_TARGET_DEFAULT (0x80)
#define APICAL_EXT_SYSTEM_SATURATION_TARGET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_saturation_target_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40144L);
	APICAL_WRITE_32(0x40144L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_saturation_target_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40144L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 0
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_0_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_0_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f0L);
	APICAL_WRITE_32(0x401f0L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_debug_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f0L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 1
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_1_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_1_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f0L);
	APICAL_WRITE_32(0x401f0L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_debug_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f0L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 2
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_2_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_2_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f0L);
	APICAL_WRITE_32(0x401f0L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_debug_2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f0L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 3
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_3_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_3_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_3_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f0L);
	APICAL_WRITE_32(0x401f0L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_debug_3_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f0L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 4
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_4_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_4_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f4L);
	APICAL_WRITE_32(0x401f4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_ext_system_debug_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 5
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_5_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_5_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_5_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f4L);
	APICAL_WRITE_32(0x401f4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_ext_system_debug_5_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 6
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_6_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_6_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_6_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f4L);
	APICAL_WRITE_32(0x401f4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_ext_system_debug_6_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Debug 7
// ------------------------------------------------------------------------------ //

#define APICAL_EXT_SYSTEM_DEBUG_7_DEFAULT (0x00)
#define APICAL_EXT_SYSTEM_DEBUG_7_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_ext_system_debug_7_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x401f4L);
	APICAL_WRITE_32(0x401f4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_ext_system_debug_7_read(void) {
	return (uint8_t)((APICAL_READ_32(0x401f4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
#endif //__APICAL_EXT_CONFIG_H__
