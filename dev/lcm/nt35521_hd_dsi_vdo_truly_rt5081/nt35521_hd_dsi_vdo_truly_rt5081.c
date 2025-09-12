#define LOG_TAG "LCM"

#include "lcm_drv.h"
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#ifndef MACH_FPGA
#include <lcm_pmic.h>
#endif

#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)           (lcm_util.mdelay(n))
#define UDELAY(n)           (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define LCM_ID_NT35521 (0xf5)
#define LCM_ID_NT35695 (0xf5)

#define FRAME_WIDTH       	480
#define FRAME_HEIGHT        640
#define LCM_PHYSICAL_WIDTH 	43000
#define LCM_PHYSICAL_HEIGHT	57000
#define LCM_DENSITY			286

#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_END_OF_TABLE    0xFFFD

static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;
static const unsigned char LCD_MODULE_ID = 0x01;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00}},
	{0xC1, 1, {0x3F}},
	{0x6C, 1, {0x60}},
	{REGFLAG_DELAY, 20, {}},
	{0xB1, 1, {0x00}},
	{0xFA, 4, {0x7F, 0x00, 0x00, 0x00}},
	{REGFLAG_DELAY, 20, {}},
	{0x6C, 1, {0x50}},
	{REGFLAG_DELAY, 10, {}},
	{0x28, 0, {}},
	{REGFLAG_DELAY, 50, {}},
	{0x10, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00}},
	{0xC2, 1, {0xCE}},
	{0xC3, 1, {0xCD}},
	{0xC6, 1, {0xFC}},
	{0xC5, 1, {0x03}},
	{0xCD, 1, {0x64}},
	{0xC4, 1, {0xFF}},
	{0xC9, 1, {0xCD}},
	{0xF6, 2, {0x5A, 0x87}},
	{0xFD, 3, {0xAA, 0xAA, 0x0A}},
	{0xFE, 2, {0x6A, 0x0A}},
	{0x78, 2, {0x2A, 0xAA}},
	{0x92, 2, {0x17, 0x08}},
	{0x77, 2, {0xAA, 0x2A}},
	{0x76, 2, {0xAA, 0xAA}},
	{0x84, 1, {0x00}},
	{0x78, 2, {0x2B, 0xBA}},
	{0x89, 1, {0x73}},
	{0x88, 1, {0x3A}},
	{0x85, 1, {0xB0}},
	{0x76, 2, {0xEB, 0xAA}},
	{0x94, 1, {0x80}},
	{0x87, 3, {0x04, 0x07, 0x30}},
	{0x93, 1, {0x27}},
	{0xAF, 1, {0x02}},
	{REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table lcm_init_setting[] = {
	{0xF0, 5, {0x55, 0xAA, 0x52, 0x08, 0x00}},
	{0xF6, 2, {0x5A, 0x87}},
	{0xC1, 1, {0x3F}},
	{0xC6, 1, {0xF8}},
	{0xC9, 1, {0x10}},
	{0xCD, 1, {0x25}},
	{0xF8, 1, {0x8A}},
	{0xAC, 1, {0x45}},
	{0xA7, 1, {0x47}},
	{0xA0, 1, {0x99}},
	{0x86, 4, {0x99, 0xA3, 0xA3, 0x41}},
	{0x87, 3, {0x04, 0x03, 0x66}},
	{0xFA, 4, {0x08, 0x08, 0x00, 0x04}},
	{0xA3, 1, {0x22}},
	{0xFD, 3, {0x3C, 0x3C, 0x00}},
	{0x6B, 1, {0x07}},
	{0x9A, 1, {0x7E}},
	{0x9B, 1, {0x8E}},
	{0x82, 2, {0x49, 0x49}},
	{0xB1, 1, {0x10}},
	{0x7A, 2, {0x13, 0x1A}},
	{0x7B, 2, {0x13, 0x1A}},
	{0x69, 7, {0x14, 0x22, 0x14, 0x22, 0x44, 0x22, 0x08}},
	{0x6D, 32, {0x1E, 0x1E, 0x13, 0x14, 0x11, 0x12, 0x1E, 0x0D, 0x02, 0x08, 0x03, 0x04, 0x00, 0x1F, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x00, 0x04, 0x03, 0x07, 0x01, 0x0D, 0x1E, 0x0A, 0x09, 0x0C, 0x0B, 0x1E, 0x1E}},
	{0x60, 8, {0x38, 0x0A, 0x21, 0x6C, 0x38, 0x09, 0x21, 0x6C}},
	{0x63, 8, {0x38, 0x08, 0x21, 0x6C, 0x38, 0x07, 0x21, 0x6C}},
	{0x61, 8, {0x32, 0x83, 0x21, 0x6C, 0x82, 0x91, 0x6C, 0x6C}},
	{0x66, 6, {0x78, 0x0E, 0x02, 0x91, 0x03, 0x03}},
	{0x64, 16, {0x38, 0x08, 0x02, 0x7E, 0x03, 0x03, 0x38, 0x06, 0x02, 0x81, 0x03, 0x03, 0x21, 0x6C, 0x21, 0x6C}},
	{0x65, 16, {0x38, 0x04, 0x02, 0x83, 0x03, 0x03, 0x38, 0x02, 0x02, 0x85, 0x03, 0x03, 0x21, 0x6C, 0x21, 0x6C}},
	{0xD1, 52, { /* data omitted for brevity */ }},
	{0xD2, 52, { /* data omitted for brevity */ }},
	{0xD3, 52, { /* data omitted for brevity */ }},
	{0xD4, 52, { /* data omitted for brevity */ }},
	{0xD5, 52, { /* data omitted for brevity */ }},
	{0xD6, 52, { /* data omitted for brevity */ }},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {}},
	{REGFLAG_END_OF_TABLE, 0, {}},
};

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
                       unsigned int count, unsigned char force_update)
{
	unsigned int i, cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY:
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
				(unsigned char *)table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH / 1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT / 1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
	params->density = LCM_DENSITY;

	params->dsi.switch_mode_enable = 0;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.LANE_NUM = LCM_TWO_LANE;

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.vertical_sync_active = 8;
	params->dsi.vertical_backporch = 20;
	params->dsi.vertical_frontporch = 12;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 16;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 32;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	params->dsi.cont_clock = 1;
	params->dsi.PLL_CLOCK = 134;
	params->dsi.clk_lp_per_line_enable = 0;

	params->dsi.customization_esd_check_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0B;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x00;
}

LCM_DRIVER nt35521_hd_dsi_vdo_truly_rt5081_lcm_drv = {
	.name = "nt35521_hd_dsi_vdo_truly_rt5081",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = NULL,
	.suspend = NULL,
	.resume = NULL,
	.init_power = NULL,
	.resume_power = NULL,
	.suspend_power = NULL,
	.set_backlight = NULL,
};
