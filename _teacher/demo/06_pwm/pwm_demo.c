#include "stm32f10x.h"

/* TIM2 的计数频率：72MHz / (71 + 1) = 1MHz。 */
#define PWM_STEPS 1000U

/* 每 10ms 调整一次占空比。 */
#define FADE_STEP_MS 10U

/* 每次让 CCR2 改变 10，灯不会一档一档跳得太粗。 */
#define FADE_STEP_VALUE 10U

/* TIM2 每 1ms 把它加一。 */
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
    TIM_OCInitTypeDef pwm;
    NVIC_InitTypeDef nvic;
    uint16_t duty = 0U;
    uint8_t rising = 1U;
    uint32_t last_step = 0U;

    /* PA1 的默认复用功能是 TIM2_CH2，PWM 会从 PA1 输出。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    led.GPIO_Pin = GPIO_Pin_1;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &led);

    /*
     * 72MHz / (71 + 1) = 1MHz，每个计数节拍是 1us。
     * 计数器从 0 数到 999，一共 1000 拍，所以一周期是 1ms。
     * 一个周期 1ms，对应频率 1000Hz。
     */
    tim.TIM_Prescaler = 71U;
    tim.TIM_Period = 999U;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM2, &tim);

    /* 先给 PWM 结构体填一套安全的默认值，再改真正要用的字段。 */
    TIM_OCStructInit(&pwm);
    pwm.TIM_OCMode = TIM_OCMode_PWM1;
    pwm.TIM_OutputState = ENABLE;
    pwm.TIM_Pulse = duty;
    pwm.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &pwm);

    /* 主程序写入的新占空比，在下一个更新事件统一生效。 */
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* 同一个 TIM2 每 1ms 产生更新中断，用来推动呼吸灯节拍。 */
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
        if ((tick_ms - last_step) >= FADE_STEP_MS) {
            last_step = last_step + FADE_STEP_MS;

            if (rising != 0U) {
                duty = duty + FADE_STEP_VALUE;
                if (duty >= PWM_STEPS) {
                    duty = PWM_STEPS;
                    rising = 0U;
                }
            } else {
                if (duty >= FADE_STEP_VALUE) {
                    duty = duty - FADE_STEP_VALUE;
                } else {
                    duty = 0U;
                }

                if (duty == 0U) {
                    rising = 1U;
                }
            }

            TIM_SetCompare2(TIM2, duty);
        }
    }
}
