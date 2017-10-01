/*
 * Solomon Systec Products SSD1306 128x64/32 Dot Matric
 * OLED/PLED Segment/Common Driver Controller
 * http://www.solomon-systech.com/
 *
 * Driver for Raspberry Pi 3 written by
 * Kai Harrekilde-Petersen (C) 2017
 *
 * 0.96" module bought off Aliexpress.com
 * Connect VCC to 5V as the module has internal 3.3V LDO (Torex XC6206)
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "i2c.h"
#include "ssd1306.h"
#include "ssd1306_regs.h"

#define VERSION "0.0.1"

#include "font_8x8.h"

uint8_t *gfxbuf = NULL;

uint8_t img8[8] = {0x7F, 0x09, 0x09, 0x9, 0x09, 0x01, 0x01, 0x00};

uint8_t img16[32] = {
  0xFF, 0xFF, 0xFF, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, /* Upper 8 pixel rows */
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00,
  0x7F, 0x7F, 0x7F, 0x01, 0x01, 0x01, 0x01, 0x01, /* Lower 8 pixel rows */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void ssd130x_power(int fd, int on)
{
  /* Powering up not controlled in SW */
  return;
}

void ssd130x_reset(int fd)
{
  /* No-op */
  return;
}

void ssd_cmd1(int fd, uint8_t c0)
{
  i2c_wr8(fd, SSD_C, c0);
}

void ssd_cmd2(int fd, uint8_t c0, uint8_t c1)
{
  i2c_wr16(fd, SSD_C, (c1<<8) | c0);
}

void ssd_cmd3(int fd, uint8_t c0, uint8_t c1, uint8_t c2)
{
  uint8_t cmd[3] = {c0, c1, c2};
  i2c_wr_blk(fd, SSD_C, 3, (uint8_t *)&cmd);
}

void ssd_cmd_blk(int fd, int len, uint8_t *cmd)
{
  i2c_wr_blk(fd, SSD_C, len, cmd);
}

void ssd_dat1(int fd, uint8_t d0)
{
  i2c_wr8(fd, SSD_D, d0);
}

void ssd_dat2(int fd, uint8_t d0, uint8_t d1)
{
  i2c_wr16(fd, SSD_D, (d1 << 8) | d0);
}

void ssd_dat_blk(int fd, int len, uint8_t *data)
{
  i2c_wr_blk(fd, SSD_D, len, data);
}

