#ifndef __APICAL_ISP_CONFIG_H__
#define __APICAL_ISP_CONFIG_H__


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
// Instance 'isp' of module 'ip_config'
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_BASE_ADDR (0x0L)
#define APICAL_ISP_SIZE (0x2000)

#define APICAL_ISP_MAX_ADDR (0x1ffff)

// ------------------------------------------------------------------------------ //
// Group: ID
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: API
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ID_API_DEFAULT (0x0)
#define APICAL_ISP_ID_API_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_id_api_read(void) {
	return APICAL_READ_32(0x0L);
}
// ------------------------------------------------------------------------------ //
// Register: Product
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ID_PRODUCT_DEFAULT (0x0)
#define APICAL_ISP_ID_PRODUCT_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_id_product_read(void) {
	return APICAL_READ_32(0x4L);
}
// ------------------------------------------------------------------------------ //
// Register: Version
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ID_VERSION_DEFAULT (0x0)
#define APICAL_ISP_ID_VERSION_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_id_version_read(void) {
	return APICAL_READ_32(0x8L);
}
// ------------------------------------------------------------------------------ //
// Register: Revision
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ID_REVISION_DEFAULT (0x0)
#define APICAL_ISP_ID_REVISION_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_id_revision_read(void) {
	return APICAL_READ_32(0xcL);
}
// ------------------------------------------------------------------------------ //
// Group: Top
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Miscellaneous top-level ISP controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Active Width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_TOP_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_top_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x10L);
	APICAL_WRITE_32(0x10L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_top_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x10L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Active Height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_TOP_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_top_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x14L);
	APICAL_WRITE_32(0x14L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_top_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x14L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: RGGB start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Starting color of the rggb pattern
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_RGGB_START_DEFAULT (0x0)
#define APICAL_ISP_TOP_RGGB_START_DATASIZE (2)
#define APICAL_ISP_TOP_RGGB_START_R_GR_GB_B (0)
#define APICAL_ISP_TOP_RGGB_START_GR_R_B_GB (1)
#define APICAL_ISP_TOP_RGGB_START_GB_B_R_GR (2)
#define APICAL_ISP_TOP_RGGB_START_B_GB_GR_R (3)

// args: data (2-bit)
static __inline void apical_isp_top_rggb_start_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x18L);
	APICAL_WRITE_32(0x18L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_top_rggb_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0x18L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Flush hblank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal blanking interval during regeneration (0=measured input interval)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_FLUSH_HBLANK_DEFAULT (0x20)
#define APICAL_ISP_TOP_FLUSH_HBLANK_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_top_flush_hblank_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x28L);
	APICAL_WRITE_32(0x28L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_top_flush_hblank_read(void) {
	return (uint16_t)((APICAL_READ_32(0x28L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Config Buffer Mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Select ISP configuration double-buffering mode
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_DEFAULT (0)
#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_DATASIZE (2)
#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_DISABLED_CONFIG_UPDATES_IMMEDIATELY (0)
#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_BLOCKED_CONFIG_NEVER_UPDATES (1)
#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_LOCAL_MODULE_CONFIG_UPDATES_DURING_LOCAL_VERTICAL_BLANKING (2)
#define APICAL_ISP_TOP_CONFIG_BUFFER_MODE_GLOBAL_ALL_MODULE_CONFIG_UPDATED_DURING_ISP_VERTICAL_BLANKING (3)

// args: data (2-bit)
static __inline void apical_isp_top_config_buffer_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x30L);
	APICAL_WRITE_32(0x30L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_top_config_buffer_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x30L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass video test gen
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass video test generator
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_VIDEO_TEST_GEN_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_VIDEO_TEST_GEN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_video_test_gen_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_top_bypass_video_test_gen_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass mirror
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass horizontal mirror
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_MIRROR_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_MIRROR_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_mirror_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_top_bypass_mirror_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass sensor offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass frontend black level adjustment
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_SENSOR_OFFSET_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_SENSOR_OFFSET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_sensor_offset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_top_bypass_sensor_offset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass digital gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass digital gain module
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DIGITAL_GAIN_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DIGITAL_GAIN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_digital_gain_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_top_bypass_digital_gain_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass gamma fe
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass WDR companded frontend lookup table
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_GAMMA_FE_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_GAMMA_FE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_gamma_fe_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_top_bypass_gamma_fe_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass RAW frontend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass RAW frontend (green equalization and dynamic defect pixel)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_RAW_FRONTEND_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_RAW_FRONTEND_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_raw_frontend_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_top_bypass_raw_frontend_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass defect pixel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass static defect pixel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DEFECT_PIXEL_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DEFECT_PIXEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_defect_pixel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_top_bypass_defect_pixel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass frame stitch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass frame stitching logic
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FRAME_STITCH_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_FRAME_STITCH_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_frame_stitch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_top_bypass_frame_stitch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: Gamma fe position
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass temper
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_GAMMA_FE_POSITION_DEFAULT (1)
#define APICAL_ISP_TOP_GAMMA_FE_POSITION_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_gamma_fe_position_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_top_gamma_fe_position_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass sinter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass sinter
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_SINTER_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_SINTER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_sinter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_top_bypass_sinter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass temper
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass temper
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_TEMPER_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_TEMPER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_temper_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_top_bypass_temper_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: Order Sinter Temper
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ORDER_SINTER_TEMPER_DEFAULT (0)
#define APICAL_ISP_TOP_ORDER_SINTER_TEMPER_DATASIZE (1)
#define APICAL_ISP_TOP_ORDER_SINTER_TEMPER_SINTER_BEFORE_TEMPER (0)
#define APICAL_ISP_TOP_ORDER_SINTER_TEMPER_SINTER_AFTER_TEMPER (1)

// args: data (1-bit)
static __inline void apical_isp_top_order_sinter_temper_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_top_order_sinter_temper_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass white balance
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass static white balance
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_WHITE_BALANCE_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_WHITE_BALANCE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_white_balance_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 12) | (curr & 0xffffefff));
}
static __inline uint8_t apical_isp_top_bypass_white_balance_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x1000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass radial shading
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Radial Shading
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_RADIAL_SHADING_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_RADIAL_SHADING_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_radial_shading_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 13) | (curr & 0xffffdfff));
}
static __inline uint8_t apical_isp_top_bypass_radial_shading_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x2000) >> 13);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass mesh shading
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Mesh Shading
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_MESH_SHADING_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_MESH_SHADING_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_mesh_shading_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 14) | (curr & 0xffffbfff));
}
static __inline uint8_t apical_isp_top_bypass_mesh_shading_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x4000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass iridix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Iridix
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_IRIDIX_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_IRIDIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_iridix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 15) | (curr & 0xffff7fff));
}
static __inline uint8_t apical_isp_top_bypass_iridix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x8000) >> 15);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass demosaic
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass demosaic module (output RAW data)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DEMOSAIC_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DEMOSAIC_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_demosaic_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_top_bypass_demosaic_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass color matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass color matrix
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_COLOR_MATRIX_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_COLOR_MATRIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_color_matrix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_top_bypass_color_matrix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass fr crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass crop (full resolution)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FR_CROP_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_FR_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_fr_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 18) | (curr & 0xfffbffff));
}
static __inline uint8_t apical_isp_top_bypass_fr_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass fr gamma RGB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass gamma table  (full resolution)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FR_GAMMA_RGB_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_FR_GAMMA_RGB_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_fr_gamma_rgb_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 19) | (curr & 0xfff7ffff));
}
static __inline uint8_t apical_isp_top_bypass_fr_gamma_rgb_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x80000) >> 19);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass fr sharpen
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass sharpen  (full resolution)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FR_SHARPEN_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_FR_SHARPEN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_fr_sharpen_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_isp_top_bypass_fr_sharpen_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass fr logo
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Logo (full resolution)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FR_LOGO_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_FR_LOGO_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_fr_logo_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 21) | (curr & 0xffdfffff));
}
static __inline uint8_t apical_isp_top_bypass_fr_logo_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x200000) >> 21);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass fr cs conv
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Colour Space Conversion (full resolution)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_FR_CS_CONV_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_FR_CS_CONV_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_fr_cs_conv_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 22) | (curr & 0xffbfffff));
}
static __inline uint8_t apical_isp_top_bypass_fr_cs_conv_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x400000) >> 22);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass crop (down scaled output 1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_CROP_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS1_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 24) | (curr & 0xfeffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x1000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 scaler
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Down Scaler 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_SCALER_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_DS1_SCALER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_scaler_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 25) | (curr & 0xfdffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_scaler_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x2000000) >> 25);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 gamma RGB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass gamma table  (down scaled output 1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_GAMMA_RGB_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS1_GAMMA_RGB_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_gamma_rgb_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 26) | (curr & 0xfbffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_gamma_rgb_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x4000000) >> 26);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 sharpen
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass sharpen  (down scaled output 1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_SHARPEN_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS1_SHARPEN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_sharpen_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 27) | (curr & 0xf7ffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_sharpen_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x8000000) >> 27);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 logo
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Logo (down scaled output 1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_LOGO_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS1_LOGO_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_logo_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 28) | (curr & 0xefffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_logo_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x10000000) >> 28);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds1 cs conv
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Colour Space Conversion (down scaled output 1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS1_CS_CONV_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS1_CS_CONV_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds1_cs_conv_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 29) | (curr & 0xdfffffff));
}
static __inline uint8_t apical_isp_top_bypass_ds1_cs_conv_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x20000000) >> 29);
}
// ------------------------------------------------------------------------------ //
// Register: DS1 DMA source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source for DS1 DMA data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_DS1_DMA_SOURCE_DEFAULT (0)
#define APICAL_ISP_TOP_DS1_DMA_SOURCE_DATASIZE (1)
#define APICAL_ISP_TOP_DS1_DMA_SOURCE_COLOR_SPACE_CONVERTOR (0)
#define APICAL_ISP_TOP_DS1_DMA_SOURCE_INPUT_RAW_DATA (1)

// args: data (1-bit)
static __inline void apical_isp_top_ds1_dma_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40L);
	APICAL_WRITE_32(0x40L, (((uint32_t) (data & 0x1)) << 30) | (curr & 0xbfffffff));
}
static __inline uint8_t apical_isp_top_ds1_dma_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40L) & 0x40000000) >> 30);
}
// ------------------------------------------------------------------------------ //
// Register: DS2 scaler source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Source selector for DS2 scaler
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_DS2_SCALER_SOURCE_DEFAULT (0)
#define APICAL_ISP_TOP_DS2_SCALER_SOURCE_DATASIZE (1)
#define APICAL_ISP_TOP_DS2_SCALER_SOURCE_COLOR_MATRIX (0)
#define APICAL_ISP_TOP_DS2_SCALER_SOURCE_DS1_SCALER_OUTPUT_CHAINING (1)

// args: data (1-bit)
static __inline void apical_isp_top_ds2_scaler_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_top_ds2_scaler_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass crop (down scaled output 2)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_CROP_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS2_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_top_bypass_ds2_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 scaler
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Down Scaler 2
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_SCALER_DEFAULT (1)
#define APICAL_ISP_TOP_BYPASS_DS2_SCALER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_scaler_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_top_bypass_ds2_scaler_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 gamma RGB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass gamma table  (down scaled output 2)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_GAMMA_RGB_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS2_GAMMA_RGB_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_gamma_rgb_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_top_bypass_ds2_gamma_rgb_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 sharpen
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass sharpen  (down scaled output 2)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_SHARPEN_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS2_SHARPEN_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_sharpen_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_top_bypass_ds2_sharpen_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 logo
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Logo (down scaled output 2)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_LOGO_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS2_LOGO_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_logo_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_top_bypass_ds2_logo_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Bypass ds2 cs conv
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bypass Colour Space Conversion (down scaled output 2)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_BYPASS_DS2_CS_CONV_DEFAULT (0)
#define APICAL_ISP_TOP_BYPASS_DS2_CS_CONV_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_bypass_ds2_cs_conv_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_top_bypass_ds2_cs_conv_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: ISP RAW bypass
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//   Used to select between sensor raw input or RGB888 or YUV422 input
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ISP_RAW_BYPASS_DEFAULT (0)
#define APICAL_ISP_TOP_ISP_RAW_BYPASS_DATASIZE (1)
#define APICAL_ISP_TOP_ISP_RAW_BYPASS_SELECT_PROCESSED__NORMAL_ISP_PROCESSING_WITH_IMAGE_SENSOR_RAW_DATA (0)
#define APICAL_ISP_TOP_ISP_RAW_BYPASS_BYPASS_ISP_RAW_PROCESSING_FOR_RGB888_OR_YUV422_INPUTSPASS_INPUT_RGB_OR_YUV_DATA_TO_BACKEND__DATA_IS_REINSERTED_INTO_PIPELINE_AFTER_DEMOSAIC_BLOCK__CROP_SCALING_GAMMA_AND_COLOR_SPACE_CONVERSION_ARE_AVAILABLE_FOR_RGB888_INPUT_ONLY (1)

// args: data (1-bit)
static __inline void apical_isp_top_isp_raw_bypass_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_top_isp_raw_bypass_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: ISP debug select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Debug port source select.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_DEFAULT (0)
#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_DATASIZE (2)
#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_DOWN_SCALED_STREAMED_VIDEO_1 (0)
#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_DOWN_SCALED_STREAMED_VIDEO_2 (1)
#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_FULL_RESOLUTION_STREAMED_VIDEO_AFTER_BYPASS_MUX (2)
#define APICAL_ISP_TOP_ISP_DEBUG_SELECT_INPUT_VIDEO_INTO_INPUT_PORT (3)

// args: data (2-bit)
static __inline void apical_isp_top_isp_debug_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x3)) << 10) | (curr & 0xfffff3ff));
}
static __inline uint8_t apical_isp_top_isp_debug_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0xc00) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: ISP processing bypass mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		ISP bypass modes.  For debug purposes only. Should be set to 0 during normal operation.
//		Used to bypass entire ISP after input port.  Data is sent to input of full resolution DMA Writer.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_DEFAULT (0)
#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_DATASIZE (2)
#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_FULL_PROCESSING (0)
#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_BYPASS_ENTIRE_ISP_PROCESSING_AND_OUTPUT_MOST_SIGNIFICANT_BITS_OF_RAW_SENSOR_DATA_IN_ALL_CHANNELS (1)
#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_BYPASS_ENTIRE_ISP_PROCESSING_AND_OUTPUT_RAW_SENSOR_DATA_IN_CHANNEL_1_AND_MOST_SIGNIFICANT_BITS_OF_CHANNEL_2 (2)
#define APICAL_ISP_TOP_ISP_PROCESSING_BYPASS_MODE_BYPASS_ENTIRE_ISP_PROCESSING_AND_OUTPUT_8_BIT_RGB_INPUT_AS_10_BIT_RGB_DATA (3)

// args: data (2-bit)
static __inline void apical_isp_top_isp_processing_bypass_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44L);
	APICAL_WRITE_32(0x44L, (((uint32_t) (data & 0x3)) << 13) | (curr & 0xffff9fff));
}
static __inline uint8_t apical_isp_top_isp_processing_bypass_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44L) & 0x6000) >> 13);
}
// ------------------------------------------------------------------------------ //
// Register: AE switch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AE tap in the pipeline.  Location of AE statistic collection.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_AE_SWITCH_DEFAULT (0)
#define APICAL_ISP_TOP_AE_SWITCH_DATASIZE (3)
#define APICAL_ISP_TOP_AE_SWITCH_AFTER_STATIC_WHITE_BALANCE (0)
#define APICAL_ISP_TOP_AE_SWITCH_AFTER_TPG (1)
#define APICAL_ISP_TOP_AE_SWITCH_AFTER_SHADING (2)
#define APICAL_ISP_TOP_AE_SWITCH_AFTER_FRONTEND_LOOKUP_TABLE (3)
#define APICAL_ISP_TOP_AE_SWITCH_LONG_EXPOSURE (4)
#define APICAL_ISP_TOP_AE_SWITCH_SHORT_EXPOSURE (5)
#define APICAL_ISP_TOP_AE_SWITCH_BEFORE_TEMPER (6)
#define APICAL_ISP_TOP_AE_SWITCH_DISABLED (7)

// args: data (3-bit)
static __inline void apical_isp_top_ae_switch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x48L);
	APICAL_WRITE_32(0x48L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_top_ae_switch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x48L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB switch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AWB tap in the pipeline.  Location of AWB statistics collection.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_AWB_SWITCH_DEFAULT (0)
#define APICAL_ISP_TOP_AWB_SWITCH_DATASIZE (2)
#define APICAL_ISP_TOP_AWB_SWITCH_IMMEDIATELY_BEFORE_COLOUR_MATRIX (0)
#define APICAL_ISP_TOP_AWB_SWITCH_IMMEDIATELY_AFTER_COLOUR_MATRIX (1)
#define APICAL_ISP_TOP_AWB_SWITCH_DISABLED (2)
#define APICAL_ISP_TOP_AWB_SWITCH_DISABLED3 (3)

// args: data (2-bit)
static __inline void apical_isp_top_awb_switch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4cL);
	APICAL_WRITE_32(0x4cL, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_top_awb_switch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4cL) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF switch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AF tap in the pipeline
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_AF_SWITCH_DEFAULT (0)
#define APICAL_ISP_TOP_AF_SWITCH_DATASIZE (2)
#define APICAL_ISP_TOP_AF_SWITCH_DEMOSAIC (0)
#define APICAL_ISP_TOP_AF_SWITCH_SHARPEN (1)
#define APICAL_ISP_TOP_AF_SWITCH_DISABLED (2)
#define APICAL_ISP_TOP_AF_SWITCH_DISABLED3 (3)

