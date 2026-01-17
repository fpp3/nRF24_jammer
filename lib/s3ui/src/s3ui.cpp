#include "s3ui.h"

/**
 * @file s3ui.cpp
 * @brief Implementation of the s3ui helper built on Adafruit_GFX.
 */

// Constructor
s3ui::s3ui()
    : gfx(nullptr), displayWidth(0), displayHeight(0), animationActive(false), animationFrames(nullptr),
      currentFrame(0), totalFrames(0), frameDelay(0), lastFrameTime(0), bitmapWidth(0), bitmapHeight(0),
      captionText(""), logActive(false), titleFont(nullptr), contentFont(nullptr), titleSize(1), contentSize(1),
      titleFontHeight(0), contentFontHeight(0) {}

void s3ui::setDisplay(Adafruit_GFX *display, uint16_t width, uint16_t height) {
  gfx = display;
  displayWidth = width;
  displayHeight = height;
}

// Font configuration methods
void s3ui::setTitleFont(const GFXfont *font) {
  titleFont = font;
  titleFontHeight = font->yAdvance;
}

void s3ui::setContentFont(const GFXfont *font) {
  contentFont = font;
  contentFontHeight = font->yAdvance;
}

void s3ui::setTitleSize(uint8_t size) { titleSize = size; }

void s3ui::setContentSize(uint8_t size) { contentSize = size; }

void s3ui::showTitleAndBorder(const String &title, const String &batteryPercentage) {
  if (!gfx)
    return;

  // Title
  gfx->setTextColor(1);
  gfx->setTextWrap(false);
  gfx->setFont(titleFont);
  gfx->setCursor(titleFontHeight / 3, titleFontHeight - 1);
  gfx->print(title);

  // BatteryPercentage
  int16_t batteryWidth = strWidth(batteryPercentage, titleFont, titleSize);
  gfx->setCursor(displayWidth - batteryWidth - titleFontHeight / 3, titleFontHeight - 1);
  gfx->print(batteryPercentage);

  // MenuBoxOutline
  gfx->fillRect(0, titleFontHeight + titleMargin, displayWidth, displayHeight - (titleFontHeight + titleMargin), 1);
  // MenuBox
  gfx->fillRect(contentBoxThickness, titleFontHeight + titleMargin + contentBoxThickness,
                displayWidth - 2 * contentBoxThickness,
                displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness, 0);
}

// OptionSelect: Display a list of selectable options
void s3ui::showOptionSelect(const String *options, uint8_t numOptions, uint8_t cursorPos) {
  if (!gfx)
    return;

  // Content metrics
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;
  uint8_t optionHeight = contentFontHeight + 2 * optionPadding;
  uint16_t sliderBoxHeight = contentHeight - 2 * sliderPadding;

  // SliderBox
  gfx->drawRect(displayWidth - contentBoxThickness - sliderWidth - sliderPadding, contentTop + sliderPadding,
                sliderWidth, sliderBoxHeight, 1);

  // Compute how many options to render (always +1 to show partial option as visual indicator)
  uint8_t fullyVisibleCount = (optionHeight == 0) ? 1 : (contentHeight / optionHeight);
  if (fullyVisibleCount == 0)
    fullyVisibleCount = 1;
  uint8_t visibleCount = (numOptions > fullyVisibleCount) ? (fullyVisibleCount + 1) : numOptions;

  // Slider height calculation
  uint8_t sliderHeight;
  if (numOptions <= fullyVisibleCount) {
    // All options fit - slider is 100% of box height
    sliderHeight = sliderBoxHeight;
  } else {
    // Not all options fit - slider height proportional to visible content
    sliderHeight = (fullyVisibleCount * sliderBoxHeight) / numOptions;
    // Minimum slider height is 4px
    if (sliderHeight < 4)
      sliderHeight = 4;
  }

  // Slider position calculation
  uint16_t sliderPos;
  if (numOptions <= 1) {
    sliderPos = titleFontHeight + titleMargin + contentBoxThickness + sliderPadding;
  } else {
    // Position based on cursor, accounting for slider height
    sliderPos =
        contentTop + sliderPadding + (uint16_t)(cursorPos * (sliderBoxHeight - sliderHeight)) / (numOptions - 1);
  }

  gfx->drawRect(displayWidth - contentBoxThickness - sliderWidth - sliderPadding + 1, sliderPos, 1, sliderHeight, 1);

  // Options with windowed scrolling to avoid drawing off-screen
  gfx->setFont(contentFont);
  uint8_t topIndex = 0;
  if (numOptions > fullyVisibleCount) {
    // Try to center the selected item when possible
    int16_t desiredTop = (int16_t)cursorPos - (int16_t)(fullyVisibleCount / 2);

    // Maximum topIndex allows us to render visibleCount rows with last one partial
    // We need topIndex + fullyVisibleCount <= numOptions - 1 for last option to exist
    // So: topIndex <= numOptions - fullyVisibleCount - 1
    int16_t maxTop = (int16_t)numOptions - (int16_t)fullyVisibleCount - 1;
    if (maxTop < 0)
      maxTop = 0;

    if (desiredTop < 0)
      desiredTop = 0;
    if (desiredTop > maxTop)
      desiredTop = maxTop;

    topIndex = (uint8_t)desiredTop;
    if (cursorPos == numOptions - 1) {
      contentTop = contentTop - contentBoxThickness - (optionHeight - contentFontHeight) / 2 -
                   optionPadding; // Adjust contentTop to show last option properly
    }
  }

  for (uint8_t row = 0; row < visibleCount; row++) {
    uint8_t i = topIndex + row;
    if (i >= numOptions)
      break;
    uint16_t optionPos = contentTop + optionHeight * row;
    bool selected = (i == cursorPos);
    if (selected) {
      gfx->fillRect(contentBoxThickness + optionPadding, optionPos + optionPadding,
                    displayWidth - 2 * contentBoxThickness - 2 * optionPadding - sliderWidth - sliderPadding,
                    optionHeight, 1);
    }
    gfx->setCursor(contentBoxThickness + 2 * optionPadding + (selected ? 4 : 0),
                   optionPos + (optionHeight + contentFontHeight) / 2 - 1);
    gfx->setTextColor(selected ? 0 : 1);
    gfx->print(options[i]);
  }
}

