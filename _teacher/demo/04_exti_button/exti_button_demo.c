#include "stm32f10x.h"

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

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(EXTI_Line0);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    while (1) {
        if (exti0_event != 0U) {
            delay(80000U);

            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0U) {
                if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != RESET) {
                    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
                } else {
                    GPIO_SetBits(GPIOC, GPIO_Pin_13);
                }
            }

            exti0_event = 0U;
        }
    }
}