// args: data (2-bit)
static __inline void apical_isp_top_af_switch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x50L);
	APICAL_WRITE_32(0x50L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_top_af_switch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x50L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: MVE switch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// MVE tap in the pipeline.  Location of DIS correlation value calculations
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_MVE_SWITCH_DEFAULT (0)
#define APICAL_ISP_TOP_MVE_SWITCH_DATASIZE (2)
#define APICAL_ISP_TOP_MVE_SWITCH_END_OF_PIPELINE (0)
#define APICAL_ISP_TOP_MVE_SWITCH_BEFORE_DEMOSAIC (1)
#define APICAL_ISP_TOP_MVE_SWITCH_BEFORE_DEFECT_PIXEL (2)
#define APICAL_ISP_TOP_MVE_SWITCH_DISABLED (3)

// args: data (2-bit)
static __inline void apical_isp_top_mve_switch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x54L);
	APICAL_WRITE_32(0x54L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_top_mve_switch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x54L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Histogram switch
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AE global histogram tap in the pipeline.  Location of statistics gathering for 256 bin global histogram
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_DEFAULT (0)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_DATASIZE (3)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_SAME_AS_AE (0)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_AFTER_TPG (1)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_AFTER_SHADING (2)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_AFTER_FRONTEND_LOOKUP_TABLE (3)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_LONG_EXPOSURE (4)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_SHORT_EXPOSURE (5)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_BEFORE_TEMPER (6)
#define APICAL_ISP_TOP_HISTOGRAM_SWITCH_DISABLED (7)

// args: data (3-bit)
static __inline void apical_isp_top_histogram_switch_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x58L);
	APICAL_WRITE_32(0x58L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_top_histogram_switch_read(void) {
	return (uint8_t)((APICAL_READ_32(0x58L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Field Status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Status of field signal
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_FIELD_STATUS_DEFAULT (0x0)
#define APICAL_ISP_TOP_FIELD_STATUS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_top_field_status_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7cL) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Global FSM reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= synchronous reset of FSMs in design (faster recovery after broken frame)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_GLOBAL_FSM_RESET_DEFAULT (0x0)
#define APICAL_ISP_TOP_GLOBAL_FSM_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_global_fsm_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x78L);
	APICAL_WRITE_32(0x78L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_top_global_fsm_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x78L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VCKE override
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= override VCKE to flush the pipeline (recovery after broken frame)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TOP_VCKE_OVERRIDE_DEFAULT (0x0)
#define APICAL_ISP_TOP_VCKE_OVERRIDE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_top_vcke_override_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x78L);
	APICAL_WRITE_32(0x78L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_top_vcke_override_read(void) {
	return (uint8_t)((APICAL_READ_32(0x78L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Group: Interrupts
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt controller
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Interrupt0 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT0_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT0_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt0_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x80L);
	APICAL_WRITE_32(0x80L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt0_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x80L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt1 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT1_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT1_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt1_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x80L);
	APICAL_WRITE_32(0x80L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt1_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x80L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt2 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT2_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT2_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt2_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x84L);
	APICAL_WRITE_32(0x84L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt2_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x84L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt3 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT3_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT3_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt3_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x84L);
	APICAL_WRITE_32(0x84L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt3_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x84L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt4 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT4_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT4_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt4_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x88L);
	APICAL_WRITE_32(0x88L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt4_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x88L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt5 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT5_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT5_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt5_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x88L);
	APICAL_WRITE_32(0x88L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt5_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x88L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt6 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT6_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT6_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt6_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8cL);
	APICAL_WRITE_32(0x8cL, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt6_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8cL) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt7 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT7_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT7_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt7_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8cL);
	APICAL_WRITE_32(0x8cL, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt7_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8cL) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt8 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT8_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT8_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt8_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x90L);
	APICAL_WRITE_32(0x90L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt8_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x90L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt9 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT9_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT9_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt9_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x90L);
	APICAL_WRITE_32(0x90L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt9_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x90L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt10 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT10_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT10_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt10_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x94L);
	APICAL_WRITE_32(0x94L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt10_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x94L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt11 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT11_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT11_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt11_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x94L);
	APICAL_WRITE_32(0x94L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt11_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x94L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt12 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT12_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT12_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt12_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x98L);
	APICAL_WRITE_32(0x98L, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt12_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x98L) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt13 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT13_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT13_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt13_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x98L);
	APICAL_WRITE_32(0x98L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt13_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x98L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt14 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT14_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT14_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt14_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9cL);
	APICAL_WRITE_32(0x9cL, (((uint32_t) (data & 0x3f)) << 0) | (curr & 0xffffffc0));
}
static __inline uint8_t apical_isp_interrupts_interrupt14_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9cL) & 0x3f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt15 source
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt source selector. See ISP guide for interrupt definitions and valid values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT15_SOURCE_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT15_SOURCE_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_interrupts_interrupt15_source_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9cL);
	APICAL_WRITE_32(0x9cL, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_interrupts_interrupt15_source_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9cL) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt event flags.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT_STATUS_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT_STATUS_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_interrupts_interrupt_status_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Interrupt clear
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Interrupt event clear register writing 0-1 transition will clear the corresponding status bits.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INTERRUPTS_INTERRUPT_CLEAR_DEFAULT (0x0)
#define APICAL_ISP_INTERRUPTS_INTERRUPT_CLEAR_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_interrupts_interrupt_clear_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xa4L);
	APICAL_WRITE_32(0xa4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_interrupts_interrupt_clear_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Input port
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls video input port.
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: preset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 	  Allows selection of various input port presets for standard sensor inputs.  See ISP Guide for details of available presets.
//0-14: Frequently used presets.  If using one of available presets, remaining bits in registers 0x100 and 0x104 are not used.
//15:   Input port configured according to registers in 0x100 and 0x104.  Consult Apical support for special input port requirements.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_PRESET_DEFAULT (2)
#define APICAL_ISP_INPUT_PORT_PRESET_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_input_port_preset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_input_port_preset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: vs_use field
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VS_USE_FIELD_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VS_USE_FIELD_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VS_USE_FIELD_USE_VSYNC_I_PORT_FOR_VERTICAL_SYNC (0)
#define APICAL_ISP_INPUT_PORT_VS_USE_FIELD_USE_FIELD_I_PORT_FOR_VERTICAL_SYNC (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vs_use_field_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_input_port_vs_use_field_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: vs toggle
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VS_TOGGLE_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VS_TOGGLE_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VS_TOGGLE_VSYNC_IS_PULSETYPE (0)
#define APICAL_ISP_INPUT_PORT_VS_TOGGLE_VSYNC_IS_TOGGLETYPE_FIELD_SIGNAL (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vs_toggle_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_input_port_vs_toggle_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: vs polarity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VS_POLARITY_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_HORIZONTAL_COUNTER_RESET_ON_RISING_EDGE (0)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_HORIZONTAL_COUNTER_RESET_ON_FALLING_EDGE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vs_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_input_port_vs_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: vs_polarity acl
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VS_POLARITY_ACL_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_ACL_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_ACL_DONT_INVERT_POLARITY_FOR_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_VS_POLARITY_ACL_INVERT_POLARITY_FOR_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vs_polarity_acl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_input_port_vs_polarity_acl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: hs_use acl
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_USE_ACL_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HS_USE_ACL_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_USE_ACL_USE_HSYNC_I_PORT_FOR_ACTIVELINE (0)
#define APICAL_ISP_INPUT_PORT_HS_USE_ACL_USE_ACL_I_PORT_FOR_ACTIVELINE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_use_acl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 12) | (curr & 0xffffefff));
}
static __inline uint8_t apical_isp_input_port_hs_use_acl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x1000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: vc_c select
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VC_C_SELECT_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VC_C_SELECT_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VC_C_SELECT_VERTICAL_COUNTER_COUNTS_ON_HS (0)
#define APICAL_ISP_INPUT_PORT_VC_C_SELECT_VERTICAL_COUNTER_COUNTS_ON_HORIZONTAL_COUNTER_OVERFLOW_OR_RESET (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vc_c_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 14) | (curr & 0xffffbfff));
}
static __inline uint8_t apical_isp_input_port_vc_c_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x4000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: vc_r select
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VC_R_SELECT_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VC_R_SELECT_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_VC_R_SELECT_VERTICAL_COUNTER_IS_RESET_ON_EDGE_OF_VS (0)
#define APICAL_ISP_INPUT_PORT_VC_R_SELECT_VERTICAL_COUNTER_IS_RESET_AFTER_TIMEOUT_ON_HS (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_vc_r_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 15) | (curr & 0xffff7fff));
}
static __inline uint8_t apical_isp_input_port_vc_r_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x8000) >> 15);
}
// ------------------------------------------------------------------------------ //
// Register: hs_xor vs
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_XOR_VS_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HS_XOR_VS_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_XOR_VS_NORMAL_MODE (0)
#define APICAL_ISP_INPUT_PORT_HS_XOR_VS_HVALID__HSYNC_XOR_VSYNC (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_xor_vs_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_input_port_hs_xor_vs_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: hs polarity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_POLARITY_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_DONT_INVERT_POLARITY_OF_HS_FOR_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_INVERT_POLARITY_OF_HS_FOR_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_input_port_hs_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: hs_polarity acl
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_POLARITY_ACL_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_ACL_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_ACL_DONT_INVERT_POLARITY_OF_HS_FOR_HS_GATE (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_ACL_INVERT_POLARITY_OF_HS_FOR_HS_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_polarity_acl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 18) | (curr & 0xfffbffff));
}
static __inline uint8_t apical_isp_input_port_hs_polarity_acl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: hs_polarity hs
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_POLARITY_HS_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_HS_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_HS_HORIZONTAL_COUNTER_IS_RESET_ON_RISING_EDGE_OF_HS (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_HS_HORIZONTAL_COUNTER_IS_RESET_ON_VSYNC_EG_WHEN_HSYNC_IS_NOT_AVAILABLE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_polarity_hs_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 19) | (curr & 0xfff7ffff));
}
static __inline uint8_t apical_isp_input_port_hs_polarity_hs_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x80000) >> 19);
}
// ------------------------------------------------------------------------------ //
// Register: hs_polarity vc
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HS_POLARITY_VC_DEFAULT (1)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_VC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_VC_VERTICAL_COUNTER_INCREMENTS_ON_RISING_EDGE_OF_HS (0)
#define APICAL_ISP_INPUT_PORT_HS_POLARITY_VC_VERTICAL_COUNTER_INCREMENTS_ON_FALLING_EDGE_OF_HS (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hs_polarity_vc_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 20) | (curr & 0xffefffff));
}
static __inline uint8_t apical_isp_input_port_hs_polarity_vc_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x100000) >> 20);
}
// ------------------------------------------------------------------------------ //
// Register: hc_r select
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_R_SELECT_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HC_R_SELECT_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HC_R_SELECT_VERTICAL_COUNTER_IS_RESET_ON_RISING_EDGE_OF_HS (0)
#define APICAL_ISP_INPUT_PORT_HC_R_SELECT_VERTICAL_COUNTER_IS_RESET_ON_RISING_EDGE_OF_VS (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hc_r_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 23) | (curr & 0xff7fffff));
}
static __inline uint8_t apical_isp_input_port_hc_r_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x800000) >> 23);
}
// ------------------------------------------------------------------------------ //
// Register: acl polarity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACL_POLARITY_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACL_POLARITY_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACL_POLARITY_DONT_INVERT_ACL_I_FOR_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACL_POLARITY_INVERT_ACL_I_FOR_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_acl_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x100L);
	APICAL_WRITE_32(0x100L, (((uint32_t) (data & 0x1)) << 24) | (curr & 0xfeffffff));
}
static __inline uint8_t apical_isp_input_port_acl_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x100L) & 0x1000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: field polarity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELD_POLARITY_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELD_POLARITY_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELD_POLARITY_DONT_INVERT_FIELD_I_FOR_FIELD_GATE (0)
#define APICAL_ISP_INPUT_PORT_FIELD_POLARITY_INVERT_FIELD_I_FOR_FIELD_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_field_polarity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_input_port_field_polarity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: field toggle
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELD_TOGGLE_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELD_TOGGLE_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELD_TOGGLE_FIELD_IS_PULSETYPE (0)
#define APICAL_ISP_INPUT_PORT_FIELD_TOGGLE_FIELD_IS_TOGGLETYPE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_field_toggle_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_input_port_field_toggle_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: aclg window0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW0_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW0_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW0_EXCLUDE_WINDOW0_SIGNAL_IN_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW0_INCLUDE_WINDOW0_SIGNAL_IN_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_aclg_window0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_input_port_aclg_window0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: aclg hsync
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACLG_HSYNC_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACLG_HSYNC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACLG_HSYNC_EXCLUDE_HSYNC_SIGNAL_IN_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACLG_HSYNC_INCLUDE_HSYNC_SIGNAL_IN_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_aclg_hsync_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_input_port_aclg_hsync_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: aclg window2
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW2_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW2_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW2_EXCLUDE_WINDOW2_SIGNAL_IN_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACLG_WINDOW2_INCLUDE_WINDOW2_SIGNAL_IN_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_aclg_window2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_input_port_aclg_window2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: aclg acl
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACLG_ACL_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACLG_ACL_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACLG_ACL_EXCLUDE_ACL_I_SIGNAL_IN_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACLG_ACL_INCLUDE_ACL_I_SIGNAL_IN_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_aclg_acl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_input_port_aclg_acl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: aclg vsync
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_ACLG_VSYNC_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_ACLG_VSYNC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_ACLG_VSYNC_EXCLUDE_VSYNC_SIGNAL_IN_ACL_GATE (0)
#define APICAL_ISP_INPUT_PORT_ACLG_VSYNC_INCLUDE_VSYNC_SIGNAL_IN_ACL_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_aclg_vsync_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 12) | (curr & 0xffffefff));
}
static __inline uint8_t apical_isp_input_port_aclg_vsync_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x1000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: hsg window1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HSG_WINDOW1_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW1_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW1_EXCLUDE_WINDOW1_SIGNAL_IN_HS_GATE (0)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW1_INCLUDE_WINDOW1_SIGNAL_IN_HS_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hsg_window1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_input_port_hsg_window1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: hsg hsync
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HSG_HSYNC_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HSG_HSYNC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HSG_HSYNC_EXCLUDE_HSYNC_SIGNAL_IN_HS_GATE (0)
#define APICAL_ISP_INPUT_PORT_HSG_HSYNC_INCLUDE_HSYNC_SIGNAL_IN_HS_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hsg_hsync_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_input_port_hsg_hsync_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: hsg vsync
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HSG_VSYNC_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HSG_VSYNC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HSG_VSYNC_EXCLUDE_VSYNC_SIGNAL_IN_HS_GATE (0)
#define APICAL_ISP_INPUT_PORT_HSG_VSYNC_INCLUDE_VSYNC_SIGNAL_IN_HS_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hsg_vsync_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 18) | (curr & 0xfffbffff));
}
static __inline uint8_t apical_isp_input_port_hsg_vsync_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: hsg window2
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HSG_WINDOW2_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW2_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW2_EXCLUDE_WINDOW2_SIGNAL_IN_HS_GATE (0)
#define APICAL_ISP_INPUT_PORT_HSG_WINDOW2_INCLUDE_WINDOW2_SIGNAL_IN_HS_GATE_MASK_OUT_SPURIOUS_HS_DURING_BLANK (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_hsg_window2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 19) | (curr & 0xfff7ffff));
}
static __inline uint8_t apical_isp_input_port_hsg_window2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x80000) >> 19);
}
// ------------------------------------------------------------------------------ //
// Register: fieldg vsync
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELDG_VSYNC_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_VSYNC_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELDG_VSYNC_EXCLUDE_VSYNC_SIGNAL_IN_FIELD_GATE (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_VSYNC_INCLUDE_VSYNC_SIGNAL_IN_FIELD_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_fieldg_vsync_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 24) | (curr & 0xfeffffff));
}
static __inline uint8_t apical_isp_input_port_fieldg_vsync_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x1000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: fieldg window2
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELDG_WINDOW2_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_WINDOW2_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELDG_WINDOW2_EXCLUDE_WINDOW2_SIGNAL_IN_FIELD_GATE (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_WINDOW2_INCLUDE_WINDOW2_SIGNAL_IN_FIELD_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_fieldg_window2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 25) | (curr & 0xfdffffff));
}
static __inline uint8_t apical_isp_input_port_fieldg_window2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x2000000) >> 25);
}
// ------------------------------------------------------------------------------ //
// Register: fieldg field
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELDG_FIELD_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_FIELD_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELDG_FIELD_EXCLUDE_FIELD_I_SIGNAL_IN_FIELD_GATE (0)
#define APICAL_ISP_INPUT_PORT_FIELDG_FIELD_INCLUDE_FIELD_I_SIGNAL_IN_FIELD_GATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_fieldg_field_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 26) | (curr & 0xfbffffff));
}
static __inline uint8_t apical_isp_input_port_fieldg_field_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x4000000) >> 26);
}
// ------------------------------------------------------------------------------ //
// Register: field mode
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FIELD_MODE_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FIELD_MODE_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FIELD_MODE_PULSE_FIELD (0)
#define APICAL_ISP_INPUT_PORT_FIELD_MODE_TOGGLE_FIELD (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_field_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x104L);
	APICAL_WRITE_32(0x104L, (((uint32_t) (data & 0x1)) << 27) | (curr & 0xf7ffffff));
}
static __inline uint8_t apical_isp_input_port_field_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x104L) & 0x8000000) >> 27);
}
// ------------------------------------------------------------------------------ //
// Register: hc limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// horizontal counter limit value (counts: 0,1,...hc_limit-1,hc_limit,0,1,...)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_LIMIT_DEFAULT (0xFFFF)
#define APICAL_ISP_INPUT_PORT_HC_LIMIT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_hc_limit_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x108L);
	APICAL_WRITE_32(0x108L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_hc_limit_read(void) {
	return (uint16_t)((APICAL_READ_32(0x108L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hc start0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window0 start for ACL gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_START0_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HC_START0_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_hc_start0_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x10cL);
	APICAL_WRITE_32(0x10cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_hc_start0_read(void) {
	return (uint16_t)((APICAL_READ_32(0x10cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hc size0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window0 size for ACL gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_SIZE0_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HC_SIZE0_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_hc_size0_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x110L);
	APICAL_WRITE_32(0x110L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_hc_size0_read(void) {
	return (uint16_t)((APICAL_READ_32(0x110L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hc start1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window1 start for HS gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_START1_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HC_START1_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_hc_start1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x114L);
	APICAL_WRITE_32(0x114L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_hc_start1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x114L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hc size1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window1 size for HS gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_HC_SIZE1_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_HC_SIZE1_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_hc_size1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x118L);
	APICAL_WRITE_32(0x118L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_hc_size1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x118L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: vc limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// vertical counter limit value (counts: 0,1,...vc_limit-1,vc_limit,0,1,...)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VC_LIMIT_DEFAULT (0xFFFF)
#define APICAL_ISP_INPUT_PORT_VC_LIMIT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_vc_limit_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x11cL);
	APICAL_WRITE_32(0x11cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_vc_limit_read(void) {
	return (uint16_t)((APICAL_READ_32(0x11cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: vc start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window2 start for ACL gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VC_START_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VC_START_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_vc_start_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x120L);
	APICAL_WRITE_32(0x120L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_vc_start_read(void) {
	return (uint16_t)((APICAL_READ_32(0x120L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: vc size
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// window2 size for ACL gate.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_VC_SIZE_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_VC_SIZE_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_input_port_vc_size_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x124L);
	APICAL_WRITE_32(0x124L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_input_port_vc_size_read(void) {
	return (uint16_t)((APICAL_READ_32(0x124L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// detected frame width.  Read only value.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FRAME_WIDTH_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FRAME_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_input_port_frame_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x128L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// detected frame height.  Read only value.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FRAME_HEIGHT_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FRAME_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_input_port_frame_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x12cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: freeze config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Used to freeze input port configuration.  Used when multiple register writes are required to change input port configuration.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_FREEZE_CONFIG_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_FREEZE_CONFIG_DATASIZE (1)
#define APICAL_ISP_INPUT_PORT_FREEZE_CONFIG_NORMAL_OPERATION (0)
#define APICAL_ISP_INPUT_PORT_FREEZE_CONFIG_HOLD_PREVIOUS_INPUT_PORT_CONFIG_STATE (1)

// args: data (1-bit)
static __inline void apical_isp_input_port_freeze_config_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x130L);
	APICAL_WRITE_32(0x130L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_input_port_freeze_config_read(void) {
	return (uint8_t)((APICAL_READ_32(0x130L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: mode request
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Used to stop and start input port.  See ISP guide for further details.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_DATASIZE (3)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_SAFE_STOP (0)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_SAFE_START (1)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_URGENT_STOP (2)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_URGENT_START (3)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_RESERVED4 (4)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_SAFER_START (5)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_RESERVED6 (6)
#define APICAL_ISP_INPUT_PORT_MODE_REQUEST_RESERVED7 (7)

// args: data (3-bit)
static __inline void apical_isp_input_port_mode_request_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x130L);
	APICAL_WRITE_32(0x130L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_input_port_mode_request_read(void) {
	return (uint8_t)((APICAL_READ_32(0x130L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: mode status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//	  Used to monitor input port status:
//	  bit 0: 1=running, 0=stopped, bits 1,2-reserved
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_INPUT_PORT_MODE_STATUS_DEFAULT (0)
#define APICAL_ISP_INPUT_PORT_MODE_STATUS_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_input_port_mode_status_read(void) {
	return (uint8_t)((APICAL_READ_32(0x134L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: video test gen
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Video test generator controls.  See ISP Guide for further details
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: test_pattern_off on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Test pattern off-on: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_TEST_PATTERN_OFF_ON_DEFAULT (0)
#define APICAL_ISP_VIDEO_TEST_GEN_TEST_PATTERN_OFF_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_video_test_gen_test_pattern_off_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9c0L);
	APICAL_WRITE_32(0x9c0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_video_test_gen_test_pattern_off_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9c0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bayer_rgb_i sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bayer or rgb select for input video: 0=bayer, 1=rgb
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_BAYER_RGB_I_SEL_DEFAULT (0)
#define APICAL_ISP_VIDEO_TEST_GEN_BAYER_RGB_I_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_video_test_gen_bayer_rgb_i_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9c0L);
	APICAL_WRITE_32(0x9c0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_video_test_gen_bayer_rgb_i_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9c0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: bayer_rgb_o sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bayer or rgb select for output video: 0=bayer, 1=rgb
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_BAYER_RGB_O_SEL_DEFAULT (0)
#define APICAL_ISP_VIDEO_TEST_GEN_BAYER_RGB_O_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_video_test_gen_bayer_rgb_o_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9c0L);
	APICAL_WRITE_32(0x9c0L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_video_test_gen_bayer_rgb_o_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9c0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: pattern type
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Pattern type select: 0=Flat field,1=Horizontal gradient,2=Vertical Gradient,3=Vertical Bars,4=Rectangle,5-255=Default white frame on black
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_PATTERN_TYPE_DEFAULT (0x03)
#define APICAL_ISP_VIDEO_TEST_GEN_PATTERN_TYPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_video_test_gen_pattern_type_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9c4L);
	APICAL_WRITE_32(0x9c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_video_test_gen_pattern_type_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: prbs rst on frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_PRBS_RST_ON_FRAME_DEFAULT (0)
#define APICAL_ISP_VIDEO_TEST_GEN_PRBS_RST_ON_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_video_test_gen_prbs_rst_on_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x9c0L);
	APICAL_WRITE_32(0x9c0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_video_test_gen_prbs_rst_on_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0x9c0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: r backgnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Red background  value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_R_BACKGND_DEFAULT (0xFFFF)
#define APICAL_ISP_VIDEO_TEST_GEN_R_BACKGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_r_backgnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9c8L);
	APICAL_WRITE_32(0x9c8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_r_backgnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9c8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: g backgnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Green background value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_G_BACKGND_DEFAULT (0xFFFF)
#define APICAL_ISP_VIDEO_TEST_GEN_G_BACKGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_g_backgnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9ccL);
	APICAL_WRITE_32(0x9ccL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_g_backgnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9ccL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: b backgnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Blue background value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_B_BACKGND_DEFAULT (0xFFFF)
#define APICAL_ISP_VIDEO_TEST_GEN_B_BACKGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_b_backgnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9d0L);
	APICAL_WRITE_32(0x9d0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_b_backgnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9d0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: r foregnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Red foreground  value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_R_FOREGND_DEFAULT (0x8FFF)
#define APICAL_ISP_VIDEO_TEST_GEN_R_FOREGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_r_foregnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9d4L);
	APICAL_WRITE_32(0x9d4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_r_foregnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9d4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: g foregnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Green foreground value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_G_FOREGND_DEFAULT (0x8FFF)
#define APICAL_ISP_VIDEO_TEST_GEN_G_FOREGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_g_foregnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9d8L);
	APICAL_WRITE_32(0x9d8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_g_foregnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9d8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: b foregnd
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Blue foreground value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_B_FOREGND_DEFAULT (0x8FFF)
#define APICAL_ISP_VIDEO_TEST_GEN_B_FOREGND_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_b_foregnd_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9dcL);
	APICAL_WRITE_32(0x9dcL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_b_foregnd_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9dcL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rgb gradient
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// RGB gradient increment per pixel (0-15)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RGB_GRADIENT_DEFAULT (0x3CAA)
#define APICAL_ISP_VIDEO_TEST_GEN_RGB_GRADIENT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_rgb_gradient_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9e0L);
	APICAL_WRITE_32(0x9e0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_rgb_gradient_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9e0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rgb_gradient start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// RGB gradient start value 16bit, MSB aligned to used width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RGB_GRADIENT_START_DEFAULT (0x0000)
#define APICAL_ISP_VIDEO_TEST_GEN_RGB_GRADIENT_START_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_video_test_gen_rgb_gradient_start_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9e4L);
	APICAL_WRITE_32(0x9e4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_video_test_gen_rgb_gradient_start_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9e4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rect top
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Rectangle top line number 1-n
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RECT_TOP_DEFAULT (0x0001)
#define APICAL_ISP_VIDEO_TEST_GEN_RECT_TOP_DATASIZE (14)

// args: data (14-bit)
static __inline void apical_isp_video_test_gen_rect_top_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9e8L);
	APICAL_WRITE_32(0x9e8L, (((uint32_t) (data & 0x3fff)) << 0) | (curr & 0xffffc000));
}
static __inline uint16_t apical_isp_video_test_gen_rect_top_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9e8L) & 0x3fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rect bot
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Rectangle bottom line number 1-n
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RECT_BOT_DEFAULT (0x0100)
#define APICAL_ISP_VIDEO_TEST_GEN_RECT_BOT_DATASIZE (14)

// args: data (14-bit)
static __inline void apical_isp_video_test_gen_rect_bot_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9ecL);
	APICAL_WRITE_32(0x9ecL, (((uint32_t) (data & 0x3fff)) << 0) | (curr & 0xffffc000));
}
static __inline uint16_t apical_isp_video_test_gen_rect_bot_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9ecL) & 0x3fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rect left
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Rectangle left pixel number 1-n
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RECT_LEFT_DEFAULT (0x0001)
#define APICAL_ISP_VIDEO_TEST_GEN_RECT_LEFT_DATASIZE (14)

// args: data (14-bit)
static __inline void apical_isp_video_test_gen_rect_left_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9f0L);
	APICAL_WRITE_32(0x9f0L, (((uint32_t) (data & 0x3fff)) << 0) | (curr & 0xffffc000));
}
static __inline uint16_t apical_isp_video_test_gen_rect_left_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9f0L) & 0x3fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rect right
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Rectangle right pixel number 1-n
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_RECT_RIGHT_DEFAULT (0x0100)
#define APICAL_ISP_VIDEO_TEST_GEN_RECT_RIGHT_DATASIZE (14)

// args: data (14-bit)
static __inline void apical_isp_video_test_gen_rect_right_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x9f8L);
	APICAL_WRITE_32(0x9f8L, (((uint32_t) (data & 0x3fff)) << 0) | (curr & 0xffffc000));
}
static __inline uint16_t apical_isp_video_test_gen_rect_right_read(void) {
	return (uint16_t)((APICAL_READ_32(0x9f8L) & 0x3fff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: PRBS seed
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  PRBS seed. The PRBS LFSR is seeded with this value when reset on a frame boundary.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_VIDEO_TEST_GEN_PRBS_SEED_DEFAULT (0x000000)
#define APICAL_ISP_VIDEO_TEST_GEN_PRBS_SEED_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_video_test_gen_prbs_seed_write(uint32_t data) {
	APICAL_WRITE_32(0x9fcL, data);
}
static __inline uint32_t apical_isp_video_test_gen_prbs_seed_read(void) {
	return APICAL_READ_32(0x9fcL);
}
// ------------------------------------------------------------------------------ //
// Group: Sensor Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset subtraction for each color channel and exposure
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Black 00
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 00 (R)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SENSOR_OFFSET_BLACK_00_DEFAULT (0x00)
#define APICAL_ISP_SENSOR_OFFSET_BLACK_00_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_sensor_offset_black_00_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x140L);
	APICAL_WRITE_32(0x140L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_sensor_offset_black_00_read(void) {
	return (uint16_t)((APICAL_READ_32(0x140L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 01
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 01 (Gr)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SENSOR_OFFSET_BLACK_01_DEFAULT (0x00)
#define APICAL_ISP_SENSOR_OFFSET_BLACK_01_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_sensor_offset_black_01_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x144L);
	APICAL_WRITE_32(0x144L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_sensor_offset_black_01_read(void) {
	return (uint16_t)((APICAL_READ_32(0x144L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 10
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 10 (Gb)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SENSOR_OFFSET_BLACK_10_DEFAULT (0x00)
#define APICAL_ISP_SENSOR_OFFSET_BLACK_10_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_sensor_offset_black_10_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x148L);
	APICAL_WRITE_32(0x148L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_sensor_offset_black_10_read(void) {
	return (uint16_t)((APICAL_READ_32(0x148L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 11 (B)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SENSOR_OFFSET_BLACK_11_DEFAULT (0x00)
#define APICAL_ISP_SENSOR_OFFSET_BLACK_11_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_sensor_offset_black_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x14cL);
	APICAL_WRITE_32(0x14cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_sensor_offset_black_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x14cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Digital Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Digital gain for RAW sensor data
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Gain
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gain applied to data in 4.8 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DIGITAL_GAIN_GAIN_DEFAULT (0x100)
#define APICAL_ISP_DIGITAL_GAIN_GAIN_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_digital_gain_gain_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1a0L);
	APICAL_WRITE_32(0x1a0L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_digital_gain_gain_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1a0L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data black level
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DIGITAL_GAIN_OFFSET_DEFAULT (0x000)
#define APICAL_ISP_DIGITAL_GAIN_OFFSET_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_digital_gain_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1a4L);
	APICAL_WRITE_32(0x1a4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_digital_gain_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1a4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Frontend LUT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frontend lookup (for companded WDR sensor inputs)
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frontend lookup0 enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_ENABLE_0_DEFAULT (0)
#define APICAL_ISP_GAMMA_FE_ENABLE_0_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_gamma_fe_enable_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x188L);
	APICAL_WRITE_32(0x188L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_gamma_fe_enable_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frontend lookup1 enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_ENABLE_1_DEFAULT (0)
#define APICAL_ISP_GAMMA_FE_ENABLE_1_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_gamma_fe_enable_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x188L);
	APICAL_WRITE_32(0x188L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_gamma_fe_enable_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Offset Mode 0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//          Lookup0 reflection mode for black offset region
//          0 = Manual curve reflection
//          1 = Automatic curve reflection
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_OFFSET_MODE_0_DEFAULT (0)
#define APICAL_ISP_GAMMA_FE_OFFSET_MODE_0_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_gamma_fe_offset_mode_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x188L);
	APICAL_WRITE_32(0x188L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_gamma_fe_offset_mode_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Offset Mode 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//          Lookup1 reflection mode for black offset region
//          0 = Manual curve reflection
//          1 = Automatic curve reflection
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_OFFSET_MODE_1_DEFAULT (0)
#define APICAL_ISP_GAMMA_FE_OFFSET_MODE_1_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_gamma_fe_offset_mode_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x188L);
	APICAL_WRITE_32(0x188L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_gamma_fe_offset_mode_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: MCU priority
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Priority of CPU writes to LUTs
//		0=low. CPU read/writes from/to the frontend LUTs are only executed when LUTs are not being accessed by ISP.  Normal operation.
//		1=high. CPU read/writes from/to the frontend LUTs are always executed.  This may result in corrupt image data and invalid read back and is not recommended.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_MCU_PRIORITY_DEFAULT (0)
#define APICAL_ISP_GAMMA_FE_MCU_PRIORITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_gamma_fe_mcu_priority_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x188L);
	APICAL_WRITE_32(0x188L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_gamma_fe_mcu_priority_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT0 status indicator.  When 1, LUT0 is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_MCU_READY0_DEFAULT (0x0)
#define APICAL_ISP_GAMMA_FE_MCU_READY0_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_gamma_fe_mcu_ready0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT1 status indicator.  When 1, LUT1 is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_GAMMA_FE_MCU_READY1_DEFAULT (0x0)
#define APICAL_ISP_GAMMA_FE_MCU_READY1_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_gamma_fe_mcu_ready1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x188L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Group: RAW Frontend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// RAW frontend processing
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: ge enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Green equalization enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_GE_ENABLE_DEFAULT (0)
#define APICAL_ISP_RAW_FRONTEND_GE_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_raw_frontend_ge_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c0L);
	APICAL_WRITE_32(0x1c0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_raw_frontend_ge_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: dp enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Dynamic Defect Pixel enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_DP_ENABLE_DEFAULT (0)
#define APICAL_ISP_RAW_FRONTEND_DP_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_raw_frontend_dp_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c0L);
	APICAL_WRITE_32(0x1c0L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_raw_frontend_dp_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: show dynamic defect pixel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Show Defect Pixel: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_SHOW_DYNAMIC_DEFECT_PIXEL_DEFAULT (0)
#define APICAL_ISP_RAW_FRONTEND_SHOW_DYNAMIC_DEFECT_PIXEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_raw_frontend_show_dynamic_defect_pixel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c0L);
	APICAL_WRITE_32(0x1c0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_raw_frontend_show_dynamic_defect_pixel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: dark disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Disable detection of dark pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_DARK_DISABLE_DEFAULT (0)
#define APICAL_ISP_RAW_FRONTEND_DARK_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_raw_frontend_dark_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c0L);
	APICAL_WRITE_32(0x1c0L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_raw_frontend_dark_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c0L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: bright disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Disable detection of bright pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_BRIGHT_DISABLE_DEFAULT (0)
#define APICAL_ISP_RAW_FRONTEND_BRIGHT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_raw_frontend_bright_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c0L);
	APICAL_WRITE_32(0x1c0L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_raw_frontend_bright_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c0L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: ge strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls strength of Green equalization.  Set during calibration.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_GE_STRENGTH_DEFAULT (0x00)
#define APICAL_ISP_RAW_FRONTEND_GE_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_raw_frontend_ge_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1c4L);
	APICAL_WRITE_32(0x1c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_raw_frontend_ge_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: debug sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Debug selection port
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_DEBUG_SEL_DEFAULT (0x0)
#define APICAL_ISP_RAW_FRONTEND_DEBUG_SEL_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_raw_frontend_debug_sel_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1c8L);
	APICAL_WRITE_32(0x1c8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_raw_frontend_debug_sel_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1c8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: dp threshold
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Defect pixel threshold.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_DP_THRESHOLD_DEFAULT (0x040)
#define APICAL_ISP_RAW_FRONTEND_DP_THRESHOLD_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_dp_threshold_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1ccL);
	APICAL_WRITE_32(0x1ccL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_dp_threshold_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1ccL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: ge threshold
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// green equalization threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_GE_THRESHOLD_DEFAULT (0x400)
#define APICAL_ISP_RAW_FRONTEND_GE_THRESHOLD_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_ge_threshold_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1d0L);
	APICAL_WRITE_32(0x1d0L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_ge_threshold_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1d0L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: dp slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope for HP Mask function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_DP_SLOPE_DEFAULT (0x200)
#define APICAL_ISP_RAW_FRONTEND_DP_SLOPE_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_dp_slope_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1d4L);
	APICAL_WRITE_32(0x1d4L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_dp_slope_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1d4L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: ge slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope for GE Mask function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_GE_SLOPE_DEFAULT (0x0AA)
#define APICAL_ISP_RAW_FRONTEND_GE_SLOPE_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_ge_slope_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1d8L);
	APICAL_WRITE_32(0x1d8L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_ge_slope_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1d8L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: ge sens
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls the sensitivity of green equalization to edges.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_GE_SENS_DEFAULT (0x80)
#define APICAL_ISP_RAW_FRONTEND_GE_SENS_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_raw_frontend_ge_sens_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1dcL);
	APICAL_WRITE_32(0x1dcL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_raw_frontend_ge_sens_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1dcL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hpdev threshold
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls the aggressiveness of the dynamic defect pixel correction near edges.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_HPDEV_THRESHOLD_DEFAULT (0x266)
#define APICAL_ISP_RAW_FRONTEND_HPDEV_THRESHOLD_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_hpdev_threshold_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1e0L);
	APICAL_WRITE_32(0x1e0L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_hpdev_threshold_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1e0L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: line thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls the directional nature of the dynamic defect pixel correction near edges..
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_LINE_THRESH_DEFAULT (0x150)
#define APICAL_ISP_RAW_FRONTEND_LINE_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_line_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1e4L);
	APICAL_WRITE_32(0x1e4L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_line_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1e4L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: hp blend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Controls blending between non-directional and directional replacement values in dynamic defect pixel correction.
//		0x00 Replace detected defects with non-directional replacement value
//		0xFF Replace detected defects with directional replacement value
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_HP_BLEND_DEFAULT (0x00)
#define APICAL_ISP_RAW_FRONTEND_HP_BLEND_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_raw_frontend_hp_blend_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1e8L);
	APICAL_WRITE_32(0x1e8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_raw_frontend_hp_blend_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1e8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Sigma In
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Manual override of noise estimation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_SIGMA_IN_DEFAULT (0x00)
#define APICAL_ISP_RAW_FRONTEND_SIGMA_IN_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_raw_frontend_sigma_in_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x1ecL);
	APICAL_WRITE_32(0x1ecL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_raw_frontend_sigma_in_read(void) {
	return (uint16_t)((APICAL_READ_32(0x1ecL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Short
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for short exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_THRESH_SHORT_DEFAULT (0x00)
#define APICAL_ISP_RAW_FRONTEND_THRESH_SHORT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_raw_frontend_thresh_short_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1f0L);
	APICAL_WRITE_32(0x1f0L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_raw_frontend_thresh_short_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1f0L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Long
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for long exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RAW_FRONTEND_THRESH_LONG_DEFAULT (0x30)
#define APICAL_ISP_RAW_FRONTEND_THRESH_LONG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_raw_frontend_thresh_long_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x1f4L);
	APICAL_WRITE_32(0x1f4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_raw_frontend_thresh_long_read(void) {
	return (uint8_t)((APICAL_READ_32(0x1f4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Defect Pixel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Detection and processing of static defect-pixels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Pointer Reset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Reset static defect-pixel table pointer each frame - set this when defect-pixel table has been written from mcu
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_POINTER_RESET_DEFAULT (0)
#define APICAL_ISP_DEFECT_PIXEL_POINTER_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_defect_pixel_pointer_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x200L);
	APICAL_WRITE_32(0x200L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_defect_pixel_pointer_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Show Reference
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// For debug purposes.  Show reference values which are compared with actual values to detect bad pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_SHOW_REFERENCE_DEFAULT (0)
#define APICAL_ISP_DEFECT_PIXEL_SHOW_REFERENCE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_defect_pixel_show_reference_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x200L);
	APICAL_WRITE_32(0x200L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_defect_pixel_show_reference_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Correction enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_ENABLE_DEFAULT (0)
#define APICAL_ISP_DEFECT_PIXEL_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_defect_pixel_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x200L);
	APICAL_WRITE_32(0x200L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_defect_pixel_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Overflow
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Table overflow flag
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_OVERFLOW_DEFAULT (0x0)
#define APICAL_ISP_DEFECT_PIXEL_OVERFLOW_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_defect_pixel_overflow_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Show Static Defect Pixels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Show which pixels have been detected as bad
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_SHOW_STATIC_DEFECT_PIXELS_DEFAULT (0)
#define APICAL_ISP_DEFECT_PIXEL_SHOW_STATIC_DEFECT_PIXELS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_defect_pixel_show_static_defect_pixels_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x200L);
	APICAL_WRITE_32(0x200L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_defect_pixel_show_static_defect_pixels_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Detection Trigger
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Starts detection on 0-1 transition
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_DETECTION_TRIGGER_DEFAULT (0)
#define APICAL_ISP_DEFECT_PIXEL_DETECTION_TRIGGER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_defect_pixel_detection_trigger_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x200L);
	APICAL_WRITE_32(0x200L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_defect_pixel_detection_trigger_read(void) {
	return (uint8_t)((APICAL_READ_32(0x200L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Defect Pixel Count
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of defect-pixels detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_DEFECT_PIXEL_COUNT_DEFAULT (0x0)
#define APICAL_ISP_DEFECT_PIXEL_DEFECT_PIXEL_COUNT_DATASIZE (12)

// args: data (12-bit)
static __inline uint16_t apical_isp_defect_pixel_defect_pixel_count_read(void) {
	return (uint16_t)((APICAL_READ_32(0x204L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Table Start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Address of first defect-pixel in defect-pixel store
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_TABLE_START_DEFAULT (0x0)
#define APICAL_ISP_DEFECT_PIXEL_TABLE_START_DATASIZE (12)

// args: data (12-bit)
static __inline uint16_t apical_isp_defect_pixel_table_start_read(void) {
	return (uint16_t)((APICAL_READ_32(0x208L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Defect Pixel Count In
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of defect-pixels in the written table
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEFECT_PIXEL_DEFECT_PIXEL_COUNT_IN_DEFAULT (0x0)
#define APICAL_ISP_DEFECT_PIXEL_DEFECT_PIXEL_COUNT_IN_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_defect_pixel_defect_pixel_count_in_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x20cL);
	APICAL_WRITE_32(0x20cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_defect_pixel_defect_pixel_count_in_read(void) {
	return (uint16_t)((APICAL_READ_32(0x20cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: WDR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Dual-exposure wide-dynamic-range blending
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: WDR Mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects WDR mode
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_WDR_MODE_DEFAULT (3)
#define APICAL_ISP_FRAME_STITCH_WDR_MODE_DATASIZE (2)
#define APICAL_ISP_FRAME_STITCH_WDR_MODE_HALF_RATE_FRAME_SWITCH_ODD_FRAMES (0)
#define APICAL_ISP_FRAME_STITCH_WDR_MODE_FULL_RATE_FRAME_SWITCH (1)
#define APICAL_ISP_FRAME_STITCH_WDR_MODE_HALF_RATE_FRAME_SWITCH_EVEN_FRAMES (2)
#define APICAL_ISP_FRAME_STITCH_WDR_MODE_FRAME_BUFFER_DISABLED (3)

// args: data (2-bit)
static __inline void apical_isp_frame_stitch_wdr_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_frame_stitch_wdr_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Long First
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//  Indicates exposure of first pixel in image: 0=short, 1=long.
//  This applies to even fields (field input is zero).
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_LONG_FIRST_DEFAULT (1)
#define APICAL_ISP_FRAME_STITCH_LONG_FIRST_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_long_first_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_frame_stitch_long_first_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Use Stitching Error
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_USE_STITCHING_ERROR_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_USE_STITCHING_ERROR_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_use_stitching_error_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_frame_stitch_use_stitching_error_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Use Long Override
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_USE_LONG_OVERRIDE_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_USE_LONG_OVERRIDE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_use_long_override_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_frame_stitch_use_long_override_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Use Max1 Intensity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_USE_MAX1_INTENSITY_DEFAULT (1)
#define APICAL_ISP_FRAME_STITCH_USE_MAX1_INTENSITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_use_max1_intensity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 15) | (curr & 0xffff7fff));
}
static __inline uint8_t apical_isp_frame_stitch_use_max1_intensity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x8000) >> 15);
}
// ------------------------------------------------------------------------------ //
// Register: Use Max2 Intensity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_USE_MAX2_INTENSITY_DEFAULT (1)
#define APICAL_ISP_FRAME_STITCH_USE_MAX2_INTENSITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_use_max2_intensity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 14) | (curr & 0xffffbfff));
}
static __inline uint8_t apical_isp_frame_stitch_use_max2_intensity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x4000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: Use Log
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_USE_LOG_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_USE_LOG_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_use_log_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 13) | (curr & 0xffffdfff));
}
static __inline uint8_t apical_isp_frame_stitch_use_log_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x2000) >> 13);
}
// ------------------------------------------------------------------------------ //
// Register: Enable exp 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_ENABLE_EXP_0_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_ENABLE_EXP_0_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_enable_exp_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_frame_stitch_enable_exp_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Enable exp 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_ENABLE_EXP_1_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_ENABLE_EXP_1_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_enable_exp_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x240L);
	APICAL_WRITE_32(0x240L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_frame_stitch_enable_exp_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x240L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Short Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data above this threshold will be taken from short exposure only
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_SHORT_THRESH_DEFAULT (0x0F00)
#define APICAL_ISP_FRAME_STITCH_SHORT_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_frame_stitch_short_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x244L);
	APICAL_WRITE_32(0x244L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_frame_stitch_short_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x244L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Long Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data below this threshold will be taken from long exposure only
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_LONG_THRESH_DEFAULT (0x0C00)
#define APICAL_ISP_FRAME_STITCH_LONG_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_frame_stitch_long_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x248L);
	APICAL_WRITE_32(0x248L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_frame_stitch_long_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x248L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Exposure Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets ratio between long and short exposures - this must match the actual exposure ratio on the sensor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_EXPOSURE_RATIO_DEFAULT (0x100)
#define APICAL_ISP_FRAME_STITCH_EXPOSURE_RATIO_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_frame_stitch_exposure_ratio_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x24cL);
	APICAL_WRITE_32(0x24cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_frame_stitch_exposure_ratio_read(void) {
	return (uint16_t)((APICAL_READ_32(0x24cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Stitch Correct
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Allows adjustment for error in sensor exposure ratio for stitching area
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_STITCH_CORRECT_DEFAULT (0x80)
#define APICAL_ISP_FRAME_STITCH_STITCH_CORRECT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_frame_stitch_stitch_correct_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x250L);
	APICAL_WRITE_32(0x250L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_frame_stitch_stitch_correct_read(void) {
	return (uint8_t)((APICAL_READ_32(0x250L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Stitch Error Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets level for detection of stitching errors due to motion
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_STITCH_ERROR_THRESH_DEFAULT (0x0040)
#define APICAL_ISP_FRAME_STITCH_STITCH_ERROR_THRESH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_frame_stitch_stitch_error_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x254L);
	APICAL_WRITE_32(0x254L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_frame_stitch_stitch_error_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x254L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Stitch Error Limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets intensity level for long exposure below which stitching error detection is disabled
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_STITCH_ERROR_LIMIT_DEFAULT (0x0200)
#define APICAL_ISP_FRAME_STITCH_STITCH_ERROR_LIMIT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_frame_stitch_stitch_error_limit_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x258L);
	APICAL_WRITE_32(0x258L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_frame_stitch_stitch_error_limit_read(void) {
	return (uint16_t)((APICAL_READ_32(0x258L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black level Long
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black level for long exposure input
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_LONG_DEFAULT (0x000)
#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_LONG_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_frame_stitch_black_level_long_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x260L);
	APICAL_WRITE_32(0x260L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_frame_stitch_black_level_long_read(void) {
	return (uint16_t)((APICAL_READ_32(0x260L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black level Short
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black level for short exposure input
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_SHORT_DEFAULT (0x00)
#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_SHORT_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_frame_stitch_black_level_short_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x264L);
	APICAL_WRITE_32(0x264L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_frame_stitch_black_level_short_read(void) {
	return (uint16_t)((APICAL_READ_32(0x264L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black level Out
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black level for module output
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_OUT_DEFAULT (0x000)
#define APICAL_ISP_FRAME_STITCH_BLACK_LEVEL_OUT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_frame_stitch_black_level_out_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x268L);
	APICAL_WRITE_32(0x268L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_frame_stitch_black_level_out_read(void) {
	return (uint16_t)((APICAL_READ_32(0x268L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Noise Profile RAW frontend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile controls for RAW frontend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Exp Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for determining long/short exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_EXP_THRESH_DEFAULT (0xFFFF)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_EXP_THRESH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_noise_profile_raw_frontend_exp_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x270L);
	APICAL_WRITE_32(0x270L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_noise_profile_raw_frontend_exp_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x270L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Short Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier applied to short exposure data for noise profile calculation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_SHORT_RATIO_DEFAULT (0x20)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_SHORT_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_noise_profile_raw_frontend_short_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x274L);
	APICAL_WRITE_32(0x274L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_noise_profile_raw_frontend_short_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x274L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Long Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier applied to long exposure data for noise profile calculation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_LONG_RATIO_DEFAULT (0x04)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_LONG_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_noise_profile_raw_frontend_long_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x278L);
	APICAL_WRITE_32(0x278L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_noise_profile_raw_frontend_long_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x278L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile black level offset
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_NP_OFF_DEFAULT (0)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_NP_OFF_DATASIZE (7)

// args: data (7-bit)
static __inline void apical_isp_noise_profile_raw_frontend_np_off_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x27cL);
	APICAL_WRITE_32(0x27cL, (((uint32_t) (data & 0x7f)) << 0) | (curr & 0xffffff80));
}
static __inline uint8_t apical_isp_noise_profile_raw_frontend_np_off_read(void) {
	return (uint8_t)((APICAL_READ_32(0x27cL) & 0x7f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off reflect
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//          Defines how values below black level are obtained.
//          0: Repeat the first table entry.
//          1: Reflect the noise profile curve below black level.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_NP_OFF_REFLECT_DEFAULT (0)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_NP_OFF_REFLECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_noise_profile_raw_frontend_np_off_reflect_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x27cL);
	APICAL_WRITE_32(0x27cL, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_noise_profile_raw_frontend_np_off_reflect_read(void) {
	return (uint8_t)((APICAL_READ_32(0x27cL) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Group: Sinter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Spatial noise reduction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Config
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_CONFIG_DEFAULT (0x0)
#define APICAL_ISP_SINTER_CONFIG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_config_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_config_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sinter enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_ENABLE_DEFAULT (1)
#define APICAL_ISP_SINTER_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_sinter_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_sinter_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: View Filter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// For debug purposes only. Set to zero for normal operation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_VIEW_FILTER_DEFAULT (0)
#define APICAL_ISP_SINTER_VIEW_FILTER_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_sinter_view_filter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_sinter_view_filter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Scale Mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// For debug purposes only. Set to 3 for normal operation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_SCALE_MODE_DEFAULT (3)
#define APICAL_ISP_SINTER_SCALE_MODE_DATASIZE (2)
#define APICAL_ISP_SINTER_SCALE_MODE_USE_FILTER_0_ONLY (0)
#define APICAL_ISP_SINTER_SCALE_MODE_USE_FILTERS_0_AND_2_ONLY (1)
#define APICAL_ISP_SINTER_SCALE_MODE_USE_FILTERS_0_2_AND_4_ONLY (2)
#define APICAL_ISP_SINTER_SCALE_MODE_USE_ALL_FILTERS (3)

// args: data (2-bit)
static __inline void apical_isp_sinter_scale_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x3)) << 2) | (curr & 0xfffffff3));
}
static __inline uint8_t apical_isp_sinter_scale_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0xc) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Filter select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sinter filter fine tuning.  Should not be modified from suggested values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_FILTER_SELECT_DEFAULT (0)
#define APICAL_ISP_SINTER_FILTER_SELECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_sinter_filter_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_sinter_filter_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Int select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Select intensity filter.  Should not be modified from suggested values.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_INT_SELECT_DEFAULT (0)
#define APICAL_ISP_SINTER_INT_SELECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_sinter_int_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_sinter_int_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: rm_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		  Adjusts sinter strength radially from center to compensate for Lens shading correction.
//		  enable: 0=off, 1=on
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_RM_ENABLE_DEFAULT (0)
#define APICAL_ISP_SINTER_RM_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_sinter_rm_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_sinter_rm_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: int config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Intensity blending with mosaic raw
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_INT_CONFIG_DEFAULT (0x4)
#define APICAL_ISP_SINTER_INT_CONFIG_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_sinter_int_config_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x280L);
	APICAL_WRITE_32(0x280L, (((uint32_t) (data & 0xf)) << 8) | (curr & 0xfffff0ff));
}
static __inline uint8_t apical_isp_sinter_int_config_read(void) {
	return (uint8_t)((APICAL_READ_32(0x280L) & 0xf00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: rm_center_x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center x coordinate of shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_RM_CENTER_X_DEFAULT (0x280)
#define APICAL_ISP_SINTER_RM_CENTER_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_sinter_rm_center_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x284L);
	APICAL_WRITE_32(0x284L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_sinter_rm_center_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x284L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rm_center_y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center y coordinate of shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_RM_CENTER_Y_DEFAULT (0x168)
#define APICAL_ISP_SINTER_RM_CENTER_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_sinter_rm_center_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x288L);
	APICAL_WRITE_32(0x288L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_sinter_rm_center_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x288L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: rm_off_center_mult
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Normalizing factor which scales the radial table to the edge of the image.
//		Calculated as 2^31/R^2 where R is the furthest distance from the center coordinate to the edge of the image in pixels.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_RM_OFF_CENTER_MULT_DEFAULT (0x0100)
#define APICAL_ISP_SINTER_RM_OFF_CENTER_MULT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_sinter_rm_off_center_mult_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x28cL);
	APICAL_WRITE_32(0x28cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_sinter_rm_off_center_mult_read(void) {
	return (uint16_t)((APICAL_READ_32(0x28cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 0h
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_0H_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_0H_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_0h_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x290L);
	APICAL_WRITE_32(0x290L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_0h_read(void) {
	return (uint8_t)((APICAL_READ_32(0x290L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 1h
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for high horizontal spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_1H_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_1H_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_1h_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x290L);
	APICAL_WRITE_32(0x290L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_sinter_thresh_1h_read(void) {
	return (uint8_t)((APICAL_READ_32(0x290L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 2h
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_2H_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_2H_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_2h_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x294L);
	APICAL_WRITE_32(0x294L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_2h_read(void) {
	return (uint8_t)((APICAL_READ_32(0x294L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 4h
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for low horizontal spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_4H_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_4H_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_4h_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x294L);
	APICAL_WRITE_32(0x294L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_sinter_thresh_4h_read(void) {
	return (uint8_t)((APICAL_READ_32(0x294L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 0v
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_0V_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_0V_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_0v_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x298L);
	APICAL_WRITE_32(0x298L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_0v_read(void) {
	return (uint8_t)((APICAL_READ_32(0x298L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 1v
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for high vertical spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_1V_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_1V_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_1v_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x298L);
	APICAL_WRITE_32(0x298L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_sinter_thresh_1v_read(void) {
	return (uint8_t)((APICAL_READ_32(0x298L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 2v
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_2V_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_2V_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_2v_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x29cL);
	APICAL_WRITE_32(0x29cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_2v_read(void) {
	return (uint8_t)((APICAL_READ_32(0x29cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh 4v
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for low vertical spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_4V_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_4V_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_4v_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x29cL);
	APICAL_WRITE_32(0x29cL, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_sinter_thresh_4v_read(void) {
	return (uint8_t)((APICAL_READ_32(0x29cL) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Short
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold adjustment for short exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_SHORT_DEFAULT (0x00)
#define APICAL_ISP_SINTER_THRESH_SHORT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_short_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2a4L);
	APICAL_WRITE_32(0x2a4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_short_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2a4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Long
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold adjustment for long exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_THRESH_LONG_DEFAULT (0x30)
#define APICAL_ISP_SINTER_THRESH_LONG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_thresh_long_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2a8L);
	APICAL_WRITE_32(0x2a8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_thresh_long_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2a8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Strength 0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_STRENGTH_0_DEFAULT (0xFF)
#define APICAL_ISP_SINTER_STRENGTH_0_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_strength_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2acL);
	APICAL_WRITE_32(0x2acL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_strength_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2acL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Strength 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise reduction effect for high spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_STRENGTH_1_DEFAULT (0xFF)
#define APICAL_ISP_SINTER_STRENGTH_1_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_strength_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2b0L);
	APICAL_WRITE_32(0x2b0L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_strength_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2b0L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Strength 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Unused - no effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_STRENGTH_2_DEFAULT (0xFF)
#define APICAL_ISP_SINTER_STRENGTH_2_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_strength_2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2b4L);
	APICAL_WRITE_32(0x2b4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_strength_2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2b4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Strength 4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise reduction effect for low spatial frequencies
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_STRENGTH_4_DEFAULT (0xFF)
#define APICAL_ISP_SINTER_STRENGTH_4_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_sinter_strength_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2b8L);
	APICAL_WRITE_32(0x2b8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_sinter_strength_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2b8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Temper
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Temporal noise reduction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Temper enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_ENABLE_DEFAULT (0)
#define APICAL_ISP_TEMPER_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_temper_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Log Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1=Normal operation, 0=disable logarithmic weighting function for debug
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_LOG_ENABLE_DEFAULT (1)
#define APICAL_ISP_TEMPER_LOG_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_log_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_temper_log_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Show Alpha
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0=Normal operation, 1=output alpha channel for debug
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_SHOW_ALPHA_DEFAULT (0)
#define APICAL_ISP_TEMPER_SHOW_ALPHA_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_show_alpha_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_temper_show_alpha_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Show AlphaAB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0=Normal operation, 1=output alpha channel for debug
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_SHOW_ALPHAAB_DEFAULT (0)
#define APICAL_ISP_TEMPER_SHOW_ALPHAAB_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_show_alphaab_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_temper_show_alphaab_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Mixer Select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Debug mixer select(Only active when Temper disabled): 0=Input video stream, 1=Frame buffer video stream
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_MIXER_SELECT_DEFAULT (0)
#define APICAL_ISP_TEMPER_MIXER_SELECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_mixer_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_temper_mixer_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Temper2 Mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0: 0=Temper3 mode 1=Temper2 mode
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_TEMPER2_MODE_DEFAULT (0)
#define APICAL_ISP_TEMPER_TEMPER2_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_temper2_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_temper_temper2_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: Frame delay
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Extra output delay: 0=normal output 1=delayed by 1 frame
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_DELAY_DEFAULT (0)
#define APICAL_ISP_TEMPER_FRAME_DELAY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_delay_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c0L);
	APICAL_WRITE_32(0x2c0L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_temper_frame_delay_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c0L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Short
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for short exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_THRESH_SHORT_DEFAULT (0x00)
#define APICAL_ISP_TEMPER_THRESH_SHORT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_temper_thresh_short_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c4L);
	APICAL_WRITE_32(0x2c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_temper_thresh_short_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Thresh Long
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise threshold for long exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_THRESH_LONG_DEFAULT (0x30)
#define APICAL_ISP_TEMPER_THRESH_LONG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_temper_thresh_long_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2c8L);
	APICAL_WRITE_32(0x2c8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_temper_thresh_long_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2c8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Recursion Limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Controls length of filter history. Low values result in longer history and stronger temporal filtering.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_RECURSION_LIMIT_DEFAULT (0x2)
#define APICAL_ISP_TEMPER_RECURSION_LIMIT_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_temper_recursion_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2ccL);
	APICAL_WRITE_32(0x2ccL, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_temper_recursion_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2ccL) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Noise Profile
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile controls for Sinter and Temper
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Exp Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for determining long/short exposure data
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_EXP_THRESH_DEFAULT (0xFFFF)
#define APICAL_ISP_NOISE_PROFILE_EXP_THRESH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_noise_profile_exp_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x2e0L);
	APICAL_WRITE_32(0x2e0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_noise_profile_exp_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x2e0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Short Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier applied to short exposure data for noise profile calculation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_SHORT_RATIO_DEFAULT (0x20)
#define APICAL_ISP_NOISE_PROFILE_SHORT_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_noise_profile_short_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2e4L);
	APICAL_WRITE_32(0x2e4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_noise_profile_short_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2e4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Long Ratio
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier applied to long exposure data for noise profile calculation
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_LONG_RATIO_DEFAULT (0x04)
#define APICAL_ISP_NOISE_PROFILE_LONG_RATIO_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_noise_profile_long_ratio_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2e8L);
	APICAL_WRITE_32(0x2e8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_noise_profile_long_ratio_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2e8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile black level offset
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_NP_OFF_DEFAULT (0)
#define APICAL_ISP_NOISE_PROFILE_NP_OFF_DATASIZE (7)

// args: data (7-bit)
static __inline void apical_isp_noise_profile_np_off_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2ecL);
	APICAL_WRITE_32(0x2ecL, (((uint32_t) (data & 0x7f)) << 0) | (curr & 0xffffff80));
}
static __inline uint8_t apical_isp_noise_profile_np_off_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2ecL) & 0x7f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off reflect
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//          Defines how values below black level are obtained.
//          0: Repeat the first table entry.
//          1: Reflect the noise profile curve below black level.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_NP_OFF_REFLECT_DEFAULT (0)
#define APICAL_ISP_NOISE_PROFILE_NP_OFF_REFLECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_noise_profile_np_off_reflect_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x2ecL);
	APICAL_WRITE_32(0x2ecL, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_noise_profile_np_off_reflect_read(void) {
	return (uint8_t)((APICAL_READ_32(0x2ecL) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Group: White Balance
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Static white balance - independent gain for each color channel
//
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Gain 00
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier for color channel 00 (R)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_WHITE_BALANCE_GAIN_00_DEFAULT (0x100)
#define APICAL_ISP_WHITE_BALANCE_GAIN_00_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_white_balance_gain_00_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x300L);
	APICAL_WRITE_32(0x300L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_white_balance_gain_00_read(void) {
	return (uint16_t)((APICAL_READ_32(0x300L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Gain 01
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier for color channel 01 (Gr)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_WHITE_BALANCE_GAIN_01_DEFAULT (0x100)
#define APICAL_ISP_WHITE_BALANCE_GAIN_01_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_white_balance_gain_01_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x304L);
	APICAL_WRITE_32(0x304L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_white_balance_gain_01_read(void) {
	return (uint16_t)((APICAL_READ_32(0x304L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Gain 10
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier for color channel 10 (Gb)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_WHITE_BALANCE_GAIN_10_DEFAULT (0x100)
#define APICAL_ISP_WHITE_BALANCE_GAIN_10_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_white_balance_gain_10_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x308L);
	APICAL_WRITE_32(0x308L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_white_balance_gain_10_read(void) {
	return (uint16_t)((APICAL_READ_32(0x308L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Gain 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Multiplier for color channel 11 (B)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_WHITE_BALANCE_GAIN_11_DEFAULT (0x100)
#define APICAL_ISP_WHITE_BALANCE_GAIN_11_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_white_balance_gain_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x30cL);
	APICAL_WRITE_32(0x30cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_white_balance_gain_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x30cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset subtraction for each color channel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Black 00
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 00 (R)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_OFFSET_BLACK_00_DEFAULT (0x00)
#define APICAL_ISP_OFFSET_BLACK_00_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_offset_black_00_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x310L);
	APICAL_WRITE_32(0x310L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_offset_black_00_read(void) {
	return (uint16_t)((APICAL_READ_32(0x310L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 01
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 01 (Gr)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_OFFSET_BLACK_01_DEFAULT (0x00)
#define APICAL_ISP_OFFSET_BLACK_01_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_offset_black_01_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x314L);
	APICAL_WRITE_32(0x314L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_offset_black_01_read(void) {
	return (uint16_t)((APICAL_READ_32(0x314L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 10
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 10 (Gb)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_OFFSET_BLACK_10_DEFAULT (0x00)
#define APICAL_ISP_OFFSET_BLACK_10_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_offset_black_10_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x318L);
	APICAL_WRITE_32(0x318L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_offset_black_10_read(void) {
	return (uint16_t)((APICAL_READ_32(0x318L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Black offset for color channel 11 (B)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_OFFSET_BLACK_11_DEFAULT (0x00)
#define APICAL_ISP_OFFSET_BLACK_11_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_offset_black_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x31cL);
	APICAL_WRITE_32(0x31cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_offset_black_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x31cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Radial Shading
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Radial Lens shading correction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Lens shading correction enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_ENABLE_DEFAULT (0)
#define APICAL_ISP_RADIAL_SHADING_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_radial_shading_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x340L);
	APICAL_WRITE_32(0x340L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_radial_shading_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x340L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: MCU priority
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Priority of CPU writes to radial shading LUTs
//		0=low. CPU read/writes from/to the shading LUTs are only executed when LUTs are not being accessed by ISP.  Normal operation.
//		1=high. CPU read/writes from/to the shading LUTs are always executed.  This may result in corrupt image data and invalid read back and is not recommended.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_MCU_PRIORITY_DEFAULT (0)
#define APICAL_ISP_RADIAL_SHADING_MCU_PRIORITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_radial_shading_mcu_priority_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x340L);
	APICAL_WRITE_32(0x340L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_radial_shading_mcu_priority_read(void) {
	return (uint8_t)((APICAL_READ_32(0x340L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_MCU_READY_DEFAULT (0x0)
#define APICAL_ISP_RADIAL_SHADING_MCU_READY_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_radial_shading_mcu_ready_read(void) {
	return (uint8_t)((APICAL_READ_32(0x340L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: centerR x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center x coordinate of the red shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERR_X_DEFAULT (0x3C0)
#define APICAL_ISP_RADIAL_SHADING_CENTERR_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerr_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x344L);
	APICAL_WRITE_32(0x344L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerr_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x344L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: centerR y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center y coordinate of the red shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERR_Y_DEFAULT (0x21C)
#define APICAL_ISP_RADIAL_SHADING_CENTERR_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerr_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x348L);
	APICAL_WRITE_32(0x348L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerr_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x348L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: centerG x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center x coordinate of the green shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERG_X_DEFAULT (0x3C0)
#define APICAL_ISP_RADIAL_SHADING_CENTERG_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerg_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x34cL);
	APICAL_WRITE_32(0x34cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerg_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x34cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: centerG y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center y coordinate of the green shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERG_Y_DEFAULT (0x21C)
#define APICAL_ISP_RADIAL_SHADING_CENTERG_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerg_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x350L);
	APICAL_WRITE_32(0x350L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerg_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x350L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: centerB x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center x coordinate of the blue shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERB_X_DEFAULT (0x3C0)
#define APICAL_ISP_RADIAL_SHADING_CENTERB_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerb_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x354L);
	APICAL_WRITE_32(0x354L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerb_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x354L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: centerB y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Center y coordinate of the blue shading map
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_CENTERB_Y_DEFAULT (0x21C)
#define APICAL_ISP_RADIAL_SHADING_CENTERB_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_centerb_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x358L);
	APICAL_WRITE_32(0x358L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_centerb_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x358L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: off center multR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Normalizing factor which scales the Red radial table to the edge of the image.
//		Calculated as 2^31/R^2 where R is the furthest distance from the center coordinate to the edge of the image in pixels.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTR_DEFAULT (0x06EA)
#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTR_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_off_center_multr_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x35cL);
	APICAL_WRITE_32(0x35cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_off_center_multr_read(void) {
	return (uint16_t)((APICAL_READ_32(0x35cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: off center multG
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Normalizing factor which scales the green radial table to the edge of the image.
//		Calculated as 2^31/R^2 where R is the furthest distance from the center coordinate to the edge of the image in pixels.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTG_DEFAULT (0x06EA)
#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTG_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_off_center_multg_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x360L);
	APICAL_WRITE_32(0x360L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_off_center_multg_read(void) {
	return (uint16_t)((APICAL_READ_32(0x360L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: off center multB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Normalizing factor which scales the blue radial table to the edge of the image.
//		Calculated as 2^31/R^2 where R is the furthest distance from the center coordinate to the edge of the image in pixels.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTB_DEFAULT (0x06EA)
#define APICAL_ISP_RADIAL_SHADING_OFF_CENTER_MULTB_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_radial_shading_off_center_multb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x364L);
	APICAL_WRITE_32(0x364L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_radial_shading_off_center_multb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x364L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Mesh Shading
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Mesh Lens shading correction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Lens shading correction enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_ENABLE_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mesh_shading_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_mesh_shading_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh show
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Lens shading correction debug: 0=off, 1=on (show mesh data)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_SHOW_DEFAULT (1)
#define APICAL_ISP_MESH_SHADING_MESH_SHOW_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mesh_shading_mesh_show_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_show_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Selects the precision and maximal gain range of mesh shading correction
//		Gain range:	00->0..2; 01->0..4; 02->0..8; 03->0..16; 04->1..2; 05->1..3; 06-> 1..5; 07->1..9(float)
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_SCALE_DEFAULT (1)
#define APICAL_ISP_MESH_SHADING_MESH_SCALE_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_mesh_shading_mesh_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x7)) << 2) | (curr & 0xffffffe3));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x1c) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh page R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects memory page for red pixels correction.  See ISP guide for further details
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_PAGE_R_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_PAGE_R_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_mesh_shading_mesh_page_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x3)) << 8) | (curr & 0xfffffcff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_page_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x300) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh page G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects memory page for green pixels correction.  See ISP guide for further details
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_PAGE_G_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_PAGE_G_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_mesh_shading_mesh_page_g_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x3)) << 10) | (curr & 0xfffff3ff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_page_g_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0xc00) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh page B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects memory page for blue pixels correction.  See ISP guide for further details
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_PAGE_B_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_PAGE_B_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_mesh_shading_mesh_page_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x3)) << 12) | (curr & 0xffffcfff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_page_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x3000) >> 12);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of horizontal nodes minus 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_WIDTH_DEFAULT (63)
#define APICAL_ISP_MESH_SHADING_MESH_WIDTH_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_mesh_shading_mesh_width_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x3f)) << 16) | (curr & 0xffc0ffff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x3f0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of vertical nodes minus 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_HEIGHT_DEFAULT (63)
#define APICAL_ISP_MESH_SHADING_MESH_HEIGHT_DATASIZE (6)

// args: data (6-bit)
static __inline void apical_isp_mesh_shading_mesh_height_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x380L);
	APICAL_WRITE_32(0x380L, (((uint32_t) (data & 0x3f)) << 24) | (curr & 0xc0ffffff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0x380L) & 0x3f000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh reload
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0-1 triggers cache reload
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_RELOAD_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_RELOAD_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mesh_shading_mesh_reload_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x384L);
	APICAL_WRITE_32(0x384L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_reload_read(void) {
	return (uint8_t)((APICAL_READ_32(0x384L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Sets alpha blending between mesh shading tables.
//		0 = no alpha blending;
//		1=2 banks (odd/even bytes)
//		2=4 banks (one per 8 bit lane in each dword)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_MODE_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_MODE_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x390L);
	APICAL_WRITE_32(0x390L, (((uint32_t) (data & 0x3)) << 0) | (curr & 0xfffffffc));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x390L) & 0x3) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha bank R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bank selection for R blend: 0: 0+1; 1: 1+2; 2: 2:3; 3: 3+0; 4:0+2; 5: 1+3; 6,7: reserved
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_R_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_R_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_bank_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x394L);
	APICAL_WRITE_32(0x394L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_bank_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x394L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha bank G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bank selection for G blend: 0: 0+1; 1: 1+2; 2: 2:3; 3: 3+0; 4:0+2; 5: 1+3; 6,7: reserved:
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_G_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_G_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_bank_g_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x394L);
	APICAL_WRITE_32(0x394L, (((uint32_t) (data & 0x7)) << 8) | (curr & 0xfffff8ff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_bank_g_read(void) {
	return (uint8_t)((APICAL_READ_32(0x394L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha bank B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bank selection for B blend: 0: 0+1; 1: 1+2; 2: 2:3; 3: 3+0; 4:0+2; 5: 1+3; 6,7: reserved
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_B_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_BANK_B_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_bank_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x394L);
	APICAL_WRITE_32(0x394L, (((uint32_t) (data & 0x7)) << 16) | (curr & 0xfff8ffff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_bank_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x394L) & 0x70000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Alpha blend coeff for R
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_R_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_R_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x398L);
	APICAL_WRITE_32(0x398L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x398L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Alpha blend coeff for G
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_G_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_G_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_g_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x398L);
	APICAL_WRITE_32(0x398L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_g_read(void) {
	return (uint8_t)((APICAL_READ_32(0x398L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh alpha B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Alpha blend coeff for B
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_B_DEFAULT (0)
#define APICAL_ISP_MESH_SHADING_MESH_ALPHA_B_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mesh_shading_mesh_alpha_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x398L);
	APICAL_WRITE_32(0x398L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_mesh_shading_mesh_alpha_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x398L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Mesh strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Mesh strength in 4.12 format, e.g. 0 - no correction, 4096 - correction to match mesh data. Can be used to reduce shading correction based on AE.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MESH_SHADING_MESH_STRENGTH_DEFAULT (0x1000)
#define APICAL_ISP_MESH_SHADING_MESH_STRENGTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_mesh_shading_mesh_strength_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x39cL);
	APICAL_WRITE_32(0x39cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_mesh_shading_mesh_strength_read(void) {
	return (uint16_t)((APICAL_READ_32(0x39cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Iridix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Iridix is an adaptive, space-variant tone mapping engine.
//It is used to maintain or enhance shadow detail while preserving highlights.
//
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Control 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_CONTROL_0_DEFAULT (0x29)
#define APICAL_ISP_IRIDIX_CONTROL_0_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_control_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_iridix_control_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_ENABLE_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_iridix_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Max Type
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_MAX_TYPE_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_MAX_TYPE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_max_type_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_iridix_max_type_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Set Black Level Amp 0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//            1=Ignore Black level (set to zero) in amplificator.
//            0=Use Black level value.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_SET_BLACK_LEVEL_AMP_0_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_SET_BLACK_LEVEL_AMP_0_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_set_black_level_amp_0_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_iridix_set_black_level_amp_0_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: Rev percept LUT position
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_LUT_REV_PERCEPT_LUT_POSITION_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_LUT_REV_PERCEPT_LUT_POSITION_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_lut_rev_percept_lut_position_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 6) | (curr & 0xffffffbf));
}
static __inline uint8_t apical_isp_iridix_lut_rev_percept_lut_position_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Control 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_CONTROL_1_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_CONTROL_1_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_control_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_iridix_control_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Collect OVL
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_COLLECT_OVL_DEFAULT (0)
#define APICAL_ISP_IRIDIX_COLLECT_OVL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_collect_ovl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_iridix_collect_ovl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Collect RND
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_COLLECT_RND_DEFAULT (1)
#define APICAL_ISP_IRIDIX_COLLECT_RND_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_collect_rnd_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_iridix_collect_rnd_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: Stat Norm
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_STAT_NORM_DEFAULT (1)
#define APICAL_ISP_IRIDIX_STAT_NORM_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_stat_norm_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_iridix_stat_norm_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: Stat Mult
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_STAT_MULT_DEFAULT (1)
#define APICAL_ISP_IRIDIX_STAT_MULT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_iridix_stat_mult_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c0L);
	APICAL_WRITE_32(0x3c0L, (((uint32_t) (data & 0x3)) << 14) | (curr & 0xffff3fff));
}
static __inline uint8_t apical_isp_iridix_stat_mult_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c0L) & 0xc000) >> 14);
}
// ------------------------------------------------------------------------------ //
// Register: Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Strength of dynamic range compression. With other parameters at defaults, increases visibility of shadows
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_STRENGTH_DEFAULT (0x80)
#define APICAL_ISP_IRIDIX_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c4L);
	APICAL_WRITE_32(0x3c4L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_iridix_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c4L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Variance
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_VARIANCE_DEFAULT (0x0)
#define APICAL_ISP_IRIDIX_VARIANCE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_variance_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c8L);
	APICAL_WRITE_32(0x3c8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_iridix_variance_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Variance Space
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets the degree of spatial sensitivity of the algorithm
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_VARIANCE_SPACE_DEFAULT (0x2)
#define APICAL_ISP_IRIDIX_VARIANCE_SPACE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_iridix_variance_space_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c8L);
	APICAL_WRITE_32(0x3c8L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_iridix_variance_space_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c8L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Variance Intensity
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets the degree of luminance sensitivity of the algorithm
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_VARIANCE_INTENSITY_DEFAULT (0x1)
#define APICAL_ISP_IRIDIX_VARIANCE_INTENSITY_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_iridix_variance_intensity_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c8L);
	APICAL_WRITE_32(0x3c8L, (((uint32_t) (data & 0xf)) << 4) | (curr & 0xffffff0f));
}
static __inline uint8_t apical_isp_iridix_variance_intensity_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c8L) & 0xf0) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Slope Max
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Restricts the maximum slope (gain) which can be generated by the adaptive algorithm
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_SLOPE_MAX_DEFAULT (0x80)
#define APICAL_ISP_IRIDIX_SLOPE_MAX_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_slope_max_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c8L);
	APICAL_WRITE_32(0x3c8L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_iridix_slope_max_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c8L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Slope Min
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Restricts the minimum slope (gain) which can be generated by the adaptive algorithm
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_SLOPE_MIN_DEFAULT (0x40)
#define APICAL_ISP_IRIDIX_SLOPE_MIN_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_slope_min_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3c8L);
	APICAL_WRITE_32(0x3c8L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_iridix_slope_min_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3c8L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Black Level
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix black level.  Values below this will not be affected by Iridix.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_BLACK_LEVEL_DEFAULT (0x000)
#define APICAL_ISP_IRIDIX_BLACK_LEVEL_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_iridix_black_level_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x3d0L);
	APICAL_WRITE_32(0x3d0L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_iridix_black_level_read(void) {
	return (uint16_t)((APICAL_READ_32(0x3d0L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: White Level
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix white level.  Values above this will not be affected by Iridix.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_WHITE_LEVEL_DEFAULT (0xFFF)
#define APICAL_ISP_IRIDIX_WHITE_LEVEL_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_iridix_white_level_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x3d4L);
	APICAL_WRITE_32(0x3d4L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_iridix_white_level_read(void) {
	return (uint16_t)((APICAL_READ_32(0x3d4L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Limit Ampl
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_LIMIT_AMPL_DEFAULT (0x00)
#define APICAL_ISP_IRIDIX_LIMIT_AMPL_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_iridix_limit_ampl_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3d8L);
	APICAL_WRITE_32(0x3d8L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_iridix_limit_ampl_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3d8L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Collection correction
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_COLLECTION_CORRECTION_DEFAULT (0x100)
#define APICAL_ISP_IRIDIX_COLLECTION_CORRECTION_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_iridix_collection_correction_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x3dcL);
	APICAL_WRITE_32(0x3dcL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_iridix_collection_correction_read(void) {
	return (uint16_t)((APICAL_READ_32(0x3dcL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Rev Percept Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix lookup 1 enable: 0=off 1=on.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_REV_PERCEPT_ENABLE_DEFAULT (0)
#define APICAL_ISP_IRIDIX_REV_PERCEPT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_rev_percept_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3e0L);
	APICAL_WRITE_32(0x3e0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_iridix_rev_percept_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3e0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Fwd Percept Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix lookup 2 enable: 0=off 1=on.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_FWD_PERCEPT_ENABLE_DEFAULT (0)
#define APICAL_ISP_IRIDIX_FWD_PERCEPT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_iridix_fwd_percept_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x3e0L);
	APICAL_WRITE_32(0x3e0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_iridix_fwd_percept_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x3e0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Group: Demosaic
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bayer Demosaic
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: VH Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of vertical/horizontal blending threshold in 4.4 logarithmic format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VH_SLOPE_DEFAULT (0xC0)
#define APICAL_ISP_DEMOSAIC_VH_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_vh_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x400L);
	APICAL_WRITE_32(0x400L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_vh_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x400L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AA Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of angular blending threshold in 4.4 logarithmic format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AA_SLOPE_DEFAULT (0xC0)
#define APICAL_ISP_DEMOSAIC_AA_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_aa_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x404L);
	APICAL_WRITE_32(0x404L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_aa_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x404L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VA Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of VH-AA (VA) blending threshold in 4.4 logarithmic format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VA_SLOPE_DEFAULT (0xAA)
#define APICAL_ISP_DEMOSAIC_VA_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_va_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x408L);
	APICAL_WRITE_32(0x408L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_va_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x408L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: UU Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of undefined blending threshold in 4.4 logarithmic format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_UU_SLOPE_DEFAULT (0xAD)
#define APICAL_ISP_DEMOSAIC_UU_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_uu_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x40cL);
	APICAL_WRITE_32(0x40cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_uu_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x40cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Sat Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of saturation blending threshold in linear format 2.6
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_SAT_SLOPE_DEFAULT (0x5D)
#define APICAL_ISP_DEMOSAIC_SAT_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_sat_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x410L);
	APICAL_WRITE_32(0x410L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_sat_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x410L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VH Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of vertical/horizontal blending
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VH_THRESH_DEFAULT (0x131)
#define APICAL_ISP_DEMOSAIC_VH_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_vh_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x414L);
	APICAL_WRITE_32(0x414L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_vh_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x414L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AA Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of angular blending
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AA_THRESH_DEFAULT (0xA0)
#define APICAL_ISP_DEMOSAIC_AA_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_aa_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x418L);
	APICAL_WRITE_32(0x418L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_aa_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x418L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VA Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of VA blending
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VA_THRESH_DEFAULT (0x70)
#define APICAL_ISP_DEMOSAIC_VA_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_va_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x41cL);
	APICAL_WRITE_32(0x41cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_va_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x41cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: UU Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of undefined blending
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_UU_THRESH_DEFAULT (0x171)
#define APICAL_ISP_DEMOSAIC_UU_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_uu_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x420L);
	APICAL_WRITE_32(0x420L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_uu_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x420L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Sat Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of saturation blending  in signed 2.9 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_SAT_THRESH_DEFAULT (0x171)
#define APICAL_ISP_DEMOSAIC_SAT_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_sat_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x424L);
	APICAL_WRITE_32(0x424L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_sat_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x424L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VH Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for vertical/horizontal blending threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VH_OFFSET_DEFAULT (0x800)
#define APICAL_ISP_DEMOSAIC_VH_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_vh_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x428L);
	APICAL_WRITE_32(0x428L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_vh_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x428L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AA Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for angular blending threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AA_OFFSET_DEFAULT (0x800)
#define APICAL_ISP_DEMOSAIC_AA_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_aa_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x42cL);
	APICAL_WRITE_32(0x42cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_aa_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x42cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VA Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for VA blending threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_VA_OFFSET_DEFAULT (0x800)
#define APICAL_ISP_DEMOSAIC_VA_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_va_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x430L);
	APICAL_WRITE_32(0x430L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_va_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x430L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: UU Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for undefined blending threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_UU_OFFSET_DEFAULT (0x000)
#define APICAL_ISP_DEMOSAIC_UU_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_uu_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x434L);
	APICAL_WRITE_32(0x434L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_uu_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x434L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Sat Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for saturation blending threshold in signed 2.9 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_SAT_OFFSET_DEFAULT (0x000)
#define APICAL_ISP_DEMOSAIC_SAT_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_sat_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x438L);
	APICAL_WRITE_32(0x438L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_sat_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x438L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: sharp_alt_d
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Directional sharp mask strength in signed 4.4 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_SHARP_ALT_D_DEFAULT (0x30)
#define APICAL_ISP_DEMOSAIC_SHARP_ALT_D_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_sharp_alt_d_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x43cL);
	APICAL_WRITE_32(0x43cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_sharp_alt_d_read(void) {
	return (uint8_t)((APICAL_READ_32(0x43cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: sharp_alt_ud
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Non-directional sharp mask strength in signed 4.4 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_SHARP_ALT_UD_DEFAULT (0x20)
#define APICAL_ISP_DEMOSAIC_SHARP_ALT_UD_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_sharp_alt_ud_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x440L);
	APICAL_WRITE_32(0x440L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_sharp_alt_ud_read(void) {
	return (uint8_t)((APICAL_READ_32(0x440L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: lum_thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Luminance threshold for directional sharpening
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_LUM_THRESH_DEFAULT (0x060)
#define APICAL_ISP_DEMOSAIC_LUM_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_lum_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x444L);
	APICAL_WRITE_32(0x444L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_lum_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x444L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: np_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile offset in logarithmic 4.4 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_NP_OFFSET_DEFAULT (0x00)
#define APICAL_ISP_DEMOSAIC_NP_OFFSET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_np_offset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x448L);
	APICAL_WRITE_32(0x448L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_np_offset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x448L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dmsc config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Debug output select. Set to 0x00 for normal operation.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_DMSC_CONFIG_DEFAULT (0x00)
#define APICAL_ISP_DEMOSAIC_DMSC_CONFIG_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_dmsc_config_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x44cL);
	APICAL_WRITE_32(0x44cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_dmsc_config_read(void) {
	return (uint8_t)((APICAL_READ_32(0x44cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AC Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold for the range of AC blending in signed 2.9 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AC_THRESH_DEFAULT (0x1B3)
#define APICAL_ISP_DEMOSAIC_AC_THRESH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_ac_thresh_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x450L);
	APICAL_WRITE_32(0x450L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_ac_thresh_read(void) {
	return (uint16_t)((APICAL_READ_32(0x450L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AC Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope of AC blending threshold in linear format 2.6
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AC_SLOPE_DEFAULT (0xCF)
#define APICAL_ISP_DEMOSAIC_AC_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_ac_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x454L);
	APICAL_WRITE_32(0x454L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_ac_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x454L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AC Offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for AC blending threshold in signed 2.9 format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_AC_OFFSET_DEFAULT (0x000)
#define APICAL_ISP_DEMOSAIC_AC_OFFSET_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_demosaic_ac_offset_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x458L);
	APICAL_WRITE_32(0x458L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_demosaic_ac_offset_read(void) {
	return (uint16_t)((APICAL_READ_32(0x458L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: FC Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope (strength) of false color correction
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_FC_SLOPE_DEFAULT (0x80)
#define APICAL_ISP_DEMOSAIC_FC_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_fc_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x45cL);
	APICAL_WRITE_32(0x45cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_fc_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x45cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: FC Alias Slope
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Slope (strength) of false colour correction after blending with saturation value in 2.6 unsigned format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_FC_ALIAS_SLOPE_DEFAULT (0x55)
#define APICAL_ISP_DEMOSAIC_FC_ALIAS_SLOPE_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_fc_alias_slope_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x460L);
	APICAL_WRITE_32(0x460L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_fc_alias_slope_read(void) {
	return (uint8_t)((APICAL_READ_32(0x460L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: FC Alias Thresh
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Threshold of false colour correction after blending with saturation valuet in in 0.8 unsigned format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_FC_ALIAS_THRESH_DEFAULT (0x00)
#define APICAL_ISP_DEMOSAIC_FC_ALIAS_THRESH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_demosaic_fc_alias_thresh_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x464L);
	APICAL_WRITE_32(0x464L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_demosaic_fc_alias_thresh_read(void) {
	return (uint8_t)((APICAL_READ_32(0x464L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile black level offset
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_NP_OFF_DEFAULT (0)
#define APICAL_ISP_DEMOSAIC_NP_OFF_DATASIZE (7)

// args: data (7-bit)
static __inline void apical_isp_demosaic_np_off_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x46cL);
	APICAL_WRITE_32(0x46cL, (((uint32_t) (data & 0x7f)) << 0) | (curr & 0xffffff80));
}
static __inline uint8_t apical_isp_demosaic_np_off_read(void) {
	return (uint8_t)((APICAL_READ_32(0x46cL) & 0x7f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: NP off reflect
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//          Defines how values below black level are obtained.
//          0: Repeat the first table entry.
//          1: Reflect the noise profile curve below black level.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_NP_OFF_REFLECT_DEFAULT (0)
#define APICAL_ISP_DEMOSAIC_NP_OFF_REFLECT_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_demosaic_np_off_reflect_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x46cL);
	APICAL_WRITE_32(0x46cL, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_demosaic_np_off_reflect_read(void) {
	return (uint8_t)((APICAL_READ_32(0x46cL) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Group: Color Matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Color correction on RGB data using a 3x3 color matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Color matrix enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_ENABLE_DEFAULT (1)
#define APICAL_ISP_MATRIX_RGB_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_matrix_rgb_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4a4L);
	APICAL_WRITE_32(0x4a4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_matrix_rgb_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4a4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft R-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_R_R_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_R_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_r_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x480L);
	APICAL_WRITE_32(0x480L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_r_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x480L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft R-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_R_G_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_R_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_r_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x484L);
	APICAL_WRITE_32(0x484L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_r_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x484L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft R-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for red-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_R_B_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_R_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_r_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x488L);
	APICAL_WRITE_32(0x488L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_r_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x488L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_G_R_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_G_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_g_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x48cL);
	APICAL_WRITE_32(0x48cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_g_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x48cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_G_G_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_G_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_g_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x490L);
	APICAL_WRITE_32(0x490L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_g_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x490L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft G-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for green-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_G_B_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_G_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_g_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x494L);
	APICAL_WRITE_32(0x494L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_g_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x494L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-red multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_B_R_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_B_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_b_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x498L);
	APICAL_WRITE_32(0x498L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_b_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x498L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-green multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_B_G_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_B_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_b_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x49cL);
	APICAL_WRITE_32(0x49cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_b_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x49cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft B-B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for blue-blue multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_B_B_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_B_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_b_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4a0L);
	APICAL_WRITE_32(0x4a0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_b_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4a0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft WB R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// gain for Red channel for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_R_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_R_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_wb_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4a8L);
	APICAL_WRITE_32(0x4a8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_wb_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4a8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft WB G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// gain for Green channel for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_G_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_G_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_wb_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4acL);
	APICAL_WRITE_32(0x4acL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_wb_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4acL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft WB B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// gain for Blue channel for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_B_DEFAULT (0x0100)
#define APICAL_ISP_MATRIX_RGB_COEFFT_WB_B_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_matrix_rgb_coefft_wb_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4b0L);
	APICAL_WRITE_32(0x4b0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_wb_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4b0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft fog offset R
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset R for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_R_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_R_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_matrix_rgb_coefft_fog_offset_r_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4b4L);
	APICAL_WRITE_32(0x4b4L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_fog_offset_r_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4b4L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft fog offset G
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset G for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_G_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_G_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_matrix_rgb_coefft_fog_offset_g_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4b8L);
	APICAL_WRITE_32(0x4b8L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_fog_offset_g_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4b8L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft fog offset B
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset B for antifog function
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_B_DEFAULT (0x0000)
#define APICAL_ISP_MATRIX_RGB_COEFFT_FOG_OFFSET_B_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_matrix_rgb_coefft_fog_offset_b_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4bcL);
	APICAL_WRITE_32(0x4bcL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_matrix_rgb_coefft_fog_offset_b_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4bcL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Crop FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Crop for full resolution path
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Crop enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CROP_ENABLE_CROP_DEFAULT (0)
#define APICAL_ISP_FR_CROP_ENABLE_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_crop_enable_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4c0L);
	APICAL_WRITE_32(0x4c0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_crop_enable_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4c0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal offset from left side of image in pixels for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CROP_START_X_DEFAULT (0x0000)
#define APICAL_ISP_FR_CROP_START_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_crop_start_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4c4L);
	APICAL_WRITE_32(0x4c4L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_crop_start_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4c4L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical offset from top of image in lines for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CROP_START_Y_DEFAULT (0x0000)
#define APICAL_ISP_FR_CROP_START_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_crop_start_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4c8L);
	APICAL_WRITE_32(0x4c8L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_crop_start_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4c8L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// width of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CROP_SIZE_X_DEFAULT (0xffff)
#define APICAL_ISP_FR_CROP_SIZE_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_crop_size_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4ccL);
	APICAL_WRITE_32(0x4ccL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_crop_size_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4ccL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// height of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CROP_SIZE_Y_DEFAULT (0xffff)
#define APICAL_ISP_FR_CROP_SIZE_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_crop_size_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x4d0L);
	APICAL_WRITE_32(0x4d0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_crop_size_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x4d0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Gamma FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma correction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_GAMMA_RGB_ENABLE_DEFAULT (1)
#define APICAL_ISP_FR_GAMMA_RGB_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_gamma_rgb_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e0L);
	APICAL_WRITE_32(0x4e0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_gamma_rgb_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: MCU priority
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Priority of CPU port
//		0=low. CPU read/writes from/to the LUTs are only executed when LUTs are not being accessed by ISP.  Normal operation.
//		1=high. CPU read/writes from/to the LUTs are always executed.  This may result in corrupt image data and invalid read back and is not recommended.		0=low, 1=high
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_GAMMA_MCU_PRIORITY_DEFAULT (1)
#define APICAL_ISP_FR_GAMMA_MCU_PRIORITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_gamma_mcu_priority_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e0L);
	APICAL_WRITE_32(0x4e0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fr_gamma_mcu_priority_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_GAMMA_RGB_MCU_READY_DEFAULT (0x0)
#define APICAL_ISP_FR_GAMMA_RGB_MCU_READY_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_gamma_rgb_mcu_ready_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Group: Gamma DS 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma correction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_GAMMA_RGB_ENABLE_DEFAULT (1)
#define APICAL_ISP_DS1_GAMMA_RGB_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_gamma_rgb_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e4L);
	APICAL_WRITE_32(0x4e4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_gamma_rgb_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: MCU priority
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Priority of CPU port
//		0=low. CPU read/writes from/to the LUTs are only executed when LUTs are not being accessed by ISP.  Normal operation.
//		1=high. CPU read/writes from/to the LUTs are always executed.  This may result in corrupt image data and invalid read back and is not recommended.		0=low, 1=high
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_GAMMA_MCU_PRIORITY_DEFAULT (1)
#define APICAL_ISP_DS1_GAMMA_MCU_PRIORITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_gamma_mcu_priority_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e4L);
	APICAL_WRITE_32(0x4e4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1_gamma_mcu_priority_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_GAMMA_RGB_MCU_READY_DEFAULT (0x0)
#define APICAL_ISP_DS1_GAMMA_RGB_MCU_READY_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_gamma_rgb_mcu_ready_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e4L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Group: Gamma DS 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma correction
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Gamma enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_GAMMA_RGB_ENABLE_DEFAULT (1)
#define APICAL_ISP_DS2_GAMMA_RGB_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_gamma_rgb_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e8L);
	APICAL_WRITE_32(0x4e8L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_gamma_rgb_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e8L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: MCU priority
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Priority of CPU port
//		0=low. CPU read/writes from/to the LUTs are only executed when LUTs are not being accessed by ISP.  Normal operation.
//		1=high. CPU read/writes from/to the LUTs are always executed.  This may result in corrupt image data and invalid read back and is not recommended.		0=low, 1=high
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_GAMMA_MCU_PRIORITY_DEFAULT (1)
#define APICAL_ISP_DS2_GAMMA_MCU_PRIORITY_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_gamma_mcu_priority_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x4e8L);
	APICAL_WRITE_32(0x4e8L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2_gamma_mcu_priority_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e8L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: MCU ready
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT is ready to receive the data from CPU
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_GAMMA_RGB_MCU_READY_DEFAULT (0x0)
#define APICAL_ISP_DS2_GAMMA_RGB_MCU_READY_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_gamma_rgb_mcu_ready_read(void) {
	return (uint8_t)((APICAL_READ_32(0x4e8L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Group: Sharpen FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Non-linear sharpening algorithm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sharpening enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_SHARPEN_ENABLE_DEFAULT (0)
#define APICAL_ISP_FR_SHARPEN_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_sharpen_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x500L);
	APICAL_WRITE_32(0x500L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_sharpen_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x500L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coring
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects LUT memory bank, value 00 connects bank 0 to sharpening and bank 1 to programming, value 01 swap banks
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_SHARPEN_CORING_DEFAULT (1)
#define APICAL_ISP_FR_SHARPEN_CORING_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_fr_sharpen_coring_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x500L);
	APICAL_WRITE_32(0x500L, (((uint32_t) (data & 0x3)) << 2) | (curr & 0xfffffff3));
}
static __inline uint8_t apical_isp_fr_sharpen_coring_read(void) {
	return (uint8_t)((APICAL_READ_32(0x500L) & 0xc) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls strength of sharpening effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_SHARPEN_STRENGTH_DEFAULT (0x30)
#define APICAL_ISP_FR_SHARPEN_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_sharpen_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x504L);
	APICAL_WRITE_32(0x504L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_fr_sharpen_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x504L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control R
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_SHARPEN_CONTROL_R_DEFAULT (0x60)
#define APICAL_ISP_FR_SHARPEN_CONTROL_R_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_sharpen_control_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x508L);
	APICAL_WRITE_32(0x508L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_fr_sharpen_control_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x508L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control B
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_SHARPEN_CONTROL_B_DEFAULT (0x40)
#define APICAL_ISP_FR_SHARPEN_CONTROL_B_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_sharpen_control_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x508L);
	APICAL_WRITE_32(0x508L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_fr_sharpen_control_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x508L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Group: Logo
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Logo Left
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets x coordinate of logo (in 16-pixel steps)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_LOGO_LOGO_LEFT_DEFAULT (0x08)
#define APICAL_ISP_LOGO_LOGO_LEFT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_logo_logo_left_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x520L);
	APICAL_WRITE_32(0x520L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_logo_logo_left_read(void) {
	return (uint8_t)((APICAL_READ_32(0x520L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Logo Top
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets y coordinate of logo (in 16-pixel steps)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_LOGO_LOGO_TOP_DEFAULT (0x08)
#define APICAL_ISP_LOGO_LOGO_TOP_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_logo_logo_top_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x524L);
	APICAL_WRITE_32(0x524L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_logo_logo_top_read(void) {
	return (uint8_t)((APICAL_READ_32(0x524L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Colour space conv FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Conversion of RGB to YUV data using a 3x3 color matrix plus offsets
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Color matrix enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_ENABLE_MATRIX_DEFAULT (0)
#define APICAL_ISP_FR_CS_CONV_ENABLE_MATRIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_cs_conv_enable_matrix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x570L);
	APICAL_WRITE_32(0x570L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_cs_conv_enable_matrix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x570L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable filter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Filter enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_ENABLE_FILTER_DEFAULT (0)
#define APICAL_ISP_FR_CS_CONV_ENABLE_FILTER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_cs_conv_enable_filter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x570L);
	APICAL_WRITE_32(0x570L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fr_cs_conv_enable_filter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x570L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Enable horizontal downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_FR_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_cs_conv_enable_horizontal_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x570L);
	APICAL_WRITE_32(0x570L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_fr_cs_conv_enable_horizontal_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x570L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Enable vertical downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_FR_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_cs_conv_enable_vertical_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x570L);
	APICAL_WRITE_32(0x570L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_fr_cs_conv_enable_vertical_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x570L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_11_DEFAULT (0x002f)
#define APICAL_ISP_FR_CS_CONV_COEFFT_11_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x540L);
	APICAL_WRITE_32(0x540L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x540L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 12
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_12_DEFAULT (0x009d)
#define APICAL_ISP_FR_CS_CONV_COEFFT_12_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_12_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x544L);
	APICAL_WRITE_32(0x544L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_12_read(void) {
	return (uint16_t)((APICAL_READ_32(0x544L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 13
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_13_DEFAULT (0x0010)
#define APICAL_ISP_FR_CS_CONV_COEFFT_13_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_13_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x548L);
	APICAL_WRITE_32(0x548L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_13_read(void) {
	return (uint16_t)((APICAL_READ_32(0x548L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 21
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_21_DEFAULT (0x801a)
#define APICAL_ISP_FR_CS_CONV_COEFFT_21_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_21_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x54cL);
	APICAL_WRITE_32(0x54cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_21_read(void) {
	return (uint16_t)((APICAL_READ_32(0x54cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 22
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_22_DEFAULT (0x8057)
#define APICAL_ISP_FR_CS_CONV_COEFFT_22_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_22_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x550L);
	APICAL_WRITE_32(0x550L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_22_read(void) {
	return (uint16_t)((APICAL_READ_32(0x550L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 23
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_23_DEFAULT (0x0070)
#define APICAL_ISP_FR_CS_CONV_COEFFT_23_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_23_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x554L);
	APICAL_WRITE_32(0x554L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_23_read(void) {
	return (uint16_t)((APICAL_READ_32(0x554L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 31
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_31_DEFAULT (0x0070)
#define APICAL_ISP_FR_CS_CONV_COEFFT_31_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_31_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x558L);
	APICAL_WRITE_32(0x558L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_31_read(void) {
	return (uint16_t)((APICAL_READ_32(0x558L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 32
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_32_DEFAULT (0x8066)
#define APICAL_ISP_FR_CS_CONV_COEFFT_32_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_32_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x55cL);
	APICAL_WRITE_32(0x55cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_32_read(void) {
	return (uint16_t)((APICAL_READ_32(0x55cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 33
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_33_DEFAULT (0x800a)
#define APICAL_ISP_FR_CS_CONV_COEFFT_33_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_cs_conv_coefft_33_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x560L);
	APICAL_WRITE_32(0x560L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_33_read(void) {
	return (uint16_t)((APICAL_READ_32(0x560L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Y
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_O1_DEFAULT (0x000)
#define APICAL_ISP_FR_CS_CONV_COEFFT_O1_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_fr_cs_conv_coefft_o1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x564L);
	APICAL_WRITE_32(0x564L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_o1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x564L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cb
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_O2_DEFAULT (0x200)
#define APICAL_ISP_FR_CS_CONV_COEFFT_O2_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_fr_cs_conv_coefft_o2_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x568L);
	APICAL_WRITE_32(0x568L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_o2_read(void) {
	return (uint16_t)((APICAL_READ_32(0x568L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o3
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cr
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_COEFFT_O3_DEFAULT (0x200)
#define APICAL_ISP_FR_CS_CONV_COEFFT_O3_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_fr_cs_conv_coefft_o3_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x56cL);
	APICAL_WRITE_32(0x56cL, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_fr_cs_conv_coefft_o3_read(void) {
	return (uint16_t)((APICAL_READ_32(0x56cL) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Y.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_CLIP_MIN_Y_DEFAULT (0x000)
#define APICAL_ISP_FR_CS_CONV_CLIP_MIN_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_clip_min_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x578L);
	APICAL_WRITE_32(0x578L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_clip_min_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x578L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Y.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_CLIP_MAX_Y_DEFAULT (0x3FF)
#define APICAL_ISP_FR_CS_CONV_CLIP_MAX_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_clip_max_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x57cL);
	APICAL_WRITE_32(0x57cL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_clip_max_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x57cL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Cb, Cr.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_CLIP_MIN_UV_DEFAULT (0x000)
#define APICAL_ISP_FR_CS_CONV_CLIP_MIN_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_clip_min_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x580L);
	APICAL_WRITE_32(0x580L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_clip_min_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x580L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Cb, Cr.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_CLIP_MAX_UV_DEFAULT (0x3FF)
#define APICAL_ISP_FR_CS_CONV_CLIP_MAX_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_clip_max_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x584L);
	APICAL_WRITE_32(0x584L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_clip_max_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x584L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask RY
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 1 (R or Y).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_DATA_MASK_RY_DEFAULT (0x3FF)
#define APICAL_ISP_FR_CS_CONV_DATA_MASK_RY_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_data_mask_ry_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x588L);
	APICAL_WRITE_32(0x588L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_data_mask_ry_read(void) {
	return (uint16_t)((APICAL_READ_32(0x588L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask GU
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 2 (G or U).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_DATA_MASK_GU_DEFAULT (0x3FF)
#define APICAL_ISP_FR_CS_CONV_DATA_MASK_GU_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_data_mask_gu_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x58cL);
	APICAL_WRITE_32(0x58cL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_data_mask_gu_read(void) {
	return (uint16_t)((APICAL_READ_32(0x58cL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask BV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 3 (B or V).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_CS_CONV_DATA_MASK_BV_DEFAULT (0x3FF)
#define APICAL_ISP_FR_CS_CONV_DATA_MASK_BV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_fr_cs_conv_data_mask_bv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x590L);
	APICAL_WRITE_32(0x590L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_fr_cs_conv_data_mask_bv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x590L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Dither FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable dither
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables dithering module
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DITHER_ENABLE_DITHER_DEFAULT (0x0)
#define APICAL_ISP_FR_DITHER_ENABLE_DITHER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dither_enable_dither_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x5e0L);
	APICAL_WRITE_32(0x5e0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_dither_enable_dither_read(void) {
	return (uint8_t)((APICAL_READ_32(0x5e0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dither amount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= dither to 9 bits; 1=dither to 8 bits; 2=dither to 7 bits; 3=dither to 6 bits
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DITHER_DITHER_AMOUNT_DEFAULT (0x0)
#define APICAL_ISP_FR_DITHER_DITHER_AMOUNT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_fr_dither_dither_amount_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x5e0L);
	APICAL_WRITE_32(0x5e0L, (((uint32_t) (data & 0x3)) << 1) | (curr & 0xfffffff9));
}
static __inline uint8_t apical_isp_fr_dither_dither_amount_read(void) {
	return (uint8_t)((APICAL_READ_32(0x5e0L) & 0x6) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Shift mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= output is LSB aligned; 1=output is MSB aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DITHER_SHIFT_MODE_DEFAULT (0x0)
#define APICAL_ISP_FR_DITHER_SHIFT_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dither_shift_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x5e0L);
	APICAL_WRITE_32(0x5e0L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_fr_dither_shift_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x5e0L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Group: Crop DS 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Crop for down-scale path
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Crop enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CROP_ENABLE_CROP_DEFAULT (0)
#define APICAL_ISP_DS1_CROP_ENABLE_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_crop_enable_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x600L);
	APICAL_WRITE_32(0x600L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_crop_enable_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x600L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal offset from left side of image in pixels for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CROP_START_X_DEFAULT (0x0000)
#define APICAL_ISP_DS1_CROP_START_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_crop_start_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x604L);
	APICAL_WRITE_32(0x604L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_crop_start_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x604L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical offset from top of image in lines for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CROP_START_Y_DEFAULT (0x0000)
#define APICAL_ISP_DS1_CROP_START_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_crop_start_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x608L);
	APICAL_WRITE_32(0x608L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_crop_start_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x608L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// width of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CROP_SIZE_X_DEFAULT (0xffff)
#define APICAL_ISP_DS1_CROP_SIZE_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_crop_size_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x60cL);
	APICAL_WRITE_32(0x60cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_crop_size_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x60cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// height of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CROP_SIZE_Y_DEFAULT (0xffff)
#define APICAL_ISP_DS1_CROP_SIZE_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_crop_size_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x610L);
	APICAL_WRITE_32(0x610L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_crop_size_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x610L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Sharpen DS1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Non-linear sharpening algorithm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sharpening enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SHARPEN_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS1_SHARPEN_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_sharpen_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x620L);
	APICAL_WRITE_32(0x620L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_sharpen_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x620L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coring
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects LUT memory bank, value 00 connects bank 0 to sharpening and bank 1 to programming, value 01 swap banks
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SHARPEN_CORING_DEFAULT (1)
#define APICAL_ISP_DS1_SHARPEN_CORING_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds1_sharpen_coring_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x620L);
	APICAL_WRITE_32(0x620L, (((uint32_t) (data & 0x3)) << 2) | (curr & 0xfffffff3));
}
static __inline uint8_t apical_isp_ds1_sharpen_coring_read(void) {
	return (uint8_t)((APICAL_READ_32(0x620L) & 0xc) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls strength of sharpening effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SHARPEN_STRENGTH_DEFAULT (0x30)
#define APICAL_ISP_DS1_SHARPEN_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_sharpen_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x624L);
	APICAL_WRITE_32(0x624L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds1_sharpen_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x624L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control R
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SHARPEN_CONTROL_R_DEFAULT (0x60)
#define APICAL_ISP_DS1_SHARPEN_CONTROL_R_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_sharpen_control_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x628L);
	APICAL_WRITE_32(0x628L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds1_sharpen_control_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x628L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control B
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SHARPEN_CONTROL_B_DEFAULT (0x40)
#define APICAL_ISP_DS1_SHARPEN_CONTROL_B_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_sharpen_control_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x628L);
	APICAL_WRITE_32(0x628L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds1_sharpen_control_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x628L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Group: Down Scaler 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: IRQSTAT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downscaler status
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_IRQSTAT_DEFAULT (0x00)
#define APICAL_ISP_DS1_SCALER_IRQSTAT_DATASIZE (8)

// args: data (8-bit)
static __inline uint8_t apical_isp_ds1_scaler_irqstat_read(void) {
	return (uint8_t)((APICAL_READ_32(0x640L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timeout IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No timeout
//             1 : Timeout on frame done
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_TIMEOUT_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS1_SCALER_TIMEOUT_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_timeout_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x640L);
	APICAL_WRITE_32(0x640L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1_scaler_timeout_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x640L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Underflow IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No underflow
//             1 : FIFO underflow has occurred
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_UNDERFLOW_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS1_SCALER_UNDERFLOW_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_underflow_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x640L);
	APICAL_WRITE_32(0x640L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds1_scaler_underflow_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x640L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Overflow IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No overflow
//             1 : FIFO overflow has occurred
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_OVERFLOW_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS1_SCALER_OVERFLOW_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_overflow_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x640L);
	APICAL_WRITE_32(0x640L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds1_scaler_overflow_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x640L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Clear Alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Scaler control
//IRQ CLR bit
// 0 : In-active
// 1 : Clear-off IRQ status to 0
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_DS1_SCALER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x644L);
	APICAL_WRITE_32(0x644L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1_scaler_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0x644L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Timeout Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
// 0 : Timeout disabled.
// 1 : Timeout enabled.  Automatic frame reset if frame has not completed after anticipated time.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_TIMEOUT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS1_SCALER_TIMEOUT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_timeout_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x644L);
	APICAL_WRITE_32(0x644L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_ds1_scaler_timeout_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x644L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Field in toggle sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
// 0 : Input Field Type = pulse.
// 1 : Input Field Type = toggle.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_FIELD_IN_TOGGLE_SEL_DEFAULT (0)
#define APICAL_ISP_DS1_SCALER_FIELD_IN_TOGGLE_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_field_in_toggle_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x644L);
	APICAL_WRITE_32(0x644L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_ds1_scaler_field_in_toggle_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x644L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: WIDTH
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Input frame width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS1_SCALER_WIDTH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds1_scaler_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x648L);
	APICAL_WRITE_32(0x648L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds1_scaler_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x648L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HEIGHT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Input frame height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS1_SCALER_HEIGHT_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds1_scaler_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x64cL);
	APICAL_WRITE_32(0x64cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds1_scaler_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x64cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: OWIDTH
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Output frame width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_OWIDTH_DEFAULT (0x500)
#define APICAL_ISP_DS1_SCALER_OWIDTH_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds1_scaler_owidth_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x650L);
	APICAL_WRITE_32(0x650L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds1_scaler_owidth_read(void) {
	return (uint16_t)((APICAL_READ_32(0x650L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: OHEIGHT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Output frame height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_OHEIGHT_DEFAULT (0x2D0)
#define APICAL_ISP_DS1_SCALER_OHEIGHT_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds1_scaler_oheight_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x654L);
	APICAL_WRITE_32(0x654L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds1_scaler_oheight_read(void) {
	return (uint16_t)((APICAL_READ_32(0x654L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HFILT_TINC
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal scaling factor equal to the
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_HFILT_TINC_DEFAULT (0x180000)
#define APICAL_ISP_DS1_SCALER_HFILT_TINC_DATASIZE (24)

// args: data (24-bit)
static __inline void apical_isp_ds1_scaler_hfilt_tinc_write(uint32_t data) {
	uint32_t curr = APICAL_READ_32(0x658L);
	APICAL_WRITE_32(0x658L, (((uint32_t) (data & 0xffffff)) << 0) | (curr & 0xff000000));
}
static __inline uint32_t apical_isp_ds1_scaler_hfilt_tinc_read(void) {
	return (uint32_t)((APICAL_READ_32(0x658L) & 0xffffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HFILT_COEFSET
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//HFILT Coeff. control.
//HFILT_COEFSET[3:0] - Selects horizontal Coef set for scaler.
// 0000 : use set 0
// 0001 : use set 1
// ......
// 1111 : use set 15
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_HFILT_COEFSET_DEFAULT (0x00)
#define APICAL_ISP_DS1_SCALER_HFILT_COEFSET_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds1_scaler_hfilt_coefset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x65cL);
	APICAL_WRITE_32(0x65cL, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds1_scaler_hfilt_coefset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x65cL) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VFILT_TINC
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// VFILT TINC
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_VFILT_TINC_DEFAULT (0x180000)
#define APICAL_ISP_DS1_SCALER_VFILT_TINC_DATASIZE (24)

// args: data (24-bit)
static __inline void apical_isp_ds1_scaler_vfilt_tinc_write(uint32_t data) {
	uint32_t curr = APICAL_READ_32(0x660L);
	APICAL_WRITE_32(0x660L, (((uint32_t) (data & 0xffffff)) << 0) | (curr & 0xff000000));
}
static __inline uint32_t apical_isp_ds1_scaler_vfilt_tinc_read(void) {
	return (uint32_t)((APICAL_READ_32(0x660L) & 0xffffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VFILT_COEFSET
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//VFILT Coeff. control
//FILT_COEFSET[3:0] - Selects vertical Coef set for scaler
//0000 : use set 0
//0001 : use set 1
//......
//1111 : use set 15
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_VFILT_COEFSET_DEFAULT (0x00)
#define APICAL_ISP_DS1_SCALER_VFILT_COEFSET_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds1_scaler_vfilt_coefset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x664L);
	APICAL_WRITE_32(0x664L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds1_scaler_vfilt_coefset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x664L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: IMGRST
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Manual frame reset control
//0 : In-active
//1 : Set to 1 for synchronous reset of Hfilter/Vfilter blocks
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_SCALER_IMGRST_DEFAULT (0x00)
#define APICAL_ISP_DS1_SCALER_IMGRST_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_scaler_imgrst_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x668L);
	APICAL_WRITE_32(0x668L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_scaler_imgrst_read(void) {
	return (uint8_t)((APICAL_READ_32(0x668L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Colour space conv DS 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Conversion of RGB to YUV data using a 3x3 color matrix plus offsets
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Color matrix enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_ENABLE_MATRIX_DEFAULT (0)
#define APICAL_ISP_DS1_CS_CONV_ENABLE_MATRIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_cs_conv_enable_matrix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6b0L);
	APICAL_WRITE_32(0x6b0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_cs_conv_enable_matrix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6b0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable filter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Filter enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_ENABLE_FILTER_DEFAULT (0)
#define APICAL_ISP_DS1_CS_CONV_ENABLE_FILTER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_cs_conv_enable_filter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6b0L);
	APICAL_WRITE_32(0x6b0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1_cs_conv_enable_filter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6b0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Enable horizontal downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_DS1_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_cs_conv_enable_horizontal_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6b0L);
	APICAL_WRITE_32(0x6b0L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds1_cs_conv_enable_horizontal_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6b0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Enable vertical downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_DS1_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_cs_conv_enable_vertical_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6b0L);
	APICAL_WRITE_32(0x6b0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1_cs_conv_enable_vertical_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6b0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_11_DEFAULT (0x002f)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_11_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x680L);
	APICAL_WRITE_32(0x680L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x680L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 12
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_12_DEFAULT (0x009d)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_12_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_12_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x684L);
	APICAL_WRITE_32(0x684L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_12_read(void) {
	return (uint16_t)((APICAL_READ_32(0x684L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 13
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_13_DEFAULT (0x0010)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_13_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_13_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x688L);
	APICAL_WRITE_32(0x688L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_13_read(void) {
	return (uint16_t)((APICAL_READ_32(0x688L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 21
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_21_DEFAULT (0x801a)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_21_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_21_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x68cL);
	APICAL_WRITE_32(0x68cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_21_read(void) {
	return (uint16_t)((APICAL_READ_32(0x68cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 22
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_22_DEFAULT (0x8057)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_22_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_22_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x690L);
	APICAL_WRITE_32(0x690L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_22_read(void) {
	return (uint16_t)((APICAL_READ_32(0x690L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 23
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_23_DEFAULT (0x0070)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_23_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_23_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x694L);
	APICAL_WRITE_32(0x694L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_23_read(void) {
	return (uint16_t)((APICAL_READ_32(0x694L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 31
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_31_DEFAULT (0x0070)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_31_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_31_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x698L);
	APICAL_WRITE_32(0x698L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_31_read(void) {
	return (uint16_t)((APICAL_READ_32(0x698L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 32
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_32_DEFAULT (0x8066)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_32_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_32_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x69cL);
	APICAL_WRITE_32(0x69cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_32_read(void) {
	return (uint16_t)((APICAL_READ_32(0x69cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 33
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_33_DEFAULT (0x800a)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_33_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_33_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6a0L);
	APICAL_WRITE_32(0x6a0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_33_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6a0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Y
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_O1_DEFAULT (0x000)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_O1_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_o1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6a4L);
	APICAL_WRITE_32(0x6a4L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_o1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6a4L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cb
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_O2_DEFAULT (0x200)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_O2_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_o2_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6a8L);
	APICAL_WRITE_32(0x6a8L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_o2_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6a8L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o3
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cr
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_COEFFT_O3_DEFAULT (0x200)
#define APICAL_ISP_DS1_CS_CONV_COEFFT_O3_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds1_cs_conv_coefft_o3_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6acL);
	APICAL_WRITE_32(0x6acL, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds1_cs_conv_coefft_o3_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6acL) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Y.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_CLIP_MIN_Y_DEFAULT (0x000)
#define APICAL_ISP_DS1_CS_CONV_CLIP_MIN_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_clip_min_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6b8L);
	APICAL_WRITE_32(0x6b8L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_clip_min_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6b8L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Y.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_CLIP_MAX_Y_DEFAULT (0x3FF)
#define APICAL_ISP_DS1_CS_CONV_CLIP_MAX_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_clip_max_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6bcL);
	APICAL_WRITE_32(0x6bcL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_clip_max_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6bcL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Cb, Cr.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_CLIP_MIN_UV_DEFAULT (0x000)
#define APICAL_ISP_DS1_CS_CONV_CLIP_MIN_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_clip_min_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6c0L);
	APICAL_WRITE_32(0x6c0L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_clip_min_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6c0L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Cb, Cr.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_CLIP_MAX_UV_DEFAULT (0x3FF)
#define APICAL_ISP_DS1_CS_CONV_CLIP_MAX_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_clip_max_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6c4L);
	APICAL_WRITE_32(0x6c4L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_clip_max_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6c4L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask RY
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 1 (R or Y).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_RY_DEFAULT (0x3FF)
#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_RY_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_data_mask_ry_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6c8L);
	APICAL_WRITE_32(0x6c8L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_data_mask_ry_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6c8L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask GU
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 2 (G or U).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_GU_DEFAULT (0x3FF)
#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_GU_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_data_mask_gu_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6ccL);
	APICAL_WRITE_32(0x6ccL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_data_mask_gu_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6ccL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask BV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 3 (B or V).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_BV_DEFAULT (0x3FF)
#define APICAL_ISP_DS1_CS_CONV_DATA_MASK_BV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds1_cs_conv_data_mask_bv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x6d0L);
	APICAL_WRITE_32(0x6d0L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds1_cs_conv_data_mask_bv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x6d0L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Dither DS 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable dither
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables dithering module
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DITHER_ENABLE_DITHER_DEFAULT (0x0)
#define APICAL_ISP_DS1_DITHER_ENABLE_DITHER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dither_enable_dither_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6e0L);
	APICAL_WRITE_32(0x6e0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_dither_enable_dither_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6e0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dither amount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= dither to 9 bits; 1=dither to 8 bits; 2=dither to 7 bits; 3=dither to 6 bits
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DITHER_DITHER_AMOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DITHER_DITHER_AMOUNT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds1_dither_dither_amount_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6e0L);
	APICAL_WRITE_32(0x6e0L, (((uint32_t) (data & 0x3)) << 1) | (curr & 0xfffffff9));
}
static __inline uint8_t apical_isp_ds1_dither_dither_amount_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6e0L) & 0x6) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Shift mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= output is LSB aligned; 1=output is MSB aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DITHER_SHIFT_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DITHER_SHIFT_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dither_shift_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x6e0L);
	APICAL_WRITE_32(0x6e0L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_ds1_dither_shift_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x6e0L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Group: Crop DS 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  Crop for down-scale path
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable crop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Crop enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CROP_ENABLE_CROP_DEFAULT (0)
#define APICAL_ISP_DS2_CROP_ENABLE_CROP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_crop_enable_crop_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x700L);
	APICAL_WRITE_32(0x700L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_crop_enable_crop_read(void) {
	return (uint8_t)((APICAL_READ_32(0x700L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal offset from left side of image in pixels for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CROP_START_X_DEFAULT (0x0000)
#define APICAL_ISP_DS2_CROP_START_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_crop_start_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x704L);
	APICAL_WRITE_32(0x704L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_crop_start_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x704L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: start y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Vertical offset from top of image in lines for output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CROP_START_Y_DEFAULT (0x0000)
#define APICAL_ISP_DS2_CROP_START_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_crop_start_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x708L);
	APICAL_WRITE_32(0x708L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_crop_start_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x708L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// width of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CROP_SIZE_X_DEFAULT (0xffff)
#define APICAL_ISP_DS2_CROP_SIZE_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_crop_size_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x70cL);
	APICAL_WRITE_32(0x70cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_crop_size_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x70cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: size y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// height of output crop window
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CROP_SIZE_Y_DEFAULT (0xffff)
#define APICAL_ISP_DS2_CROP_SIZE_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_crop_size_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x710L);
	APICAL_WRITE_32(0x710L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_crop_size_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x710L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Sharpen DS2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Non-linear sharpening algorithm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sharpening enable: 0=off, 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SHARPEN_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS2_SHARPEN_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_sharpen_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x720L);
	APICAL_WRITE_32(0x720L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_sharpen_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x720L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coring
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Selects LUT memory bank, value 00 connects bank 0 to sharpening and bank 1 to programming, value 01 swap banks
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SHARPEN_CORING_DEFAULT (1)
#define APICAL_ISP_DS2_SHARPEN_CORING_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds2_sharpen_coring_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x720L);
	APICAL_WRITE_32(0x720L, (((uint32_t) (data & 0x3)) << 2) | (curr & 0xfffffff3));
}
static __inline uint8_t apical_isp_ds2_sharpen_coring_read(void) {
	return (uint8_t)((APICAL_READ_32(0x720L) & 0xc) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Strength
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls strength of sharpening effect
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SHARPEN_STRENGTH_DEFAULT (0x30)
#define APICAL_ISP_DS2_SHARPEN_STRENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_sharpen_strength_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x724L);
	APICAL_WRITE_32(0x724L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds2_sharpen_strength_read(void) {
	return (uint8_t)((APICAL_READ_32(0x724L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control R
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SHARPEN_CONTROL_R_DEFAULT (0x60)
#define APICAL_ISP_DS2_SHARPEN_CONTROL_R_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_sharpen_control_r_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x728L);
	APICAL_WRITE_32(0x728L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds2_sharpen_control_r_read(void) {
	return (uint8_t)((APICAL_READ_32(0x728L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Control B
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SHARPEN_CONTROL_B_DEFAULT (0x40)
#define APICAL_ISP_DS2_SHARPEN_CONTROL_B_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_sharpen_control_b_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x728L);
	APICAL_WRITE_32(0x728L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds2_sharpen_control_b_read(void) {
	return (uint8_t)((APICAL_READ_32(0x728L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Group: Down Scaler 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: IRQSTAT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downscaler status
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_IRQSTAT_DEFAULT (0x00)
#define APICAL_ISP_DS2_SCALER_IRQSTAT_DATASIZE (8)

// args: data (8-bit)
static __inline uint8_t apical_isp_ds2_scaler_irqstat_read(void) {
	return (uint8_t)((APICAL_READ_32(0x740L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Timeout IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No timeout
//             1 : Timeout on frame done
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_TIMEOUT_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS2_SCALER_TIMEOUT_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_timeout_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x740L);
	APICAL_WRITE_32(0x740L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2_scaler_timeout_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x740L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Underflow IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No underflow
//             1 : FIFO underflow has occurred
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_UNDERFLOW_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS2_SCALER_UNDERFLOW_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_underflow_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x740L);
	APICAL_WRITE_32(0x740L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds2_scaler_underflow_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x740L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Overflow IRQ
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//             0 : No overflow
//             1 : FIFO overflow has occurred
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_OVERFLOW_IRQ_DEFAULT (0x0)
#define APICAL_ISP_DS2_SCALER_OVERFLOW_IRQ_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_overflow_irq_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x740L);
	APICAL_WRITE_32(0x740L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds2_scaler_overflow_irq_read(void) {
	return (uint8_t)((APICAL_READ_32(0x740L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Timeout Enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
// 0 : Timeout disabled.
// 1 : Timeout enabled.  Automatic frame reset if frame has not completed after anticipated time.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_TIMEOUT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS2_SCALER_TIMEOUT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_timeout_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x744L);
	APICAL_WRITE_32(0x744L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_ds2_scaler_timeout_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0x744L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Field in toggle sel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
// 0 : Input Field Type = pulse.
// 1 : Input Field Type = toggle.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_FIELD_IN_TOGGLE_SEL_DEFAULT (0)
#define APICAL_ISP_DS2_SCALER_FIELD_IN_TOGGLE_SEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_field_in_toggle_sel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x744L);
	APICAL_WRITE_32(0x744L, (((uint32_t) (data & 0x1)) << 5) | (curr & 0xffffffdf));
}
static __inline uint8_t apical_isp_ds2_scaler_field_in_toggle_sel_read(void) {
	return (uint8_t)((APICAL_READ_32(0x744L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: WIDTH
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Input frame width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS2_SCALER_WIDTH_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds2_scaler_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x748L);
	APICAL_WRITE_32(0x748L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds2_scaler_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x748L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HEIGHT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Input frame height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS2_SCALER_HEIGHT_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds2_scaler_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x74cL);
	APICAL_WRITE_32(0x74cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds2_scaler_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x74cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: OWIDTH
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Output frame width in pixels
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_OWIDTH_DEFAULT (0x500)
#define APICAL_ISP_DS2_SCALER_OWIDTH_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds2_scaler_owidth_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x750L);
	APICAL_WRITE_32(0x750L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds2_scaler_owidth_read(void) {
	return (uint16_t)((APICAL_READ_32(0x750L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: OHEIGHT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Output frame height in lines
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_OHEIGHT_DEFAULT (0x2D0)
#define APICAL_ISP_DS2_SCALER_OHEIGHT_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_ds2_scaler_oheight_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x754L);
	APICAL_WRITE_32(0x754L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_ds2_scaler_oheight_read(void) {
	return (uint16_t)((APICAL_READ_32(0x754L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HFILT_TINC
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Horizontal scaling factor equal to the
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_HFILT_TINC_DEFAULT (0x180000)
#define APICAL_ISP_DS2_SCALER_HFILT_TINC_DATASIZE (24)

// args: data (24-bit)
static __inline void apical_isp_ds2_scaler_hfilt_tinc_write(uint32_t data) {
	uint32_t curr = APICAL_READ_32(0x758L);
	APICAL_WRITE_32(0x758L, (((uint32_t) (data & 0xffffff)) << 0) | (curr & 0xff000000));
}
static __inline uint32_t apical_isp_ds2_scaler_hfilt_tinc_read(void) {
	return (uint32_t)((APICAL_READ_32(0x758L) & 0xffffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: HFILT_COEFSET
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//HFILT Coeff. control.
//HFILT_COEFSET[3:0] - Selects horizontal Coef set for scaler.
// 0000 : use set 0
// 0001 : use set 1
// ......
// 1111 : use set 15
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_HFILT_COEFSET_DEFAULT (0x00)
#define APICAL_ISP_DS2_SCALER_HFILT_COEFSET_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds2_scaler_hfilt_coefset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x75cL);
	APICAL_WRITE_32(0x75cL, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds2_scaler_hfilt_coefset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x75cL) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VFILT_TINC
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// VFILT TINC
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_VFILT_TINC_DEFAULT (0x180000)
#define APICAL_ISP_DS2_SCALER_VFILT_TINC_DATASIZE (24)

// args: data (24-bit)
static __inline void apical_isp_ds2_scaler_vfilt_tinc_write(uint32_t data) {
	uint32_t curr = APICAL_READ_32(0x760L);
	APICAL_WRITE_32(0x760L, (((uint32_t) (data & 0xffffff)) << 0) | (curr & 0xff000000));
}
static __inline uint32_t apical_isp_ds2_scaler_vfilt_tinc_read(void) {
	return (uint32_t)((APICAL_READ_32(0x760L) & 0xffffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: VFILT_COEFSET
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//VFILT Coeff. control
//FILT_COEFSET[3:0] - Selects vertical Coef set for scaler
//0000 : use set 0
//0001 : use set 1
//......
//1111 : use set 15
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_VFILT_COEFSET_DEFAULT (0x00)
#define APICAL_ISP_DS2_SCALER_VFILT_COEFSET_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds2_scaler_vfilt_coefset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x764L);
	APICAL_WRITE_32(0x764L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds2_scaler_vfilt_coefset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x764L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: IMGRST
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//Manual frame reset control
//0 : In-active
//1 : Set to 1 for synchronous reset of Hfilter/Vfilter blocks
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_SCALER_IMGRST_DEFAULT (0x00)
#define APICAL_ISP_DS2_SCALER_IMGRST_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_scaler_imgrst_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x768L);
	APICAL_WRITE_32(0x768L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_scaler_imgrst_read(void) {
	return (uint8_t)((APICAL_READ_32(0x768L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Colour space conv DS 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Conversion of RGB to YUV data using a 3x3 color matrix plus offsets
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable matrix
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Color matrix enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_ENABLE_MATRIX_DEFAULT (0)
#define APICAL_ISP_DS2_CS_CONV_ENABLE_MATRIX_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_cs_conv_enable_matrix_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7b0L);
	APICAL_WRITE_32(0x7b0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_cs_conv_enable_matrix_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7b0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Enable filter
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Filter enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_ENABLE_FILTER_DEFAULT (0)
#define APICAL_ISP_DS2_CS_CONV_ENABLE_FILTER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_cs_conv_enable_filter_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7b0L);
	APICAL_WRITE_32(0x7b0L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2_cs_conv_enable_filter_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7b0L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Enable horizontal downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_DS2_CS_CONV_ENABLE_HORIZONTAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_cs_conv_enable_horizontal_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7b0L);
	APICAL_WRITE_32(0x7b0L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds2_cs_conv_enable_horizontal_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7b0L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Enable vertical downsample
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Downsample enable: 0=off 1=on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DEFAULT (0)
#define APICAL_ISP_DS2_CS_CONV_ENABLE_VERTICAL_DOWNSAMPLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_cs_conv_enable_vertical_downsample_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7b0L);
	APICAL_WRITE_32(0x7b0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2_cs_conv_enable_vertical_downsample_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7b0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 11
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_11_DEFAULT (0x002f)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_11_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_11_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x780L);
	APICAL_WRITE_32(0x780L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_11_read(void) {
	return (uint16_t)((APICAL_READ_32(0x780L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 12
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_12_DEFAULT (0x009d)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_12_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_12_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x784L);
	APICAL_WRITE_32(0x784L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_12_read(void) {
	return (uint16_t)((APICAL_READ_32(0x784L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 13
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Y multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_13_DEFAULT (0x0010)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_13_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_13_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x788L);
	APICAL_WRITE_32(0x788L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_13_read(void) {
	return (uint16_t)((APICAL_READ_32(0x788L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 21
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_21_DEFAULT (0x801a)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_21_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_21_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x78cL);
	APICAL_WRITE_32(0x78cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_21_read(void) {
	return (uint16_t)((APICAL_READ_32(0x78cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 22
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_22_DEFAULT (0x8057)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_22_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_22_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x790L);
	APICAL_WRITE_32(0x790L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_22_read(void) {
	return (uint16_t)((APICAL_READ_32(0x790L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 23
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cb multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_23_DEFAULT (0x0070)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_23_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_23_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x794L);
	APICAL_WRITE_32(0x794L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_23_read(void) {
	return (uint16_t)((APICAL_READ_32(0x794L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 31
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for R-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_31_DEFAULT (0x0070)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_31_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_31_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x798L);
	APICAL_WRITE_32(0x798L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_31_read(void) {
	return (uint16_t)((APICAL_READ_32(0x798L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 32
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for G-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_32_DEFAULT (0x8066)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_32_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_32_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x79cL);
	APICAL_WRITE_32(0x79cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_32_read(void) {
	return (uint16_t)((APICAL_READ_32(0x79cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft 33
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Matrix coefficient for B-Cr multiplier
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_33_DEFAULT (0x800a)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_33_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_33_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7a0L);
	APICAL_WRITE_32(0x7a0L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_33_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7a0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Y
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_O1_DEFAULT (0x000)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_O1_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_o1_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7a4L);
	APICAL_WRITE_32(0x7a4L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_o1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7a4L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cb
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_O2_DEFAULT (0x200)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_O2_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_o2_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7a8L);
	APICAL_WRITE_32(0x7a8L, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_o2_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7a8L) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Coefft o3
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Offset for Cr
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_COEFFT_O3_DEFAULT (0x200)
#define APICAL_ISP_DS2_CS_CONV_COEFFT_O3_DATASIZE (11)

// args: data (11-bit)
static __inline void apical_isp_ds2_cs_conv_coefft_o3_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7acL);
	APICAL_WRITE_32(0x7acL, (((uint32_t) (data & 0x7ff)) << 0) | (curr & 0xfffff800));
}
static __inline uint16_t apical_isp_ds2_cs_conv_coefft_o3_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7acL) & 0x7ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Y.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_CLIP_MIN_Y_DEFAULT (0x000)
#define APICAL_ISP_DS2_CS_CONV_CLIP_MIN_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_clip_min_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7b8L);
	APICAL_WRITE_32(0x7b8L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_clip_min_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7b8L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max Y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Y.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_CLIP_MAX_Y_DEFAULT (0x3FF)
#define APICAL_ISP_DS2_CS_CONV_CLIP_MAX_Y_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_clip_max_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7bcL);
	APICAL_WRITE_32(0x7bcL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_clip_max_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7bcL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip min UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimal value for Cb, Cr.  Values below this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_CLIP_MIN_UV_DEFAULT (0x000)
#define APICAL_ISP_DS2_CS_CONV_CLIP_MIN_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_clip_min_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7c0L);
	APICAL_WRITE_32(0x7c0L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_clip_min_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7c0L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clip max UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximal value for Cb, Cr.  Values above this are clipped.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_CLIP_MAX_UV_DEFAULT (0x3FF)
#define APICAL_ISP_DS2_CS_CONV_CLIP_MAX_UV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_clip_max_uv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7c4L);
	APICAL_WRITE_32(0x7c4L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_clip_max_uv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7c4L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask RY
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 1 (R or Y).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_RY_DEFAULT (0x3FF)
#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_RY_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_data_mask_ry_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7c8L);
	APICAL_WRITE_32(0x7c8L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_data_mask_ry_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7c8L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask GU
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 2 (G or U).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_GU_DEFAULT (0x3FF)
#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_GU_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_data_mask_gu_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7ccL);
	APICAL_WRITE_32(0x7ccL, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_data_mask_gu_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7ccL) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Data mask BV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Data mask for channel 3 (B or V).  Bit-wise and of this value and video data.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_BV_DEFAULT (0x3FF)
#define APICAL_ISP_DS2_CS_CONV_DATA_MASK_BV_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_ds2_cs_conv_data_mask_bv_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x7d0L);
	APICAL_WRITE_32(0x7d0L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_ds2_cs_conv_data_mask_bv_read(void) {
	return (uint16_t)((APICAL_READ_32(0x7d0L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Dither DS 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Enable dither
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enables dithering module
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DITHER_ENABLE_DITHER_DEFAULT (0x0)
#define APICAL_ISP_DS2_DITHER_ENABLE_DITHER_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dither_enable_dither_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7e0L);
	APICAL_WRITE_32(0x7e0L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_dither_enable_dither_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7e0L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dither amount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= dither to 9 bits; 1=dither to 8 bits; 2=dither to 7 bits; 3=dither to 6 bits
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DITHER_DITHER_AMOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DITHER_DITHER_AMOUNT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds2_dither_dither_amount_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7e0L);
	APICAL_WRITE_32(0x7e0L, (((uint32_t) (data & 0x3)) << 1) | (curr & 0xfffffff9));
}
static __inline uint8_t apical_isp_ds2_dither_dither_amount_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7e0L) & 0x6) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Shift mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= output is LSB aligned; 1=output is MSB aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DITHER_SHIFT_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DITHER_SHIFT_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dither_shift_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x7e0L);
	APICAL_WRITE_32(0x7e0L, (((uint32_t) (data & 0x1)) << 4) | (curr & 0xffffffef));
}
static __inline uint8_t apical_isp_ds2_dither_shift_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x7e0L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Group: Statistics
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Derives information for use by the AE and AWB modules
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Hist Thresh 0 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram threshold for bin 0/1 boundary
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_THRESH_0_1_DEFAULT (0x10)
#define APICAL_ISP_METERING_HIST_THRESH_0_1_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_hist_thresh_0_1_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x800L);
	APICAL_WRITE_32(0x800L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_hist_thresh_0_1_read(void) {
	return (uint8_t)((APICAL_READ_32(0x800L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist Thresh 1 2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram threshold for bin 1/2 boundary
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_THRESH_1_2_DEFAULT (0x20)
#define APICAL_ISP_METERING_HIST_THRESH_1_2_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_hist_thresh_1_2_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x804L);
	APICAL_WRITE_32(0x804L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_hist_thresh_1_2_read(void) {
	return (uint8_t)((APICAL_READ_32(0x804L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist Thresh 3 4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram threshold for bin 2/3 boundary
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_THRESH_3_4_DEFAULT (0xD0)
#define APICAL_ISP_METERING_HIST_THRESH_3_4_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_hist_thresh_3_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x808L);
	APICAL_WRITE_32(0x808L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_hist_thresh_3_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0x808L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist Thresh 4 5
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram threshold for bin 3/4 boundary
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_THRESH_4_5_DEFAULT (0xE0)
#define APICAL_ISP_METERING_HIST_THRESH_4_5_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_hist_thresh_4_5_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x80cL);
	APICAL_WRITE_32(0x80cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_hist_thresh_4_5_read(void) {
	return (uint8_t)((APICAL_READ_32(0x80cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist 0
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Normalized histogram results for bin 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_0_DEFAULT (0x0)
#define APICAL_ISP_METERING_HIST_0_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_hist_0_read(void) {
	return (uint16_t)((APICAL_READ_32(0x820L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist 1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Normalized histogram results for bin 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_1_DEFAULT (0x0)
#define APICAL_ISP_METERING_HIST_1_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_hist_1_read(void) {
	return (uint16_t)((APICAL_READ_32(0x824L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist 3
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Normalized histogram results for bin 3
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_3_DEFAULT (0x0)
#define APICAL_ISP_METERING_HIST_3_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_hist_3_read(void) {
	return (uint16_t)((APICAL_READ_32(0x828L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Hist 4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Normalized histogram results for bin 4
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_HIST_4_DEFAULT (0x0)
#define APICAL_ISP_METERING_HIST_4_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_hist_4_read(void) {
	return (uint16_t)((APICAL_READ_32(0x82cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AEXP Nodes Used Horiz
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones horizontally for AE stats collection
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AEXP_NODES_USED_HORIZ_DEFAULT (15)
#define APICAL_ISP_METERING_AEXP_NODES_USED_HORIZ_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_aexp_nodes_used_horiz_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x830L);
	APICAL_WRITE_32(0x830L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_aexp_nodes_used_horiz_read(void) {
	return (uint8_t)((APICAL_READ_32(0x830L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AEXP Nodes Used Vert
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones vertically for AE stats collection
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AEXP_NODES_USED_VERT_DEFAULT (15)
#define APICAL_ISP_METERING_AEXP_NODES_USED_VERT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_aexp_nodes_used_vert_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x830L);
	APICAL_WRITE_32(0x830L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_metering_aexp_nodes_used_vert_read(void) {
	return (uint8_t)((APICAL_READ_32(0x830L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: AWB stats mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Statistics mode: 0 - legacy(G/R,B/R), 1 - current (R/G, B/G)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_STATS_MODE_DEFAULT (0)
#define APICAL_ISP_METERING_AWB_STATS_MODE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_metering_awb_stats_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x868L);
	APICAL_WRITE_32(0x868L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_metering_awb_stats_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x868L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: White Level AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Upper limit of valid data for AWB
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_WHITE_LEVEL_AWB_DEFAULT (0x3FF)
#define APICAL_ISP_METERING_WHITE_LEVEL_AWB_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_metering_white_level_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x840L);
	APICAL_WRITE_32(0x840L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_metering_white_level_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x840L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Black Level AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Lower limit of valid data for AWB
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_BLACK_LEVEL_AWB_DEFAULT (0x000)
#define APICAL_ISP_METERING_BLACK_LEVEL_AWB_DATASIZE (10)

// args: data (10-bit)
static __inline void apical_isp_metering_black_level_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x844L);
	APICAL_WRITE_32(0x844L, (((uint32_t) (data & 0x3ff)) << 0) | (curr & 0xfffffc00));
}
static __inline uint16_t apical_isp_metering_black_level_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x844L) & 0x3ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cr Ref Max AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum value of R/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CR_REF_MAX_AWB_DEFAULT (0x1FF)
#define APICAL_ISP_METERING_CR_REF_MAX_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cr_ref_max_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x848L);
	APICAL_WRITE_32(0x848L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cr_ref_max_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x848L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cr Ref Min AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum value of R/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CR_REF_MIN_AWB_DEFAULT (0x040)
#define APICAL_ISP_METERING_CR_REF_MIN_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cr_ref_min_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x84cL);
	APICAL_WRITE_32(0x84cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cr_ref_min_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x84cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cb Ref Max AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum value of B/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CB_REF_MAX_AWB_DEFAULT (0x1FF)
#define APICAL_ISP_METERING_CB_REF_MAX_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cb_ref_max_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x850L);
	APICAL_WRITE_32(0x850L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cb_ref_max_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x850L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cb Ref Min AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum value of B/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CB_REF_MIN_AWB_DEFAULT (0x040)
#define APICAL_ISP_METERING_CB_REF_MIN_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cb_ref_min_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x854L);
	APICAL_WRITE_32(0x854L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cb_ref_min_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x854L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB RG
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AWB statistics R/G color ratio output
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_RG_DEFAULT (0x0)
#define APICAL_ISP_METERING_AWB_RG_DATASIZE (12)

// args: data (12-bit)
static __inline uint16_t apical_isp_metering_awb_rg_read(void) {
	return (uint16_t)((APICAL_READ_32(0x858L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB BG
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AWB statistics B/G color ratio output
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_BG_DEFAULT (0x0)
#define APICAL_ISP_METERING_AWB_BG_DATASIZE (12)

// args: data (12-bit)
static __inline uint16_t apical_isp_metering_awb_bg_read(void) {
	return (uint16_t)((APICAL_READ_32(0x85cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB SUM
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AWB output population.  Number of pixels used for AWB statistics
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_SUM_DEFAULT (0x0)
#define APICAL_ISP_METERING_AWB_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_metering_awb_sum_read(void) {
	return APICAL_READ_32(0x860L);
}
// ------------------------------------------------------------------------------ //
// Register: AWB Nodes Used Horiz
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones horizontally for AWB stats
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_NODES_USED_HORIZ_DEFAULT (15)
#define APICAL_ISP_METERING_AWB_NODES_USED_HORIZ_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_awb_nodes_used_horiz_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x870L);
	APICAL_WRITE_32(0x870L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_awb_nodes_used_horiz_read(void) {
	return (uint8_t)((APICAL_READ_32(0x870L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AWB Nodes Used Vert
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones vertically for AWB stats
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AWB_NODES_USED_VERT_DEFAULT (15)
#define APICAL_ISP_METERING_AWB_NODES_USED_VERT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_awb_nodes_used_vert_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x870L);
	APICAL_WRITE_32(0x870L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_metering_awb_nodes_used_vert_read(void) {
	return (uint8_t)((APICAL_READ_32(0x870L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Cr Ref High AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum value of R/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CR_REF_HIGH_AWB_DEFAULT (0xFFF)
#define APICAL_ISP_METERING_CR_REF_HIGH_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cr_ref_high_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x8d0L);
	APICAL_WRITE_32(0x8d0L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cr_ref_high_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x8d0L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cr Ref Low AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum value of R/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CR_REF_LOW_AWB_DEFAULT (0x000)
#define APICAL_ISP_METERING_CR_REF_LOW_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cr_ref_low_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x8d4L);
	APICAL_WRITE_32(0x8d4L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cr_ref_low_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x8d4L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cb Ref High AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Maximum value of B/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CB_REF_HIGH_AWB_DEFAULT (0xFFF)
#define APICAL_ISP_METERING_CB_REF_HIGH_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cb_ref_high_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x8d8L);
	APICAL_WRITE_32(0x8d8L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cb_ref_high_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x8d8L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Cb Ref Low AWB
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Minimum value of B/G for white region
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_CB_REF_LOW_AWB_DEFAULT (0x000)
#define APICAL_ISP_METERING_CB_REF_LOW_AWB_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_metering_cb_ref_low_awb_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x8dcL);
	APICAL_WRITE_32(0x8dcL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_metering_cb_ref_low_awb_read(void) {
	return (uint16_t)((APICAL_READ_32(0x8dcL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF metrics shift
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Metrics scaling factor, 0x03 is default.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_METRICS_SHIFT_DEFAULT (0x3)
#define APICAL_ISP_METERING_AF_METRICS_SHIFT_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_metering_af_metrics_shift_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x88cL);
	APICAL_WRITE_32(0x88cL, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_metering_af_metrics_shift_read(void) {
	return (uint8_t)((APICAL_READ_32(0x88cL) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF metrics
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The integrated and normalized measure of contrast
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_METRICS_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_METRICS_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_af_metrics_read(void) {
	return (uint16_t)((APICAL_READ_32(0x880L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF metrics alt
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The integrated and normalized measure of contrast - with alternative threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_METRICS_ALT_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_METRICS_ALT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_af_metrics_alt_read(void) {
	return (uint16_t)((APICAL_READ_32(0x880L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: AF threshold write
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The suggested value of AF threshold (or 0 to use internallly calculated value)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_THRESHOLD_WRITE_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_THRESHOLD_WRITE_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_metering_af_threshold_write_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x884L);
	APICAL_WRITE_32(0x884L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_metering_af_threshold_write_read(void) {
	return (uint16_t)((APICAL_READ_32(0x884L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF threshold alt write
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The suggested value of alternative AF threshold (or 0 to use threshold from previous frame)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_THRESHOLD_ALT_WRITE_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_THRESHOLD_ALT_WRITE_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_metering_af_threshold_alt_write_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x884L);
	APICAL_WRITE_32(0x884L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_metering_af_threshold_alt_write_read(void) {
	return (uint16_t)((APICAL_READ_32(0x884L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: AF threshold read
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The calculated value of AF threshold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_THRESHOLD_READ_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_THRESHOLD_READ_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_af_threshold_read_read(void) {
	return (uint16_t)((APICAL_READ_32(0x888L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF intensity read
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The calculated value of AF intensity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_INTENSITY_READ_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_INTENSITY_READ_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_af_intensity_read_read(void) {
	return (uint16_t)((APICAL_READ_32(0x888L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: AF intensity zone read
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// The calculated value of AF intensity
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_INTENSITY_ZONE_READ_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_INTENSITY_ZONE_READ_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_metering_af_intensity_zone_read_read(void) {
	return (uint16_t)((APICAL_READ_32(0x89cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF Nodes Used Horiz
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones horizontally for AF stats
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_NODES_USED_HORIZ_DEFAULT (15)
#define APICAL_ISP_METERING_AF_NODES_USED_HORIZ_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_af_nodes_used_horiz_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x890L);
	APICAL_WRITE_32(0x890L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_af_nodes_used_horiz_read(void) {
	return (uint8_t)((APICAL_READ_32(0x890L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF Nodes Used Vert
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of active zones vertically for AF stats
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_NODES_USED_VERT_DEFAULT (15)
#define APICAL_ISP_METERING_AF_NODES_USED_VERT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_af_nodes_used_vert_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x890L);
	APICAL_WRITE_32(0x890L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_metering_af_nodes_used_vert_read(void) {
	return (uint8_t)((APICAL_READ_32(0x890L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: AF NP offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AF noise profile offset
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_NP_OFFSET_DEFAULT (0x9f)
#define APICAL_ISP_METERING_AF_NP_OFFSET_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_metering_af_np_offset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x894L);
	APICAL_WRITE_32(0x894L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_metering_af_np_offset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x894L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: AF intensity norm mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// AF intensity normalization mode
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_AF_INTENSITY_NORM_MODE_DEFAULT (0x0)
#define APICAL_ISP_METERING_AF_INTENSITY_NORM_MODE_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_metering_af_intensity_norm_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x898L);
	APICAL_WRITE_32(0x898L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_metering_af_intensity_norm_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0x898L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: skip x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram decimation in horizontal direction: 0=every 2nd pixel; 1=every 3rd pixel; 2=every 4th pixel; 3=every 5th pixel; 4=every 8th pixel ; 5+=every 9th pixel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_SKIP_X_DEFAULT (0)
#define APICAL_ISP_METERING_SKIP_X_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_metering_skip_x_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c0L);
	APICAL_WRITE_32(0x8c0L, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_metering_skip_x_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c0L) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: skip y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Histogram decimation in vertical direction: 0=every pixel; 1=every 2nd pixel; 2=every 3rd pixel; 3=every 4th pixel; 4=every 5th pixel; 5=every 8th pixel ; 6+=every 9th pixel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_SKIP_Y_DEFAULT (0)
#define APICAL_ISP_METERING_SKIP_Y_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_metering_skip_y_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c0L);
	APICAL_WRITE_32(0x8c0L, (((uint32_t) (data & 0x7)) << 4) | (curr & 0xffffff8f));
}
static __inline uint8_t apical_isp_metering_skip_y_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c0L) & 0x70) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: offset x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= start from the first column;  1=start from second column
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_OFFSET_X_DEFAULT (0)
#define APICAL_ISP_METERING_OFFSET_X_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_metering_offset_x_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c0L);
	APICAL_WRITE_32(0x8c0L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_metering_offset_x_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c0L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: offset y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0= start from the first row; 1= start from second row
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_OFFSET_Y_DEFAULT (0)
#define APICAL_ISP_METERING_OFFSET_Y_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_metering_offset_y_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c0L);
	APICAL_WRITE_32(0x8c0L, (((uint32_t) (data & 0x1)) << 7) | (curr & 0xffffff7f));
}
static __inline uint8_t apical_isp_metering_offset_y_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c0L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: scale bottom
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// scale of bottom half of the range: 0=1x ,1=2x, 2=4x, 4=8x, 4=16x
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_SCALE_BOTTOM_DEFAULT (0)
#define APICAL_ISP_METERING_SCALE_BOTTOM_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_metering_scale_bottom_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c4L);
	APICAL_WRITE_32(0x8c4L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_metering_scale_bottom_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c4L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: scale top
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// scale of top half of the range: 0=1x ,1=2x, 2=4x, 4=8x, 4=16x
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_SCALE_TOP_DEFAULT (0)
#define APICAL_ISP_METERING_SCALE_TOP_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_metering_scale_top_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x8c4L);
	APICAL_WRITE_32(0x8c4L, (((uint32_t) (data & 0xf)) << 4) | (curr & 0xffffff0f));
}
static __inline uint8_t apical_isp_metering_scale_top_read(void) {
	return (uint8_t)((APICAL_READ_32(0x8c4L) & 0xf0) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: Total Pixels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Total number of pixels processed (skip x and skip y are taken into account)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_TOTAL_PIXELS_DEFAULT (0)
#define APICAL_ISP_METERING_TOTAL_PIXELS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_metering_total_pixels_read(void) {
	return APICAL_READ_32(0x8c8L);
}
// ------------------------------------------------------------------------------ //
// Register: Counted Pixels
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of pixels accumulated (with nonzero weight)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_METERING_COUNTED_PIXELS_DEFAULT (0)
#define APICAL_ISP_METERING_COUNTED_PIXELS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_metering_counted_pixels_read(void) {
	return APICAL_READ_32(0x8ccL);
}
// ------------------------------------------------------------------------------ //
// Group: MVE
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frame Active width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_MVE_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_mve_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x900L);
	APICAL_WRITE_32(0x900L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_mve_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0x900L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Frame Active height
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_MVE_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_mve_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x904L);
	APICAL_WRITE_32(0x904L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_mve_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0x904L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Zone cols
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of zone clumns. Should be Zero
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ZONE_COLS_DEFAULT (0x4)
#define APICAL_ISP_MVE_ZONE_COLS_DATASIZE (5)

// args: data (5-bit)
static __inline void apical_isp_mve_zone_cols_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x908L);
	APICAL_WRITE_32(0x908L, (((uint32_t) (data & 0x1f)) << 0) | (curr & 0xffffffe0));
}
static __inline uint8_t apical_isp_mve_zone_cols_read(void) {
	return (uint8_t)((APICAL_READ_32(0x908L) & 0x1f) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Zone rows
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of zone rows
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ZONE_ROWS_DEFAULT (0x08)
#define APICAL_ISP_MVE_ZONE_ROWS_DATASIZE (5)

// args: data (5-bit)
static __inline void apical_isp_mve_zone_rows_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x908L);
	APICAL_WRITE_32(0x908L, (((uint32_t) (data & 0x1f)) << 8) | (curr & 0xffffe0ff));
}
static __inline uint8_t apical_isp_mve_zone_rows_read(void) {
	return (uint8_t)((APICAL_READ_32(0x908L) & 0x1f00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: Border x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of pixels for left border
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_BORDER_X_DEFAULT (0x010)
#define APICAL_ISP_MVE_BORDER_X_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_mve_border_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x90cL);
	APICAL_WRITE_32(0x90cL, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_mve_border_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x90cL) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Border y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of pixels for top border
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_BORDER_Y_DEFAULT (0x010)
#define APICAL_ISP_MVE_BORDER_Y_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_mve_border_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x910L);
	APICAL_WRITE_32(0x910L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_mve_border_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x910L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Zone size x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of pixels for zone width
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ZONE_SIZE_X_DEFAULT (0x020)
#define APICAL_ISP_MVE_ZONE_SIZE_X_DATASIZE (9)

// args: data (9-bit)
static __inline void apical_isp_mve_zone_size_x_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x914L);
	APICAL_WRITE_32(0x914L, (((uint32_t) (data & 0x1ff)) << 0) | (curr & 0xfffffe00));
}
static __inline uint16_t apical_isp_mve_zone_size_x_read(void) {
	return (uint16_t)((APICAL_READ_32(0x914L) & 0x1ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Zone size y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Number of pixels for zone height
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_ZONE_SIZE_Y_DEFAULT (0x020)
#define APICAL_ISP_MVE_ZONE_SIZE_Y_DATASIZE (9)

// args: data (9-bit)
static __inline void apical_isp_mve_zone_size_y_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x918L);
	APICAL_WRITE_32(0x918L, (((uint32_t) (data & 0x1ff)) << 0) | (curr & 0xfffffe00));
}
static __inline uint16_t apical_isp_mve_zone_size_y_read(void) {
	return (uint16_t)((APICAL_READ_32(0x918L) & 0x1ff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Range x
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Search range for offset_x
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_RANGE_X_DEFAULT (0x10)
#define APICAL_ISP_MVE_RANGE_X_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_range_x_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x91cL);
	APICAL_WRITE_32(0x91cL, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_mve_range_x_read(void) {
	return (uint8_t)((APICAL_READ_32(0x91cL) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Range y
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Search range for offset_y
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_RANGE_Y_DEFAULT (0x10)
#define APICAL_ISP_MVE_RANGE_Y_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_range_y_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x920L);
	APICAL_WRITE_32(0x920L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_mve_range_y_read(void) {
	return (uint8_t)((APICAL_READ_32(0x920L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F1 length
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F1 filter length for high freq
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F1_LENGTH_DEFAULT (0x08)
#define APICAL_ISP_MVE_F1_LENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_f1_length_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x924L);
	APICAL_WRITE_32(0x924L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_mve_f1_length_read(void) {
	return (uint8_t)((APICAL_READ_32(0x924L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F2 length
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F2 filter length for high freq
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F2_LENGTH_DEFAULT (0x10)
#define APICAL_ISP_MVE_F2_LENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_f2_length_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x924L);
	APICAL_WRITE_32(0x924L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_mve_f2_length_read(void) {
	return (uint8_t)((APICAL_READ_32(0x924L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: F3 length
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F3 filter length for high freq
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F3_LENGTH_DEFAULT (0x20)
#define APICAL_ISP_MVE_F3_LENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_f3_length_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x928L);
	APICAL_WRITE_32(0x928L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_mve_f3_length_read(void) {
	return (uint8_t)((APICAL_READ_32(0x928L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F4 length
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F4 filter length for high freq
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F4_LENGTH_DEFAULT (0x40)
#define APICAL_ISP_MVE_F4_LENGTH_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_mve_f4_length_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x928L);
	APICAL_WRITE_32(0x928L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_mve_f4_length_read(void) {
	return (uint8_t)((APICAL_READ_32(0x928L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: F0blk scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F0 block mixer scale factor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F0BLK_SCALE_DEFAULT (0x1)
#define APICAL_ISP_MVE_F0BLK_SCALE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_mve_f0blk_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x930L);
	APICAL_WRITE_32(0x930L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_mve_f0blk_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x930L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F1 scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F1 filter mixer scale factor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F1_SCALE_DEFAULT (0x1)
#define APICAL_ISP_MVE_F1_SCALE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_mve_f1_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x930L);
	APICAL_WRITE_32(0x930L, (((uint32_t) (data & 0xf)) << 8) | (curr & 0xfffff0ff));
}
static __inline uint8_t apical_isp_mve_f1_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x930L) & 0xf00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: F2 scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F2 filter mixer scale factor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F2_SCALE_DEFAULT (0x2)
#define APICAL_ISP_MVE_F2_SCALE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_mve_f2_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x930L);
	APICAL_WRITE_32(0x930L, (((uint32_t) (data & 0xf)) << 16) | (curr & 0xfff0ffff));
}
static __inline uint8_t apical_isp_mve_f2_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x930L) & 0xf0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: F3 scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F3 filter mixer scale factor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F3_SCALE_DEFAULT (0x3)
#define APICAL_ISP_MVE_F3_SCALE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_mve_f3_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x930L);
	APICAL_WRITE_32(0x930L, (((uint32_t) (data & 0xf)) << 24) | (curr & 0xf0ffffff));
}
static __inline uint8_t apical_isp_mve_f3_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x930L) & 0xf000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: F4 scale
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F3 filter mixer scale factor
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F4_SCALE_DEFAULT (0x4)
#define APICAL_ISP_MVE_F4_SCALE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_mve_f4_scale_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x934L);
	APICAL_WRITE_32(0x934L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_mve_f4_scale_read(void) {
	return (uint8_t)((APICAL_READ_32(0x934L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F0blk clip
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F0 filter mixer clip value
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F0BLK_CLIP_DEFAULT (0x3FF)
#define APICAL_ISP_MVE_F0BLK_CLIP_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_mve_f0blk_clip_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x938L);
	APICAL_WRITE_32(0x938L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_mve_f0blk_clip_read(void) {
	return (uint16_t)((APICAL_READ_32(0x938L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F1 clip
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F1 filter mixer clip value
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F1_CLIP_DEFAULT (0x3FF)
#define APICAL_ISP_MVE_F1_CLIP_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_mve_f1_clip_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x93cL);
	APICAL_WRITE_32(0x93cL, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_mve_f1_clip_read(void) {
	return (uint16_t)((APICAL_READ_32(0x93cL) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F2 clip
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F1 filter mixer clip value
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F2_CLIP_DEFAULT (0x7FF)
#define APICAL_ISP_MVE_F2_CLIP_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_mve_f2_clip_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x940L);
	APICAL_WRITE_32(0x940L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_mve_f2_clip_read(void) {
	return (uint16_t)((APICAL_READ_32(0x940L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F3 clip
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F3 filter mixer clip value
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F3_CLIP_DEFAULT (0xBFF)
#define APICAL_ISP_MVE_F3_CLIP_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_mve_f3_clip_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x944L);
	APICAL_WRITE_32(0x944L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_mve_f3_clip_read(void) {
	return (uint16_t)((APICAL_READ_32(0x944L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: F4 clip
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// F4 filter mixer clip value
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_F4_CLIP_DEFAULT (0xFFF)
#define APICAL_ISP_MVE_F4_CLIP_DATASIZE (12)

// args: data (12-bit)
static __inline void apical_isp_mve_f4_clip_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0x948L);
	APICAL_WRITE_32(0x948L, (((uint32_t) (data & 0xfff)) << 0) | (curr & 0xfffff000));
}
static __inline uint16_t apical_isp_mve_f4_clip_read(void) {
	return (uint16_t)((APICAL_READ_32(0x948L) & 0xfff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Video out select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Video out select
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_VIDEO_OUT_SELECT_DEFAULT (0x0)
#define APICAL_ISP_MVE_VIDEO_OUT_SELECT_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_mve_video_out_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x94cL);
	APICAL_WRITE_32(0x94cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_mve_video_out_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0x94cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Clear of internal alarms
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_CLEAR_ALARMS_DEFAULT (0x0)
#define APICAL_ISP_MVE_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mve_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x950L);
	APICAL_WRITE_32(0x950L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_mve_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0x950L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Dump restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Restart request for first frame of a sequence
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_DUMP_RESTART_DEFAULT (0x0)
#define APICAL_ISP_MVE_DUMP_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mve_dump_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x950L);
	APICAL_WRITE_32(0x950L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_mve_dump_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0x950L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: Dump on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Enabling dumping of stats
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_DUMP_ON_DEFAULT (0x0)
#define APICAL_ISP_MVE_DUMP_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_mve_dump_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x950L);
	APICAL_WRITE_32(0x950L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_mve_dump_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0x950L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: Dump overflow fail
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_MVE_DUMP_OVERFLOW_FAIL_DEFAULT (0x0)
#define APICAL_ISP_MVE_DUMP_OVERFLOW_FAIL_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_mve_dump_overflow_fail_read(void) {
	return (uint8_t)((APICAL_READ_32(0x950L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Group: Frame Stats
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: stats reset
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_STATS_RESET_DEFAULT (0)
#define APICAL_ISP_FRAME_STATS_STATS_RESET_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stats_stats_reset_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x970L);
	APICAL_WRITE_32(0x970L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_frame_stats_stats_reset_read(void) {
	return (uint8_t)((APICAL_READ_32(0x970L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: stats hold
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_STATS_HOLD_DEFAULT (0)
#define APICAL_ISP_FRAME_STATS_STATS_HOLD_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stats_stats_hold_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0x974L);
	APICAL_WRITE_32(0x974L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_frame_stats_stats_hold_read(void) {
	return (uint8_t)((APICAL_READ_32(0x974L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active width min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_MIN_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_width_min_read(void) {
	return APICAL_READ_32(0x980L);
}
// ------------------------------------------------------------------------------ //
// Register: active width max
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_MAX_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_width_max_read(void) {
	return APICAL_READ_32(0x984L);
}
// ------------------------------------------------------------------------------ //
// Register: active width sum
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_SUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_width_sum_read(void) {
	return APICAL_READ_32(0x988L);
}
// ------------------------------------------------------------------------------ //
// Register: active width num
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_NUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_WIDTH_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_width_num_read(void) {
	return APICAL_READ_32(0x98cL);
}
// ------------------------------------------------------------------------------ //
// Register: active height min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_MIN_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_height_min_read(void) {
	return APICAL_READ_32(0x990L);
}
// ------------------------------------------------------------------------------ //
// Register: active height max
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_MAX_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_height_max_read(void) {
	return APICAL_READ_32(0x994L);
}
// ------------------------------------------------------------------------------ //
// Register: active height sum
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_SUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_height_sum_read(void) {
	return APICAL_READ_32(0x998L);
}
// ------------------------------------------------------------------------------ //
// Register: active height num
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_NUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_ACTIVE_HEIGHT_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_active_height_num_read(void) {
	return APICAL_READ_32(0x99cL);
}
// ------------------------------------------------------------------------------ //
// Register: hblank min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_HBLANK_MIN_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_HBLANK_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_hblank_min_read(void) {
	return APICAL_READ_32(0x9a0L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank max
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_HBLANK_MAX_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_HBLANK_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_hblank_max_read(void) {
	return APICAL_READ_32(0x9a4L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank sum
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_HBLANK_SUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_HBLANK_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_hblank_sum_read(void) {
	return APICAL_READ_32(0x9a8L);
}
// ------------------------------------------------------------------------------ //
// Register: hblank num
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_HBLANK_NUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_HBLANK_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_hblank_num_read(void) {
	return APICAL_READ_32(0x9acL);
}
// ------------------------------------------------------------------------------ //
// Register: vblank min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_VBLANK_MIN_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_VBLANK_MIN_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_vblank_min_read(void) {
	return APICAL_READ_32(0x9b0L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank max
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_VBLANK_MAX_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_VBLANK_MAX_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_vblank_max_read(void) {
	return APICAL_READ_32(0x9b4L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank sum
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_VBLANK_SUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_VBLANK_SUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_vblank_sum_read(void) {
	return APICAL_READ_32(0x9b8L);
}
// ------------------------------------------------------------------------------ //
// Register: vblank num
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STATS_VBLANK_NUM_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STATS_VBLANK_NUM_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stats_vblank_num_read(void) {
	return APICAL_READ_32(0x9bcL);
}
// ------------------------------------------------------------------------------ //
// Group: Temper Frame Buffer
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_temper_frame_buffer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_temper_frame_buffer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_temper_frame_buffer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_temper_frame_buffer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_temper_frame_buffer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_temper_frame_buffer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Frame write on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_WRITE_ON_DEFAULT (1)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_buffer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_temper_frame_buffer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Frame read on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_READ_ON_DEFAULT (1)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_READ_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_buffer_frame_read_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_temper_frame_buffer_frame_read_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Frame write cancel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_buffer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0x1)) << 24) | (curr & 0xfeffffff));
}
static __inline uint8_t apical_isp_temper_frame_buffer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0x1000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Frame read cancel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_READ_CANCEL_DEFAULT (0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_FRAME_READ_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_buffer_frame_read_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa00L);
	APICAL_WRITE_32(0xa00L, (((uint32_t) (data & 0x1)) << 25) | (curr & 0xfdffffff));
}
static __inline uint8_t apical_isp_temper_frame_buffer_frame_read_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa00L) & 0x2000000) >> 25);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_temper_frame_buffer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xa08L, data);
}
static __inline uint32_t apical_isp_temper_frame_buffer_blk_config_read(void) {
	return APICAL_READ_32(0xa08L);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_BLK_STATUS_DEFAULT (0x0000)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_temper_frame_buffer_blk_status_read(void) {
	return APICAL_READ_32(0xa0cL);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_temper_frame_buffer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xa10L);
	APICAL_WRITE_32(0xa10L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_temper_frame_buffer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa10L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_temper_frame_buffer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xa14L);
	APICAL_WRITE_32(0xa14L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_temper_frame_buffer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa14L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_temper_frame_buffer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xa18L, data);
}
static __inline uint32_t apical_isp_temper_frame_buffer_bank0_base_read(void) {
	return APICAL_READ_32(0xa18L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_temper_frame_buffer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xa1cL, data);
}
static __inline uint32_t apical_isp_temper_frame_buffer_bank1_base_read(void) {
	return APICAL_READ_32(0xa1cL);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_temper_frame_buffer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xa24L, data);
}
static __inline uint32_t apical_isp_temper_frame_buffer_line_offset_read(void) {
	return APICAL_READ_32(0xa24L);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_TEMPER_FRAME_BUFFER_AXI_PORT_ENABLE_DEFAULT (0x0)
#define APICAL_ISP_TEMPER_FRAME_BUFFER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_temper_frame_buffer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa28L);
	APICAL_WRITE_32(0xa28L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_temper_frame_buffer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa28L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Frame stitch Frame Buffer
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: Frame write on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_WRITE_ON_DEFAULT (1)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0x1)) << 16) | (curr & 0xfffeffff));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: Frame read on
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_READ_ON_DEFAULT (1)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_READ_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_frame_read_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0x1)) << 17) | (curr & 0xfffdffff));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_frame_read_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: Frame write cancel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0x1)) << 24) | (curr & 0xfeffffff));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0x1000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: Frame read cancel
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_READ_CANCEL_DEFAULT (0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_FRAME_READ_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_frame_read_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa40L);
	APICAL_WRITE_32(0xa40L, (((uint32_t) (data & 0x1)) << 25) | (curr & 0xfdffffff));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_frame_read_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa40L) & 0x2000000) >> 25);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xa48L, data);
}
static __inline uint32_t apical_isp_frame_stitch_frame_buffer_blk_config_read(void) {
	return APICAL_READ_32(0xa48L);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BLK_STATUS_DEFAULT (0x0000)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_frame_stitch_frame_buffer_blk_status_read(void) {
	return APICAL_READ_32(0xa4cL);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xa50L);
	APICAL_WRITE_32(0xa50L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_frame_stitch_frame_buffer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa50L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xa54L);
	APICAL_WRITE_32(0xa54L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_frame_stitch_frame_buffer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xa54L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xa58L, data);
}
static __inline uint32_t apical_isp_frame_stitch_frame_buffer_bank0_base_read(void) {
	return APICAL_READ_32(0xa58L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xa5cL, data);
}
static __inline uint32_t apical_isp_frame_stitch_frame_buffer_bank1_base_read(void) {
	return APICAL_READ_32(0xa5cL);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xa64L, data);
}
static __inline uint32_t apical_isp_frame_stitch_frame_buffer_line_offset_read(void) {
	return APICAL_READ_32(0xa64L);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_AXI_PORT_ENABLE_DEFAULT (0x0)
#define APICAL_ISP_FRAME_STITCH_FRAME_BUFFER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_frame_stitch_frame_buffer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xa68L);
	APICAL_WRITE_32(0xa68L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_frame_stitch_frame_buffer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xa68L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer FR
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Full resolution video DMA writer controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_fr_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_fr_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_fr_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_fr_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_fr_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_fr_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_fr_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_fr_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb00L);
	APICAL_WRITE_32(0xb00L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_fr_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb00L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_FR_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xb04L);
	APICAL_WRITE_32(0xb04L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fr_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb04L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_FR_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fr_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xb04L);
	APICAL_WRITE_32(0xb04L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_fr_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb04L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb08L, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xb08L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb0cL, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xb0cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb10L, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xb10L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb14L, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xb14L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb18L, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xb18L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_fr_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb1cL);
	APICAL_WRITE_32(0xb1cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_fr_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb1cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb1cL);
	APICAL_WRITE_32(0xb1cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_fr_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb1cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_FR_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xb20L, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xb20L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb24L);
	APICAL_WRITE_32(0xb24L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb24L);
	APICAL_WRITE_32(0xb24L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fr_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_fr_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_fr_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb24L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fr_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xb28L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fr_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xb2cL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_fr_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb30L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_fr_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb30L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fr_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fr_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_fr_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_FR_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fr_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_fr_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FR_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_fr_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FR_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_fr_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FR_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fr_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb34L);
	APICAL_WRITE_32(0xb34L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_fr_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb34L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fr_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb38L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_FR_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fr_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xb3cL, data);
}
static __inline uint32_t apical_isp_fr_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xb3cL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FR_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_FR_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fr_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xb40L);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer FR UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Full resolution video DMA writer controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fruv_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_fruv_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_fruv_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_fruv_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_fruv_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_fruv_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb80L);
	APICAL_WRITE_32(0xb80L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb80L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_FRUV_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fruv_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xb84L);
	APICAL_WRITE_32(0xb84L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_fruv_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb84L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_FRUV_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_fruv_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xb84L);
	APICAL_WRITE_32(0xb84L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_fruv_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xb84L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb88L, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xb88L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb8cL, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xb8cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb90L, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xb90L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb94L, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xb94L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xb98L, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xb98L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_fruv_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb9cL);
	APICAL_WRITE_32(0xb9cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_fruv_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb9cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xb9cL);
	APICAL_WRITE_32(0xb9cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_fruv_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xb9cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_FRUV_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xba0L, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xba0L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xba4L);
	APICAL_WRITE_32(0xba4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fruv_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xba4L);
	APICAL_WRITE_32(0xba4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fruv_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xba4L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fruv_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xba8L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fruv_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xbacL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_fruv_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xbb0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_fruv_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xbb0L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_fruv_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_fruv_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_fruv_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_FRUV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_fruv_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_fruv_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FRUV_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fruv_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FRUV_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fruv_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_FRUV_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_fruv_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xbb4L);
	APICAL_WRITE_32(0xbb4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_fruv_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_fruv_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xbb8L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_FRUV_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_fruv_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xbbcL, data);
}
static __inline uint32_t apical_isp_fruv_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xbbcL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_FRUV_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_FRUV_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_fruv_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xbc0L);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer DS1
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Down scaled video DMA writer 1 controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds1_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds1_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds1_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds1_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_ds1_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc00L);
	APICAL_WRITE_32(0xc00L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc00L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS1_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xc04L);
	APICAL_WRITE_32(0xc04L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc04L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS1_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xc04L);
	APICAL_WRITE_32(0xc04L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_ds1_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc04L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc08L, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xc08L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc0cL, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xc0cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc10L, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xc10L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc14L, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xc14L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc18L, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xc18L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_ds1_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc1cL);
	APICAL_WRITE_32(0xc1cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_ds1_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc1cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc1cL);
	APICAL_WRITE_32(0xc1cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc1cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_DS1_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xc20L, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xc20L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc24L);
	APICAL_WRITE_32(0xc24L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc24L);
	APICAL_WRITE_32(0xc24L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc24L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xc28L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xc2cL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds1_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc30L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds1_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc30L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds1_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_DS1_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc34L);
	APICAL_WRITE_32(0xc34L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_ds1_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc34L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc38L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_DS1_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xc3cL, data);
}
static __inline uint32_t apical_isp_ds1_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xc3cL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_DS1_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xc40L);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer DS1 UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Down scaled video DMA writer 1 controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1uv_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds1uv_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds1uv_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc80L);
	APICAL_WRITE_32(0xc80L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc80L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS1UV_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1uv_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xc84L);
	APICAL_WRITE_32(0xc84L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds1uv_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc84L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS1UV_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds1uv_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xc84L);
	APICAL_WRITE_32(0xc84L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_ds1uv_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xc84L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc88L, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xc88L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc8cL, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xc8cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc90L, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xc90L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc94L, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xc94L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xc98L, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xc98L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_ds1uv_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc9cL);
	APICAL_WRITE_32(0xc9cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc9cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xc9cL);
	APICAL_WRITE_32(0xc9cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xc9cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_DS1UV_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xca0L, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xca0L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xca4L);
	APICAL_WRITE_32(0xca4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xca4L);
	APICAL_WRITE_32(0xca4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xca4L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1uv_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xca8L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1uv_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xcacL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds1uv_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xcb0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds1uv_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xcb0L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_DS1UV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds1uv_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1UV_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1uv_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1UV_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1uv_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS1UV_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds1uv_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xcb4L);
	APICAL_WRITE_32(0xcb4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_ds1uv_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds1uv_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xcb8L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_DS1UV_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds1uv_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xcbcL, data);
}
static __inline uint32_t apical_isp_ds1uv_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xcbcL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS1UV_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_DS1UV_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds1uv_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xcc0L);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer DS2
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Down scaled video DMA writer 2 controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds2_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds2_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds2_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds2_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_ds2_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd00L);
	APICAL_WRITE_32(0xd00L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd00L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS2_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xd04L);
	APICAL_WRITE_32(0xd04L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd04L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS2_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xd04L);
	APICAL_WRITE_32(0xd04L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_ds2_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd04L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd08L, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xd08L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd0cL, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xd0cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd10L, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xd10L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd14L, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xd14L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd18L, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xd18L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_ds2_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd1cL);
	APICAL_WRITE_32(0xd1cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_ds2_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd1cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd1cL);
	APICAL_WRITE_32(0xd1cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd1cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_DS2_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xd20L, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xd20L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd24L);
	APICAL_WRITE_32(0xd24L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd24L);
	APICAL_WRITE_32(0xd24L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd24L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xd28L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xd2cL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds2_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd30L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds2_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd30L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds2_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_DS2_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd34L);
	APICAL_WRITE_32(0xd34L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_ds2_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd34L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd38L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_DS2_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xd3cL, data);
}
static __inline uint32_t apical_isp_ds2_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xd3cL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_DS2_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xd40L);
}
// ------------------------------------------------------------------------------ //
// Group: Video DMA Writer DS2 UV
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Down scaled video DMA writer 2 controls
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Format
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Format
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_FORMAT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_FORMAT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2uv_dma_writer_format_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0xff)) << 0) | (curr & 0xffffff00));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_format_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0xff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Base mode
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Base DMA packing mode for RGB/RAW/YUV etc (see ISP guide)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BASE_MODE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BASE_MODE_DATASIZE (4)

// args: data (4-bit)
static __inline void apical_isp_ds2uv_dma_writer_base_mode_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0xf)) << 0) | (curr & 0xfffffff0));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_base_mode_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0xf) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: Plane select
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Plane select for planar base modes.  Only used if planar outputs required.  Not used.  Should be set to 0
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_PLANE_SELECT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_PLANE_SELECT_DATASIZE (2)

// args: data (2-bit)
static __inline void apical_isp_ds2uv_dma_writer_plane_select_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0x3)) << 6) | (curr & 0xffffff3f));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_plane_select_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0xc0) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: single frame
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = All frames are written(after frame_write_on= 1), 1= only 1st frame written ( after frame_write_on =1)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_SINGLE_FRAME_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_SINGLE_FRAME_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_single_frame_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0x1)) << 8) | (curr & 0xfffffeff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_single_frame_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: frame write on
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		0 = no frames written(when switched from 1, current frame completes writing before stopping),
//		1= write frame(s) (write single or continous frame(s) )
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WRITE_ON_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WRITE_ON_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_frame_write_on_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0x1)) << 9) | (curr & 0xfffffdff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_frame_write_on_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: half irate
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation , 1= write half(alternate) of input frames( only valid for continuous mode)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_HALF_IRATE_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_HALF_IRATE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_half_irate_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0x1)) << 10) | (curr & 0xfffffbff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_half_irate_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0x400) >> 10);
}
// ------------------------------------------------------------------------------ //
// Register: axi xact comp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = dont wait for axi transaction completion at end of frame(just all transfers accepted). 1 = wait for all transactions completed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_XACT_COMP_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_XACT_COMP_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_axi_xact_comp_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd80L);
	APICAL_WRITE_32(0xd80L, (((uint32_t) (data & 0x1)) << 11) | (curr & 0xfffff7ff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_xact_comp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd80L) & 0x800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: active width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video width in pixels 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_ACTIVE_WIDTH_DEFAULT (0x780)
#define APICAL_ISP_DS2UV_DMA_WRITER_ACTIVE_WIDTH_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2uv_dma_writer_active_width_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xd84L);
	APICAL_WRITE_32(0xd84L, (((uint32_t) (data & 0xffff)) << 0) | (curr & 0xffff0000));
}
static __inline uint16_t apical_isp_ds2uv_dma_writer_active_width_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd84L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: active height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Active video height in lines 128-8000
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_ACTIVE_HEIGHT_DEFAULT (0x438)
#define APICAL_ISP_DS2UV_DMA_WRITER_ACTIVE_HEIGHT_DATASIZE (16)

// args: data (16-bit)
static __inline void apical_isp_ds2uv_dma_writer_active_height_write(uint16_t data) {
	uint32_t curr = APICAL_READ_32(0xd84L);
	APICAL_WRITE_32(0xd84L, (((uint32_t) (data & 0xffff)) << 16) | (curr & 0xffff));
}
static __inline uint16_t apical_isp_ds2uv_dma_writer_active_height_read(void) {
	return (uint16_t)((APICAL_READ_32(0xd84L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: bank0_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 0 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK0_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK0_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank0_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd88L, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_bank0_base_read(void) {
	return APICAL_READ_32(0xd88L);
}
// ------------------------------------------------------------------------------ //
// Register: bank1_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 1 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK1_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK1_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank1_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd8cL, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_bank1_base_read(void) {
	return APICAL_READ_32(0xd8cL);
}
// ------------------------------------------------------------------------------ //
// Register: bank2_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 2 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK2_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK2_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank2_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd90L, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_bank2_base_read(void) {
	return APICAL_READ_32(0xd90L);
}
// ------------------------------------------------------------------------------ //
// Register: bank3_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 3 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK3_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK3_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank3_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd94L, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_bank3_base_read(void) {
	return APICAL_READ_32(0xd94L);
}
// ------------------------------------------------------------------------------ //
// Register: bank4_base
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// bank 4 base address for frame buffer, should be word-aligned
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK4_BASE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK4_BASE_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank4_base_write(uint32_t data) {
	APICAL_WRITE_32(0xd98L, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_bank4_base_read(void) {
	return APICAL_READ_32(0xd98L);
}
// ------------------------------------------------------------------------------ //
// Register: max bank
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// highest bank*_base to use for frame writes before recycling to bank0_base, only 0 to 4 are valid
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BANK_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BANK_DATASIZE (3)

// args: data (3-bit)
static __inline void apical_isp_ds2uv_dma_writer_max_bank_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd9cL);
	APICAL_WRITE_32(0xd9cL, (((uint32_t) (data & 0x7)) << 0) | (curr & 0xfffffff8));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_max_bank_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd9cL) & 0x7) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: bank0 restart
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= restart bank counter to bank0 for next frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BANK0_RESTART_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BANK0_RESTART_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_bank0_restart_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xd9cL);
	APICAL_WRITE_32(0xd9cL, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_bank0_restart_read(void) {
	return (uint8_t)((APICAL_READ_32(0xd9cL) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: Line_offset
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		Indicates the offset in bytes from the start of one line to the next line.
//		This value should be equal to or larger than one line of image data and should be word-aligned
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_LINE_OFFSET_DEFAULT (0x1000)
#define APICAL_ISP_DS2UV_DMA_WRITER_LINE_OFFSET_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_line_offset_write(uint32_t data) {
	APICAL_WRITE_32(0xda0L, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_line_offset_read(void) {
	return APICAL_READ_32(0xda0L);
}
// ------------------------------------------------------------------------------ //
// Register: frame write cancel
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0 = normal operation, 1= cancel current/future frame write(s), any unstarted AXI bursts cancelled and fifo flushed
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WRITE_CANCEL_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WRITE_CANCEL_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_frame_write_cancel_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xda4L);
	APICAL_WRITE_32(0xda4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_frame_write_cancel_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_port_enable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// enables axi, active high, 1=enables axi write transfers, 0= reset axi domain( via reset synchroniser)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_PORT_ENABLE_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_PORT_ENABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_axi_port_enable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xda4L);
	APICAL_WRITE_32(0xda4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_port_enable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: wbank curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank currently active. valid values =0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_CURR_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_wbank_curr_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x700) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wbank last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// write bank last active. valid values = 0-4. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_LAST_DATASIZE (3)

// args: data (3-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_wbank_last_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x3800) >> 11);
}
// ------------------------------------------------------------------------------ //
// Register: wbank active
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = wbank_curr is being written to. Goes high at start of writes, low at last write transfer/completion on axi.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_ACTIVE_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_ACTIVE_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_wbank_active_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x10000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wbank start
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at start of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_START_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_START_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_wbank_start_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x20000) >> 17);
}
// ------------------------------------------------------------------------------ //
// Register: wbank stop
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1 = High pulse at end of frame write to bank.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_STOP_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBANK_STOP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_wbank_stop_read(void) {
	return (uint8_t)((APICAL_READ_32(0xda4L) & 0x40000) >> 18);
}
// ------------------------------------------------------------------------------ //
// Register: wbase curr
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// currently active bank base addr - in bytes. updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBASE_CURR_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBASE_CURR_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2uv_dma_writer_wbase_curr_read(void) {
	return APICAL_READ_32(0xda8L);
}
// ------------------------------------------------------------------------------ //
// Register: wbase last
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// last active bank base addr - in bytes. Updated at start of frame write
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WBASE_LAST_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WBASE_LAST_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2uv_dma_writer_wbase_last_read(void) {
	return APICAL_READ_32(0xdacL);
}
// ------------------------------------------------------------------------------ //
// Register: frame icount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of incomming frames (starts) to vdma_writer on video input, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_ICOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_ICOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds2uv_dma_writer_frame_icount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xdb0L) & 0xffff) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: frame wcount
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// count of outgoing frame writes (starts) from vdma_writer sent to AXI output, non resetable, rolls over, updates at pixel 1 of new frame on video in
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WCOUNT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_FRAME_WCOUNT_DATASIZE (16)

// args: data (16-bit)
static __inline uint16_t apical_isp_ds2uv_dma_writer_frame_wcount_read(void) {
	return (uint16_t)((APICAL_READ_32(0xdb0L) & 0xffff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: clear alarms
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 0>1 transition(synchronous detection) causes local axi/video alarm clear
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_CLEAR_ALARMS_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_CLEAR_ALARMS_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_clear_alarms_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0x1)) << 0) | (curr & 0xfffffffe));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_clear_alarms_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_8
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 8, 0= Dont reduce
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BURST_LENGTH_IS_8_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_max_burst_length_is_8_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0x1)) << 1) | (curr & 0xfffffffd));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_max_burst_length_is_8_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: max_burst_length_is_4
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// 1= Reduce default AXI max_burst_length from 16 to 4, 0= Dont reduce( has priority overmax_burst_length_is_8!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_MAX_BURST_LENGTH_IS_4_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_max_burst_length_is_4_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0x1)) << 2) | (curr & 0xfffffffb));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_max_burst_length_is_4_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: write timeout disable
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//
//		At end of frame an optional timeout is applied to wait for AXI writes to completed/accepted befotre caneclling and flushing.
//		0= Timeout Enabled, timeout count can decrement.
//		1 = Disable timeout, timeout count can't decrement.
//
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DEFAULT (0)
#define APICAL_ISP_DS2UV_DMA_WRITER_WRITE_TIMEOUT_DISABLE_DATASIZE (1)

// args: data (1-bit)
static __inline void apical_isp_ds2uv_dma_writer_write_timeout_disable_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0x1)) << 3) | (curr & 0xfffffff7));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_write_timeout_disable_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: awmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// awvalid maxwait limit(cycles) to raise axi_fail_awmaxwait alarm . zero disables alarm raise.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AWMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2UV_DMA_WRITER_AWMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2uv_dma_writer_awmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0xff)) << 8) | (curr & 0xffff00ff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_awmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0xff00) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: wmaxwait_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// wvalid maxwait limit(cycles) to raise axi_fail_wmaxwait alarm . zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WMAXWAIT_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2UV_DMA_WRITER_WMAXWAIT_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2uv_dma_writer_wmaxwait_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0xff)) << 16) | (curr & 0xff00ffff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_wmaxwait_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0xff0000) >> 16);
}
// ------------------------------------------------------------------------------ //
// Register: wxact_ostand_limit
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// number oustsanding write transactions(bursts)(responses..1 per burst) limit to raise axi_fail_wxact_ostand. zero disables alarm raise
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_WXACT_OSTAND_LIMIT_DEFAULT (0x00)
#define APICAL_ISP_DS2UV_DMA_WRITER_WXACT_OSTAND_LIMIT_DATASIZE (8)

// args: data (8-bit)
static __inline void apical_isp_ds2uv_dma_writer_wxact_ostand_limit_write(uint8_t data) {
	uint32_t curr = APICAL_READ_32(0xdb4L);
	APICAL_WRITE_32(0xdb4L, (((uint32_t) (data & 0xff)) << 24) | (curr & 0xffffff));
}
static __inline uint8_t apical_isp_ds2uv_dma_writer_wxact_ostand_limit_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb4L) & 0xff000000) >> 24);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_bresp
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate bad  bresp captured
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_BRESP_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_BRESP_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_fail_bresp_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x1) >> 0);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_awmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when awmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_AWMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_fail_awmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x2) >> 1);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wmaxwait
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wmaxwait_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_WMAXWAIT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_fail_wmaxwait_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x4) >> 2);
}
// ------------------------------------------------------------------------------ //
// Register: axi_fail_wxact_ostand
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high when wxact_ostand_limit reached
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_FAIL_WXACT_OSTAND_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_fail_wxact_ostand_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x8) >> 3);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_width
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_width detected
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_ACTIVE_WIDTH_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_vi_fail_active_width_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x10) >> 4);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_active_height
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate mismatched active_height detected ( also raised on missing field!)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_ACTIVE_HEIGHT_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_vi_fail_active_height_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x20) >> 5);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interline_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interline blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_INTERLINE_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_vi_fail_interline_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x40) >> 6);
}
// ------------------------------------------------------------------------------ //
// Register: vi_fail_interframe_blanks
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  clearable alarm, high to indicate interframe blanking below min
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_VI_FAIL_INTERFRAME_BLANKS_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_vi_fail_interframe_blanks_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x80) >> 7);
}
// ------------------------------------------------------------------------------ //
// Register: video_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on video port(s) ( active width/height or interline/frame blanks failure)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_VIDEO_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_VIDEO_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_video_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x100) >> 8);
}
// ------------------------------------------------------------------------------ //
// Register: axi_alarm
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
//  active high, problem found on axi port(s)( bresp or awmaxwait or wmaxwait or wxact_ostand failure )
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_ALARM_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_AXI_ALARM_DATASIZE (1)

// args: data (1-bit)
static __inline uint8_t apical_isp_ds2uv_dma_writer_axi_alarm_read(void) {
	return (uint8_t)((APICAL_READ_32(0xdb8L) & 0x200) >> 9);
}
// ------------------------------------------------------------------------------ //
// Register: blk_config
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block configuration (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BLK_CONFIG_DEFAULT (0x0000)
#define APICAL_ISP_DS2UV_DMA_WRITER_BLK_CONFIG_DATASIZE (32)

// args: data (32-bit)
static __inline void apical_isp_ds2uv_dma_writer_blk_config_write(uint32_t data) {
	APICAL_WRITE_32(0xdbcL, data);
}
static __inline uint32_t apical_isp_ds2uv_dma_writer_blk_config_read(void) {
	return APICAL_READ_32(0xdbcL);
}
// ------------------------------------------------------------------------------ //
// Register: blk_status
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// block status output (reserved)
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DS2UV_DMA_WRITER_BLK_STATUS_DEFAULT (0x0)
#define APICAL_ISP_DS2UV_DMA_WRITER_BLK_STATUS_DATASIZE (32)

// args: data (32-bit)
static __inline uint32_t apical_isp_ds2uv_dma_writer_blk_status_read(void) {
	return APICAL_READ_32(0xdc0L);
}
// ------------------------------------------------------------------------------ //
// Group: Iridix LUT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT: Asymmetry
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix target curve.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_LUT_ASYMMETRY_LUT_NODES (33)
#define APICAL_ISP_IRIDIX_LUT_ASYMMETRY_LUT_ADDRBITS (6)
#define APICAL_ISP_IRIDIX_LUT_ASYMMETRY_LUT_DATASIZE (16)

// args: index (0-32), data (16-bit)
static __inline void apical_isp_iridix_lut_asymmetry_lut_write(uint8_t index,uint16_t data) {
	APICAL_WRITE_32(0x1c00L, index);
	APICAL_WRITE_32(0x1c04L, data);
}
// ------------------------------------------------------------------------------ //
// LUT: Rev Percept
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix look-up-table 1
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_LUT_REV_PERCEPT_LUT_NODES (65)
#define APICAL_ISP_IRIDIX_LUT_REV_PERCEPT_LUT_ADDRBITS (7)
#define APICAL_ISP_IRIDIX_LUT_REV_PERCEPT_LUT_DATASIZE (16)

// args: index (0-64), data (16-bit)
static __inline void apical_isp_iridix_lut_rev_percept_lut_write(uint8_t index,uint16_t data) {
	APICAL_WRITE_32(0x1c20L, index);
	APICAL_WRITE_32(0x1c24L, data);
}
// ------------------------------------------------------------------------------ //
// LUT: Fwd Percept
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Iridix look-up-table 2
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_IRIDIX_LUT_FWD_PERCEPT_LUT_NODES (65)
#define APICAL_ISP_IRIDIX_LUT_FWD_PERCEPT_LUT_ADDRBITS (7)
#define APICAL_ISP_IRIDIX_LUT_FWD_PERCEPT_LUT_DATASIZE (12)

// args: index (0-64), data (12-bit)
static __inline void apical_isp_iridix_lut_fwd_percept_lut_write(uint8_t index,uint16_t data) {
	APICAL_WRITE_32(0x1c30L, index);
	APICAL_WRITE_32(0x1c34L, data);
}
// ------------------------------------------------------------------------------ //
// Group: Sinter Shading
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// LUT: rm_shading_lut
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Radial Sinter LUT.  See ISP guide for more details
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_SINTER_SHADING_RM_SHADING_LUT_NODES (33)
#define APICAL_ISP_SINTER_SHADING_RM_SHADING_LUT_ADDRBITS (6)
#define APICAL_ISP_SINTER_SHADING_RM_SHADING_LUT_DATASIZE (8)

// args: index (0-32), data (8-bit)
static __inline void apical_isp_sinter_shading_rm_shading_lut_write(uint8_t index,uint8_t data) {
	uint32_t addr = 0x1c40L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	uint32_t curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_sinter_shading_rm_shading_lut_read(uint8_t index) {
	uint32_t addr = 0x1c40L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
// Group: Noise Profile RAW frontend LUT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile controls for RAW frontend
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Weight lut
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile LUT.  Calculated during calibration process.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_LUT_WEIGHT_LUT_DEFAULT (0x0)
#define APICAL_ISP_NOISE_PROFILE_RAW_FRONTEND_LUT_WEIGHT_LUT_DATASIZE (8)

// index (0-127), args: data (8-bit)
static __inline void apical_isp_noise_profile_raw_frontend_lut_weight_lut_write(uint32_t index,uint8_t data) {
	uint32_t addr = 0x1c80L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	uint32_t curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_noise_profile_raw_frontend_lut_weight_lut_read(uint32_t index) {
	uint32_t addr = 0x1c80L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
// Group: Noise Profile LUT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile controls for Sinter and Temper
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Weight lut
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile LUT.  Calculated during calibration process.
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_NOISE_PROFILE_LUT_WEIGHT_LUT_DEFAULT (0x0)
#define APICAL_ISP_NOISE_PROFILE_LUT_WEIGHT_LUT_DATASIZE (8)

// index (0-127), args: data (8-bit)
static __inline void apical_isp_noise_profile_lut_weight_lut_write(uint32_t index,uint8_t data) {
	uint32_t addr = 0x1d00L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	uint32_t curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_noise_profile_lut_weight_lut_read(uint32_t index) {
	uint32_t addr = 0x1d00L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
// Group: Demosaic LUT
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Bayer Demosaic lookup
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: Weight lut
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Noise profile LUT
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_DEMOSAIC_LUT_WEIGHT_LUT_DEFAULT (0x0)
#define APICAL_ISP_DEMOSAIC_LUT_WEIGHT_LUT_DATASIZE (8)

// index (0-127), args: data (8-bit)
static __inline void apical_isp_demosaic_lut_weight_lut_write(uint32_t index,uint8_t data) {
	uint32_t addr = 0x1d80L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	uint32_t curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_demosaic_lut_weight_lut_read(uint32_t index) {
	uint32_t addr = 0x1d80L + (index & 0xFFFFFFFC);
	uint8_t offset = (index & 3) << 3;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
// Group: Zones
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Controls zone weighting for auto-exposure and auto-white-balance
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Register: AEXP Weight
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets zone weighting for auto exposure. Index is (row,col) where (0,0) is top-left zonea
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ZONES_AEXP_WEIGHT_DEFAULT (0xF)
#define APICAL_ISP_ZONES_AEXP_WEIGHT_DATASIZE (4)

// index1 (0-14), index2 (0-14), args: data (4-bit)
static __inline void apical_isp_zones_aexp_weight_write(uint32_t index1, uint32_t index2,uint8_t data) {
	uint32_t addr;
	uint8_t offset;
	uint32_t curr;
	addr = 0x1e00L + (index1 * 15 + index2);
	offset = (addr & 3) << 3;
	addr &= 0xFFFFFFFC;
	curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_zones_aexp_weight_read(uint32_t index1, uint32_t index2) {
	uint32_t addr = 0x1e00L + ((index1 * 15 + index2));
	uint8_t offset = (addr & 3) << 3;
	addr &= 0xFFFFFFFC;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
// Register: AWB Weight
// ------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------ //
// Sets zone weighting for auto white balance. Index is (row,col) where (0,0) is top-left zone
// ------------------------------------------------------------------------------ //

#define APICAL_ISP_ZONES_AWB_WEIGHT_DEFAULT (0xF)
#define APICAL_ISP_ZONES_AWB_WEIGHT_DATASIZE (4)

// index1 (0-14), index2 (0-14), args: data (4-bit)
static __inline void apical_isp_zones_awb_weight_write(uint32_t index1, uint32_t index2,uint8_t data) {
	uint32_t addr;
	uint8_t offset;
	uint32_t curr;
	addr = 0x1f00L + (index1 * 15 + index2);
	offset = (addr & 3) << 3;
	addr &= 0xFFFFFFFC;
	curr = APICAL_READ_32(addr);
	APICAL_WRITE_32(addr, ((uint32_t)data << offset) | (curr & ~(0xFF << offset)));
}
static __inline uint8_t apical_isp_zones_awb_weight_read(uint32_t index1, uint32_t index2) {
	uint32_t addr = 0x1f00L + ((index1 * 15 + index2));
	uint8_t offset = (addr & 3) << 3;
	addr &= 0xFFFFFFFC;
	return (uint8_t)(APICAL_READ_32(addr) >> offset);
}
// ------------------------------------------------------------------------------ //
#endif //__APICAL_ISP_CONFIG_H__
