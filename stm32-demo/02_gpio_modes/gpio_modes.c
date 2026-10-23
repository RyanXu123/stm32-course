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

    /* GPIOA 和 GPIOC 的时钟分开打开，避免把两个数字直接相加。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PC13：推挽输出。高电平和低电平都由芯片主动输出。 */
    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &led);

    /*
     * PA0：上拉输入。
     * 没有外力时读到高电平；以后接按键时，按键另一端接 GND，
     * 按下后把 PA0 拉到低电平。
     */
    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    while (1) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_13);
        delay(400000U);

        GPIO_SetBits(GPIOA, GPIO_Pin_13);
        delay(400000U);
    }
}