// OptionSelect: Display a list of selectable options (screen wrapper)
void s3ui::optionSelectScreen(const String &title, const String &batteryPercentage, const String *options,
                              uint8_t numOptions, uint8_t cursorPos) {
  gfx->fillScreen(0);

  animationActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showOptionSelect(options, numOptions, cursorPos);
}

// OptionValueSet: Display options with editable values
void s3ui::showOptionValueSet(const String *optionNames, const String *optionValues, uint8_t numOptions,
                              uint8_t cursorPos, bool optionSelected) {
  if (!gfx)
    return;

  // Content metrics
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;
  uint8_t optionHeight = contentFontHeight + 4 * optionPadding;
  uint16_t sliderBoxHeight = contentHeight - 2 * sliderPadding;

  // SliderBox
  gfx->drawRect(displayWidth - contentBoxThickness - sliderWidth - sliderPadding, contentTop + sliderPadding,
                sliderWidth, sliderBoxHeight, 1);

  // Compute how many options to render (always +1 to show partial option as visual indicator)
  uint8_t fullyVisibleCount = (optionHeight == 0) ? 1 : (contentHeight / optionHeight);
  if (fullyVisibleCount == 0)
    fullyVisibleCount = 1;
  uint8_t visibleCount = (numOptions > fullyVisibleCount) ? (fullyVisibleCount + 1) : numOptions;

  // Slider height calculation
  uint8_t sliderHeight;
  if (numOptions <= fullyVisibleCount) {
    // All options fit - slider is 100% of box height
    sliderHeight = sliderBoxHeight;
  } else {
    // Not all options fit - slider height proportional to visible content
    sliderHeight = (fullyVisibleCount * sliderBoxHeight) / numOptions;
    // Minimum slider height is 4px
    if (sliderHeight < 4)
      sliderHeight = 4;
  }

  // Slider position calculation
  uint16_t sliderPos;
  if (numOptions <= 1) {
    sliderPos = titleFontHeight + titleMargin + contentBoxThickness + sliderPadding;
  } else {
    // Position based on cursor, accounting for slider height
    sliderPos =
        contentTop + sliderPadding + (uint16_t)(cursorPos * (sliderBoxHeight - sliderHeight)) / (numOptions - 1);
  }

  gfx->drawRect(displayWidth - contentBoxThickness - sliderWidth - sliderPadding + 1, sliderPos, 1, sliderHeight, 1);

  // Options with windowed scrolling to avoid drawing off-screen
  gfx->setFont(contentFont);
  uint8_t topIndex = 0;
  if (numOptions > fullyVisibleCount) {
    // Try to center the selected item when possible
    int16_t desiredTop = (int16_t)cursorPos - (int16_t)(fullyVisibleCount / 2);

    // Maximum topIndex allows us to render visibleCount rows with last one partial
    // We need topIndex + fullyVisibleCount <= numOptions - 1 for last option to exist
    // So: topIndex <= numOptions - fullyVisibleCount - 1
    int16_t maxTop = (int16_t)numOptions - (int16_t)fullyVisibleCount - 1;
    if (maxTop < 0)
      maxTop = 0;

    if (desiredTop < 0)
      desiredTop = 0;
    if (desiredTop > maxTop)
      desiredTop = maxTop;

    topIndex = (uint8_t)desiredTop;
    if (cursorPos == numOptions - 1) {
      contentTop = contentTop - contentBoxThickness - (optionHeight - contentFontHeight) / 2 -
                   optionPadding; // Adjust contentTop to show last option properly
    }
  }

  for (uint8_t row = 0; row < visibleCount; row++) {
    uint8_t i = topIndex + row;
    if (i >= numOptions)
      break;
    uint16_t optionPos = contentTop + optionHeight * row;
    bool selected = (i == cursorPos);
    if (selected) {
      if (optionSelected) {
        // Highlight value area
        gfx->fillRect(contentBoxThickness + optionPadding, optionPos + optionPadding,
                      displayWidth - 2 * contentBoxThickness - 2 * optionPadding - sliderWidth - sliderPadding,
                      optionHeight, 1);
      } else {
        // Highlight entire option
        gfx->drawRect(contentBoxThickness + optionPadding, optionPos + optionPadding,
                      displayWidth - 2 * contentBoxThickness - 2 * optionPadding - sliderWidth - sliderPadding,
                      optionHeight, 1);
      }
    }
    gfx->setCursor(contentBoxThickness + 2 * optionPadding + (selected ? 4 : 0),
                   optionPos + (optionHeight + contentFontHeight) / 2 - 1);
    gfx->setTextColor((selected && optionSelected) ? 0 : 1);
    gfx->print(optionNames[i]);

    // Draw increment/decrement icons if selected and editing
    if (selected && optionSelected) {
      String spacing = "  ";
      String value = "<" + spacing + optionValues[i] + spacing + ">";
      int16_t valueX = displayWidth - contentBoxThickness - sliderWidth - sliderPadding - optionPadding -
                       strWidth(value, contentFont, contentSize);
      gfx->setCursor(valueX, optionPos + (optionHeight + contentFontHeight) / 2 - 1);
      gfx->setTextColor(0);
      gfx->print(value);
    } else {
      // Draw value right-aligned when not editing
      int16_t valueX = displayWidth - contentBoxThickness - sliderWidth - sliderPadding - optionPadding -
                       strWidth(optionValues[i], contentFont, contentSize);
      gfx->setCursor(valueX, optionPos + (optionHeight + contentFontHeight) / 2 - 1);
      gfx->setTextColor((selected && optionSelected) ? 0 : 1);
      gfx->print(optionValues[i]);
    }
  }
}

