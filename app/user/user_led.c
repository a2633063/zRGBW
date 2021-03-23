#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "pwm.h"

#include "user_led.h"

#define LED_GRADIENT_STEP 100

static uint8_t r_old;
static uint8_t g_old;
static uint8_t b_old;
static uint8_t w_old;

static uint8_t r_new;
static uint8_t g_new;
static uint8_t b_new;
static uint8_t w_new;

LOCAL os_timer_t timer_led;
void ICACHE_FLASH_ATTR
user_led_timer_func(void *arg0) {
	uint8_t arg = (uint8_t) arg0;
	static uint16_t h = 0;
	static uint8_t step = 0;
	uint8_t r_val, g_val, b_val;
	uint8_t h_val, s_val, l_val;
	if (arg == 1) {

		HSL2RGB(h, 255, 10, &r_val, &g_val, &b_val);
		user_led_set(r_val, g_val, b_val, 0, 0);
		h += 3;
		if (h > 360)
			h %= 360;
	} else if (arg == 2) {
		step++;
		os_printf("[%d] ", step);

		r = r_old + ((int16_t) r_new - (int16_t) r_old)*step/LED_GRADIENT_STEP ;	//+50是为了四舍五入
		g = g_old + ((int16_t) g_new - (int16_t) g_old)*step/LED_GRADIENT_STEP;
		b = b_old + ((int16_t) b_new - (int16_t) b_old)*step/LED_GRADIENT_STEP;
		w = w_old + ((int16_t) w_new - (int16_t) w_old)*step/LED_GRADIENT_STEP;

		user_led_set(r, g, b, w, 0);
		if (step >= LED_GRADIENT_STEP) {
			step = 0;
			user_led_set(r_new, g_new, b_new, w_new, 0);
			os_timer_disarm(&timer_led);
		}

	}
}

void ICACHE_FLASH_ATTR
user_led_init(void) {
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	uint32 pwm_duty_init[4] = { 0, 0, 0, 0 };

	//初始化 PWM，1000周期,pwm_duty_init占空比,3通道数
	uint32 io_info[][3] = { { PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, 15 }, 	//w	r
			{ PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 5 },	//g	g
			{ PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12 }, //r	b
			{ PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, 14 }	//b	w
	};

	//开始初始化
	pwm_init(1000, pwm_duty_init, 4, io_info);
	pwm_start();
}

void ICACHE_FLASH_ATTR
user_led_set(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint8_t w_val, uint8_t is_gradient) {

	if (is_gradient != 1) {
		if (r_val == 0 && g_val == 0 && b_val == 0 && w_val == 00) {
			on = 0;
		} else {
			on = 1;
			r = r_val;
			g = g_val;
			b = b_val;
			w = w_val;
		}

		os_printf("user_led_set:%d,%d,%d,%d\n", r, g, b, w);
		pwm_set_duty((uint32) r_val * 40000 / 459, 0);
		pwm_set_duty((uint32) g_val * 40000 / 459, 1);
		pwm_set_duty((uint32) b_val * 40000 / 459, 2);
		pwm_set_duty((uint32) w_val * 40000 / 459, 3);
		pwm_start();
	} else {
		r_old = r;
		g_old = g;
		b_old = b;
		w_old = w;

		r_new = r_val;
		g_new = g_val;
		b_new = b_val;
		w_new = w_val;

		user_led_timer_func((void *) 2);
		os_timer_disarm(&timer_led);
		os_timer_setfn(&timer_led, (os_timer_func_t *) user_led_timer_func, (void *) 2);
		os_timer_arm(&timer_led, 10, 1);
	}
}

void ICACHE_FLASH_ATTR
user_led_mode_wificonfig() {
	os_printf("user_led_mode_wificonfig\n");
	os_timer_disarm(&timer_led);
	os_timer_setfn(&timer_led, (os_timer_func_t *) user_led_timer_func, (void *) 1);
	os_timer_arm(&timer_led, 10, 1);
}

#define min(a,b)  ((a)<(b)?(a):(b))
#define max(a,b)  ((a)>(b)?(a):(b))
void ICACHE_FLASH_ATTR
RGB2HSL(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint16_t *h_val, uint8_t *s_val, uint8_t *l_val) {
	double R, G, B, Max, Min, del_R, del_G, del_B, del_Max, H = 0, S, L;

//	if (r_val > 255)
//		r_val = 255;
//	if (g_val > 255)
//		g_val = 255;
//	if (b_val > 255)
//		b_val = 255;

	R = (double) r_val / 255.0;       //Where RGB values = 0 ÷ 255
	G = (double) g_val / 255.0;
	B = (double) b_val / 255.0;

	Min = min(R, min(G, B));       //Min. value of RGB
	Max = max(R, max(G, B));       //Max. value of RGB
	del_Max = Max - Min;       //Delta RGB value

	L = (Max + Min) / 2.0;

	if (del_Max == 0) {
		H = 0;
		S = 0;
	} else {
		if (L < 0.5)
			S = del_Max / (Max + Min);
		else
			S = del_Max / (2 - Max - Min);

		del_R = (((Max - R) / 6.0) + (del_Max / 2.0)) / del_Max;
		del_G = (((Max - G) / 6.0) + (del_Max / 2.0)) / del_Max;
		del_B = (((Max - B) / 6.0) + (del_Max / 2.0)) / del_Max;

		if (R == Max)
			H = del_B - del_G;
		else if (G == Max)
			H = (1.0 / 3.0) + del_R - del_B;
		else if (B == Max)
			H = (2.0 / 3.0) + del_G - del_R;

		if (H < 0)
			H += 1;
		if (H > 1)
			H -= 1;
	}

	*h_val = (uint16_t) (H * 360.0);
	*s_val = (uint8_t) (S * 255.0);
	*l_val = (uint8_t) (L * 255.0);
}

void ICACHE_FLASH_ATTR
HSL2RGB(uint16_t h_val, uint8_t s_val, uint8_t l_val, uint8_t *r_val, uint8_t *g_val, uint8_t *b_val) {
	double R, G, B, H, S, L;
	double var_1, var_2;

	if (h_val > 360)
		h_val %= 360;
//	if (s_val > 100)
//		s_val = 100;
//	if (l_val > 100)
//		l_val = 100;

	H = (double) h_val / 360.0;
	S = (double) s_val / 255.0;
	L = (double) l_val / 255.0;

	if (S == 0) {
		R = L * 255.0;                   //RGB results = 0 ÷ 255
		G = L * 255.0;
		B = L * 255.0;
	} else {
		if (L < 0.5)
			var_2 = L * (1 + S);
		else
			var_2 = (L + S) - (S * L);

		var_1 = 2.0 * L - var_2;

		R = 255.0 * Hue2RGB(var_1, var_2, H + (1.0 / 3.0));
		G = 255.0 * Hue2RGB(var_1, var_2, H);
		B = 255.0 * Hue2RGB(var_1, var_2, H - (1.0 / 3.0));
	}

	*r_val = (uint8_t) (R + 0.5);
	*g_val = (uint8_t) (G + 0.5);
	*b_val = (uint8_t) (B + 0.5);

}
//---------------------------------------------------------------------------
double ICACHE_FLASH_ATTR
Hue2RGB(double v1, double v2, double vH) {
	if (vH < 0)
		vH += 1;
	if (vH > 1)
		vH -= 1;
	if (6.0 * vH < 1)
		return v1 + (v2 - v1) * 6.0 * vH;
	if (2.0 * vH < 1)
		return v2;
	if (3.0 * vH < 2)
		return v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0;
	return (v1);
}
