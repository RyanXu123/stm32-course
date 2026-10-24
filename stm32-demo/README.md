# stm32-demo —— 主人的工程

这个夹只放主人自己打开、修改、写练习的 STM32 工程。

- 示例可以从 `..\_teacher\demo\` 复制过来再改。
- 这里每个工程都可以自由编辑，本座的参考答案不会混进来。
- 课程讲义仍在上层 `stm32-course\`，不塞进工程目录。
- 打开、仿真、下载和新文件夹规则见 `工程索引.md`。

当前工程：

- `01_led_blink\led_blink.uvprojx`，主文件 `led_blink.c`
- `02_gpio_modes\gpio_modes.uvprojx`，主文件 `gpio_modes.c`
- `03_button_input\button_input.uvprojx`，主文件 `button_input.c`
- `04_button_hold_ryan\button_input.uvprojx`，作业 01，主文件 `button_input.c`
- `04_exti_button\exti_button.uvprojx`，主文件 `exti_button.c`
- `05_tim_tick\tim_tick.uvprojx`，主文件 `tim_tick.c`
- `05b_lock_timer\lock_timer.uvprojx`，主文件 `lock_timer.c`，第 05 讲作业 3 的 10 秒锁机示例
- `06_pwm\pwm.uvprojx`，主文件 `pwm.c`，TIM2_CH2 从 PA1 输出呼吸灯 PWM

各工程的 Keil 左侧 Project 树都分成：

- `01_我的代码`：真正参与编译的主程序；
- `02_本座示例`：本座的参考版本，只供查看，已排除编译。

- `02_本座示例` 指向 `_teacher\demo\`，没有在主人的工程里复制第二份源码。

文件名可以自己改。C 不看“文件是否叫 main.c”，看里面有没有入口函数 `int main(void)`；
Keil 编译的是工程里勾选参与构建的所有 `.c`，不是整个文件夹里见到的所有文件。
