#define LOG_TAG "LCM"

#include <linux/string.h>
#include <linux/kernel.h>
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
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

/*
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
        lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
      lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

*/
//#ifndef MACH_FPGA
//#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
//#endif

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
	
#define LCM_ID_NT35521 (0xf5)
#define LCM_ID_NT35695 (0xf5)


#define FRAME_WIDTH                                     (480)
#define FRAME_HEIGHT                                    (640)
#define LCM_PHYSICAL_WIDTH	(43000)
#define LCM_PHYSICAL_HEIGHT	(57000)
#define LCM_DENSITY		(286)


#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_END_OF_TABLE    0xFFFD

//#define REGFLAG_UDELAY  0xFFFB
//#define REGFLAG_RESET_LOW   0xFFFE
//#define REGFLAG_RESET_HIGH  0xFFFF

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

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(void *cmdq, struct LCM_setting_table *table,	unsigned int count, unsigned char force_update)
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

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	/* Panel type */
	params->type = LCM_TYPE_DSI;

	/* Display size parameters */
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
	params->density = LCM_DENSITY;

	/* The panel runs in video mode */
	params->dsi.switch_mode_enable = 0;
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;

	/* Number of DSI lanes */
	params->dsi.LANE_NUM = LCM_TWO_LANE;

	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;

	/* Video mode timing */
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

	/* ESD check parameters */
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0xb;
	params->dsi.lcm_esd_check_table[0].count = 0x1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x0;
}


static void lcm_init_power(void)
{
}

static void lcm_suspend_power(void)
{
}

static void lcm_resume_power(void)
{
}


static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(NULL, lcm_init_setting,
		sizeof(lcm_init_setting) / sizeof(struct LCM_setting_table), true);
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting,
		sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), true);
	SET_RESET_PIN(1);
	MDELAY(10);
}


static void lcm_resume(void)
{
	lcm_init();
}

/*
static void lcm_init_power(void)
{
	display_bias_enable();
}

static void lcm_suspend_power(void)
{
	display_bias_disable();
}
*/

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
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


static unsigned int lcm_esd_check(void)
{
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x53, buffer, 1);

	return (buffer[0] != 0x24);
}










static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];
	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	        && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}


static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{

	LCM_LOGI("%s,nt35521 backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	//push_table(bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_LOGI("%s,nt35521 backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	//push_table(bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void *lcm_switch_mode(int mode)
{
#ifndef BUILD_LK
	/* customization: 1. V2C config 2 values, C2V config 1 value; 2. config mode control register */
	if (mode == 0) {    /* V2C */
		lcm_switch_mode_cmd.mode = CMD_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;    /* mode control addr */
		lcm_switch_mode_cmd.val[0] = 0x13;  /* enabel GRAM firstly, ensure writing one frame to GRAM */
		lcm_switch_mode_cmd.val[1] = 0x10;  /* disable video mode secondly */
	} else {        /* C2V */
		lcm_switch_mode_cmd.mode = SYNC_PULSE_VDO_MODE;
		lcm_switch_mode_cmd.addr = 0xBB;
		lcm_switch_mode_cmd.val[0] = 0x03;  /* disable GRAM and enable video mode */
	}
	return (void *)(&lcm_switch_mode_cmd);
#else
	return NULL;
#endif
}


LCM_DRIVER nt35521_hd_dsi_vdo_truly_rt5081_lcm_drv = {
	.name = "nt35521_hd_dsi_vdo_truly_rt5081_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
//	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
//	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
/*	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
*/
};
