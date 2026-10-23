#include "stm32f10x.h"

/* 中断只负责记录“PA0 出现了下降沿”，主循环再慢慢处理。 */
volatile uint8_t exti0_event = 0U;

void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count = count - 1U;
    }
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        exti0_event = 1U;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    GPIO_InitTypeDef button;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    /* GPIOC 管灯，GPIOA 管按键，AFIO 负责把 PA0 接到 EXTI0。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* PC13 推挽输出，低电平点亮。 */
    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    /* PA0 上拉输入：松开读 1，按下读 0。 */
    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    /* 把 GPIOA 的 0 号脚映射到 EXTI0。 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    /* 按下时 1 变 0，所以选择下降沿触发。 */
    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(EXTI_Line0);

    /* NVIC 允许 EXTI0 这个中断打断 CPU。 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    while (1) {
        if (exti0_event != 0U) {
            /* 等过抖动，再确认 PA0 仍然是低电平。 */
            delay(80000U);

            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0U) {
                if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != RESET) {
                    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
                } else {
                    GPIO_SetBits(GPIOC, GPIO_Pin_13);
                }
            }

            /* 消抖期间又来的边沿也一起清掉，一次按下只切换一次。 */
            exti0_event = 0U;
        }
    }
}
