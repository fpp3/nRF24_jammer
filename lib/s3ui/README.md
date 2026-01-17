# s3ui - Simple Small Screen UI Library

A high-level UI helper library built on Adafruit_GFX for small monochrome displays.

## Description

s3ui provides ready-to-use screens for common UI patterns on small displays:
- **Option Selection**: Display a list of selectable options with cursor
- **Editable Values**: Options with right-aligned editable values
- **Running Activity**: Static or animated bitmap displays with captions
- **Activity Log**: Scrolling live activity log
- **Confirmation Screen**: Action confirmation with text, optional bitmap, and smart button layout

The library is designed to be non-blocking and works with any Adafruit_GFX-compatible display (e.g., PCF8814/Nokia 1100, SSD1306, etc.).

## Features

- Non-blocking animations and updates
- Works with any Adafruit_GFX-compatible display
- Customizable fonts and sizes
- Auto-scrolling log with word wrapping
- Smooth frame-based animations
- Automatic layout calculations
- Smart button layout (horizontal, 2+1, or vertical stack)

## Installation

### Arduino IDE
1. Download this library as a ZIP file
2. In Arduino IDE, go to Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### PlatformIO
Add to your `platformio.ini`:
```ini
lib_deps =
    adafruit/Adafruit-GFX-Library
    adafruit/Adafruit_BusIO
    fpp3/s3ui
```

## Dependencies

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO)
- A compatible display driver (e.g., PCF8814, Adafruit_SSD1306)

## Quick Start

```cpp
#include <s3ui.h>
#include <PCF8814.h>

PCF8814 display = PCF8814(/* your pins */);
s3ui ui;

void setup() {
  display.begin();
  ui.setDisplay(&display, 96, 65);
  ui.setTitleFont(&Picopixel);
  ui.setContentFont(&Picopixel);
  ui.setTitleSize(1);
  ui.setContentSize(1);
}

void loop() {
  String options[] = {"Option 1", "Option 2", "Option 3"};
  ui.optionSelectScreen("Menu", "100%", options, 3, 0);
  display.display();
  
  ui.update(); // Handle animations and updates
}
```

## API Overview

### Initialization
- `setDisplay(Adafruit_GFX *display, uint16_t width, uint16_t height)` - Set the display instance
- `setTitleFont(const GFXfont *font)` - Set font for titles
- `setContentFont(const GFXfont *font)` - Set font for content
- `setTitleSize(uint8_t size)` - Set title text size
- `setContentSize(uint8_t size)` - Set content text size

### Screen Rendering
- `optionSelectScreen(...)` - Display selectable options
- `optionValueSetScreen(...)` - Display options with editable values
- `runningActivityScreen(...)` - Display static or animated activity
- `activityLiveLogScreen(...)` - Display scrolling log
- `confirmScreen(...)` - Display confirmation dialog with optional bitmap

### Updates
- `update()` - Call in loop() to handle animations and log refresh

### Log Management
- `appendLogLine(const String &line)` - Add a line to the log
- `clearLog()` - Clear all log lines
- `getLogLineCount()` - Get number of stored lines

### Utility
- `clear()` - Clear entire display
- `clearContentBox()` - Clear only content area
- `getGFX()` - Access underlying graphics context

## Examples

See the `examples/` folder for complete working examples:
- `activityLiveLog_test` - Live scrolling log demonstration
- `animated_runningActivityScreen` - Animated bitmap display
- `optionSelect_test` - Option selection menu
- `optionValueSet_test` - Editable values interface
- `static_runningActivityScreen_test` - Static activity display
- `confirmScreen_test` - Confirmation dialog with smart layout

## License

See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.
