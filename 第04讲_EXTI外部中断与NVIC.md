# 第 04 讲　让按键主动打断 CPU

> 2026-10-22 上。接第 03 讲《按键输入与消抖》。
> 主人工程：`stm32-course\stm32-demo\04_exti_button\exti_button.uvprojx`
> 主程序：`stm32-course\stm32-demo\04_exti_button\exti_button.c`
> 本座示例：`stm32-course\_teacher\demo\04_exti_button\exti_button_demo.c`

## 零、先认词，再上代码

这一讲不是 C 语言语法课，而是在认一颗芯片内部有哪些硬件角色。先把新词全部摆出来：

- `GPIOA`、`GPIOC`：芯片外面的引脚组。不要把这里的“端口”理解成电脑上的 USB 口，它更像一排编号固定的接线柱。
- `PA0`：GPIOA 这一组里面的第 0 号引脚；`PC13` 是 GPIOC 这一组里面的第 13 号引脚。
- 高电平、低电平：引脚上接近 3.3V 的状态叫高电平，程序读作 1；接近 0V 的状态叫低电平，程序读作 0。
- 边沿：电平从 1 变成 0 的那一瞬间叫下降沿，从 0 变成 1 的那一瞬间叫上升沿。边沿说的是“变化发生的时刻”，不是一直保持的电平。
- `AFIO`：一组负责内部接线切换的外设。这一讲把它当成“接线总台”：PA0、PB0、PC0 都在排队，它决定哪一个接到 EXTI0。
- `EXTI`：外部中断控制器，负责盯着某个电平变化，并在指定边沿出现时举起一面“有事发生”的旗子。
- `EXTI0`：EXTI 里面的第 0 号监听通道。它不是芯片外面的一根脚，而是芯片内部的信号线。
- `NVIC`：CPU 旁边负责接收中断请求的门卫。EXTI 报告以后，NVIC 决定 CPU 现在要不要停下原来的工作去响应。
- 中断：CPU 暂停主程序，跳去执行一段专门的处# 第 03 讲　把按键读进程序

> 2026-10-21 上。接第 02 讲《GPIO 八种模式》。
> 主人工程：`stm32-course\stm32-demo\03_button_input\button_input.uvprojx`
> 主程序：`stm32-course\stm32-demo\03_button_input\button_input.c`
> 本座示例：`stm32-course\_teacher\demo\03_button_input\button_input_demo.c`

## 一、第 02 讲只把输入脚配置好了

上一讲把 PA0 配成了上拉输入，但程序一次都没有读过它。配置成输入，只是把芯片内部电路接通；
要知道外面现在按没按，还得调用读取函数。

本讲完成一条完整链路：

```text
手指按键
→ 按键把 PA0 接到 GND
→ 芯片读 PA0
→ 程序判断 0 还是 1
→ 控制 PC13
→ 板载 LED 亮灭
```

输入负责“看外面”，输出负责“动外面”。这一进一出，就是 GPIO 最基础的用途。

## 二、按钮为什么能读出 0 和 1

先按最常见的接法：

```text
PA0 ─────┤ 按键 ├──── GND
         内部上拉
```

PA0 已经配置成 `GPIO_Mode_IPU`。芯片内部通过一个很弱的电阻把引脚拉向 3.3V：

```text
松开：没人把 PA0 拉到 0V → 读 1
按下：按键把 PA0 接到 GND → 读 0
```

所以这个接法里，按钮是低电平有效：

```text
松开 = 1
按下 = 0
```

别把按钮另一端直接接 3.3V。那等于按下以后仍然给高电平，程序看不到从 1 变成 0。

如果按钮模块自己带了上拉或下拉，实际接法可能不同。先看模块丝印和元件表，不能只凭按钮外壳猜。

## 三、读 PA0 的库函数

标准外设库用：

```c
GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)
```

两个参数回答两件事：

```text
GPIOA       读哪一组端口
GPIO_Pin_0  读这一组里的第 0 号脚
```

返回值只有 0 或 1：

```text
0：读到低电平
1：读到高电平
```

在这一讲的外部电路里，它暂时可以理解成：

```text
0：按钮按下
1：按钮松开
```

## 四、什么叫按键抖动

机械按键不是一按下就立刻从 1 稳定变成 0。金属片第一次碰在一起时，会在几毫秒内快速弹开、再碰上，
引脚可能读到：

```text
1 → 0 → 1 → 0 → 0 → 1 → 0
```

如果每读到一个 0 就切换一次 LED，按一下可能亮灭好几次。看起来像“程序抽风”，其实是机械触点抖动。

软件消抖的办法很朴素：

```text
第一次读到 0
→ 等一小会儿
→ 再读一次
→ 仍然是 0，才承认按钮真的按下
```

