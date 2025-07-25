# SimpleBoot

一个为 STM32F103C8T6 微控制器设计的鲁棒引导程序实现，使用 Y-modem 协议通过 UART 进行固件更新。也可用于其他 STM32F1x 微控制器或 STM32F4x 微控制器。

## 特性

- **Y-Modem 协议支持**：完整的 Y-modem 文件传输协议实现
- **UART 通信**：115200 波特率通信用于固件更新
- **Flash 管理**：自动 flash 擦除和编程，带验证功能
- **应用程序验证**：跳转前检查有效应用程序
- **多种进入方式**：按键按下、魔术数字或无有效应用程序
- **CRC32 验证**：确保固件完整性
- **LED 指示**：操作期间的视觉反馈
- **紧凑设计**：适配 16KB 引导程序空间

## 硬件要求

- **MCU**：STM32F103C8T6（蓝色药丸板或类似板子）
- **UART**：USART1（PB6-TX，PB7-RX）
- **LED**：连接到 PA1（可选）
- **按键**：连接到 PC13 并带上拉电阻（可选）
- **晶振**：8MHz 外部晶振

## 内存布局

```
Flash 内存 (64KB):
├── 0x08000000 - 0x08003FFF: 引导程序 (16KB)
└── 0x08004000 - 0x0800FFFF: 应用程序 (48KB)

RAM (20KB):
├── 0x20000000: 魔术数字位置
└── 0x20000010 - 0x20004FFF: 引导程序/应用程序可用空间
```

## 构建

### 前置条件

- ARM GCC 工具链（`arm-none-eabi-gcc`）
- Make 工具
- ST-Link 工具（用于烧录）

### 克隆和构建

```bash
# 克隆仓库
git clone <repository-url>
cd simpleboot

# 构建引导程序
make all

# 烧录到设备
make flash

# 清理构建文件
make clean
```

### 构建输出

成功构建后，您将得到：

- `build/bootloader.elf` - 用于调试的 ELF 文件
- `build/bootloader.hex` - Intel HEX 文件
- `build/bootloader.bin` - 用于烧录的二进制文件

## 使用方法

### 进入引导程序模式

如果满足以下任一条件，引导程序将进入更新模式：

1. **按键按下**：复位期间按住按键（PC13）
2. **魔术数字**：在 RAM 地址 `0x20000000` 设置魔术数字 `0xDEADBEEF` 并复位
3. **无有效应用程序**：当 flash 中未找到有效应用程序时

### 固件更新流程

1. **连接 UART**：将串口终端连接到 USART1（115200 波特率，8N1）
2. **进入引导程序**：使用上述进入方法之一
3. **开始传输**：使用支持 Y-modem 的终端（如 Tera Term、PuTTY 或 minicom）
4. **发送固件**：选择您的应用程序二进制文件进行 Y-modem 传输
5. **等待完成**：引导程序将编程 flash 并验证
6. **自动启动**：成功更新后，引导程序跳转到应用程序

### 终端命令

引导程序通过 UART 发送状态消息：

```
Entering bootloader mode
Waiting for firmware... Send file using Y-modem
Starting Y-modem reception...
Firmware received, programming flash...
Flash programming completed!
Verifying firmware...
Firmware verification successful!
Starting application...
```

### 更新应用程序

> 注意：引导程序日志使用与 ymodem 相同的 USART，因此需要跳过一些字符

- sz/rz

```bash
sz --delay-startup=2 -v --ymodem example_app/build/app.bin < /dev/ttyUSB0 >/dev/ttyUSB0
```

- Xshell/PuTTY/minicom/moserial

直接使用界面，通过 ymodem 协议发送文件

## 应用程序开发

### 链接脚本配置

您的应用程序必须配置为从 `0x08004000` 开始：

```ld
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 48K
  RAM (xrw)  : ORIGIN = 0x20000010, LENGTH = 20K - 0x10
}
```

### 中断向量表重定位

在应用程序的 `main()` 函数中：

```c
int main(void) {
  // 将中断向量表重定位到应用程序区域
  SCB->VTOR = 0x08004000;

  // 您的应用程序代码...
}
```

### 从应用程序触发引导程序

从应用程序进入引导程序：

```c
// 设置魔术数字并复位
*((uint32_t *)0x20000000) = 0xDEADBEEF;
HAL_NVIC_SystemReset();
```

## 文件结构

```
simpleboot/
├── example_app/            # 支持 OTA 的示例应用程序
├── Src/
│   ├── main.c              # 主程序和系统初始化
│   ├── bootloader.c        # 引导程序核心功能
│   └── ymodem.c           # Y-modem 协议实现
├── Inc/
│   ├── bootloader.h        # 引导程序定义
│   ├── ymodem.h           # Y-modem 协议定义
│   └── stm32f1xx_hal_conf.h # HAL 配置
├── startup/
│   └── startup_stm32f103c8tx.s # 启动汇编文件
├── linker/
│   └── STM32F103C8TX_BOOTLOADER.ld # 链接脚本
├── lib/                    # STM32 HAL 库文件
├── build/                  # 构建输出目录
├── Makefile               # 构建配置
└── README.md              # 英文说明文档
```

## 配置

`inc/bootloader.h` 中的关键配置参数：

```c
#define APPLICATION_START_ADDR 0x08004000  // 应用程序启动地址
#define BOOTLOADER_UART_BAUDRATE 115200   // UART 波特率
#define BOOTLOADER_TIMEOUT_MS 5000        // 用户输入超时时间
```

## Y-Modem 协议详情

该实现支持：

- **128 字节和 1024 字节数据包**
- **CRC16 错误检测**
- **错误时自动重试**
- **文件大小信息**

## 故障排除

### 常见问题

1. **引导程序无法进入**：
   - 检查按键接线和上拉电阻
   - 验证 UART 连接
   - 确保正确的复位时序

2. **Y-modem 传输失败**：
   - 检查 UART 设置（115200，8N1）
   - 验证终端软件的 Y-modem 支持
   - 尝试不同的终端软件

3. **应用程序无法启动**：
   - 验证应用程序从 0x08004000 开始
   - 检查中断向量表重定位
   - 确保应用程序大小不超过 48KB

4. **Flash 编程错误**：
   - 检查 flash 是否写保护
   - 验证应用程序二进制格式
   - 确保有足够的 flash 空间

### 调试选项

通过在 Makefile 中定义 `DEBUG=1` 启用调试输出：

```bash
make DEBUG=1 all
```

## 许可证

本项目按原样提供，用于教育和开发目的。

## 贡献

1. Fork 仓库
2. 创建功能分支
3. 进行更改
4. 彻底测试
5. 提交拉取请求

## 支持

如有问题和疑问：

- 查看故障排除部分
- 查阅 STM32F103 参考手册
- 参考 STM32 HAL 文档

---

**注意**：在部署到生产设备之前，请务必彻底测试引导程序。如果引导程序损坏，请保留恢复设备的备用方法。