// OptionValueSet: Display a list of options with editable values (screen wrapper)
void s3ui::optionValueSetScreen(const String &title, const String &batteryPercentage, const String *optionNames,
                                const String *optionValues, uint8_t numOptions, uint8_t cursorPos,
                                bool optionSelected) {
  gfx->fillScreen(0);

  animationActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showOptionValueSet(optionNames, optionValues, numOptions, cursorPos, optionSelected);
}

// RunningActivity: Display with static bitmap
void s3ui::showRunningActivity(const uint8_t *bitmap, uint16_t bitmapW, uint16_t bitmapH, const String &caption) {
  // center bitmap on contentBox considering that there has to be space for a caption
  if (!gfx)
    return;

  // Compute content box metrics
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;
  uint16_t contentLeft = contentBoxThickness;
  uint16_t contentWidth = displayWidth - 2 * contentBoxThickness;

  // Prepare caption: auto-wrap and interpret \n and \r like live log
  String capText = caption;
  bool hasCaption = capText.length() > 0 && contentFont;
  uint16_t availWidth = contentWidth - 2 * optionPadding;
  uint16_t lineHeight = contentFontHeight + contentFontHeight * 0.2; // match live log spacing

  // Compute wrapped caption lines
  std::vector<String> captionLines;
  if (hasCaption) {
    int16_t segmentStart = 0;
    for (int16_t pos = 0; pos <= (int16_t)capText.length(); pos++) {
      bool isNewline = (pos < (int16_t)capText.length()) && (capText[pos] == '\n' || capText[pos] == '\r');
      bool isEnd = (pos == (int16_t)capText.length());

      if (isNewline || isEnd) {
        String segment = capText.substring(segmentStart, pos);
        if (segment.length() > 0) {
          int16_t segmentWidth = strWidth(segment, contentFont, contentSize);
          if (segmentWidth <= availWidth) {
            captionLines.push_back(segment);
          } else {
            // Wrap segment into chunks
            uint16_t chunkStart = 0;
            while (chunkStart < segment.length()) {
              uint16_t chunkLen = findWrapPoint(segment, chunkStart, availWidth);
              if (chunkLen == 0)
                chunkLen = 1;
              captionLines.push_back(segment.substring(chunkStart, chunkStart + chunkLen));
              chunkStart += chunkLen;
            }
          }
        }
        segmentStart = pos + 1;
      }
    }
  }

  // Compute group vertical layout (bitmap + caption below)
  uint16_t captionTotalHeight = hasCaption ? (uint16_t)captionLines.size() * lineHeight : 0;
  uint16_t groupHeight = bitmapH + captionTotalHeight;
  int16_t groupTop = (int16_t)contentTop;
  if (groupHeight <= contentHeight) {
    groupTop = (int16_t)contentTop + ((int16_t)contentHeight - (int16_t)groupHeight) / 2;
  }

  // Center bitmap horizontally within content box
  int16_t bmpX = (bitmapW >= contentWidth) ? (int16_t)contentLeft
                                           : (int16_t)contentLeft + ((int16_t)contentWidth - (int16_t)bitmapW) / 2;
  int16_t bmpY = groupTop;

  // Draw bitmap
  gfx->drawBitmap(bmpX, bmpY, bitmap, bitmapW, bitmapH, 1);

  // Draw wrapped caption lines below the bitmap
  if (hasCaption && !captionLines.empty()) {
    gfx->setFont(contentFont);
    gfx->setTextColor(1);
    gfx->setTextWrap(false);

    int16_t drawY = bmpY + (int16_t)bitmapH;
    int16_t maxBaseline = (int16_t)contentTop + (int16_t)contentHeight - 1;
    for (size_t i = 0; i < captionLines.size(); i++) {
      const String &line = captionLines[i];
      int16_t lineW = strWidth(line, contentFont, contentSize);
      int16_t lineX = (int16_t)contentLeft + ((int16_t)contentWidth - lineW) / 2;
      int16_t baselineY = drawY + (int16_t)contentFontHeight - 1;
      if (baselineY > maxBaseline)
        break;
      gfx->setCursor(lineX, baselineY);
      gfx->print(line);
      drawY += lineHeight;
    }
  }
}

