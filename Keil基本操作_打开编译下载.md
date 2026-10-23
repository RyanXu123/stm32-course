# Keil 基本操作：打开、编译、模拟运行、下载

> 配套工程：`stm32-demo\01_led_blink\led_blink.uvprojx`
> 桌面快捷方式：`STM32第一讲点灯`

## 一、项目到底在哪

项目文件夹：

```text
stm32-course\stm32-demo\01_led_blink
```

要打开的项目文件：

```text
led_blink.uvprojx
```

`.uvprojx` 才是 Keil 工程文件。目录里的 `.c`、`.h`、`.s` 都是工程用到的零件，不能只双击 `main.c` 就以为打开了完整工程。

## 二、最简单的打开方法

从主人的工程目录 `stm32-demo` 进入对应讲次，双击里面的 `.uvprojx`。

完整入口表在：

```text
stm32-demo\工程索引.md
```

也可以：

1. 双击桌面的 `Keil uVision5`。
2. 点顶部菜单 `Project`。
3. 点 `Open Project...`。
4. 找到上面那个 `led_blink.uvprojx`。
5. 双击项目文件。

打开后，左边会看到：

```text
Project: led_blink
  STM32F103C8 LED Blink
    Application
    Startup
    System
    Library
```

`01_我的代码` 里的 `led_blink.c` 是主要业务代码。文件名只是文件名，真正被启动文件调用的是里面的 `int main(void)` 函数。`Startup`、`System`、`Library` 是工程运行需要的底座，第一讲不用改。

注意：`Add Files to Group` 是“把工程外的文件加进来”，不是“打开文件”。本工程已经把 `led_blink.c` 放进 `01_我的代码` 了，不需要再 Add。关掉添加窗口，直接双击工程树里的 `led_blink.c` 才会打开代码。

## 三、编译：F7

先别急着找“运行”按钮。Keil 的第一步是把 C 代码翻译成 STM32 能执行的机器指令：

1. 点一下左边工程树。
2. 按键盘 `F7`。
3. 看窗口最下方的 `Build Output`。

成功长这样：

```text
0 Error(s), 0 Warning(s)
```

只有 `Build Output` 里出现 `0 Error`，才说明编译成功。

编译产物在：

```text
stm32-demo\01_led_blink\Objects\led_blink.hex
```

`.axf` 用于调试，`.hex` 用于烧写。现在这个工程的 `led_blink.hex` 已经由本座实际生成过。

## 四、把误关的窗口找回来

右上角或窗格标题旁的 `X` 只会关闭“观察窗口”，不会删除工程、代码或编译结果。

点顶部 `View`，看到勾选项：

- `Project Window`：左边的工程文件树。
- `Build Output Window`：底部显示编译错误和结果的地方。
- `Functions Window`：函数列表。
- `Books Window`：芯片资料和手册。
- `Templates Window`：代码模板。

点一下对应项目即可重新打开；前面有勾表示现在正开着。若窗口变成浮动的，拖到主窗口边缘就能重新停靠。

## 五、没有开发板：用 Simulator 运行

当前工程配置的是 Keil Simulator。先按 `F7` 编译，确认 0 Error、0 Warning，再按 `Ctrl+F5` 进入调试。

进入调试后按 `F5` 运行。想看 GPIO 寄存器，打开：

```text
Peripherals → General Purpose I/O → GPIOC
```

Simulator 能执行程序和模拟寄存器，但板上的 LED 不会真的亮。想看到实物灯光，仍然需要开发板和 ST-Link。

## 六、有开发板：下载到 STM32

接线先确认四根：

```text
ST-Link 3.3V  → 板子 3.3V
ST-Link GND   → 板子 GND
ST-Link SWDIO → 板子 SWDIO
ST-Link SWCLK → 板子 SWCLK
```

不要拿 7.4V 电池直接接单片机供电。

下载步骤：

1. 接好 ST-Link 和板子。
2. 菜单 `Project → Options for Target...`。
3. 进入 `Debug` 页。
4. 右上角选择 `ST-Link Debugger`。
5. 点旁边的 `Settings`，接口选 `SW`，能识别到芯片后再继续。
6. 关掉设置窗口。
7. 按 `F8` 下载。
8. 下载完成后按板子的复位键，程序开始运行。

开发板上的 LED 引脚必须先确认。常见 STM32F103C8T6 最小板是 PC13，并且低电平点亮；如果实际板子的 LED 不在 PC13，不能直接照烧。

## 七、四个词不要混

| 操作 | 实际意思 |
|---|---|
| 编译 / Build | 把 C 变成机器指令，不碰芯片 |
| 下载 / Download | 通过 ST-Link 把程序写进芯片 Flash |
| 复位 / Reset | 让芯片从程序开头重新执行 |

电脑上的 C 程序编译完可以弹控制台。STM32 没有自动弹出来的控制台；程序要真正动硬件，必须下载到板上运行。

## 八、第一次操作练习

1. 打开 `stm32-demo\01_led_blink\led_blink.uvprojx`。
2. 找到左边 `01_我的代码`，点开加号，双击 `led_blink.c`。
3. 按 `F7`，确认底部出现 `0 Error(s), 0 Warning(s)`。
4. 故意删掉 `led_blink.c` 末尾一个分号，再按 `F7`，看看错误怎样出现在 `Build Output`。
5. 把分号补回来，再按 `F7`，确认恢复 `0 Error(s), 0 Warning(s)`。

