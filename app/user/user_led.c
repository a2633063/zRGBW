#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "pwm.h"

#include "user_led.h"

#define LED_GRADIENT_STEP 100

static uint8_t r_old;	//记录渐变前开始显示的颜色,可以为0
static uint8_t g_old;
static uint8_t b_old;
static uint8_t w_old;

static uint8_t r_real;	//记录当前颜色,包含渐变过程
static uint8_t g_real;
static uint8_t b_real;
static uint8_t w_real;
static bool led_init_flag = false;
LOCAL os_timer_t timer_led;

uint32_t gpio[][3] = { { PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0, 0 },  //GPIO0
        { 0, 0, 0 },  //PERIPHS_IO_MUX_U0TXD_U,FUNC_GPIO1, 1  //GPIO1 不可使用
        { PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, 2 },  //GPIO2
        { 0, 0, 0 },  //PERIPHS_IO_MUX_U0RXD_U,FUNC_GPIO3,3 //GPIO3 不可使用
        { PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, 4 },   //GPIO4
        { PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 5 },   //GPIO5
        { 0, 0, 0 },  //GPIO6
        { 0, 0, 0 },  //GPIO7
        { 0, 0, 0 },  //GPIO8
        { PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9, 9 },   //GPIO9
        { PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10, 10 },   //GPIO10
        { 0, 0, 0 },  //GPIO11
        { PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12 },   //GPIO12
        { PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, 13 },   //GPIO13
        { PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, 14 },   //GPIO14
        { PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, 15 },   //GPIO15
        };
/**
 * 函  数  名: user_led_timer_func
 * 函数说明: led定时器回调函数.led动态变化效果
 * 参        数: 无
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR
user_led_timer_func(void *arg0)
{
    uint8_t arg = (uint8_t) arg0;
    static uint16_t h = 0;
    static uint8_t step = 0;
    uint8_t r_val, g_val, b_val, w_val;
    uint8_t h_val, s_val, l_val;
    if(arg == 1)
    {

        HSL2RGB(h, 255, 10, &r_val, &g_val, &b_val);
        user_led_set(r_val, g_val, b_val, 0, 0);
        h += 3;
        if(h > 360) h %= 360;
    }
    else if(arg == 2)
    {
        step++;
        //os_printf("[%d] ", step);

        r_val = r_old + ((int16_t) r_now - (int16_t) r_old) * step / LED_GRADIENT_STEP;	//+50是为了四舍五入
        g_val = g_old + ((int16_t) g_now - (int16_t) g_old) * step / LED_GRADIENT_STEP;
        b_val = b_old + ((int16_t) b_now - (int16_t) b_old) * step / LED_GRADIENT_STEP;
        w_val = w_old + ((int16_t) w_now - (int16_t) w_old) * step / LED_GRADIENT_STEP;

        user_led_set_temp(r_val, g_val, b_val, w_val);
        if(step >= LED_GRADIENT_STEP)
        {
            step = 0;
            user_led_set_temp(r_now, g_now, b_now, w_now);
            os_timer_disarm(&timer_led);
        }

    }
}
/**
 * 函  数  名: user_led_gpio_item
 * 函数说明: 根据配置信息,设置led的pwm GPIO口
 * 参        数: item: 0-3:分别为RGBW
 * 返        回: 无
 */
static uint8_t ICACHE_FLASH_ATTR
user_led_gpio_item(uint8_t item)
{
    if(item > 3) return 255;
    //gpio编号超出范围时直接返回
//    if(user_config.gpio[item] > 16 && user_config.gpio[item] < 100) return 255;
//    if(user_config.gpio[item] > 116) return 255;
//    if(user_config.gpio[item] >= 100) return user_config.gpio[item] - 100;

    if(user_config.gpio[item] <= 16) return user_config.gpio[item];

    return 255;

}
/**
 * 函  数  名: user_led_gpio_config
 * 函数说明: 根据配置信息,设置led的pwm GPIO口
 * 参        数: 无
 * 返        回: 无
 */