// RunningActivity: Display with static bitmap (screen wrapper)
void s3ui::runningActivityScreen(const String &title, const String &batteryPercentage, const uint8_t *bitmap,
                                 uint16_t bitmapW, uint16_t bitmapH, const String &caption) {
  gfx->fillScreen(0);

  animationActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showRunningActivity(bitmap, bitmapW, bitmapH, caption);
}

// RunningActivity: Display with animated bitmap (screen wrapper)
void s3ui::runningActivityScreen(const String &title, const String &batteryPercentage, const uint8_t **bitmaps,
                                 uint8_t numFrames, uint16_t bitmapW, uint16_t bitmapH, uint16_t msPerFrame,
                                 const String &caption) {
  gfx->fillScreen(0);

  // Setup animation state
  animationActive = true;
  animationFrames = bitmaps;
  totalFrames = numFrames;
  currentFrame = 0;
  frameDelay = msPerFrame;
  lastFrameTime = millis();
  bitmapWidth = bitmapW;
  bitmapHeight = bitmapH;
  captionText = caption;

  showTitleAndBorder(title, batteryPercentage);
  showRunningActivity(animationFrames[currentFrame], bitmapW, bitmapH, captionText);
}

// ActivityLiveLog: Display scrolling log (screen wrapper)
void s3ui::activityLiveLogScreen(const String &title, const String &batteryPercentage) {
  gfx->fillScreen(0);

  logActive = true;
  animationActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showActivityLiveLog();
}