本讲还再记一个 `level_last`：上一次稳定电平。只有上一次是 1、这次稳定读到 0，才算“发生了一次按下”。
按住不放时，后续循环一直是 0，不会每圈都切换。

## 五、完整主程序

文件：`stm32-demo\03_button_input\button_input.c`

```c
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

    /* GPIOA 和 GPIOC 的时钟分开打开。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PC13 推挽输出，按低电平点亮处理。 */
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
```

## 六、新出现的 C 写法

### `uint8_t`

这是一个 0 到 255 的无符号小整数类型。这里只用它存 0 和 1，比 `int` 更贴切，也省空间。

### `0U`、`1U`

`U` 表示这是无符号常量。数值仍然是 0 和 1，只是类型更明确。

### `level_now`

这一轮刚读到的电平。

### `level_last`

上一次承认的稳定电平，用来判断“是从松开变成按下”，还是已经按住很久。

### `led_on`

记住灯现在是不是亮。`0U` 表示灭，`1U` 表示亮。

## 七、每一段到底在做什么

一开始先把 PC13 写成高电平，让低电平点亮的 LED 先灭：

```c
GPIO_SetBits(GPIOC, GPIO_Pin_13);
```

然后每轮循环读 PA0：

```c
level_now = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
```

读到 0 后不立刻相信，先等一小会儿再读一次：

```c
delay(80000U);
level_now = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
```

两次都是 0，并且上一次稳定状态是 1，才切换 LED。切换以后把 `level_last` 改成 0；
只要还按着，就不会重复切换。

松开后读到 1，把 `level_last` 恢复成 1，下一次按下才重新允许切换。

## 八、编译与上板

打开：

```text
stm32-course\stm32-demo\03_button_input\button_input.uvprojx
```

按 `F7`。本讲工程已实编译：

```text
0 Error(s), 0 Warning(s)
```

在真实硬件上烧录前，必须先确认两件事：

1. 板载 LED 到底接在哪个 GPIO，亮灯是高电平还是低电平；
2. PA0 是否真的接了一个按钮，按钮另一端是否接 GND。

程序里的 PC13、PA0 是这一讲的默认接法，不代表任何开发板都一定这么接。

如果实际 LED 是 PA5，输出部分至少要改：

```text
GPIOC → GPIOA
GPIO_Pin_13 → GPIO_Pin_5
对应时钟 GPIOC → GPIOA
```

只改 `GPIO_Pin_13` 不够，端口和时钟也要一起改。

## 九、动手题

1. 把消抖延时删掉，观察或推演按住按钮时为什么可能反复切换。
2. 把 `button.GPIO_Mode` 改成 `GPIO_Mode_IPD`，解释松开和按下分别读到什么。
3. 改成“按住亮、松开灭”，不再使用“按一下切换一次”，需要删掉哪些状态变量？
4. 如果把按钮改成“一端接 3.3V、另一端接 PA0”，应该用上拉还是下拉输入？
5. 如果 LED 实际接在 PA5，列出本程序必须改的端口、引脚、时钟和输出函数。

## 十、小测

1. 配成输入和真正读取输入是不是一回事？
2. `GPIO_ReadInputDataBit` 两个参数分别表示什么？
3. 上拉输入松开为什么默认读 1？
4. 机械按键为什么会在几毫秒内读出一串 0、1？
5. `level_last` 不用会怎样？
6. 为什么低电平点亮的 LED 要用 `GPIO_ResetBits` 打开？
7. PC13 改成 PA5 时，为什么不能只改 `GPIO_Pin_13`？
8. 烧录前为什么必须先确认板上按钮接法？

---

**下一讲**：第 04 讲 EXTI 外部中断。按钮不再靠主循环不停轮询，而由引脚电平变化直接打断 CPU。
理代码，结束以后再回到刚才停下的地方。
- 挂起位：EXTI 用来表示“这件事还没处理”的标记，可以理解成一面小旗子。处理完必须把旗子放回去。
- 中断服务函数：中断发生后自动被调用的函数。本讲的名字是 `EXTI0_IRQHandler`，它不是普通函数名，启动文件已经提前认准了这个名字。

最容易混的是“脚”和“内部通道”：

```text
PA0       是芯片外面看得见的引脚
EXTI0     是芯片里面收报告的 0 号通道
AFIO      决定把哪一组端口里的 0 号脚接到 EXTI0
```

芯片这样设计是为了省内部线路，不让每一个引脚都单独占一条中断线。规则是：

```text
EXTI0 只有一条
PA0、PB0、PC0、……都想用它
AFIO 同一时刻只能选其中一个接进去
```

这就是“端口共用”。本讲选择了 PA0，所以 EXTI0 听见的是 PA0 的变化；这时 PB0 和 PC0 不能再同时占用 EXTI0。

可以把整条链想成一栋楼：

