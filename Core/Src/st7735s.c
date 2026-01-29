/*
 * st7735s.c
 *
 *  Created on: Jan 10, 2026
 *      Author: H.Dani
 */

#include "st7735s.h"
#include "main.h"
#include "stdlib.h"

static void cs_low(st7735s_t *lcd) {
    HAL_GPIO_WritePin(lcd->st7735s_cs.gpio_port, lcd->st7735s_cs.gpio_pin, 0);
}

static void cs_high(st7735s_t *lcd) {
    HAL_GPIO_WritePin(lcd->st7735s_cs.gpio_port, lcd->st7735s_cs.gpio_pin, 1);
}

static void write_cmd(st7735s_t *lcd, uint8_t cmd)
{
	cs_low(lcd);
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 0);
	HAL_SPI_Transmit(lcd->st7735s_spi, &cmd, 1, HAL_MAX_DELAY);
	cs_high(lcd);
}

static void write_data(st7735s_t *lcd, uint8_t data)
{
	cs_low(lcd);
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);
	HAL_SPI_Transmit(lcd->st7735s_spi, &data, 1, HAL_MAX_DELAY);
	cs_high(lcd);
}

static void write_data_buf(st7735s_t *lcd, uint8_t *buf, size_t len)
{
	cs_low(lcd);
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);
	HAL_SPI_Transmit(lcd->st7735s_spi, buf, len, HAL_MAX_DELAY);
	cs_high(lcd);
}

static const uint8_t init_seq[] = {
    // SWRESET
    1, 0x01,
    0, 150,   // delay 150ms

    // SLPOUT
    1, 0x11,
    0, 150,

    // COLMOD = 16-bit
    2, 0x3A, 0x05,

    // MADCTL = row/column order
    2, 0x36, 0xC8,  // RGB order + orientation

    // DISPON
    1, 0x29,
    0, 100,

    0xFF           // end marker
};

st7735s_t st7735s_create(SPI_HandleTypeDef *lcd_spi,
						 GPIO_st7735s lcd_dc,
						 GPIO_st7735s lcd_rst,
						 GPIO_st7735s lcd_cs)
{
	st7735s_t lcd;

	lcd.st7735s_spi = lcd_spi;
	lcd.st7735s_dc = lcd_dc;
	lcd.st7735s_rst = lcd_rst;
	lcd.st7735s_cs = lcd_cs;
	lcd.busy = 0;

	return lcd;
}

static void run_init_sequence(st7735s_t *lcd)
{
    const uint8_t *p = init_seq;

    while (1) {
        uint8_t count = *p++;

        if (count == 0xFF)
            break;

        if (count > 0) {
            uint8_t cmd = *p++;
            write_cmd(lcd, cmd);

            for (int i = 1; i < count; i++)
               write_data(lcd, *p++);
        } else {
            // delay
            uint8_t ms = *p++;
            HAL_Delay(ms);
        }
    }
}

void st7735s_init(st7735s_t *lcd)
{
	HAL_GPIO_WritePin(lcd->st7735s_rst.gpio_port, lcd->st7735s_rst.gpio_pin, 0);
	HAL_Delay(20);
	HAL_GPIO_WritePin(lcd->st7735s_rst.gpio_port, lcd->st7735s_rst.gpio_pin, 1);
	HAL_Delay(20);

	run_init_sequence(lcd);
}

void st7735s_set_addr_window(st7735s_t *lcd,
							 uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1)
{
//    write_cmd(lcd, 0x2A); // CASET
//    write_data(lcd, 0);
//    write_data(lcd, x0 + ST7735S_X_OFFSET);
//    write_data(lcd, 0);
//    write_data(lcd, x1 + ST7735S_X_OFFSET);
//
//    write_cmd(lcd, 0x2B); // RASET
//    write_data(lcd, 0);
//    write_data(lcd, y0 + ST7735S_Y_OFFSET);
//    write_data(lcd, 0);
//    write_data(lcd, y1 + ST7735S_Y_OFFSET);
//
//    write_cmd(lcd, 0x2C); // RAMWR

	cs_low(lcd);

	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 0);
	uint8_t cmd = 0x2A;
	HAL_SPI_Transmit(lcd->st7735s_spi, &cmd, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);
	uint8_t data[] = {
			0, x0 + ST7735S_X_OFFSET,
			0, x1 + ST7735S_X_OFFSET
	};
	HAL_SPI_Transmit(lcd->st7735s_spi, data, sizeof(data), HAL_MAX_DELAY);

	cs_high(lcd);

	cs_low(lcd);

	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 0);
	uint8_t cmd2 = 0x2B;
	HAL_SPI_Transmit(lcd->st7735s_spi, &cmd2, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);
	uint8_t data2[] = {
			0, y0 + ST7735S_Y_OFFSET,
			0, y1 + ST7735S_Y_OFFSET
	};
	HAL_SPI_Transmit(lcd->st7735s_spi, data2, sizeof(data2), HAL_MAX_DELAY);

	cs_high(lcd);

	write_cmd(lcd, 0x2C); // RAMWR
}