// ActivityLiveLog: Display scrolling log
void s3ui::showActivityLiveLog() {
  if (!gfx || !contentFont)
    return;

  // Content box metrics
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentLeft = contentBoxThickness;
  uint16_t contentWidth = displayWidth - 2 * contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;

  // "Log:" label height
  uint16_t labelHeight = contentFontHeight;
  uint16_t labelY = contentTop;

  // Log sub-window position and size
  uint16_t logWindowTop = contentTop + labelHeight + optionPadding;
  uint16_t logWindowHeight = contentHeight - labelHeight - 2 * optionPadding;
  uint16_t logWindowLeft = contentLeft + optionPadding;
  uint16_t logWindowWidth = contentWidth - 2 * optionPadding;

  // Draw "Log:" label
  gfx->setFont(contentFont);
  gfx->setTextColor(1);
  gfx->setTextWrap(false);
  gfx->setCursor(logWindowLeft, labelY + (labelHeight + contentFontHeight) / 2 - 1);
  gfx->print("Log:");

  // Draw log window border
  gfx->drawRect(logWindowLeft, logWindowTop, logWindowWidth, logWindowHeight, 1);

  // Render visible log lines
  gfx->setFont(contentFont);
  uint8_t lineHeight = contentFontHeight + contentFontHeight * 0.2; // 1px spacing between lines
  uint16_t availWidth = logWindowWidth - 4 * optionPadding;         // Space for padding on both sides
  int16_t spaceWidth = strWidth(" ", contentFont, contentSize);

  // Calculate starting index to show the latest lines
  uint16_t totalLines = logLines.size();

  // First pass: calculate how many display lines each log line consumes
  std::vector<uint8_t> lineDisplayCounts;
  uint16_t totalDisplayLines = 0;
  for (uint16_t i = 0; i < totalLines; i++) {
    uint8_t displayLines = 0;
    const String &line = logLines[i];

    int16_t segmentStart = 0;
    for (int16_t pos = 0; pos <= (int16_t)line.length(); pos++) {
      bool isNewline = (pos < (int16_t)line.length()) && (line[pos] == '\n');
      bool isEnd = (pos == (int16_t)line.length());

      if (isNewline || isEnd) {
        String segment = line.substring(segmentStart, pos);

        // Skip empty segments (e.g., at end of line after final newline)
        if (segment.length() > 0) {
          int16_t segmentWidth = strWidth(segment, contentFont, contentSize);

          if (segmentWidth <= availWidth) {
            displayLines++;
          } else {
            // Count how many wrapped lines this segment takes
            uint16_t chunkStart = 0;
            while (chunkStart < segment.length()) {
              displayLines++;
              chunkStart += findWrapPoint(segment, chunkStart, availWidth);
            }
          }
        }
        segmentStart = pos + 1;
      }
    }
    lineDisplayCounts.push_back(displayLines);
    totalDisplayLines += displayLines;
  }

  // Calculate starting index so we show the latest lines that fit
  uint8_t visibleLineCount = (logWindowHeight - 2 * optionPadding) / lineHeight;
  if (visibleLineCount == 0)
    visibleLineCount = 1;

  int16_t startIndex = 0;
  uint16_t accumulatedLines = 0;
  for (int16_t i = (int16_t)totalLines - 1; i >= 0; i--) {
    if (accumulatedLines + lineDisplayCounts[i] > visibleLineCount) {
      startIndex = i + 1;
      break;
    }
    accumulatedLines += lineDisplayCounts[i];
  }

  // Second pass: render from startIndex onwards
  uint16_t drawY = logWindowTop + optionPadding;
  for (uint16_t i = startIndex; i < totalLines; i++) {
    const String &line = logLines[i];

    int16_t segmentStart = 0;
    for (int16_t pos = 0; pos <= (int16_t)line.length(); pos++) {
      bool isNewline = (pos < (int16_t)line.length()) && (line[pos] == '\n');
      bool isEnd = (pos == (int16_t)line.length());

      if (isNewline || isEnd) {
        String segment = line.substring(segmentStart, pos);

        // Skip empty segments (don't render or advance if segment is empty)
        if (segment.length() > 0) {
          int16_t segmentWidth = strWidth(segment, contentFont, contentSize);

          if (segmentWidth <= availWidth) {
            // Segment fits on one line
            gfx->setCursor(logWindowLeft + 2 * optionPadding, drawY + contentFontHeight - 1);
            gfx->setTextColor(1);
            gfx->print(segment);
            drawY += lineHeight;
          } else {
            // Segment needs wrapping: break at whitespace when possible
            uint16_t chunkStart = 0;
            while (chunkStart < segment.length()) {
              uint16_t chunkLen = findWrapPoint(segment, chunkStart, availWidth);
              if (chunkLen == 0)
                chunkLen = 1; // At least one character

              String chunk = segment.substring(chunkStart, chunkStart + chunkLen);
              gfx->setCursor(logWindowLeft + 2 * optionPadding, drawY + contentFontHeight - 1);
              gfx->setTextColor(1);
              gfx->print(chunk);
              drawY += lineHeight;
              chunkStart += chunkLen;
            }
          }
        }
        segmentStart = pos + 1;
      }
    }
  }
}

// Confirm: content-only (no screen clear) without bitmap
void s3ui::showConfirm(const String &question, const String *options, uint8_t numOptions, uint8_t selectedIndex) {
  // Delegate to the bitmap overload with nullptr bitmap
  showConfirm(nullptr, 0, 0, question, options, numOptions, selectedIndex);
}