```text
PA0 是 A 区 0 号房间
PB0 是 B 区 0 号房间
PC0 是 C 区 0 号房间
AFIO 是总机，负责把“0 号房间”接到 0 号报警线路
EXTI0 是 0 号报警线路
NVIC 是前台，决定报警后要不要通知 CPU
EXTI0_IRQHandler 是 CPU 接警后照着执行的处理流程
```

## 一、上一讲与这一讲到底差在哪

第 03 讲让 CPU 一直待在 `while (1)` 里，不停执行：

```text
读 PA0
看看是不是按下
再读 PA0
看看是不是按下
```

这叫轮询。只要按钮没有变化，CPU 仍在一圈一圈地看，像一个人不停抬头检查门口有没有人。

这一讲改成中断：

```text
CPU 平时做别的事
PA0 真的从 1 变成 0
EXTI 发出中断请求
NVIC 允许这次请求
CPU 暂停原来的工作
跳进 EXTI0_IRQHandler
处理完再回到原来的地方
```

按钮没有变化时，CPU 不需要一圈圈读它。发生变化时，硬件会主动来报告。

## 二、再把关键链路串起来

### EXTI

EXTI 是外部中断控制器。它盯住某个信号有没有出现指定的边沿。

本讲按钮是低电平有效：

```text
松开 = 1
按下 = 0
```

所以“按下”的瞬间是：

```text
1 → 0
```

这叫下降沿。`EXTI_Trigger_Falling` 就是在选择下降沿触发。

### EXTI 线

PA0、PB0、PC0 不是各有一根独立的 EXTI0。STM32F103 的基本规则是：

```text
PA0、PB0、PC0……共用 EXTI0
PA1、PB1、PC1……共用 EXTI1
```

同一时刻，EXTI0 只能接其中一组端口的第 0 号脚。本讲通过：

```c
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
```

把 GPIOA 的第 0 号脚接到 EXTI0。这个映射由 AFIO 管理，所以 AFIO 时钟也必须打开。

### NVIC

EXTI 发现边沿以后，只会举手说“有事件”。NVIC 才是 CPU 里的中断门卫，决定这件事让不让 CPU 处理，以及优先级是多少。

EXTI 和 NVIC 少开一个，中断都进不来：

```text
EXTI 没开：按键变化没人报告
NVIC 没开：报告到了，CPU 仍不响应
```

### 中断服务函数

`EXTI0_IRQHandler` 是中断发生后 CPU 跳去执行的 C 函数。这个名字不能随便改，它必须和启动文件里的中断向量表完全一致。

## 三、信号链路

```text
手指按下
→ PA0 从 3.3V 被按键接到 GND
→ 引脚出现下降沿 1 → 0
→ AFIO 早已把 PA0 接到 EXTI0
→ EXTI0 挂起位变成 1
→ NVIC 允许 EXTI0 中断
→ CPU 跳进 EXTI0_IRQHandler
→ 中断函数记录事件并清挂起位
→ CPU 回到主循环
→ 主循环消抖并切换 LED
```

## 四、硬件沿用第 03 讲

```text
PA0：上拉输入，松开读 1，按下接 GND 读 0
PC13：推挽输出，低电平点亮，写 0 亮，写 1 灭
```

换板子前仍要确认实际 LED 和按钮接在哪个引脚，不能只看“F103 最小板都这样”就烧录。

## 五、完整程序

文件：`stm32-demo\04_exti_button\exti_button.c`