bool ICACHE_FLASH_ATTR
user_led_gpio_config(void)
{
    int i, j;
    uint32 pwm_duty_init[4] = { 0, 0, 0, 0 };

    //初始化 PWM，1000周期,pwm_duty_init占空比,3通道数
    uint32 io_info[][3] = { { PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, 15 },     //w r
            { PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 5 },  //g g
            { PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12 }, //r b
            { PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, 14 }  //b w
    };

    for (i = 0; i < 4; i++)
    {
        if(user_led_gpio_item(i) > 16) return false;

        if(gpio[user_led_gpio_item(i)][0] == 0) return false;

        for (j = 0; j < 3; j++)
        {
            io_info[i][j] = gpio[user_led_gpio_item(i)][j];
        }
    }

    //开始初始化
    pwm_init(1000, pwm_duty_init, 4, io_info);
    pwm_start();
    led_init_flag = true;
    return true;
}
void ICACHE_FLASH_ATTR
user_led_init(void)
{
    user_led_gpio_config();
    user_led_set(0,0,0,255,0);
}
/**
 * 函  数  名: user_led_set_temp
 * 函数说明: 设置led当前颜色,立刻生效
 * 参        数:
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR
user_led_set_temp(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint8_t w_val)
{
    //os_printf("user_led_set:%d,%d,%d,%d\n", r_val, g_val, b_val, w_val);
    r_real = r_val;
    g_real = g_val;
    b_real = b_val;
    w_real = w_val;
    if(!led_init_flag)
    {
        os_printf("user led gpio fail!\n");
        return;
    }

    pwm_set_duty((uint32) r_val * 40000 / 459, 0);  //r_val * 1000*1000/45/255 => r_val * 40000 / 459
    pwm_set_duty((uint32) g_val * 40000 / 459, 1);
    pwm_set_duty((uint32) b_val * 40000 / 459, 2);
    pwm_set_duty((uint32) w_val * 40000 / 459, 3);

    pwm_start();
}
/**
 * 函  数  名: user_led_set
 * 函数说明: 设置led当前颜色(动态变化)
 * 参        数:
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR
user_led_set(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint8_t w_val, uint8_t is_gradient)
{
    os_printf("user_led_set:%d,%d,%d,%d   %d\n", r_val, g_val, b_val, w_val,is_gradient);
    if(is_gradient != 1)
    {
        if(r_val == 0 && g_val == 0 && b_val == 0 && w_val == 00)
        {
            on = 0;
            r_now = 0;
            g_now = 0;
            b_now = 0;
            w_now = 0;
        }
        else
        {
            on = 1;
            r = r_val;
            g = g_val;
            b = b_val;
            w = w_val;
            r_now = r;
            g_now = g;
            b_now = b;
            w_now = w;
        }
        user_led_set_temp(r_now, g_now, b_now, w_now);

    }
    else
    {

        r_old = r_real;
        g_old = g_real;
        b_old = b_real;
        w_old = w_real;

        r_now = r_val;
        g_now = g_val;
        b_now = b_val;
        w_now = w_val;

        if(r_val != 0 || g_val != 0 || b_val != 0 || w_val != 0)
        {
            on = 1;
            r = r_val;
            g = g_val;
            b = b_val;
            w = w_val;
        }
        else
        {
            on = 0;
        }

        user_led_timer_func((void *) 2);
        os_timer_disarm(&timer_led);
        os_timer_setfn(&timer_led, (os_timer_func_t *) user_led_timer_func, (void *) 2);
        os_timer_arm(&timer_led, 10, 1);
    }
}
/**
 * 函  数  名: user_led_mode_wificonfig
 * 函数说明: ap模式时led显示效果
 * 参        数: 无
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR
user_led_mode_wificonfig()
{
    os_printf("user_led_mode_wificonfig\n");
    os_timer_disarm(&timer_led);
    os_timer_setfn(&timer_led, (os_timer_func_t *) user_led_timer_func, (void *) 1);
    os_timer_arm(&timer_led, 10, 1);
}

#define min(a,b)  ((a)<(b)?(a):(b))
#define max(a,b)  ((a)>(b)?(a):(b))
void ICACHE_FLASH_ATTR
RGB2HSL(uint8_t r_val, uint8_t g_val, uint8_t b_val, uint16_t *h_val, uint8_t *s_val, uint8_t *l_val)
{
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

    if(del_Max == 0)
    {
        H = 0;
        S = 0;
    }
    else
    {
        if(L < 0.5)
            S = del_Max / (Max + Min);
        else
            S = del_Max / (2 - Max - Min);

        del_R = (((Max - R) / 6.0) + (del_Max / 2.0)) / del_Max;
        del_G = (((Max - G) / 6.0) + (del_Max / 2.0)) / del_Max;
        del_B = (((Max - B) / 6.0) + (del_Max / 2.0)) / del_Max;

        if(R == Max)
            H = del_B - del_G;
        else if(G == Max)
            H = (1.0 / 3.0) + del_R - del_B;
        else if(B == Max) H = (2.0 / 3.0) + del_G - del_R;

        if(H < 0) H += 1;
        if(H > 1) H -= 1;
    }

    *h_val = (uint16_t) (H * 360.0);
    *s_val = (uint8_t) (S * 255.0);
    *l_val = (uint8_t) (L * 255.0);
}

void ICACHE_FLASH_ATTR
HSL2RGB(uint16_t h_val, uint8_t s_val, uint8_t l_val, uint8_t *r_val, uint8_t *g_val, uint8_t *b_val)
{
    double R, G, B, H, S, L;
    double var_1, var_2;

    if(h_val > 360) h_val %= 360;
//	if (s_val > 100)
//		s_val = 100;
//	if (l_val > 100)
//		l_val = 100;

    H = (double) h_val / 360.0;
    S = (double) s_val / 255.0;
    L = (double) l_val / 255.0;

    if(S == 0)
    {
        R = L * 255.0;                   //RGB results = 0 ÷ 255
        G = L * 255.0;
        B = L * 255.0;
    }
    else
    {
        if(L < 0.5)
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
Hue2RGB(double v1, double v2, double vH)
{
    if(vH < 0) vH += 1;
    if(vH > 1) vH -= 1;
    if(6.0 * vH < 1) return v1 + (v2 - v1) * 6.0 * vH;
    if(2.0 * vH < 1) return v2;
    if(3.0 * vH < 2) return v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0;
    return (v1);
}
