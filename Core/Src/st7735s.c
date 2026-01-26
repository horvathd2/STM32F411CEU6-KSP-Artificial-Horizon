/*
 * st7735s.c
 *
 *  Created on: Jan 10, 2026
 *      Author: H.Dani
 */

#include "st7735s.h"
#include "main.h"
#include "stdlib.h"
#include "malloc.h"

static void write_cmd(st7735s_t *lcd, uint8_t cmd)
{
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(lcd->st7735s_spi, &cmd, 1, HAL_MAX_DELAY);
    //spi_write_chunked(&lcd->st7735s_spi, &cmd, 1);
}

static void write_data(st7735s_t *lcd, uint8_t data)
{
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(lcd->st7735s_spi, &data, 1, HAL_MAX_DELAY);
    //spi_write_chunked(&lcd->spi, &data, 1);
}

static void write_data_buf(st7735s_t *lcd, uint8_t *buf, size_t len)
{
	HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(lcd->st7735s_spi, buf, len, HAL_MAX_DELAY);
    //spi_write_chunked(&lcd->spi, buf, len);
}

static void cs_low(st7735s_t *lcd) {
    HAL_GPIO_WritePin(lcd->st7735s_cs.gpio_port, lcd->st7735s_cs.gpio_pin, GPIO_PIN_RESET);
}

static void cs_high(st7735s_t *lcd) {
    HAL_GPIO_WritePin(lcd->st7735s_cs.gpio_port, lcd->st7735s_cs.gpio_pin, GPIO_PIN_SET);
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
            cs_low(lcd);
            write_cmd(lcd, cmd);

            for (int i = 1; i < count; i++)
               write_data(lcd, *p++);
        } else {
            // delay
            uint8_t ms = *p++;
            HAL_Delay(ms);
        }
    }

    cs_high(lcd);
}

void st7735s_init(st7735s_t *lcd)
{
	//HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);

	HAL_GPIO_WritePin(lcd->st7735s_rst.gpio_port, lcd->st7735s_rst.gpio_pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(lcd->st7735s_rst.gpio_port, lcd->st7735s_rst.gpio_pin, GPIO_PIN_SET);
	HAL_Delay(20);

	run_init_sequence(lcd);

	//HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 0);
}

void st7735s_set_addr_window(st7735s_t *lcd,
							 uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1)
{
	cs_low(lcd);
    write_cmd(lcd, 0x2A); // CASET
    write_data(lcd, 0);
    write_data(lcd, x0 + ST7735S_X_OFFSET);
    write_data(lcd, 0);
    write_data(lcd, x1 + ST7735S_X_OFFSET);

    write_cmd(lcd, 0x2B); // RASET
    write_data(lcd, 0);
    write_data(lcd, y0 + ST7735S_Y_OFFSET);
    write_data(lcd, 0);
    write_data(lcd, y1 + ST7735S_Y_OFFSET);

    write_cmd(lcd, 0x2C); // RAMWR
    cs_high(lcd);
}

void st7735s_draw_pixel(st7735s_t *lcd,
		                uint8_t x, uint8_t y,
                        uint16_t color)
{
    st7735s_set_addr_window(lcd, x, y, x, y);
    uint8_t buf[2] = { color >> 8, color & 0xFF };
    cs_low(lcd);
    write_data_buf(lcd, buf, 2);
    cs_high(lcd);
}

void st7735s_fill_rect(st7735s_t *lcd,
					   uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h,
                       uint16_t color)
{
    st7735s_set_addr_window(lcd, x, y, x + w - 1, y + h - 1);

    size_t pixels = w * h;
    size_t bytes = pixels * 2;

    uint8_t *buf = malloc(bytes);
    for (int i = 0; i < pixels; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }
    cs_low(lcd);
    write_data_buf(lcd, buf, bytes);
    cs_high(lcd);
    free(buf);
}

void st7735s_fill_screen(st7735s_t *lcd, uint16_t color)
{
    st7735s_fill_rect(lcd, 0, 0,
                      ST7735S_WIDTH,
                      ST7735S_HEIGHT,
                      color);
}

void st7735s_draw_hline(st7735s_t *lcd,
					    int x, int y,
                        int w,
                        uint16_t color)
{
    if (w < 0) {
        x += w;
        w = -w;
    }

    st7735s_set_addr_window(lcd, x, y, x + w - 1, y);
    uint8_t buf[w * 2];

    for (int i = 0; i < w; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }
    cs_low(lcd);
    write_data_buf(lcd, buf, w * 2);
    cs_high(lcd);
}

void st7735s_draw_vline(st7735s_t *lcd,
                        int x, int y,
                        int h,
                        uint16_t color)
{
    if (h < 0) {
        y += h;
        h = -h;
    }

    st7735s_set_addr_window(lcd, x, y, x, y + h - 1);
    uint8_t buf[h * 2];

    for (int i = 0; i < h; i++) {
        buf[2*i]   = color >> 8;
        buf[2*i+1] = color & 0xFF;
    }
    cs_low(lcd);
    write_data_buf(lcd, buf, h * 2);
    cs_high(lcd);
}

void st7735s_push_framebuffer(st7735s_t *lcd,
                              uint16_t *fb,
                              int w, int h)
{
    // Set window to full screen
    st7735s_set_addr_window(lcd, 0, 0, w - 1, h - 1);

    // Data mode
    //gpio_set(lcd->pin_dc, 1);
    HAL_GPIO_WritePin(lcd->st7735s_dc.gpio_port, lcd->st7735s_dc.gpio_pin, 1);

    // Send entire framebuffer in one big SPI write
    //spi_write_chunked(&lcd->spi, (uint8_t*)fb,  w * h * sizeof(uint16_t));
    cs_low(lcd);
    HAL_SPI_Transmit(lcd->st7735s_spi, (uint8_t*)fb, (w * h * sizeof(uint16_t)), HAL_MAX_DELAY);
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

