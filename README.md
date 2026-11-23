# 大符长条灯板

大符长条灯板的低亮度点灯测试。当前固件按 231 颗灯珠发送数据，上电先清灯，再点亮蓝色。

## 测试主控与固件

- **主控**为 Nordic nRF52840（Arm Cortex-M4F，64 MHz）。测试工程使用 Arduino nRF52 的 `PCA10056` 板级配置，数据从 `P0.09` 输出，由 `PWM0 + EasyDMA` 发送 24 位 GRB 单线数据，位周期为 1.25 µs。[Nordic nRF52840 规格](https://www.nordicsemi.com/products/nrf52840)
- `led231_blue.cpp` 是 C++ 点灯测试源码。`build.py` 使用 Arm GNU 工具链、CMSIS 和 Arduino nRF52 core 构建 `led231_blue.uf2`。该 UF2 针对使用 S140 6.1.1 的 nRF52840 UF2 引导程序。
- `P0.09` 与 NFC 复用，沿用测试接法时需要将其配置为 GPIO。Nordic 将该脚标为低频 I/O，正式版本可选用支持高速输出的普通 GPIO，并在首颗灯珠的 `DIN` 处复测波形。[Nordic 引脚规格](https://docs.nordicsemi.com/r/bundle/ps_nrf52840/page/pin.html?contentId=hMpuE~dcsjrrIlIt8ErcrQ)

## 移植到其他 Arm 主控

把 `led231_blue.cpp` 中的颜色、像素数量和 GRB 帧编码逻辑放进目标主控工程。按目标板替换 GPIO、PWM/DMA 等外设代码，保留单线脉宽和帧末低电平锁存时序，再用目标芯片的 SDK 和工具链重新编译烧录。移植时从源码构建目标固件，现有 UF2 对应上述 nRF52840 测试配置。

## 5 V 供电与接线

- **装机供电**使用 5 V DC-DC。主控板的 5 V 供电入口与灯板的 `5V/VIN` 均使用稳压 5 V。5 V 接到各板的供电入口，数据线电平按主控 GPIO 和灯板 `DIN` 的规格匹配。
- **主控与灯板必须共地。** DC-DC 输出地、主控 `GND` 和灯板 `GND` 连通。数据输出接首颗灯珠的 `DIN`，灯板的供电回流直接回 DC-DC 输出地。
- **点灯结果。** 主控板和灯板由同一台电脑供电，两板 `GND` 直接相连时，点灯正常。装机时使用相同的共地接法。
- 初次上电先核对 5 V 极性，从低亮度开始。提高亮度后，按实际电流检查 DC-DC 供电余量和灯板端电压。
