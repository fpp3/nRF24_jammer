#ifndef S3UI_H
#define S3UI_H

/**
 * @file s3ui.h
 * @brief High-level UI helper built on Adafruit_GFX for small monochrome displays.
 *
 * This library provides ready-to-use screens for option selection, editable option
 * values, running activity (static and animated bitmaps), and a live activity log.
 * It is designed to be non-blocking and to work with any Adafruit_GFX-compatible
 * display (e.g., PCF8814/Nokia 1100, Adafruit_SSD1306, etc.).
 */

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include <vector>

/**
 * @class s3ui
 * @brief UI rendering facade for common screens on Adafruit_GFX displays.
 *
 * Typical usage:
 * - Call setDisplay() with your Adafruit_GFX instance and dimensions.
 * - Configure fonts and sizes using setTitleFont()/setContentFont() etc.
 * - Render screens such as optionSelectScreen(), runningActivityScreen(), activityLiveLogScreen().
 * - Call update() from your loop() to advance animations and refresh the log.
 */
class s3ui {
private:
  /** @brief Target graphics context (must be set via setDisplay()). */
  Adafruit_GFX *gfx;
  /** @brief Physical display width in pixels. */
  uint16_t displayWidth;
  /** @brief Physical display height in pixels. */
  uint16_t displayHeight;

  // Animation state for RunningActivity (non-blocking)
  bool animationActive;            ///< True while an animated activity is active.
  const uint8_t **animationFrames; ///< Frame pointers for the current animation.
  uint8_t currentFrame;            ///< Current frame index.
  uint8_t totalFrames;             ///< Total number of frames in the animation.
  uint16_t frameDelay;             ///< Milliseconds per frame.
  unsigned long lastFrameTime;     ///< Millis timestamp of last frame switch.
  uint16_t bitmapWidth;            ///< Width of the animated bitmap.
  uint16_t bitmapHeight;           ///< Height of the animated bitmap.
  String captionText;              ///< Caption to render under the bitmap.

  // Logging state for ActivityLiveLog
  bool logActive;               ///< True while the live log screen is active.
  std::vector<String> logLines; ///< Stored log lines for display.

  // Font configuration
  const GFXfont *titleFont;   ///< Font used for the title and battery.
  const GFXfont *contentFont; ///< Font used for content areas.
  uint8_t titleSize;          ///< Logical scale factor applied to title metrics.
  uint8_t contentSize;        ///< Logical scale factor applied to content metrics.
  uint16_t titleFontHeight;   ///< Cached title font height (yAdvance).
  uint16_t contentFontHeight; ///< Cached content font height (yAdvance).

  // Constants that define how the UI looks
  const uint8_t titleMargin = 2;         ///< Vertical margin under the title bar (px).
  const uint8_t contentBoxThickness = 2; ///< Border thickness for the content box (px).
  const uint8_t sliderWidth = 3;         ///< Slider width for lists (px).
  const uint8_t sliderPadding = 1;       ///< Padding around slider (px).
  const uint8_t optionPadding = 1;       ///< Padding inside option rows (px).

  // Private helper methods
  /**
   * @brief Compute text width using the given font and size.
   * @param str String to measure.
   * @param font Font to use; must not be nullptr.
   * @param size Logical scale factor (1 = native font metrics).
   * @return Width in pixels.
   */
  int16_t strWidth(const String &str, const GFXfont *font, uint8_t size);
  /**
   * @brief Determine a wrapping point that fits within a maximum width.
   * @param str Source string.
   * @param startIdx Index to begin measuring.
   * @param maxWidth Maximum allowed width for the chunk.
   * @return Number of characters that fit, preferring a break at whitespace.
   */
  uint16_t findWrapPoint(const String &str, uint16_t startIdx, uint16_t maxWidth);

public:
  /** @brief Construct a new, uninitialized s3ui facade. */
  s3ui();

  // Display initialization - allows any Adafruit_GFX compatible display
  /**
   * @brief Bind an Adafruit_GFX display and set its dimensions.
   * @param display Pointer to an Adafruit_GFX-compatible instance.
   * @param width Display width in pixels.
   * @param height Display height in pixels.
   */
  void setDisplay(Adafruit_GFX *display, uint16_t width, uint16_t height);

