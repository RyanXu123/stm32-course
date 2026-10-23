#include "stm32f10x.h"

/* 锁机持续时间：10000ms，也就是 10 秒。 */
#define LOCK_TIME_MS 10000U

/* 未锁机时，PC13 每 500ms 翻转一次。 */
#define BLINK_INTERVAL_MS 500U

/* 定时器每 1ms 把它加一。 */
volatile uint32_t tick_ms = 0U;

/* 大于 0 表示机器正在锁机；数字就是还剩多少毫秒。 */
volatile uint32_t lock_remaining_ms = 0U;

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        /*
         * ===== 锁机第 1 段：开始锁机 =====
         * 如果当前没有锁机，就把剩余时间设成 10000ms。
         * 已经锁机时再按，不会把 10 秒重新加满。
         */
        if (lock_remaining_ms == 0U) {
            lock_remaining_ms = LOCK_TIME_MS;
        }

        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        tick_ms = tick_ms + 1U;

        /*
         * ===== 锁机第 2 段：倒计时 =====
         * 定时器每 1ms 进来一次，锁机剩余时间就减 1ms。
         */
        if (lock_remaining_ms > 0U) {
            lock_remaining_ms = lock_remaining_ms - 1U;
        }

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

int main(void)
{
    GPIO_InitTypeDef led;
    GPIO_InitTypeDef button;
    EXTI_InitTypeDef exti;
    TIM_TimeBaseInitTypeDef tim;
    NVIC_InitTypeDef nvic;
    uint32_t last_toggle = 0U;

    /* GPIOC 管灯，GPIOA 管按键，AFIO 把 PA0 接到 EXTI0。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PC13 推挽输出，低电平点亮。 */
    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    /* PA0 上拉输入：松开读 1，按下接 GND 读 0。 */
    button.GPIO_Pin = GPIO_Pin_0;
    button.GPIO_Speed = GPIO_Speed_50MHz;
    button.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &button);

    /* PA0 从 1 变 0 时产生下降沿，EXTI0 收到后进入中断。 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    exti.EXTI_Line = EXTI_Line0;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);
    EXTI_ClearITPendingBit(EXTI_Line0);

    /* 72MHz / (71 + 1) = 1MHz，硬件计数一次就是 1us。 */
    tim.TIM_Prescaler = 71U;
    tim.TIM_Period = 999U;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);

    /* 数满 1000 次产生更新事件，也就是每 1ms 进来一次。 */
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* 同时允许 EXTI0 和 TIM2 这两个中断进入 CPU。 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    nvic.NVIC_IRQChannel = EXTI0_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 3U;
    nvic.NVIC_IRQChannelSubPriority = 0U;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM2, ENABLE);

    while (1) {
        /*
         * ===== 锁机第 3 段：业务动作总闸 =====
         * 这里才是真正“不许机器继续动作”的位置。
         * 以后接电机时，就在这个分支里停止电机，并把开门、启动等命令放进 else。
         */
        if (lock_remaining_ms > 0U) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            last_toggle = tick_ms;
        } else {
            if ((tick_ms - last_toggle) >= BLINK_INTERVAL_MS) {
                last_toggle = last_toggle + BLINK_INTERVAL_MS;

                if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) != RESET) {
                    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
                } else {
                    GPIO_SetBits(GPIOC, GPIO_Pin_13);
                }
            }
        }
    }
}