```c
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

    /* PC13 推挽输出，低电平点亮。 */# 第 04 讲　让按键主动打断 CPU

> 2026-10-22 上。接第 03 讲《按键输入与消抖》。
> 主人工程：`stm32-course\stm32-demo\04_exti_button\exti_button.uvprojx`
> 主程序：`stm32-course\stm32-demo\04_exti_button\exti_button.c`
> 本座示例：`stm32-course\_teacher\demo\04_exti_button\exti_button_demo.c`

## 零、先认词，再上代码

这一讲不是 C 语言语法课，而是在认一颗芯片内部有哪些硬件角色。先把新词全部摆出来：

- `GPIOA`、`GPIOC`：芯片外面的引脚组。不要把这里的“端口”理解成电脑上的 USB 口，它更像一排编号固定的接线柱。
- `PA0`：GPIOA 这一组里面的第 0 号引脚；`PC13` 是 GPIOC 这一组里面的第 13 号引脚。
- 高电平、低电平：引脚上接近 3.3V 的状态叫高电平，程序读作 1；接近 0V 的状态叫低电平，程序读作 0。
- 边沿：电平从 1 变成 0 的那一瞬间叫下降沿，从 0 变成 1 的那一瞬间叫上升沿。边沿说的是“变化发生的时刻”，不是一直保持的电平。
- `AFIO`：一组负责内部接线切换的外设。这一讲把它当成“接线总台”：PA0、PB0、PC0 都在排队，它决定哪一个接到 EXTI0。
- `EXTI`：外部中断控制器，负责盯着某个电平变化，并在指定边沿出现时举起一面“有事发生”的旗子。
- `EXTI0`：EXTI 里面的第 0 号监听通道。它不是芯片外面的一根脚，而是芯片内部的信号线。
- `NVIC`：CPU 旁边负责接收中断请求的门卫。EXTI 报告以后，NVIC 决定 CPU 现在要不要停下原来的工作去响应。
- 中断：CPU 暂停主程序，跳去执行一段专门的处理代码，结束以后再回到刚才停下的地方。
- 挂起位：EXTI 用来表示“这件事还没处理”的标记，可以理解成一面小旗子。处理完必须把旗子放回去。
- 中断服务函数：中断发生后自动被调用的函数。本讲的名字是 `EXTI0_IRQHandler`，它不是普通函数名，启动文件已经提前认准了这个名字。

最容易混的是“脚”和“内部通道”：

```text
PA0       是芯片外面看得见的引脚
EXTI0     是芯片里面收报告的 0 号通道
AFIO      决定把哪一组端口里的 0 号脚接到 EXTI0
```

芯片这样设计是为了省内部线路，不让每一个引脚都单独占一条中断线。规则是：

```text
EXTI0 只有一条
PA0、PB0、PC0、……都想用它
AFIO 同一时刻只能选其中一个接进去
```

这就是“端口共用”。本讲选择了 PA0，所以 EXTI0 听见的是 PA0 的变化；这时 PB0 和 PC0 不能再同时占用 EXTI0。

可以把整条链想成一栋楼：

```text
PA0 是 A 区 0 号房间
PB0 是 B 区 0 号房间
PC0 是 C 区 0 号房间
AFIO 是总机，负责把“0 号房间”接到 0 号报警线路
EXTI0 是 0 号报警线路
NVIC 是前台，决定报警后要不要通知 CPU
EXTI0_IRQHandler 是 CPU 接警后照着执行的处理流程
```

## 一、上一讲与这一讲到底差在哪

第 03 讲让 CPU 一直待在 `while (1)` 里，不停执行：

```text
读 PA0
看看是不是按下
再读 PA0
看看是不是按下
```

这叫轮询。只要按钮没有变化，CPU 仍在一圈一圈地看，像一个人不停抬头检查门口有没有人。

这一讲改成中断：

```text
CPU 平时做别的事
PA0 真的从 1 变成 0
EXTI 发出中断请求
NVIC 允许这次请求
CPU 暂停原来的工作
跳进 EXTI0_IRQHandler
处理完再回到原来的地方
```

按钮没有变化时，CPU 不需要一圈圈读它。发生变化时，硬件会主动来报告。

## 二、再把关键链路串起来

### EXTI

EXTI 是外部中断控制器。它盯住某个信号有没有出现指定的边沿。

本讲按钮是低电平有效：

```text
松开 = 1
按下 = 0
```

所以“按下”的瞬间是：

```text
1 → 0
```

这叫下降沿。`EXTI_Trigger_Falling` 就是在选择下降沿触发。

### EXTI 线

PA0、PB0、PC0 不是各有一根独立的 EXTI0。STM32F103 的基本规则是：

```text
PA0、PB0、PC0……共用 EXTI0
PA1、PB1、PC1……共用 EXTI1
```

同一时刻，EXTI0 只能接其中一组端口的第 0 号脚。本讲通过：

```c
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
```

把 GPIOA 的第 0 号脚接到 EXTI0。这个映射由 AFIO 管理，所以 AFIO 时钟也必须打开。

### NVIC

EXTI 发现边沿以后，只会举手说“有事件”。NVIC 才是 CPU 里的中断门卫，决定这件事让不让 CPU 处理，以及优先级是多少。

EXTI 和 NVIC 少开一个，中断都进不来：

```text
EXTI 没开：按键变化没人报告
NVIC 没开：报告到了，CPU 仍不响应
```

### 中断服务函数

`EXTI0_IRQHandler` 是中断发生后 CPU 跳去执行的 C 函数。这个名字不能随便改，它必须和启动文件里的中断向量表完全一致。

## 三、信号链路

```text
手指按下
→ PA0 从 3.3V 被按键接到 GND
→ 引脚出现下降沿 1 → 0
→ AFIO 早已把 PA0 接到 EXTI0
→ EXTI0 挂起位变成 1
→ NVIC 允许 EXTI0 中断
→ CPU 跳进 EXTI0_IRQHandler
→ 中断函数记录事件并清挂起位
→ CPU 回到主循环
→ 主循环消抖并切换 LED
```

## 四、硬件沿用第 03 讲

```text
PA0：上拉输入，松开读 1，按下接 GND 读 0
PC13：推挽输出，低电平点亮，写 0 亮，写 1 灭
```

换板子前仍要确认实际 LED 和按钮接在哪个引脚，不能只看“F103 最小板都这样”就烧录。

## 五、完整程序

文件：`stm32-demo\04_exti_button\exti_button.c`

```c
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
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

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
```

### 完整程序里每个新名字分别做什么

`exti0_event` 是普通 C 变量，不是芯片寄存器。它只保存“主循环还有没有一件按键事件要处理”：

```text
0U：没有新事件
1U：至少发生过一次下降沿，等主循环处理
```

它只用 0 和 1，是因为这里只需要表达“有”或“没有”。`volatile` 则让主循环每次重新读取它，防止编译器一直使用旧值。

`EXTI_GetITStatus(EXTI_Line0)` 是标准外设库里已经写好的普通函数，声明在 `stm32f10x_exti.h`，实现放在本工程的
`Library\stm32f10x_exti.c` 第 223 行。主文件没有重新实现它，只是在调用它。这个函数同时检查两件事：EXTI0 是否被允许工作，以及 EXTI0 的挂起旗子是否已经立起；成立时返回 `SET`，否则返回 `RESET`。

`EXTI_Line0` 不是“读取第 0 条线”，而是一个编号常量，真实值是 `0x00001`，告诉库函数“这次操作针对 0 号通道”。读取或清除由后面的函数决定：

```text
EXTI_GetITStatus(EXTI_Line0)          看挂起旗子有没有立起
EXTI_ClearITPendingBit(EXTI_Line0)    把挂起旗子放下
```

第一段配置里的六项分别是：

```text
exti.EXTI_Line = EXTI_Line0;
选择 0 号 EXTI 通道。

