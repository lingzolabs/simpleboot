# SimpleBoot

A robust bootloader implementation for STM32F103C8T6 microcontroller using Y-modem protocol for firmware updates over UART. Can be used with other STM32F1x microcontrollers or STM32F4x microcontrollers.

## Features

- **Y-Modem Protocol Support**: Full implementation of Y-modem file transfer protocol
- **UART Communication**: 115200 baud rate communication for firmware updates
- **Flash Management**: Automatic flash erase and programming with verification
- **Application Validation**: Checks for valid application before jumping
- **Multiple Entry Methods**: Button press, magic number, or no valid application
- **CRC32 Verification**: Ensures firmware integrity
- **LED Indicators**: Visual feedback during operation
- **Compact Design**: Fits in 16KB bootloader space

## Hardware Requirements

- **MCU**: STM32F103C8T6 (Blue Pill board or similar)
- **UART**: USART1 (PB6-TX, PB7-RX)
- **LED**: Connected to PA1 (optional)
- **Button**: Connected to PC13 with pull-up (optional)
- **Crystal**: 8MHz external crystal

## Memory Layout

```
Flash Memory (64KB):
├── 0x08000000 - 0x08003FFF: Bootloader (16KB)
└── 0x08004000 - 0x0800FFFF: Application (48KB)

RAM (20KB):
├── 0x20000000: Magic number location
└── 0x20000010 - 0x20004FFF: Available for bootloader/app
```

## Building

### Prerequisites

- ARM GCC toolchain (`arm-none-eabi-gcc`)
- Make utility
- ST-Link tools (for flashing)

### Clone and Build

```bash
# Clone the repository
git clone <repository-url>
cd simpleboot

# Build the bootloader
make all

# Flash to device
make flash

# Clean build files
make clean
```

### Build Output

After successful build, you'll find:

- `build/bootloader.elf` - ELF file for debugging
- `build/bootloader.hex` - Intel HEX file
- `build/bootloader.bin` - Binary file for flashing

## Usage

### Entering Bootloader Mode

The bootloader will enter update mode if any of these conditions are met:

1. **Button Press**: Hold the button (PC13) during reset
2. **Magic Number**: Set magic number `0xDEADBEEF` at RAM address `0x20000000` and reset
3. **No Valid Application**: When no valid application is found in flash

### Firmware Update Process

1. **Connect UART**: Connect your serial terminal to USART1 (115200 baud, 8N1)
2. **Enter Bootloader**: Use one of the entry methods above
3. **Start Transfer**: Use a Y-modem capable terminal (like Tera Term, PuTTY, or minicom)
4. **Send Firmware**: Select your application binary file for Y-modem transfer
5. **Wait for Completion**: Bootloader will program flash and verify
6. **Automatic Boot**: After successful update, bootloader jumps to application

### Terminal Commands

The bootloader sends status messages via UART:

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

### Update Application

> NOTE: BOOTLOADER LOG use same usart with ymodem, so need skip some chars

- sz/rz

```bash
sz --delay-startup=2 -v --ymodem example_app/build/app.bin < /dev/ttyUSB0 >/dev/ttyUSB0
```

- Xshell/PuTTY/minicom/moserial

just use ui, send file with ymodem protocol

## Application Development

### Linker Script Configuration

Your application must be configured to start at `0x08004000`:

```ld
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 48K
  RAM (xrw)  : ORIGIN = 0x20000010, LENGTH = 20K - 0x10
}
```

### Vector Table Relocation

In your application's `main()` function:

```c
int main(void) {
  // Relocate vector table to application area
  SCB->VTOR = 0x08004000;

  // Your application code here...
}
```

### Triggering Bootloader from Application

To enter bootloader from your application:

```c
// Set magic number and reset
*((uint32_t *)0x20000000) = 0xDEADBEEF;
HAL_NVIC_SystemReset();
```

## File Structure

```
simpleboot/
├── example_app/            # example application support OTA
├── Src/
│   ├── main.c              # Main program and system init
│   ├── bootloader.c        # Bootloader core functionality
│   └── ymodem.c           # Y-modem protocol implementation
├── Inc/
│   ├── bootloader.h        # Bootloader definitions
│   ├── ymodem.h           # Y-modem protocol definitions
│   └── stm32f1xx_hal_conf.h # HAL configuration
├── startup/
│   └── startup_stm32f103c8tx.s # Startup assembly file
├── linker/
│   └── STM32F103C8TX_BOOTLOADER.ld # Linker script
├── lib/                    # STM32 HAL library files
├── build/                  # Build output directory
├── Makefile               # Build configuration
└── README.md              # This file
```

## Configuration

Key configuration parameters in `inc/bootloader.h`:

```c
#define APPLICATION_START_ADDR 0x08004000  // App start address
#define BOOTLOADER_UART_BAUDRATE 115200   // UART baud rate
#define BOOTLOADER_TIMEOUT_MS 5000        // Timeout for user input
```

## Y-Modem Protocol Details

The implementation supports:

- **128-byte and 1024-byte packets**
- **CRC16 error detection**
- **Automatic retry on errors**
- **File size information**

## Troubleshooting

### Common Issues

1. **Bootloader won't enter**:
   - Check button wiring and pull-up resistor
   - Verify UART connections
   - Ensure proper reset sequence

2. **Y-modem transfer fails**:
   - Check UART settings (115200, 8N1)
   - Verify terminal software Y-modem support
   - Try different terminal software

3. **Application won't start**:
   - Verify application starts at 0x08004000
   - Check vector table relocation
   - Ensure application size doesn't exceed 48KB

4. **Flash programming errors**:
   - Check if flash is write-protected
   - Verify application binary format
   - Ensure sufficient flash space

### Debug Options

Enable debug output by defining `DEBUG=1` in Makefile:

```bash
make DEBUG=1 all
```

## License

This project is provided as-is for educational and development purposes.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:

- Check the troubleshooting section
- Review STM32F103 reference manual
- Consult STM32 HAL documentation

---

**Note**: Always test the bootloader thoroughly before deploying to production devices. Keep a backup method to recover the device if bootloader becomes corrupted.
