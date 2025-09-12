#include <platform/upmu_common.h>`
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#include "lcm_drv.h"

#include "data_rgba4444_roundedpattern.h"



#define LCM_LOGI(string, args...)	dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)	dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#define SET_RESET_PIN(v)			(lcm_util.set_reset_pin((v)))
#define MDELAY(n)					(lcm_util.mdelay(n))
#define UDELAY(n)					(lcm_util.udelay(n))
#define LCM_ID_NT35695 (0xf5)


#define LOG_TAG "LCM"
#define LCM_DSI_CMD_MODE		0
#define FRAME_WIDTH				(1080)
#define FRAME_HEIGHT			(1920)

#define REGFLAG_DELAY       	0xFFFC
#define REGFLAG_END_OF_TABLE    0xFFFD
//#define REGFLAG_UDELAY  		0xFFFB
//#define REGFLAG_RESET_LOW   	0xFFFE
//#define REGFLAG_RESET_HIGH  	0xFFFF

static LCM_UTIL_FUNCS lcm_util;
static const unsigned char LCD_MODULE_ID = 0x01;

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size) \
        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
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

static struct LCM_setting_table init_setting[] = {
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
	{0xD1, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0xD2, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0xD3, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0xD4, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0xD5, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0xD6, 52, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5D, 0x00, 0x60, 0x00, 0xB0, 0x00, 0xB3, 0x00, 0xEE, 0x01, 0x1D, 0x01, 0x6D, 0x01, 0xA6, 0x02, 0x0B, 0x02, 0x55, 0x02, 0x56, 0x02, 0x9D, 0x02, 0xE4, 0x03, 0x10, 0x03, 0x44, 0x03, 0x84, 0x03, 0x90, 0x03, 0xAE, 0x03, 0xB5, 0x03, 0xD8, 0x03, 0xE6, 0x03, 0xF7, 0x03, 0xFF}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {}},
	{REGFLAG_END_OF_TABLE, 0, {}},
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
				table[i].para_list, force_update);
		}
	}
}





static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 8;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_frontporch_for_low_power = 620;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 20;
	params->dsi.horizontal_frontporch = 40;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* params->dsi.ssc_disable                                                   = 1; */
#ifndef MACH_FPGA
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 420;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 440;	/* this value must be in MTK suggested table */
#endif
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.CLK_HS_POST = 36;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x53;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x24;

#ifdef NT35695_LANESWAP
	params->dsi.lane_swap_en = 1;

	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_CK;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_2;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_0;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_1;
	params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_1;
#endif

#ifdef MTK_ROUND_CORNER_SUPPORT
	params->round_corner_params.round_corner_en = 1;
	params->round_corner_params.full_content = 0;
	params->round_corner_params.w = ROUND_CORNER_W;
	params->round_corner_params.h = ROUND_CORNER_H;
	params->round_corner_params.lt_addr = left_top;
	params->round_corner_params.lb_addr = left_bottom;
	params->round_corner_params.rt_addr = right_top;
	params->round_corner_params.rb_addr = right_bottom;
#endif
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(NULL, init_setting,
		sizeof(init_setting) / sizeof(struct LCM_setting_table), true);
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), true);
	SET_RESET_PIN(1);
	MDELAY(10);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0, version_id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0];     /* we only need ID */

	read_reg_v2(0xDB, buffer, 1);
	version_id = buffer[0];

	/* return uVar1 == 0xf5 && local_6c[0] == 0x81; */
	return (id == LCM_ID_NT35695 && version_id == 0x81);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_LOGI("%s,nt35695 backlight: level = %d\n", __func__, level);
	bl_level[0].para_list[0] = level;
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void){lcm_init();}
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height){}
static unsigned int lcm_esd_check(void){return 0;}
static unsigned int lcm_ata_check(unsigned char *buffer){return 0;}
static void *lcm_switch_mode(int mode){return NULL;}
static void lcm_init_power(void){}
static void lcm_suspend_power(void){}
static void lcm_resume_power(void){}
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util){memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));}


LCM_DRIVER nt35695B_fhd_dsi_vdo_auo_rt5081_lcm_drv = {
	.name = "nt35695B_fhd_dsi_vdo_auo_rt5081_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
};

/*
static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	LCM_LOGI("%s,nt35695 backlight: level = %d\n", __func__, level);
	bl_level[0].para_list[0] = level;
	push_table(NULL, bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}



*/