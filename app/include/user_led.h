#ifndef __USER_LED_H__
#define __USER_LED_H__

#include "gpio.h"


void user_led_init(void);

void user_led_set(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

void RGB2HSL(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint16_t *h_val, uint8_t *s_val, uint8_t *l_val) ;
void HSL2RGB(uint16_t h_val, uint8_t s_val, uint8_t l_val, uint8_t *r_val, uint8_t *g_val, uint8_t *b_val);
double Hue2RGB(double v1, double v2, double vH) ;


#endif