  // Font configuration methods
  /** @brief Set the font used for the title and battery indicator. */
  void setTitleFont(const GFXfont *font);
  /** @brief Set the font used for content areas (lists, captions, logs). */
  void setContentFont(const GFXfont *font);
  /** @brief Set the logical title text size used for layout calculations. */
  void setTitleSize(uint8_t size);
  /** @brief Set the logical content text size used for layout calculations. */
  void setContentSize(uint8_t size);

  // Element rendering methods
  // These methods do not clear the screen by themselves; caller must do so if needed.
  // Its recommended to use the convenience screen methods below for most use cases.

  /**
   * @brief Render the title bar, battery percentage, and content border box.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @note This method does not clear the screen when called.
   */
  void showTitleAndBorder(const String &title, const String &batteryPercentage);

  // OptionSelect: Display a list of selectable options with cursor
  /**
   * @brief Render selectable options list inside the content box.
   * @param options Array of option strings.
   * @param numOptions Number of entries in options (max 256).
   * @param cursorPos Zero-based index of the currently selected option.
   * @note This method does not clear the screen when called.
   */
  void showOptionSelect(const String *options, uint8_t numOptions, uint8_t cursorPos);

  // OptionValueSet: Display options with editable values
  /**
   * @brief Render options with right-aligned values; highlights selection and edit state.
   * @param optionNames Array of option name strings.
   * @param optionValues Array of value strings for each option.
   * @param numOptions Number of entries (max 256).
   * @param cursorPos Zero-based index of the selection cursor.
   * @param optionSelected True while editing a value; false while navigating options.
   * @note This method does not clear the screen when called.
   */
  void showOptionValueSet(const String *optionNames, const String *optionValues, uint8_t numOptions, uint8_t cursorPos,
                          bool optionSelected);

  // RunningActivity: Display static bitmap with title and caption
  /**
   * @brief Render a centered static bitmap with an optional caption below it.
   * @param bitmap Pointer to 1-bit bitmap data.
   * @param bitmapW Bitmap width in pixels.
   * @param bitmapH Bitmap height in pixels.
   * @param caption Caption text; truncated with ellipsis if too wide.
   * @note This method does not clear the screen when called.
   */
  void showRunningActivity(const uint8_t *bitmap, uint16_t bitmapW, uint16_t bitmapH, const String &caption);

  // ActivityLiveLog: Display scrolling log of activity
  /**
   * @brief Render the current activity log within the content box.
   * @note This method does not clear the screen when called.
   */
  void showActivityLiveLog();

  // Confirm: Text/question with up to 3 selectable options, optional bitmap
  /**
   * @brief Render a confirmation content: centered question and up to 3 options.
   * @param question Text/question to render, horizontally centered.
   * @param options Array of option labels (1-3).
   * @param numOptions Number of options (1-3).
   * @param selectedIndex Zero-based index of the selected option.
   * @note This method does not clear the screen when called.
   */
  void showConfirm(const String &question, const String *options, uint8_t numOptions, uint8_t selectedIndex);

  /**
   * @brief Render a confirmation content with an optional bitmap above the question.
   * @param bitmap Pointer to 1-bit bitmap data (centered). If nullptr, behaves like the overload without bitmap.
   * @param bitmapW Bitmap width in pixels.
   * @param bitmapH Bitmap height in pixels.
   * @param question Text/question to render, centered horizontally.
   * @param options Array of option labels (1-3).
   * @param numOptions Number of options (1-3).
   * @param selectedIndex Zero-based index of the selected option.
   * @note This method does not clear the screen when called.
   */
  void showConfirm(const uint8_t *bitmap, uint16_t bitmapW, uint16_t bitmapH, const String &question,
                   const String *options, uint8_t numOptions, uint8_t selectedIndex);

  // Screen rendering methods

  /**
   * @brief Convenience screen: title+border + option list.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param options Array of option strings.
   * @param numOptions Number of entries in options (max 256).
   * @param cursorPos Zero-based index of the currently selected option.
   * @note This method clears the screen each time it is called.
   */
  void optionSelectScreen(const String &title, const String &batteryPercentage, const String *options,
                          uint8_t numOptions, uint8_t cursorPos);

