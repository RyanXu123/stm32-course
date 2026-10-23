#include "stm32f10x.h"

volatile uint32_t tick_ms = 0U;

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        tick_ms = tick_ms + 1U;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;
    uint32_t last_toggle = 0U;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    tim.TIM_Prescaler = 71U;
    tim.TIM_Period = 999U;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM2, ENABLE);

    while (1) {
        if ((tick_ms - last_toggle) >= 500U) {
            last_toggle = last_toggle + 500U;

            if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != RESET) {
                GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            } else {
                GPIO_SetBits(GPIOC, GPIO_Pin_13);
            }
        }
    }
}
