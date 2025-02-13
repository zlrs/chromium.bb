/*
 * Copyright (C) 2006, 2007 Apple Computer, Inc.
 * Copyright (c) 2006, 2007, 2008, 2009, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/fonts/FontPlatformData.h"

#include "SkTypeface.h"
#include "platform/LayoutTestSupport.h"
#include "platform/fonts/FontCache.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/skia/SkiaUtils.h"
#include <windows.h>

namespace blink {

// Maximum font size, in pixels, at which embedded bitmaps will be used
// if available.
const float kMaxSizeForEmbeddedBitmap = 24.0f;

void FontPlatformData::setupPaint(SkPaint* paint, float, const Font* font) const {
  const float ts = m_textSize >= 0 ? m_textSize : 12;
  paint->setTextSize(SkFloatToScalar(m_textSize));
  paint->setTypeface(m_typeface);
  paint->setFakeBoldText(m_syntheticBold);
  paint->setTextSkewX(m_syntheticItalic ? -SK_Scalar1 / 4 : 0);

  uint32_t textFlags;
  if (font) {
      switch (font->getFontDescription().fontSmoothing()) {
      case NoSmoothing:
          textFlags = 0;
          break;
      case Antialiased:
          textFlags = SkPaint::kAntiAlias_Flag;
          break;
      case SubpixelAntialiased:
          textFlags = (SkPaint::kAntiAlias_Flag | SkPaint::kLCDRenderText_Flag);
          break;
      default:
          textFlags = paintTextFlags();
          break;
      }
  }
  else {
      textFlags = paintTextFlags();
  }

  uint32_t flags = paint->getFlags();
  static const uint32_t textFlagsMask =
      SkPaint::kAntiAlias_Flag | SkPaint::kLCDRenderText_Flag |
      SkPaint::kEmbeddedBitmapText_Flag | SkPaint::kSubpixelText_Flag;
  flags &= ~textFlagsMask;

  if (ts <= kMaxSizeForEmbeddedBitmap)
    flags |= SkPaint::kEmbeddedBitmapText_Flag;

  if (ts >= m_minSizeForAntiAlias) {
    // Disable subpixel text for certain older fonts at smaller sizes as
    // they tend to get quite blurry at non-integer sizes and positions.
    // For high-DPI this workaround isn't required.
    if ((ts >= m_minSizeForSubpixel ||
         FontCache::fontCache()->deviceScaleFactor() >= 1.5)

        // Subpixel text positioning looks pretty bad without font
        // smoothing. Disable it unless some type of font smoothing is used.
        // As most tests run without font smoothing we enable it for tests
        // to ensure we get good test coverage matching the more common
        // smoothing enabled behavior.
        && ((textFlags & SkPaint::kAntiAlias_Flag) ||
            LayoutTestSupport::isRunningLayoutTest()))
      flags |= SkPaint::kSubpixelText_Flag;

    SkASSERT(!(textFlags & ~textFlagsMask));
    flags |= textFlags;
  }

  paint->setFlags(flags);
}

static bool isWebFont(const String& familyName) {
  // Web-fonts have artifical names constructed to always be:
  // 1. 24 characters, followed by a '\0'
  // 2. the last two characters are '=='
  return familyName.length() == 24 && '=' == familyName[22] &&
         '=' == familyName[23];
}

static int computePaintTextFlags(String fontFamilyName) {
  if (LayoutTestSupport::isRunningLayoutTest())
    return LayoutTestSupport::isFontAntialiasingEnabledForTest()
               ? SkPaint::kAntiAlias_Flag
               : 0;

  int textFlags = 0;
  if (FontCache::fontCache()->antialiasedTextEnabled()) {
    int lcdFlag = FontCache::fontCache()->lcdTextEnabled()
                      ? SkPaint::kLCDRenderText_Flag
                      : 0;
    textFlags = SkPaint::kAntiAlias_Flag | lcdFlag;
  }

  // Many web-fonts are so poorly hinted that they are terrible to read when
  // drawn in BW.  In these cases, we have decided to FORCE these fonts to be
  // drawn with at least grayscale AA, even when the System (getSystemTextFlags)
  // tells us to draw only in BW.
  if (isWebFont(fontFamilyName))
    textFlags |= SkPaint::kAntiAlias_Flag;

  return textFlags;
}

void FontPlatformData::querySystemForRenderStyle() {
  m_paintTextFlags = computePaintTextFlags(fontFamilyName());
}

}  // namespace blink
