#include "stm32f10x.h"

void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count = count - 1;
    }
}

int main(void)
{
    GPIO_InitTypeDef led;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);

    while (1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        delay(400000U);

        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        delay(400000U);
    }
}
