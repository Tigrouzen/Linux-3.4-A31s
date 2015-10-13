#include "B079XAN01.h"
#include "panels.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

#define spi_csx_set(v)	(LCD_GPIO_write(0, 3, v))
#define spi_sck_set(v)  (LCD_GPIO_write(0, 0, v))
#define spi_sdi_set(v)  (LCD_GPIO_write(0, 1, v))

#define ssd2828_rst(v)  (LCD_GPIO_write(0, 4, v))
#define panel_rst(v)    (LCD_GPIO_write(0, 2, v))

static void spi_24bit_3wire(__u32 tx)
{
	__u8 i;

	spi_csx_set(0);

	for(i=0;i<24;i++)
	{
		sunxi_lcd_delay_us(1);
		spi_sck_set(0);
		sunxi_lcd_delay_us(1);
		if(tx & 0x800000)
			spi_sdi_set(1);
		else
			spi_sdi_set(0);
		sunxi_lcd_delay_us(1);
		spi_sck_set(1);
		sunxi_lcd_delay_us(1);
		tx <<= 1;
	}
	spi_sdi_set(1);
	sunxi_lcd_delay_us(1);
	spi_csx_set(1);
	sunxi_lcd_delay_us(3);
}

static void lp079x01_init(__panel_para_t * info)
{
	__u32 pll_config = 0;

	if(info->lcd_xtal_freq == 12)
	{
		/* 12M xtal freq */
		pll_config = 0xc02D;
	} else if(info->lcd_xtal_freq == 27)
	{
		pll_config = 0xc014; // 0xc013
	} else if(info->lcd_xtal_freq == 24)
	{
		pll_config = 0xc22d;
	} else
	{
		/* default 12Mhz */
		pll_config = 0xc02D;
	}

	spi_24bit_3wire(0x7000B7); //enter LP mode
	spi_24bit_3wire(0x720340);
	ssd2828_rst(0);
	panel_rst(0);
	sunxi_lcd_delay_ms(10);
	ssd2828_rst(1);
	panel_rst(1);
	sunxi_lcd_delay_ms(10);

	spi_24bit_3wire(0x7000B1);  //VSA=50, HAS=64
	spi_24bit_3wire(0x723240);

	spi_24bit_3wire(0x7000B2); //VBP=30+50, HBP=56+64
	spi_24bit_3wire(0x725078);

	spi_24bit_3wire(0x7000B3); //VFP=36, HFP=60
	spi_24bit_3wire(0x72243C);

	spi_24bit_3wire(0x7000B4); //HACT=768
	spi_24bit_3wire(0x720300);

	spi_24bit_3wire(0x7000B5); //VACT=1240
	spi_24bit_3wire(0x720400);

	spi_24bit_3wire(0x7000B6);
	spi_24bit_3wire(0x720009); //0x720009:burst mode, 18bpp packed
	//0x72000A:burst mode, 18bpp loosely packed

	spi_24bit_3wire(0x7000DE); //no of lane=4
	spi_24bit_3wire(0x720003);

	spi_24bit_3wire(0x7000D6); //RGB order and packet number in blanking period
	spi_24bit_3wire(0x720005);

	spi_24bit_3wire(0x7000B9); //disable PLL
	spi_24bit_3wire(0x720000);

	pll_config |= 0x720000;
	pr_warn("[MINI]pll_config=0x%x\n", pll_config);
	spi_24bit_3wire(0x7000BA); //lane speed=560
	spi_24bit_3wire(pll_config); //may modify according to requirement, 500Mbps to  560Mbps, clk_in / (bit12-8) * (bit7-0)
	
	spi_24bit_3wire(0x7000BB); //LP clock
	spi_24bit_3wire(0x720008);

	spi_24bit_3wire(0x7000B9); //enable PPL
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000c4); //enable BTA
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000B7); //enter LP mode
	spi_24bit_3wire(0x720342);

	spi_24bit_3wire(0x7000B8); //VC
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x7000BC); //set packet size
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x700011); //sleep out cmd

	sunxi_lcd_delay_ms(100);
	spi_24bit_3wire(0x700029); //display on

	sunxi_lcd_delay_ms(200);
	spi_24bit_3wire(0x7000B7); //video mode on
	spi_24bit_3wire(0x72030b);
}


static void lp079x01_exit(void)
{
	spi_24bit_3wire(0x7000B7); //enter LP mode
	spi_24bit_3wire(0x720342);

	sunxi_lcd_delay_ms(50);
	spi_24bit_3wire(0x700028); //display off
	sunxi_lcd_delay_ms(10);	
	spi_24bit_3wire(0x700010); //sleep in cmd
	sunxi_lcd_delay_ms(20);
	ssd2828_rst(0);
	panel_rst(0);
}

static void LCD_cfg_panel_info(__panel_extend_para_t  * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u8 lcd_bright_curve_tbl[][2] =
	{
		//{input value, corrected value}
		{0    ,0  },//0
		{15   ,3  },//0
		{30   ,6  },//0
		{45   ,9  },// 1
		{60   ,12  },// 2
		{75   ,16  },// 5
		{90   ,22  },//9
		{105   ,28 }, //15
		{120  ,36 },//23
		{135  ,44 },//33
		{150  ,54 },
		{165  ,67 },
		{180  ,84 },
		{195  ,108},
		{210  ,137},
		{225 ,171},
		{240 ,210},
		{255 ,255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	memset(info,0,sizeof(__panel_extend_para_t));

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	items = sizeof(lcd_bright_curve_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] - lcd_bright_curve_tbl[i][1]) * j)/num;
			info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}
	info->lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 0);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power0
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power0
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);//open pwm module
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);//close pwm module
}

static void LCD_panel_init(u32 sel)
{
	__panel_para_t* info = kmalloc(sizeof(__panel_para_t), GFP_KERNEL | __GFP_ZERO);
	
	sunxi_lcd_dsi_clk_enable(sel);
    lcd_get_panel_para(sel, info);
    
	lp079x01_init(info);
	kfree(info);
	
	return;
}

static void LCD_panel_exit(u32 sel)
{
	lp079x01_exit();
	return;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t B079XAN01_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "b079xan01",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};