void ssd130x_init(int fd)
{
/*
 * Software Configuration, according to <SSD1306.pdf, pg64>;
 * <UG-2832HSWEG02.pdf, pg10>, <UG-2832HSWEG04.pdf, pg11>,
 * <UG-2842HSWEG01+user+guide.pdf, pg 19>:
 *  1) Set Display OFF (0xAE)
 *  2) Set Column Page Lo = 0
 *  3) Set Column Page Hi = 0
 *  4) Addressing Mode = PAGE
 *  5) Set Display Start Line (0x40)
 *  6) Set Contrast Control (0x81, 0x8F)
 *  7) Set Segment re-map (0xA1)
 *  8) Set COM Output Scan Dir (0xC8)
 *  9) Set MUX ratio (0xA8, 0x1F) [x32, x64=0x3F]
 * 10) Set Display offset (0xD3, 0x00)
 * 11) Set Display Clock (0xD5, 0x80)
 * 12) Set precharge period (0xD9, 0xF1)
 * 13) Set COM pin HW config (0xDA, 0x02)
 * 14) Set VCOMH deselect level (0xDB, 0x40)
 * 15) Set Charge-pump Internal (0x8D, 0x14)
 * 16) Set Display Normal (0xA6)
 * 17) Set Entire display on (0xA4)
 * 18) Set Display ON (0xAF)
 * 19) /Clear Screen/
 */
#if 0
  /* Code from UG-2864HSWEG01+user+guide.pdf, pg 19 */
  ssd_cmd1(fd, SSD_DISP_SLEEP);        /* 0xAE: DISP_ENTIRE_OFF */
  ssd_cmd1(fd, SSD_COL_PAGE_LO);       /* 0x00: SET_COL_PAGE_LO = 0 */
  ssd_cmd1(fd, SSD_COL_PAGE_HI);       /* 0x10: SET_COL_PAGE_HI = 0 */
  ssd_cmd2(fd, SSD_ADDR_MODE, 0);      /* 0x20: ADDR_MODE = 0h */
  ssd_cmd2(fd, SSD_CONTRAST, 0xCF);    /* 0x81: CONTRAST = 0xCF */
  ssd_cmd2(fd, SSD_MUX_RATIO, 0x1F);   /* 0xA8: MUX_RATIO = 0x1F (x32 display) */
  ssd_cmd1(fd, SSD_SEG_REMAP127);      /* 0xA1: SEG_REMAP = Y */
  ssd_cmd1(fd, SSD_COM_SCAN_NORM);     /* 0xC8: COM_SCAN = Normal (0..7) */
  ssd_cmd2(fd, SSD_DISP_OFFSET, 0);    /* 0xD3: DISP_OFFSET = 0 */
  ssd_cmd1(fd, SSD_DISP_ST_LINE | 0);  /* 0x40: DISP_STARTLINE = 0 */
  ssd_cmd2(fd, SSD_DCLK_DIV, 0x80);    /* 0xD5: DCLK_DIV = 0x80 */
  ssd_cmd2(fd, SSD_PRECHARGE, 0xF1);   /* 0xD9: PRECHARGE = 0xF1 */
  ssd_cmd2(fd, SSD_COM_HW_CFG, 0x02);  /* 0xDA: COM_HW PINS = 0x02 */
  ssd_cmd2(fd, SSD_VCOM_LVL, 0x40);    /* 0xDB: VCOMH LEVEL = 0x40 */
  ssd_cmd2(fd, SSD_CHARGEPUMP, 0x14);  /* 0x8D: CHARGEPUMP = 0x14 */
  ssd_cmd1(fd, SSD_DISP_INV);         /* 0xA6: DISP_NORMAL */
  ssd_cmd1(fd, SSD_DISP_ENT_NORM);     /* 0xA4: DISP_ENTIRE_RESUME */
  ssd_cmd1(fd, SSD_DISP_AWAKE);        /* 0xAF: DISP = ON */
#endif
#if 1
  /* Code from UG-2864HSWEG01+user+guide.pdf, pg 19 */
  ssd_cmd1(fd, SSD_DISP_SLEEP);        /* 0xAE: DISP_ENTIRE_OFF */
  ssd_cmd1(fd, SSD_COL_PAGE_LO);       /* 0x00: SET_COL_PAGE_LO = 0 */
  ssd_cmd1(fd, SSD_COL_PAGE_HI);       /* 0x10: SET_COL_PAGE_HI = 0 */
  ssd_cmd2(fd, SSD_ADDR_MODE, 0);      /* 0x20: ADDR_MODE = 0h */
  ssd_cmd1(fd, SSD_DISP_ST_LINE | 0);  /* 0x40: DISP_STARTLINE = 0 */
  ssd_cmd2(fd, SSD_CONTRAST, 0xCF);    /* 0x81: CONTRAST = 0xCF */
  ssd_cmd1(fd, SSD_SEG_REMAP127);      /* 0xA1: SEG_REMAP = Y */
  ssd_cmd1(fd, SSD_COM_SCAN_NORM);     /* 0xC8: COM_SCAN = Normal (0..7) */
  ssd_cmd1(fd, SSD_DISP_NORM);         /* 0xA6: DISP_NORMAL */
  ssd_cmd2(fd, SSD_MUX_RATIO, 0x1F);   /* 0xA8: MUX_RATIO = 0x1F (x32 display) */
  ssd_cmd2(fd, SSD_DISP_OFFSET, 0);    /* 0xD3: DISP_OFFSET = 0 */
  ssd_cmd2(fd, SSD_DCLK_DIV, 0x80);    /* 0xD5: DCLK_DIV = 0x80 */
  ssd_cmd2(fd, SSD_PRECHARGE, 0xF1);   /* 0xD9: PRECHARGE = 0xF1 */
  ssd_cmd2(fd, SSD_COM_HW_CFG, 0x02);  /* 0xDA: COM_HW PINS = 0x02 */
  ssd_cmd2(fd, SSD_VCOM_LVL, 0x40);    /* 0xDB: VCOMH LEVEL = 0x40 */
  ssd_cmd2(fd, SSD_CHARGEPUMP, 0x14);  /* 0x8D: CHARGEPUMP = 0x14 */
  ssd_cmd1(fd, SSD_DISP_ENT_NORM);     /* 0xA4: DISP_ENTIRE_RESUME */
  ssd_cmd1(fd, SSD_DISP_AWAKE);        /* 0xAF: DISP = ON */
#endif

  /* If not allocated, allocate gfx buffer; else: zero contents */
  if(NULL==gfxbuf) {
    gfxbuf = calloc((size_t) SSD_LCD_WIDTH*SSD_LCD_HEIGHT/4, sizeof(uint8_t));
    if(NULL==gfxbuf) {
      puts("Unable to allocate memory for graphics buffer");
      exit(-1);
    }
  } else {
    memset(gfxbuf, 0, (size_t) SSD_LCD_WIDTH*SSD_LCD_HEIGHT/4);
  }

  /* Clear GDDRAM memory */
  ssd_disp_init();

  ssd_disp_update(fd);

#if 0
  /* Horizontal scroll */
  ssd_cmd1(fd, SSD_SCROLL_OFF);
  uint8_t hscroll[] = {SSD_SCROLL_H_R, 0, 0, 0, 3, 0, 0xFF};
  ssd_cmd_blk(fd, sizeof(hscroll), (uint8_t *)&hscroll);
  ssd_cmd1(fd, SSD_SCROLL_ON);
#endif

  /* 2017/09/29 22:54: Changing SSD_DISP_OFFSET (Cmd 0xD3) has no impact on OLED output */

#if 0
  int i;
  for(i=0;i<SSD_LCD_HEIGHT;i++) {
    ssd_cmd1(fd, SSD_DISP_ST_LINE | i);
    usleep(100000);
  }

#endif

#if 0
  int i;
  for(i=0;i<SSD_LCD_HEIGHT;i++) {
    ssd_cmd1(fd, SSD_DISP_ST_LINE | i);
    usleep(100000);
  }
  ssd_cmd1(fd, SSD_DISP_ST_LINE | 0);
#endif
}