// Confirm: content-only (no screen clear) with optional bitmap
void s3ui::showConfirm(const uint8_t *bitmap, uint16_t bitmapW, uint16_t bitmapH, const String &question,
                       const String *options, uint8_t numOptions, uint8_t selectedIndex) {
  if (!gfx || !contentFont)
    return;

  // Validate numOptions to be in range 1-3
  if (numOptions == 0)
    numOptions = 1;
  if (numOptions > 3)
    numOptions = 3;

  // Validate selectedIndex to not exceed numOptions-1
  if (selectedIndex >= numOptions)
    selectedIndex = numOptions - 1;

  // Content box metrics
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentLeft = contentBoxThickness;
  uint16_t contentWidth = displayWidth - 2 * contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;
  uint16_t contentBottom = contentTop + contentHeight;

  // Optional bitmap: top at optionPadding below inner border, horizontally centered
  int16_t bmpX = 0;
  int16_t bmpY = 0;
  bool hasBitmap = (bitmap != nullptr) && (bitmapW > 0) && (bitmapH > 0);
  if (hasBitmap) {
    bmpY = contentTop + optionPadding;
    bmpX = (bitmapW >= contentWidth) ? (int16_t)contentLeft
                                     : (int16_t)contentLeft + ((int16_t)contentWidth - (int16_t)bitmapW) / 2;
    gfx->drawBitmap(bmpX, bmpY, bitmap, bitmapW, bitmapH, 1);
  }

  // Question: wrap text to fit, placed half line below bitmap bottom (accounting for baseline positioning)
  gfx->setFont(contentFont);
  gfx->setTextColor(1);
  gfx->setTextWrap(false);
  
  int16_t maxQWidth = (int16_t)contentWidth - 2 * optionPadding;
  int16_t qStartY;
  if (hasBitmap) {
    // Half line below bitmap bottom, plus full font height to account for baseline
    qStartY = bmpY + (int16_t)bitmapH + (int16_t)(contentFontHeight / 2) + (int16_t)contentFontHeight;
  } else {
    // Without bitmap, start one line below inner top plus font height for baseline
    qStartY = contentTop + optionPadding + (int16_t)contentFontHeight;
  }

  // Wrap and render question text
  String qText = question;
  uint16_t qIdx = 0;
  int16_t currentY = qStartY;
  while (qIdx < qText.length()) {
    uint16_t chunkLen = findWrapPoint(qText, qIdx, maxQWidth);
    if (chunkLen == 0)
      chunkLen = 1;
    
    String line = qText.substring(qIdx, qIdx + chunkLen);
    int16_t lineW = strWidth(line, contentFont, contentSize);
    int16_t lineX = (int16_t)contentLeft + ((int16_t)contentWidth - lineW) / 2;
    
    gfx->setCursor(lineX, currentY - 1);
    gfx->print(line);
    
    currentY += contentFontHeight;
    qIdx += chunkLen;
  }

  // Options: calculate layout (horizontal if they fit, otherwise stacked)
  if (numOptions == 0 || options == nullptr)
    return;

  uint16_t buttonHeight = contentFontHeight + 2 * optionPadding;
  uint8_t hSpacing = optionPadding; // horizontal spacing between buttons
  uint8_t vSpacing = optionPadding; // vertical spacing between buttons

  // Calculate button widths
  int16_t btnWidths[3];
  int16_t totalWidth = 0;
  for (uint8_t i = 0; i < numOptions; i++) {
    int16_t labelW = strWidth(options[i], contentFont, contentSize);
    int16_t hPadding = 2 * optionPadding;
    btnWidths[i] = labelW + 2 * hPadding;
    totalWidth += btnWidths[i];
  }
  int16_t totalWidthWithSpacing = totalWidth + (numOptions - 1) * hSpacing;
  int16_t availWidth = (int16_t)contentWidth - 2 * optionPadding;

  // Bottom of the last option must be optionPadding px higher than inner border bottom
  int16_t buttonsBlockBottom = (int16_t)contentBottom - (int16_t)optionPadding;

  // Determine layout: try horizontal first
  bool allHorizontal = (numOptions <= 3 && totalWidthWithSpacing <= availWidth);
  bool twoTopOneBottom = false;
  
  if (!allHorizontal && numOptions == 3) {
    // Try 2 on top, 1 on bottom
    int16_t topTwoWidth = btnWidths[0] + hSpacing + btnWidths[1];
    twoTopOneBottom = (topTwoWidth <= availWidth && btnWidths[2] <= availWidth);
  }

  if (allHorizontal) {
    // All buttons on one row
    int16_t rowY = buttonsBlockBottom - buttonHeight;
    int16_t rowStartX = (int16_t)contentLeft + ((int16_t)contentWidth - totalWidthWithSpacing) / 2;
    int16_t currentX = rowStartX;

    for (uint8_t i = 0; i < numOptions; i++) {
      bool selected = (i == selectedIndex);
      if (selected) {
        gfx->fillRect(currentX, rowY, btnWidths[i], buttonHeight, 1);
        gfx->drawRect(currentX, rowY, btnWidths[i], buttonHeight, 1);
      } else {
        gfx->drawRect(currentX, rowY, btnWidths[i], buttonHeight, 1);
      }

      // Label
      int16_t labelW = strWidth(options[i], contentFont, contentSize);
      int16_t textX = currentX + (btnWidths[i] - labelW) / 2;
      int16_t textBaselineY = rowY + (buttonHeight + contentFontHeight - 1) / 2 - 1;
      gfx->setCursor(textX, textBaselineY);
      gfx->setTextColor(selected ? 0 : 1);
      gfx->print(options[i]);

      currentX += btnWidths[i] + hSpacing;
    }
  } else if (twoTopOneBottom) {
    // Two buttons on top row, one on bottom
    int16_t topRowY = buttonsBlockBottom - (2 * buttonHeight + vSpacing);
    int16_t bottomRowY = buttonsBlockBottom - buttonHeight;
    
    // Top row (buttons 0 and 1)
    int16_t topRowWidth = btnWidths[0] + hSpacing + btnWidths[1];
    int16_t topRowStartX = (int16_t)contentLeft + ((int16_t)contentWidth - topRowWidth) / 2;
    
    for (uint8_t i = 0; i < 2; i++) {
      int16_t btnX = topRowStartX + (i == 0 ? 0 : btnWidths[0] + hSpacing);
      bool selected = (i == selectedIndex);
      
      if (selected) {
        gfx->fillRect(btnX, topRowY, btnWidths[i], buttonHeight, 1);
        gfx->drawRect(btnX, topRowY, btnWidths[i], buttonHeight, 1);
      } else {
        gfx->drawRect(btnX, topRowY, btnWidths[i], buttonHeight, 1);
      }

      int16_t labelW = strWidth(options[i], contentFont, contentSize);
      int16_t textX = btnX + (btnWidths[i] - labelW) / 2;
      int16_t textBaselineY = topRowY + (buttonHeight + contentFontHeight - 1) / 2 - 1;
      gfx->setCursor(textX, textBaselineY);
      gfx->setTextColor(selected ? 0 : 1);
      gfx->print(options[i]);
    }
    
    // Bottom row (button 2)
    int16_t btnX = (int16_t)contentLeft + ((int16_t)contentWidth - btnWidths[2]) / 2;
    bool selected = (2 == selectedIndex);
    
    if (selected) {
      gfx->fillRect(btnX, bottomRowY, btnWidths[2], buttonHeight, 1);
      gfx->drawRect(btnX, bottomRowY, btnWidths[2], buttonHeight, 1);
    } else {
      gfx->drawRect(btnX, bottomRowY, btnWidths[2], buttonHeight, 1);
    }

    int16_t labelW = strWidth(options[2], contentFont, contentSize);
    int16_t textX = btnX + (btnWidths[2] - labelW) / 2;
    int16_t textBaselineY = bottomRowY + (buttonHeight + contentFontHeight - 1) / 2 - 1;
    gfx->setCursor(textX, textBaselineY);
    gfx->setTextColor(selected ? 0 : 1);
    gfx->print(options[2]);
  } else {
    // Vertical stack
    uint16_t totalButtonsHeight = (uint16_t)numOptions * buttonHeight + (uint16_t)(numOptions - 1) * vSpacing;
    int16_t buttonsBlockTop = buttonsBlockBottom - (int16_t)totalButtonsHeight;

    for (uint8_t i = 0; i < numOptions; i++) {
      int16_t btnX = (int16_t)contentLeft + ((int16_t)contentWidth - btnWidths[i]) / 2;
      int16_t btnY = buttonsBlockTop + i * (buttonHeight + vSpacing);

      bool selected = (i == selectedIndex);
      if (selected) {
        gfx->fillRect(btnX, btnY, btnWidths[i], buttonHeight, 1);
        gfx->drawRect(btnX, btnY, btnWidths[i], buttonHeight, 1);
      } else {
        gfx->drawRect(btnX, btnY, btnWidths[i], buttonHeight, 1);
      }

      int16_t labelW = strWidth(options[i], contentFont, contentSize);
      int16_t textX = btnX + (btnWidths[i] - labelW) / 2;
      int16_t textBaselineY = btnY + (buttonHeight + contentFontHeight - 1) / 2 - 1;
      gfx->setCursor(textX, textBaselineY);
      gfx->setTextColor(selected ? 0 : 1);
      gfx->print(options[i]);
    }
  }
}