exti.EXTI_Mode = EXTI_Mode_Interrupt;
选择产生中断请求，交给 NVIC 和 CPU 处理。

exti.EXTI_Trigger = EXTI_Trigger_Falling;
只在 1 变成 0 的下降沿触发。

exti.EXTI_LineCmd = ENABLE;
启用这条 EXTI 通道。

EXTI_Init(&exti);
把前面填好的四项真正写进 EXTI 寄存器。

EXTI_ClearITPendingBit(EXTI_Line0);
先清掉之前可能留下的挂起旗子，免得刚配置完就误进中断。
```

第二段配置里的五级分别决定“哪个中断、多重要、开不开”：

```text
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
把 4 位优先级分成 2 位抢占优先级和 2 位子优先级。
在这组规则下，两个数字都可取 0 到 3。

nvic.NVIC_IRQChannel = EXTI0_IRQn;
告诉 NVIC：这次配置的是 EXTI0 对应的 CPU 中断通道。
EXTI0_IRQn 是 stm32f10x.h 里的枚举值 6，与 EXTI_Line0 不是同一个东西。

nvic.NVIC_IRQChannelPreemptionPriority = 2U;
设置抢占优先级为 2。数字越小越优先；更高优先级的中断可以打断正在执行的低优先级中断。

nvic.NVIC_IRQChannelSubPriority = 0U;
设置子优先级为 0。只有抢占优先级相同时才比较它，它只能决定谁先排队，不能打断正在执行的中断。

nvic.NVIC_IRQChannelCmd = ENABLE;
允许 NVIC 把 EXTI0 的请求送给 CPU。
```

最后还有一句没有划进选中范围，但必须存在：

```c
NVIC_Init(&nvic);
```

它负责把 `nvic` 这张配置表真正写进 NVIC 的寄存器。前面五个字段只是填表，`NVIC_Init` 才是提交。

## 六、初始化顺序

### 1. 打开时钟

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
```

GPIOA 和 GPIOC 的时钟已经在前面讲过。这里第一次看见 AFIO：它负责外部中断的端口映射，所以不能漏。

### 2. 配置两个普通 GPIO

PC13 仍是推挽输出，PA0 仍是上拉输入。到这一步，引脚电平已经能正常变化，但还没有人监听边沿。

### 3. 把 PA0 接到 EXTI0

```c
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
```

第一项选择 GPIOA，第二项选择第 0 号脚。忽略这一句，EXTI0 可能听的是 PB0 或 PC0，不是主人的 PA0。

### 4. 配置 EXTI

```c
exti.EXTI_Line = EXTI_Line0;
exti.EXTI_Mode = EXTI_Mode_Interrupt;
exti.EXTI_Trigger = EXTI_Trigger_Falling;
exti.EXTI_LineCmd = ENABLE;
EXTI_Init(&exti);
```

四项分别是：

```text
监听哪一条线：EXTI0
产生中断还是事件：产生中断
监听哪个边沿：下降沿
现在是否启用：启用
```

接着清除一次挂起位。挂起位可以理解成 EXTI 的小旗子：发生边沿后旗子立起来，CPU 处理完必须把旗子放回去。

