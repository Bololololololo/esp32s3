# ESP32 S3 LED Blink Project - AI Agent Instructions

## Project Overview

This is an ESP-IDF LED blink example targeting ESP32-S3 with support for multiple chip targets (ESP32, S2, S3, C2-C6, H2, P4). The application demonstrates GPIO-based or addressable LED (WS2812) control using either GPIO driver or the `led_strip` library.

## Architecture & Key Components

### LED Configuration Pattern
The project uses **conditional compilation** to support two LED types:
- **GPIO LED**: Direct GPIO level control via `driver/gpio.h`
- **LED Strip (Addressable)**: RMT or SPI backend via `espressif/led_strip` component

Configuration is determined at **build time** via `menuconfig` (see `main/Kconfig.projbuild`):
- `CONFIG_BLINK_LED_GPIO` vs `CONFIG_BLINK_LED_STRIP`
- `CONFIG_BLINK_LED_STRIP_BACKEND_RMT` vs `CONFIG_BLINK_LED_STRIP_BACKEND_SPI`

This means the compiled binary only includes code for the selected LED type—no runtime branching.

### File Structure
- `main/blink_example_main.c`: Main application with conditional LED logic
- `main/Kconfig.projbuild`: Configuration menu for LED type, GPIO pin, and blink period
- `main/idf_component.yml`: Declares `espressif/led_strip` dependency (v3.0.0+)
- `CMakeLists.txt`: Top-level ESP-IDF project with `MINIMAL_BUILD` for lean binaries
- `sdkconfig.defaults.esp32s3`: ESP32-S3 specific defaults (target for this workspace)

### Build System
- **ESP-IDF 5.x+** with CMake-based build system
- **Minimal build mode**: Only components the app directly uses are built
- **Multi-target support**: Use `idf.py set-target <chip>` to switch targets (esp32, esp32s2, esp32s3, etc.)
- **Component manager**: Dependencies auto-managed; run `idf-build` to fetch `led_strip` from registry

## Critical Developer Workflows

### Initial Setup
```bash
# Set target (default: ESP32-S3 for this project)
idf.py set-target esp32s3

# Configure LED type, GPIO, and timing
idf.py menuconfig
# Navigate: Example Configuration → Blink LED type, GPIO, period
```

### Build, Flash & Monitor (Combined)
```bash
idf.py -p /dev/ttyUSB0 flash monitor
# (Exit monitor: Ctrl-])
```

### Individual Steps
```bash
idf.py build                    # Compile only
idf.py -p /dev/ttyUSB0 flash   # Flash binary
idf.py -p /dev/ttyUSB0 monitor # Serial monitor (logs)
idf.py fullclean               # Clean build artifacts
```

### Testing
```bash
pytest pytest_blink.py -v
# Uses pytest-embedded-idf; tests check binary size and device interaction
```

### Size Analysis
```bash
idf.py size
# Shows binary segment breakdown; use this when optimizing for minimal builds
```

## Code Patterns & Conventions

### Logging
All ESP-IDF components use `ESP_LOG*` macros with a static TAG:
```c
static const char *TAG = "example";
ESP_LOGI(TAG, "Example configured to blink addressable LED!");
```
Logs appear on serial monitor; filter via `idf.py monitor | grep example`.

### GPIO & Peripheral Configuration
- **Always reset pin first** before configuring:
  ```c
  gpio_reset_pin(BLINK_GPIO);
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
  ```
- Use `CONFIG_*` macros from `sdkconfig.h` for build-time configuration
- Call `ESP_ERROR_CHECK()` on driver init to catch misconfigurations

### LED Strip Initialization Pattern
- Create config struct: `led_strip_config_t`
- Create backend config: `led_strip_rmt_config_t` or `led_strip_spi_config_t`
- Instantiate: `led_strip_new_rmt_device()` or `led_strip_new_spi_device()`
- Clear before use: `led_strip_clear(led_strip)`

### FreeRTOS Task Loop
```c
while (1) {
    blink_led();
    s_led_state = !s_led_state;
    vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
}
```
`vTaskDelay()` takes ticks (convert ms: `ms / portTICK_PERIOD_MS`).

## External Dependencies

### Component Manager
- **led_strip** (v3.0.0+): Manages WS2812 LED strips
  - Repo: https://components.espressif.com/component/espressif/led_strip
  - Auto-resolved via `main/idf_component.yml`
  - Provides: `led_strip_config_t`, `led_strip_handle_t`, init and control functions

### IDF Components (Built-in)
- `freertos` (kernel, tasks, delays)
- `driver/gpio` (GPIO control)
- `esp_log` (logging)
- `sdkconfig` (build configuration)

## Configuration & Customization

### Changing LED Behavior
Edit `Kconfig.projbuild` to add new options; changes appear in `idf.py menuconfig`.

### GPIO Constraints
- Check `sdkconfig.h` `ENV_GPIO_OUT_RANGE_MAX` for valid GPIO pins per target
- Some GPIOs reserved for flash/JTAG (see ESP32-S3 datasheet)
- Default: GPIO 8 (commonly safe for most boards)

### Extending the Example
To add a new peripheral (e.g., button):
1. Add Kconfig option in `main/Kconfig.projbuild`
2. Include driver header (e.g., `driver/gpio.h`)
3. Use `#ifdef CONFIG_NEW_FEATURE` for conditional compilation
4. Call `ESP_ERROR_CHECK()` on init
5. Run `idf.py fullclean` before rebuild (config changes require recompile)

## Troubleshooting Checklist

- **LED not blinking**: Verify GPIO selection in menuconfig matches schematic; check power
- **Build fails on new target**: Run `idf.py set-target <chip>` then `fullclean`
- **Component not found**: Run `idf-build` to sync component manager
- **Garbage on serial monitor**: Wrong baud rate (default 115200); check `idf.py monitor -b <baud>`
- **Binary size unexpectedly large**: Check `idf.py size`; verify `MINIMAL_BUILD` in root CMakeLists.txt

## AI Agent Priorities

When modifying this codebase:
1. **Maintain conditional compilation logic**—avoid runtime branching where compile-time exists
2. **Use `ESP_ERROR_CHECK()`** on all driver inits; fail early with informative logs
3. **Run tests**: `pytest pytest_blink.py -v` after code changes
4. **Verify multi-target support**: Test on at least ESP32 and ESP32-S3 if changing core logic
5. **Keep build minimal**: Don't add components unless necessary; review CMakeLists.txt impact