// Confirm screen: wrapper without bitmap
void s3ui::confirmScreen(const String &title, const String &batteryPercentage, const String &question,
                         const String *options, uint8_t numOptions, uint8_t selectedIndex) {
  gfx->fillScreen(0);

  animationActive = false;
  logActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showConfirm(question, options, numOptions, selectedIndex);
}

// Confirm screen: wrapper with optional bitmap
void s3ui::confirmScreen(const String &title, const String &batteryPercentage, const uint8_t *bitmap, uint16_t bitmapW,
                         uint16_t bitmapH, const String &question, const String *options, uint8_t numOptions,
                         uint8_t selectedIndex) {
  gfx->fillScreen(0);

  animationActive = false;
  logActive = false;

  showTitleAndBorder(title, batteryPercentage);
  showConfirm(bitmap, bitmapW, bitmapH, question, options, numOptions, selectedIndex);
}

// Non-blocking update - handles animation frames and log screen refresh
void s3ui::update() {
  if (!gfx)
    return;

  // Handle animation frame updates
  if (animationActive) {
    unsigned long currentTime = millis();

    // Check if it's time to advance to next frame
    if (currentTime - lastFrameTime >= frameDelay) {
      currentFrame++;
      if (currentFrame >= totalFrames) {
        currentFrame = 0; // Loop animation
      }
      lastFrameTime = currentTime;
      clearContentBox();
      showRunningActivity(animationFrames[currentFrame], bitmapWidth, bitmapHeight, captionText);
    }
  }

  // Handle log screen refresh
  if (logActive) {
    clearContentBox();
    showActivityLiveLog();
  }
}