### 5. 配置 NVIC

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
nvic.NVIC_IRQChannel = EXTI0_IRQn;
nvic.NVIC_IRQChannelPreemptionPriority = 2U;
nvic.NVIC_IRQChannelSubPriority = 0U;
nvic.NVIC_IRQChannelCmd = ENABLE;
NVIC_Init(&nvic);
```

`NVIC_PriorityGroup_2` 是在划分“抢占优先级”和“子优先级”各占多少位。本讲只有一个中断，数字 2 和 0 暂时不会产生冲突；以后急停和定时器一起出现时，数字越小，抢占优先级越高。

`EXTI0_IRQn`、预抢占优先级、子优先级和启用开关四样凑齐，NVIC 才会把 EXTI0 放进 CPU 的响应名单。

## 七、中断函数为什么这么短

```c
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        exti0_event = 1U;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
```

它只做两件事：记下“按键发生过下降沿”，清掉 EXTI 的挂起位。

挂起位如果不清理，CPU 刚退出中断又会看见同一面旗子，可能不断重新进入中断。`EXTI_ClearITPendingBit(EXTI_Line0);` 就是告诉 EXTI：这次报告已经收到。

中断函数里先不自作主张切换 LED，只留下事件。主循环看到 `exti0_event` 后再消抖、确认和切换。这样中断执行得短，不会让 CPU 长时间停在一段处理里。

## 八、`exti0_event` 为什么要加 `volatile`

```c
volatile uint8_t exti0_event = 0U;
```

这个变量会被两个地方改：

```text
EXTI0_IRQHandler：发生中断时写成 1
main：处理完写成 0
```

编译器原本可能觉得主循环里的 `exti0_event` 没有被当前函数修改，于是把读取结果缓存到寄存器。`volatile` 是在告诉编译器：这个值可能被程序外部随时改变，每次都得重新从内存读。

它不是“把变量变快”，而是“别擅自省略对变量的重新读取”。

## 九、一次按下怎样只切换一次

机械按键产生下降沿后，金属触点还可能继续抖动。中断函数先记下事件，主循环随后：

```text
等一小会儿
再读 PA0
如果仍是 0，才切换 LED
最后把 exti0_event 清回 0
```

等待期间再来的抖动只会让 `exti0_event` 继续等于 1，不会堆成一串要执行的命令；主循环在确认后统一清掉，因此这一次按下只切换一次。

## 十、打开与编译

打开：

```text
stm32-course\stm32-demo\04_exti_button\exti_button.uvprojx
```

Keil 左侧：

```text
01_我的代码：exti_button.c，真正参与编译
02_本座示例：exti_button_demo.c，只供对照，不参与构建
```

按 `F7` 编译。本讲工程用 Arm Compiler 6.18 实编译，结果必须达到：

```text
0 Error(s), 0 Warning(s)
```

没有开发板时，可以在 Keil 模拟器里查看 EXTI、NVIC 和 GPIOC 的状态。模拟器不会让桌面上的实物灯发光。

## 十一、上板前检查

1. 开发板的 LED 是否真的接 PC13，是否低电平点亮。
2. PA0 是否真的有按钮，按钮另一端是否接 GND。
3. Keil 的 Debug 是否选择 ST-Link，接口是否为 SW。
4. 下载前先确认芯片能被识别，下载后再按复位键。

如果实际按钮一端接 3.3V，按下是高电平，本讲应改成 `EXTI_Trigger_Rising`，输入也应改成下拉，不能只改触发边沿。

## 十二、小测

1. 轮询和中断对 CPU 的要求有什么不同？
2. 为什么按一下 PA0 会出现下降沿？
3. PA0、PB0、PC0 与 EXTI0 是什么关系？
4. 为什么 AFIO 时钟也要打开？
5. EXTI 和 NVIC 少配置一个，分别会怎样？
6. `EXTI0_IRQHandler` 这个名字为什么不能随便改？
7. 清挂起位是在清什么？
8. `exti0_event` 为什么需要 `volatile`？

## 十三、动手题

1. 把 `EXTI_Trigger_Falling` 改成上升沿，推演什么时候会进中断。
2. 删掉 `EXTI_ClearITPendingBit`，解释为什么会反复进中断。
3. 删掉 `GPIO_EXTILineConfig`，说明为什么 EXTI0 不一定听得见 PA0。
4. 把 LED 换成 PA5 高电平点亮，列出要改的端口、引脚、时钟和输出电平。
5. 保留本讲工程不动，自己复制一份，改成“按一下亮，再按一下灭”和“按住亮、松开灭”两个版本，并比较哪个版本需要读取完整的按下与松开边沿。

---

**下一讲**：第 05 讲 TIM 定时器中断，用稳定的毫秒节拍替代长 `delay`，处理红闪和 10 秒锁机。

    led.GPIO_Pin = GPIO_Pin_13;
    led.GPIO_Speed = GPIO_Speed_50MHz;
    led.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &led);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    /* PA0 上拉输入：松开读 1，按下读 0。 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

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
```

