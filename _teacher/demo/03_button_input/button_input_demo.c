#include "stm32f10x.h"

void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count = count - 1U;
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    GPIO_InitTypeDef button;
    uint8_t level_now;
    uint8_t level_last;
    uint8_t led_on;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    level_last = 1U;
    led_on = 0U;

    while (1) {
        level_now = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

        if (level_now == 0U) {
            delay(80000U);
            level_now = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

            if (level_now == 0U) {
                if (level_last == 1U) {
                    if (led_on == 0U) {
                        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
                        led_on = 1U;
                    } else {
                        GPIO_SetBits(GPIOC, GPIO_Pin_13);
                        led_on = 0U;
                    }
                }
                level_last = 0U;
            }
        } else {
            level_last = 1U;
        }
    }
}