#if 0
/* DON'T TOUCH, THIS ONE WORKS! */
/* Code from UG-2864HSWEG01+user+guide.pdf, pg 19 */
  ssd_cmd1(fd, 0xAE);       /* DISP_ENTIRE_OFF */
  ssd_cmd1(fd, 0x00);       /* SET_COL_PAGE_LO = 0 */
  ssd_cmd1(fd, 0x10);       /* SET_COL_PAGE_HI = 0 */
  ssd_cmd1(fd, 0x40);       /* DISP_STARTLINE = 0 */
  ssd_cmd2(fd, 0x20, 0x00); /* ADDR_MODE = 0h */
  ssd_cmd2(fd, 0x81, 0xCF); /* CONTRAST = 0xCF */
  ssd_cmd1(fd, 0xA1);       /* SEG_REMAP = 1 */
  ssd_cmd1(fd, 0xC8);       /* COM_SCAN = REVERSE */
  ssd_cmd1(fd, 0xA6);       /* DISP_NORMAL */
  ssd_cmd2(fd, 0xA8, 0x1F); /* MUX_RATIO = 0x1F */
  ssd_cmd2(fd, 0xD3, 0x00); /* DISP_OFFSET = 0 */
  ssd_cmd2(fd, 0xD5, 0x80); /* DCLK_DIV = 0x80 */
  ssd_cmd2(fd, 0xD9, 0xF1); /* PRECHARGE = 0xF1 */
  ssd_cmd2(fd, 0xDA, 0x02); /* COM_HW PINS = 0x02 */
  ssd_cmd2(fd, 0xDB, 0x40); /* VCOMH LEVEL = 0x40 */
  ssd_cmd2(fd, 0x8D, 0x14); /* CHARGEPUMP = 0x14 */
  ssd_cmd1(fd, 0xA6);       /* DISP_NORMAL */
  ssd_cmd1(fd, 0xA4);       /* DISP_ENTIRE_RESUME */
  ssd_cmd1(fd, 0xAF);       /* DISP = ON */
#endif  

void ssd_disp_awake(int fd, int on)
{
  if(on) {
    i2c_wr8(fd, SSD_C, SSD_DISP_AWAKE);
  } else {
    i2c_wr8(fd, SSD_C, SSD_DISP_SLEEP);
  }
}