// Clear the display
void s3ui::clear() {
  if (!gfx)
    return;
  gfx->fillScreen(0);
  animationActive = false;
}

// Clear only the content box area
void s3ui::clearContentBox() {
  if (!gfx)
    return;
  uint16_t contentTop = titleFontHeight + titleMargin + contentBoxThickness;
  uint16_t contentHeight = displayHeight - (titleFontHeight + titleMargin) - 2 * contentBoxThickness;
  gfx->fillRect(contentBoxThickness, contentTop, displayWidth - 2 * contentBoxThickness, contentHeight, 0);
}

// Calculate the width in pixels of a string with given font and size
int16_t s3ui::strWidth(const String &str, const GFXfont *font, uint8_t size) {
  if (!gfx || !font)
    return 0;

  int16_t totalWidth = 0;
  gfx->setFont(font);
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c < font->first || c > font->last) {
      continue; // Character not in font
    }
    GFXglyph *glyph = &font->glyph[c - font->first];
    totalWidth += (glyph->xAdvance * size);
  }
  return totalWidth;
}

// Find wrap point: returns the number of characters from startIdx that fit within maxWidth
// Tries to break at whitespace; if no whitespace found, breaks at character limit
uint16_t s3ui::findWrapPoint(const String &str, uint16_t startIdx, uint16_t maxWidth) {
  if (startIdx >= str.length())
    return 0;

  // Find how many characters fit within maxWidth
  uint16_t endIdx = startIdx;
  int16_t lastSpaceIdx = -1;

  while (endIdx < str.length()) {
    String chunk = str.substring(startIdx, endIdx + 1);
    int16_t chunkWidth = strWidth(chunk, contentFont, contentSize);

    if (chunkWidth > maxWidth) {
      // This character doesn't fit; break before it
      if (lastSpaceIdx > startIdx) {
        // Use the last space we found (skip the space itself; next iteration will skip it)
        return lastSpaceIdx - startIdx + 1;
      } else {
        // No space found, break at previous character
        return (endIdx > startIdx) ? (endIdx - startIdx) : 1;
      }
    }

    // Character fits; remember if it's a space
    if (str[endIdx] == ' ') {
      lastSpaceIdx = endIdx;
    }
    endIdx++;
  }

  // All remaining characters fit - consume them all (don't break at space)
  return endIdx - startIdx;
}

// Append a line to the log
void s3ui::appendLogLine(const String &line) { logLines.push_back(line); }

// Clear all log lines
void s3ui::clearLog() { logLines.clear(); }