void st7735s_draw_pixel(st7735s_t *lcd,
		                uint8_t x, uint8_t y,
                        uint16_t color)
{
    st7735s_set_addr_window(lcd, x, y, x, y);
    uint8_t buf[2] = { color >> 8, color & 0xFF };
    write_data_buf(lcd, buf, 2);
}

void st7735s_fill_rect(st7735s_t *lcd,
					   uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h,
                       uint16_t color)
{
    uint8_t line[2 * w];

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (uint8_t i = 0; i < w; i++) {
        line[2*i]     = hi;
        line[2*i + 1] = lo;
    }

    st7735s_set_addr_window(lcd, x, y, x + w - 1, y + h - 1);

    for (uint8_t row = 0; row < h; row++) {
        write_data_buf(lcd, line, sizeof(line));
    }
}

void st7735s_fill_screen(st7735s_t *lcd, uint16_t color)
{
    st7735s_fill_rect(lcd, 0, 0,
                      ST7735S_WIDTH,
                      ST7735S_HEIGHT,
                      color);
}

void st7735s_push_framebuffer(st7735s_t *lcd,
                              uint16_t *fb,
                              int w, int h)
{
    // Set window to full screen
    st7735s_set_addr_window(lcd, 0, 0, w - 1, h - 1);

    // Data mode
    HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);

    // Send entire framebuffer in one big SPI write
    cs_low(lcd);
    HAL_SPI_Transmit(lcd->st7735s_spi, (uint8_t*)fb, (w * h * sizeof(uint16_t)), HAL_MAX_DELAY); // ADD DMA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    cs_high(lcd);
}

void st7735s_draw_line(st7735s_t *lcd,
                       int x0, int y0,
                       int x1, int y1,
                       uint16_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;
    int e2;

    while (1) {
        st7735s_draw_pixel(lcd, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void st7735s_draw_circle(st7735s_t *lcd, uint8_t radius,
                         uint16_t X0, uint16_t Y0,
                         uint16_t color){
	int x = 0;
	int y = radius;
	int d = 3-2*radius;

	while(x<=y){
		st7735s_draw_pixel(lcd, X0 + x, Y0 + y, color);
		st7735s_draw_pixel(lcd, X0 - x, Y0 + y, color);
		st7735s_draw_pixel(lcd, X0 + x, Y0 - y, color);
		st7735s_draw_pixel(lcd, X0 - x, Y0 - y, color);

		st7735s_draw_pixel(lcd, X0 + y, Y0 + x, color);
		st7735s_draw_pixel(lcd, X0 - y, Y0 + x, color);
		st7735s_draw_pixel(lcd, X0 + y, Y0 - x, color);
		st7735s_draw_pixel(lcd, X0 - y, Y0 - x, color);

		if(d < 0){
			d+=4*x+6;
		}else{
			d+=4*(x-y)+10;
			y--;
		}

		x++;
	}
}

void st7735s_push_framebuffer_dma(st7735s_t *lcd,
                                  uint16_t *fb,
                                  int w, int h)
{
	if (lcd->busy) return;   // or block, or return error

	lcd->busy = 1;

	st7735s_set_addr_window(lcd, 0, 0, w - 1, h - 1);

	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);

	cs_low(lcd);

	HAL_SPI_Transmit_DMA(lcd->st7735s_spi, (uint8_t *)fb, w * h * 2);
}

void st7735s_dma_tx_complete(st7735s_t *lcd)
{
    cs_high(lcd);
    lcd->busy = 0;
}