void ssd_disp_update(int fd)
{
  int i;
  int len=I2C_BLK_MAX;
  for(i=0;i<SSD_LCD_WIDTH*SSD_LCD_HEIGHT/4;i+=len) {
    ssd_dat_blk(fd, len, (uint8_t *) &gfxbuf[i]);
  }
}
void ssd_disp_init()
{
  static uint8_t blank[SSD_LCD_HEIGHT*SSD_LCD_WIDTH/8] = {
    /* Row 0-7, col 0-127, bit0 = topmost row, bit7 = lowest row */
    0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    /* Row 8-15, col 0-127 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Row 16-23, col 0-127 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Row 24-31, col 0-127 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  static uint8_t adafruit[SSD_LCD_HEIGHT*SSD_LCD_WIDTH/8] = {
    /* Row 0-7, col 0-127 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x80, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Row 8-15, col 0-127 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00,
    0x80, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x8C, 0x8E, 0x84, 0x00, 0x00, 0x80, 0xF8,
    0xF8, 0xF8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Row 16-23, col 0-127 */
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x80,
    0x00, 0xE0, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xC7, 0x01, 0x01,
    0x01, 0x01, 0x83, 0xFF, 0xFF, 0x00, 0x00, 0x7C, 0xFE, 0xC7, 0x01, 0x01, 0x01, 0x01, 0x83, 0xFF,
    0xFF, 0xFF, 0x00, 0x38, 0xFE, 0xC7, 0x83, 0x01, 0x01, 0x01, 0x83, 0xC7, 0xFF, 0xFF, 0x00, 0x00,
    0x01, 0xFF, 0xFF, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0x07, 0x01, 0x01, 0x01, 0x00, 0x00, 0x7F, 0xFF,
    0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0xFF,
    0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Row 24-31, col 0-127 */
    0x03, 0x0F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC7, 0xC7, 0x8F,
    0x8F, 0x9F, 0xBF, 0xFF, 0xFF, 0xC3, 0xC0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC, 0xFC,
    0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xC0, 0x00, 0x01, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x03, 0x03, 0x00, 0x00,
    0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x00, 0x00, 0x00, 0x03,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int i;
  static int len=32;
#if 0
  ssd_cmd2(fd, SSD_ADDR_MODE, ADDR_MODE_PAGE);
  ssd_cmd1(fd, SSD_COL_PAGE_LO);
  ssd_cmd1(fd, SSD_COL_PAGE_HI);
#endif
#if 0
  memcpy(gfxbuf,&blank,sizeof(blank)); /* dst, src, sz */
  memcpy(gfxbuf+sizeof(blank),&blank,sizeof(blank)); /* dst, src, sz */
#endif
#if 1
  memcpy(gfxbuf,&adafruit,sizeof(adafruit)); /* dst, src, sz */
  memcpy(gfxbuf+sizeof(adafruit),&adafruit,sizeof(adafruit)); /* dst, src, sz */
#endif
}

int width()
{
  return SSD_LCD_WIDTH;
}

int height()
{
  return SSD_LCD_HEIGHT;
}

// the most basic function, set a single pixel
void ssd_plot(int16_t x, int16_t y, int col) 
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
    return;
}

void ssd_coor(uint8_t row, uint8_t col)
{
#if 0
  const uint8_t cmd[] = {SSD_COL_PAGE_HI+5, SSD_COL_PAGE_LO+4*col,
			 SSD_
    //Column Address
    sendCommand(0x15);             /* Set Column Address */
    sendCommand(0x08+(Column*4));  /* Start Column: Start from 8 */
    sendCommand(0x37);             /* End Column */
    // Row Address
    sendCommand(0x75);             /* Set Row Address */
    sendCommand(0x00+(Row*8));     /* Start Row*/
    sendCommand(0x07+(Row*8));     /* End Row*/
#endif
}

void ssd_putc(char ch)
{
  /* All characters 0-255 are implemented */
  if(ch<0 || ch>255) {
    ch=' '; //Space
  }
#if 0
  for(char i=0;i<8;i=i+2) {
    for(char j=0;j<8;j++) {
      /* Character is constructed two pixel at a time using vertical mode from the default 8x8 font */
      char c=0x00;
      char bit1= (seedfont[ch][i]   >> j) & 0x01;  
      char bit2= (seedfont[ch][i+1] >> j) & 0x01;
      /* Each bit is changed to a nibble */
      c|=(bit1)?grayH:0x00;
      c|=(bit2)?grayL:0x00;
      sendData(c);
    }
  }
#endif
}

void ssd_puts(const char *str)
{
  unsigned char i=0;
  while(str[i]) {
    ssd_putc(str[i]);     
    i++;
  }
}
 
int main (void)
{
  int disp = i2c_init_dev(RA_SSD1306);
  if(-1==disp) {
    puts("Failed to setup OLED display\n");
    exit(-1);
  }
  ssd130x_init(disp);
  exit(0);
}

/* ************************************************************
 * End of file.
 * ************************************************************/
