/*
 * st7735s.h
 *
 *  Created on: Jan 10, 2026
 *      Author: H.Dani
 */

#ifndef INC_ST7735S_H_
#define INC_ST7735S_H_

#include "main.h"

// Display resolution (common ST7735S)
#define ST7735S_WIDTH   128
#define ST7735S_HEIGHT  160

#define ST7735S_X_OFFSET 2
#define ST7735S_Y_OFFSET 1

typedef struct {
	GPIO_TypeDef *gpio_port;
	uint16_t gpio_pin;
} GPIO_st7735s;

typedef struct {
	SPI_HandleTypeDef *st7735s_spi;
	GPIO_st7735s st7735s_dc;
	GPIO_st7735s st7735s_rst;
	GPIO_st7735s st7735s_cs;

	volatile uint8_t busy;
} st7735s_t;

st7735s_t st7735s_create(SPI_HandleTypeDef *lcd_spi,
						 GPIO_st7735s lcd_dc,
						 GPIO_st7735s lcd_rst,
						 GPIO_st7735s lcd_cs);

void st7735s_init(st7735s_t *lcd);

void st7735s_set_addr_window(st7735s_t *lcd,
							 uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1);

void st7735s_draw_pixel(st7735s_t *lcd,
		                uint8_t x, uint8_t y,
                        uint16_t color);

void st7735s_fill_rect(st7735s_t *lcd,
					   uint8_t x, uint8_t y,
                       uint8_t w, uint8_t h,
                       uint16_t color);

void st7735s_fill_screen(st7735s_t *lcd, uint16_t color);

void st7735s_push_framebuffer(st7735s_t *lcd,
                              uint16_t *fb,
                              int w, int h);

void st7735s_draw_line(st7735s_t *lcd,
                       int x0, int y0,
                       int x1, int y1,
                       uint16_t color);

void st7735s_draw_circle(st7735s_t *lcd, uint8_t radius,
                         uint16_t X0, uint16_t Y0,
                         uint16_t color);

void st7735s_push_framebuffer_dma(st7735s_t *lcd,
                                  uint16_t *fb,
                                  int w, int h);

void st7735s_dma_tx_complete(st7735s_t *lcd);

#endif /* INC_ST7735S_H_ */