### 完整程序里每个新名字分别做什么

`exti0_event` 是普通 C 变量，不是芯片寄存器。它只保存“主循环还有没有一件按键事件要处理”：

```text
0U：没有新事件
1U：至少发生过一次下降沿，等主循环处理
```

它只用 0 和 1，是因为这里只需要表达“有”或“没有”。`volatile` 则让主循环每次重新读取它，防止编译器一直使用旧值。

`EXTI_GetITStatus(EXTI_Line0)` 是标准外设库里已经写好的普通函数，声明在 `stm32f10x_exti.h`，实现放在本工程的
`Library\stm32f10x_exti.c` 第 223 行。主文件没有重新实现它，只是在调用它。这个函数同时检查两件事：EXTI0 是否被允许工作，以及 EXTI0 的挂起旗子是否已经立起；成立时返回 `SET`，否则返回 `RESET`。

`EXTI_Line0` 不是“读取第 0 条线”，而是一个编号常量，真实值是 `0x00001`，告诉库函数“这次操作针对 0 号通道”。读取或清除由后面的函数决定：

```text
EXTI_GetITStatus(EXTI_Line0)          看挂起旗子有没有立起
EXTI_ClearITPendingBit(EXTI_Line0)    把挂起旗子放下
```

第一段配置里的六项分别是：

```text
exti.EXTI_Line = EXTI_Line0;
选择 0 号 EXTI 通道。

exti.EXTI_Mode = EXTI_Mode_Interrupt;
选择产生中断请求，交给 NVIC 和 CPU 处理。

exti.EXTI_Trigger = EXTI_Trigger_Falling;
只在 1 变成 0 的下降沿触发。

exti.EXTI_LineCmd = ENABLE;
启用这条 EXTI 通道。

EXTI_Init(&exti);
把前面填好的四项真正写进 EXTI 寄存器。

EXTI_ClearITPendingBit(EXTI_Line0);
先清掉之前可能留下的挂起旗子，免得刚配置完就误进中断。
```

第二段配置里的五级分别决定“哪个中断、多重要、开不开”：

```text
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
把 4 位优先级分成 2 位抢占优先级和 2 位子优先级。
在这组规则下，两个数字都可取 0 到 3。

nvic.NVIC_IRQChannel = EXTI0_IRQn;
告诉 NVIC：这次配置的是 EXTI0 对应的 CPU 中断通道。
EXTI0_IRQn 是 stm32f10x.h 里的枚举值 6，与 EXTI_Line0 不是同一个东西。

nvic.NVIC_IRQChannelPreemptionPriority = 2U;
设置抢占优先级为 2。数字越小越优先；更高优先级的中断可以打断正在执行的低优先级中断。

nvic.NVIC_IRQChannelSubPriority = 0U;
设置子优先级为 0。只有抢占优先级相同时才比较它，它只能决定谁先排队，不能打断正在执行的中断。

nvic.NVIC_IRQChannelCmd = ENABLE;
允许 NVIC 把 EXTI0 的请求送给 CPU。
```

最后还有一句没有划进选中范围，但必须存在：

```c
NVIC_Init(&nvic);
```

它负责把 `nvic` 这张配置表真正写进 NVIC 的寄存器。前面五个字段只是填表，`NVIC_Init` 才是提交。

## 六、初始化顺序

### 1. 打开时钟

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
```

GPIOA 和 GPIOC 的时钟已经在前面讲过。这里第一次看见 AFIO：它负责外部中断的端口映射，所以不能漏。

### 2. 配置两个普通 GPIO

PC13 仍是推挽输出，PA0 仍是上拉输入。到这一步，引脚电平已经能正常变化，但还没有人监听边沿。

### 3. 把 PA0 接到 EXTI0

```c
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
```

第一项选择 GPIOA，第二项选择第 0 号脚。忽略这一句，EXTI0 可能听的是 PB0 或 PC0，不是主人的 PA0。

### 4. 配置 EXTI

```c
exti.EXTI_Line = EXTI_Line0;
exti.EXTI_Mode = EXTI_Mode_Interrupt;
exti.EXTI_Trigger = EXTI_Trigger_Falling;
exti.EXTI_LineCmd = ENABLE;
EXTI_Init(&exti);
```

四项分别是：

```text
监听哪一条线：EXTI0
产生中断还是事件：产生中断
监听哪个边沿：下降沿
现在是否启用：启用
```

接着清除一次挂起位。挂起位可以理解成 EXTI 的小旗子：发生边沿后旗子立起来，CPU 处理完必须把旗子放回去。

### 5. 配置 NVIC

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
nvic.NVIC_IRQChannel = EXTI0_IRQn;
nvic.NVIC_IRQChannelPreemptionPriority = 2U;
nvic.NVIC_IRQChannelSubPriority = 0U;
nvic.NVIC_IRQChannelCmd = ENABLE;
NVIC_Init(&nvic);
```