  /**
   * @brief Convenience screen: title+border + editable options list.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param optionNames Array of option name strings.
   * @param optionValues Array of value strings for each option.
   * @param numOptions Number of entries (max 256).
   * @param cursorPos Zero-based index of the selection cursor.
   * @param optionSelected True while editing a value; false while navigating options.
   * @note This method clears the screen each time it is called.
   */
  void optionValueSetScreen(const String &title, const String &batteryPercentage, const String *optionNames,
                            const String *optionValues, uint8_t numOptions, uint8_t cursorPos, bool optionSelected);

  /**
   * @brief Convenience screen: title+border + static running activity.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param bitmap Pointer to 1-bit bitmap data.
   * @param bitmapW Bitmap width in pixels.
   * @param bitmapH Bitmap height in pixels.
   * @param caption Caption text to show below the bitmap.
   * @note This method clears the screen each time it is called.
   */
  void runningActivityScreen(const String &title, const String &batteryPercentage, const uint8_t *bitmap,
                             uint16_t bitmapW, uint16_t bitmapH, const String &caption);

  /**
   * @brief Convenience screen: title+border + animated running activity.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param bitmaps Array of pointers to 1-bit bitmap frame data.
   * @param numFrames Number of frames in the animation.
   * @param bitmapW Bitmap width in pixels.
   * @param bitmapH Bitmap height in pixels.
   * @param msPerFrame Milliseconds to display each frame.
   * @param caption Caption text to show below the bitmap.
   * @note This method clears the screen each time it is called.
   */
  void runningActivityScreen(const String &title, const String &batteryPercentage, const uint8_t **bitmaps,
                             uint8_t numFrames, uint16_t bitmapW, uint16_t bitmapH, uint16_t msPerFrame,
                             const String &caption);

  /**
   * @brief Convenience screen: title+border + live log.
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @note This method clears the screen each time it is called.
   */
  void activityLiveLogScreen(const String &title, const String &batteryPercentage);

  /**
   * @brief Convenience screen: title+border + confirm (no bitmap).
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param question Text/question to render.
   * @param options Array of option labels (1-3).
   * @param numOptions Number of options (1-3).
   * @param selectedIndex Zero-based index of the selected option.
   * @note This method clears the screen each time it is called.
   */
  void confirmScreen(const String &title, const String &batteryPercentage, const String &question,
                     const String *options, uint8_t numOptions, uint8_t selectedIndex);

  /**
   * @brief Convenience screen: title+border + confirm (with optional bitmap).
   * @param title Title text to show in the top-left.
   * @param batteryPercentage Battery status text (e.g. "84%") aligned to top-right.
   * @param bitmap Pointer to 1-bit bitmap data (centered). Pass nullptr to omit.
   * @param bitmapW Bitmap width in pixels.
   * @param bitmapH Bitmap height in pixels.
   * @param question Text/question to render.
   * @param options Array of option labels (1-3).
   * @param numOptions Number of options (1-3).
   * @param selectedIndex Zero-based index of the selected option.
   * @note This method clears the screen each time it is called.
   */
  void confirmScreen(const String &title, const String &batteryPercentage, const uint8_t *bitmap, uint16_t bitmapW,
                     uint16_t bitmapH, const String &question, const String *options, uint8_t numOptions,
                     uint8_t selectedIndex);

  /**
   * @brief Non-blocking update; advances animations and refreshes live log.
   * @note Call this from loop() when using animated activity or live log screens.
   */
  void update();

  // Utility methods
  /** @brief Clear entire display and stop any active animation. */
  void clear();
  /** @brief Clear only the content area inside the border box. */
  void clearContentBox();
  /** @brief Access the underlying graphics context (for custom drawing). */
  Adafruit_GFX *getGFX() { return gfx; }

  // Log management methods (line-by-line append model)
  /**
   * @brief Append a line to the live activity log.
   * @param line Text to append; embedded '\n' creates multi-line entries.
   */
  void appendLogLine(const String &line);
  /** @brief Clear all stored log lines. */
  void clearLog();
  /** @brief Number of stored log lines. */
  uint16_t getLogLineCount() const { return logLines.size(); }

  // Font getters
  /** @brief Currently configured title font pointer. */
  const GFXfont *getTitleFont() { return titleFont; }
  /** @brief Currently configured content font pointer. */
  const GFXfont *getContentFont() { return contentFont; }
  /** @brief Current logical title size. */
  uint8_t getTitleSize() { return titleSize; }
  /** @brief Current logical content size. */
  uint8_t getContentSize() { return contentSize; }
};

#endif