`NVIC_PriorityGroup_2` 是在划分“抢占优先级”和“子优先级”各占多少位。本讲只有一个中断，数字 2 和 0 暂时不会产生冲突；以后急停和定时器一起出现时，数字越小，抢占优先级越高。

`EXTI0_IRQn`、预抢占优先级、子优先级和启用开关四样凑齐，NVIC 才会把 EXTI0 放进 CPU 的响应名单。

## 七、中断函数为什么这么短

```c
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        exti0_event = 1U;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
```

它只做两件事：记下“按键发生过下降沿”，清掉 EXTI 的挂起位。

挂起位如果不清理，CPU 刚退出中断又会看见同一面旗子，可能不断重新进入中断。`EXTI_ClearITPendingBit(EXTI_Line0);` 就是告诉 EXTI：这次报告已经收到。

中断函数里先不自作主张切换 LED，只留下事件。主循环看到 `exti0_event` 后再消抖、确认和切换。这样中断执行得短，不会让 CPU 长时间停在一段处理里。

## 八、`exti0_event` 为什么要加 `volatile`

```c
volatile uint8_t exti0_event = 0U;
```

这个变量会被两个地方改：

```text
EXTI0_IRQHandler：发生中断时写成 1
main：处理完写成 0
```

编译器原本可能觉得主循环里的 `exti0_event` 没有被当前函数修改，于是把读取结果缓存到寄存器。`volatile` 是在告诉编译器：这个值可能被程序外部随时改变，每次都得重新从内存读。

它不是“把变量变快”，而是“别擅自省略对变量的重新读取”。

## 九、一次按下怎样只切换一次

机械按键产生下降沿后，金属触点还可能继续抖动。中断函数先记下事件，主循环随后：

```text
等一小会儿
再读 PA0
如果仍是 0，才切换 LED
最后把 exti0_event 清回 0
```

等待期间再来的抖动只会让 `exti0_event` 继续等于 1，不会堆成一串要执行的命令；主循环在确认后统一清掉，因此这一次按下只切换一次。

## 十、打开与编译

打开：

```text
stm32-course\stm32-demo\04_exti_button\exti_button.uvprojx
```

Keil 左侧：

```text
01_我的代码：exti_button.c，真正参与编译
02_本座示例：exti_button_demo.c，只供对照，不参与构建
```

按 `F7` 编译。本讲工程用 Arm Compiler 6.18 实编译，结果必须达到：

```text
0 Error(s), 0 Warning(s)
```

没有开发板时，可以在 Keil 模拟器里查看 EXTI、NVIC 和 GPIOC 的状态。模拟器不会让桌面上的实物灯发光。

## 十一、上板前检查

1. 开发板的 LED 是否真的接 PC13，是否低电平点亮。
2. PA0 是否真的有按钮，按钮另一端是否接 GND。
3. Keil 的 Debug 是否选择 ST-Link，接口是否为 SW。
4. 下载前先确认芯片能被识别，下载后再按复位键。

如果实际按钮一端接 3.3V，按下是高电平，本讲应改成 `EXTI_Trigger_Rising`，输入也应改成下拉，不能只改触发边沿。

## 十二、小测

1. 轮询和中断对 CPU 的要求有什么不同？
2. 为什么按一下 PA0 会出现下降沿？
3. PA0、PB0、PC0 与 EXTI0 是什么关系？
4. 为什么 AFIO 时钟也要打开？
5. EXTI 和 NVIC 少配置一个，分别会怎样？
6. `EXTI0_IRQHandler` 这个名字为什么不能随便改？
7. 清挂起位是在清什么？
8. `exti0_event` 为什么需要 `volatile`？

## 十三、动手题

1. 把 `EXTI_Trigger_Falling` 改成上升沿，推演什么时候会进中断。
2. 删掉 `EXTI_ClearITPendingBit`，解释为什么会反复进中断。
3. 删掉 `GPIO_EXTILineConfig`，说明为什么 EXTI0 不一定听得见 PA0。
4. 把 LED 换成 PA5 高电平点亮，列出要改的端口、引脚、时钟和输出电平。
5. 保留本讲工程不动，自己复制一份，改成“按一下亮，再按一下灭”和“按住亮、松开灭”两个版本，并比较哪个版本需要读取完整的按下与松开边沿。

---

**下一讲**：第 05 讲 TIM 定时器中断，用稳定的毫秒节拍替代长 `delay`，处理红闪和 10 秒锁机。
