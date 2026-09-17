/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"
#include "synth_constants.h"

class Paths {
  public:
    static constexpr int kLogoWidth = 1701;

    Paths() = delete;

    static Path fromSvgData(const void* data, size_t data_size) {
      std::unique_ptr<Drawable> drawable(Drawable::createFromImageData(data, data_size));
      return drawable->getOutlineAsPath();
    }

    // The Audial wave mark, taken from the brand asset `wave.svg` (all <path d="..."> data,
    // concatenated). Replaces the upstream logo, which may not be redistributed.
    static const char* audialWavePathData() {
      static const char* kAudialWave =
        "M8.33301 16.6667C8.33301 15.1326 9.57666 13.8889 11.1108 13.8889C12.6449 13.8889 13.8886 15.1326 13.8886"
        " 16.6667V33.8611C13.8886 34.6652 13.8886 35.0673 13.7271 35.3722C13.5968 35.6182 13.3956 35.8194 13.1496"
        " 35.9497C12.8447 36.1111 12.4426 36.1111 11.6386 36.1111H10.583C9.77893 36.1111 9.37689 36.1111 9.07199 "
        "35.9497C8.82594 35.8194 8.62472 35.6182 8.49444 35.3722C8.33301 35.0673 8.33301 34.6652 8.33301 "
        "33.8611V16.6667Z M8.33301 58.3334C8.33301 59.8675 9.57666 61.1111 11.1108 61.1111C12.6449 61.1111 "
        "13.8886 59.8675 13.8886 58.3334V41.1389C13.8886 40.3348 13.8886 39.9328 13.7271 39.6279C13.5968 39.3819 "
        "13.3956 39.1806 13.1496 39.0504C12.8447 38.8889 12.4426 38.8889 11.6386 38.8889H10.583C9.77893 38.8889 "
        "9.37689 38.8889 9.07199 39.0504C8.82594 39.1806 8.62472 39.3819 8.49444 39.6279C8.33301 39.9328 8.33301 "
        "40.3348 8.33301 41.1389V58.3334Z M33.333 27.7778C33.333 26.2437 34.5767 25 36.1108 25C37.6449 25 38.8886"
        " 26.2437 38.8886 27.7778V34.5238C38.8886 34.7081 38.8886 34.8002 38.8798 34.8777C38.8073 35.5216 38.2991"
        " 36.0298 37.6552 36.1024C37.5777 36.1111 37.4855 36.1111 37.3013 36.1111H34.9203C34.736 36.1111 34.6439 "
        "36.1111 34.5664 36.1024C33.9225 36.0298 33.4143 35.5216 33.3417 34.8777C33.333 34.8002 33.333 34.7081 "
        "33.333 34.5238V27.7778Z M33.333 47.2222C33.333 48.7564 34.5767 50 36.1108 50C37.6449 50 38.8886 48.7564 "
        "38.8886 47.2222V40.4762C38.8886 40.2919 38.8886 40.1998 38.8798 40.1223C38.8073 39.4784 38.2991 38.9702 "
        "37.6552 38.8976C37.5777 38.8889 37.4855 38.8889 37.3013 38.8889H34.9203C34.736 38.8889 34.6439 38.8889 "
        "34.5664 38.8976C33.9225 38.9702 33.4143 39.4784 33.3417 40.1223C33.333 40.1998 33.333 40.2919 33.333 "
        "40.4762V47.2222Z M0 27.7778C0 26.2437 1.24365 25 2.77778 25C4.3119 25 5.55556 26.2437 5.55556 "
        "27.7778V34.5238C5.55556 34.7081 5.55556 34.8002 5.54682 34.8777C5.47427 35.5216 4.96609 36.0298 4.32217 "
        "36.1024C4.24466 36.1111 4.15253 36.1111 3.96825 36.1111H1.5873C1.40303 36.1111 1.31089 36.1111 1.23338 "
        "36.1024C0.589466 36.0298 0.081285 35.5216 0.00873304 34.8777C0 34.8002 0 34.7081 0 34.5238V27.7778Z M0 "
        "47.2222C0 48.7564 1.24365 50 2.77778 50C4.3119 50 5.55556 48.7564 5.55556 47.2222V40.4762C5.55556 "
        "40.2919 5.55556 40.1998 5.54682 40.1223C5.47427 39.4784 4.96609 38.9702 4.32217 38.8976C4.24466 38.8889 "
        "4.15253 38.8889 3.96825 38.8889H1.5873C1.40303 38.8889 1.31089 38.8889 1.23338 38.8976C0.589466 38.9702 "
        "0.081285 39.4784 0.00873304 40.1223C0 40.1998 0 40.2919 0 40.4762V47.2222Z M66.6665 27.7778C66.6665 "
        "26.2437 67.9102 25 69.4443 25C70.9784 25 72.2221 26.2437 72.2221 27.7778V34.5238C72.2221 34.7081 72.2221"
        " 34.8002 72.2133 34.8777C72.1408 35.5216 71.6326 36.0298 70.9887 36.1024C70.9112 36.1111 70.819 36.1111 "
        "70.6348 36.1111H68.2538C68.0695 36.1111 67.9774 36.1111 67.8999 36.1024C67.256 36.0298 66.7478 35.5216 "
        "66.6752 34.8777C66.6665 34.8002 66.6665 34.7081 66.6665 34.5238V27.7778Z M66.6665 47.2222C66.6665 "
        "48.7564 67.9102 50 69.4443 50C70.9784 50 72.2221 48.7564 72.2221 47.2222V40.4762C72.2221 40.2919 72.2221"
        " 40.1998 72.2133 40.1223C72.1408 39.4784 71.6326 38.9702 70.9887 38.8976C70.9112 38.8889 70.819 38.8889 "
        "70.6348 38.8889H68.2538C68.0695 38.8889 67.9774 38.8889 67.8999 38.8976C67.256 38.9702 66.7478 39.4784 "
        "66.6752 40.1223C66.6665 40.1998 66.6665 40.2919 66.6665 40.4762V47.2222Z M41.6665 16.6667C41.6665 "
        "15.1326 42.9102 13.8889 44.4443 13.8889C45.9784 13.8889 47.2221 15.1326 47.2221 16.6667V33.8611C47.2221 "
        "34.6652 47.2221 35.0673 47.0606 35.3722C46.9303 35.6182 46.7291 35.8194 46.4831 35.9497C46.1782 36.1111 "
        "45.7761 36.1111 44.9721 36.1111H43.9165C43.1124 36.1111 42.7104 36.1111 42.4055 35.9497C42.1594 35.8194 "
        "41.9582 35.6182 41.8279 35.3722C41.6665 35.0673 41.6665 34.6652 41.6665 33.8611V16.6667Z M41.6665 "
        "58.3334C41.6665 59.8675 42.9102 61.1111 44.4443 61.1111C45.9784 61.1111 47.2221 59.8675 47.2221 "
        "58.3334V41.1389C47.2221 40.3348 47.2221 39.9328 47.0606 39.6279C46.9303 39.3819 46.7291 39.1806 46.4831 "
        "39.0504C46.1782 38.8889 45.7761 38.8889 44.9721 38.8889H43.9165C43.1124 38.8889 42.7104 38.8889 42.4055 "
        "39.0504C42.1594 39.1806 41.9582 39.3819 41.8279 39.6279C41.6665 39.9328 41.6665 40.3348 41.6665 "
        "41.1389V58.3334Z M58.333 16.6667C58.333 15.1326 59.5767 13.8889 61.1108 13.8889C62.6449 13.8889 63.8886 "
        "15.1326 63.8886 16.6667V33.8611C63.8886 34.6652 63.8886 35.0673 63.7271 35.3722C63.5968 35.6182 63.3956 "
        "35.8194 63.1496 35.9497C62.8447 36.1111 62.4426 36.1111 61.6386 36.1111H60.583C59.7789 36.1111 59.3769 "
        "36.1111 59.072 35.9497C58.8259 35.8194 58.6247 35.6182 58.4944 35.3722C58.333 35.0673 58.333 34.6652 "
        "58.333 33.8611V16.6667Z M58.333 58.3334C58.333 59.8675 59.5767 61.1111 61.1108 61.1111C62.6449 61.1111 "
        "63.8886 59.8675 63.8886 58.3334V41.1389C63.8886 40.3348 63.8886 39.9328 63.7271 39.6279C63.5968 39.3819 "
        "63.3956 39.1806 63.1496 39.0504C62.8447 38.8889 62.4426 38.8889 61.6386 38.8889H60.583C59.7789 38.8889 "
        "59.3769 38.8889 59.072 39.0504C58.8259 39.1806 58.6247 39.3819 58.4944 39.6279C58.333 39.9328 58.333 "
        "40.3348 58.333 41.1389V58.3334Z M50 2.77778C50 1.24365 51.2437 0 52.7778 0C54.3119 0 55.5556 1.24365 "
        "55.5556 2.77778V33.8611C55.5556 34.6652 55.5556 35.0672 55.3941 35.3721C55.2638 35.6182 55.0626 35.8194 "
        "54.8166 35.9497C54.5117 36.1111 54.1096 36.1111 53.3056 36.1111H52.25C51.4459 36.1111 51.0439 36.1111 "
        "50.739 35.9497C50.4929 35.8194 50.2917 35.6182 50.1614 35.3721C50 35.0672 50 34.6652 50 33.8611V2.77778Z"
        " M50 72.2222C50 73.7564 51.2437 75 52.7778 75C54.3119 75 55.5556 73.7564 55.5556 72.2222V41.1389C55.5556"
        " 40.3348 55.5556 39.9328 55.3941 39.6279C55.2638 39.3819 55.0626 39.1806 54.8166 39.0504C54.5117 38.8889"
        " 54.1096 38.8889 53.3056 38.8889H52.25C51.4459 38.8889 51.0439 38.8889 50.739 39.0504C50.4929 39.1806 "
        "50.2917 39.3819 50.1614 39.6279C50 39.9328 50 40.3348 50 41.1389V72.2222Z M25 16.6667C25 15.1326 26.2437"
        " 13.8889 27.7778 13.8889C29.3119 13.8889 30.5556 15.1326 30.5556 16.6667V33.8611C30.5556 34.6652 30.5556"
        " 35.0673 30.3941 35.3722C30.2638 35.6182 30.0626 35.8194 29.8166 35.9497C29.5117 36.1111 29.1096 36.1111"
        " 28.3056 36.1111H27.25C26.4459 36.1111 26.0439 36.1111 25.739 35.9497C25.4929 35.8194 25.2917 35.6182 "
        "25.1614 35.3722C25 35.0673 25 34.6652 25 33.8611V16.6667Z M25 58.3334C25 59.8675 26.2437 61.1111 27.7778"
        " 61.1111C29.3119 61.1111 30.5556 59.8675 30.5556 58.3334V41.1389C30.5556 40.3348 30.5556 39.9328 30.3941"
        " 39.6279C30.2638 39.3819 30.0626 39.1806 29.8166 39.0504C29.5117 38.8889 29.1096 38.8889 28.3056 "
        "38.8889H27.25C26.4459 38.8889 26.0439 38.8889 25.739 39.0504C25.4929 39.1806 25.2917 39.3819 25.1614 "
        "39.6279C25 39.9328 25 40.3348 25 41.1389V58.3334Z M16.6665 2.77778C16.6665 1.24365 17.9102 0 19.4443 "
        "0C20.9784 0 22.2221 1.24365 22.2221 2.77778V33.8611C22.2221 34.6652 22.2221 35.0672 22.0606 "
        "35.3721C21.9303 35.6182 21.7291 35.8194 21.4831 35.9497C21.1782 36.1111 20.7761 36.1111 19.9721 "
        "36.1111H18.9165C18.1124 36.1111 17.7104 36.1111 17.4055 35.9497C17.1594 35.8194 16.9582 35.6182 16.8279 "
        "35.3721C16.6665 35.0672 16.6665 34.6652 16.6665 33.8611V2.77778Z M16.6665 72.2222C16.6665 73.7564 "
        "17.9102 75 19.4443 75C20.9784 75 22.2221 73.7564 22.2221 72.2222V41.1389C22.2221 40.3348 22.2221 39.9328"
        " 22.0606 39.6279C21.9303 39.3819 21.7291 39.1806 21.4831 39.0504C21.1782 38.8889 20.7761 38.8889 19.9721"
        " 38.8889H18.9165C18.1124 38.8889 17.7104 38.8889 17.4055 39.0504C17.1594 39.1806 16.9582 39.3819 16.8279"
        " 39.6279C16.6665 39.9328 16.6665 40.3348 16.6665 41.1389V72.2222Z";
      return kAudialWave;
    }

    static Path vitalV() {
      Path path = Drawable::parseSVGPath(audialWavePathData());
      path.applyTransform(path.getTransformToScaleToFit(0.0f, 0.0f, 1.0f, 1.0f, true));
      return path;
    }

    static Path vitalRing() {
      Path path;
      path.addEllipse(0.0f, 0.0f, 1.0f, 1.0f);
      path.addEllipse(0.12f, 0.12f, 0.76f, 0.76f);
      path.setUsingNonZeroWinding(false);
      return path;
    }

    static Path vitalWord() { return vitalV(); }

    static Path vitalWordRing() { return vitalRing(); }

    static Path chorus() {
      return fromSvgData((const void*)BinaryData::chorus_svg, BinaryData::chorus_svgSize);
    }

    static Path compressor() {
      return fromSvgData((const void*)BinaryData::compressor_svg, BinaryData::compressor_svgSize);
    }

    static Path delay() {
      return fromSvgData((const void*)BinaryData::delay_svg, BinaryData::delay_svgSize);
    }

    static Path distortion() {
      return fromSvgData((const void*)BinaryData::distortion_svg, BinaryData::distortion_svgSize);
    }

    static Path equalizer() {
      return fromSvgData((const void*)BinaryData::equalizer_svg, BinaryData::equalizer_svgSize);
    }

    static Path effectsFilter() {
      return fromSvgData((const void*)BinaryData::effects_filter_svg, BinaryData::effects_filter_svgSize);
    }

    static Path flanger() {
      return fromSvgData((const void*)BinaryData::flanger_svg, BinaryData::flanger_svgSize);
    }

    static Path folder() {
      return fromSvgData((const void*)BinaryData::folder_svg, BinaryData::folder_svgSize);
    }

    static Path phaser() {
      return fromSvgData((const void*)BinaryData::phaser_svg, BinaryData::phaser_svgSize);
    }

    static Path reverb() {
      return fromSvgData((const void*)BinaryData::reverb_svg, BinaryData::reverb_svgSize);
    }

    static Path prev() {
      static const PathStrokeType arrow_stroke(0.1f, PathStrokeType::JointStyle::curved,
                                               PathStrokeType::EndCapStyle::rounded);

      Path prev_line, prev_shape;
      prev_line.startNewSubPath(0.65f, 0.3f);
      prev_line.lineTo(0.35f, 0.5f);
      prev_line.lineTo(0.65f, 0.7f);

      arrow_stroke.createStrokedPath(prev_shape, prev_line);
      prev_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      prev_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return prev_shape;
    }

    static Path next() {
      static const PathStrokeType arrow_stroke(0.1f, PathStrokeType::JointStyle::curved,
                                               PathStrokeType::EndCapStyle::rounded);

      Path next_line, next_shape;
      next_line.startNewSubPath(0.35f, 0.3f);
      next_line.lineTo(0.65f, 0.5f);
      next_line.lineTo(0.35f, 0.7f);

      arrow_stroke.createStrokedPath(next_shape, next_line);
      next_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      next_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return next_shape;
    }

    static Path clock() {
      static const float kClockAngle = 1.0f;
      static const float kClockWidth = 0.4f;
      static const float kBuffer = (1.0f - kClockWidth) / 2.0f;

      Path path;
      path.addPieSegment(kBuffer, kBuffer, kClockWidth, kClockWidth, 0.0f, kClockAngle - 2.0f * vital::kPi, 0.0f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path dragDropArrows() {
      return fromSvgData((const void*)BinaryData::drag_drop_arrows_svg, BinaryData::drag_drop_arrows_svgSize);
    }

    static Path note() {
      static constexpr float kLeftAdjustment = 1.0f / 32.0f;
      static constexpr float kNoteWidth = 1.0f / 4.0f;
      static constexpr float kNoteHeight = 1.0f / 5.0f;
      static constexpr float kBarHeight = 2.0f / 5.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight + kNoteHeight / 2.0f) / 2.0f;
      static constexpr float kX = (1.0f - kNoteWidth) / 2.0f - kLeftAdjustment;

      Path path;
      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path tripletNotes() {
      static constexpr float kNoteWidth = 1.0f / 5.0f;
      static constexpr float kNoteHeight = 1.0f / 6.0f;
      static constexpr float kX = (1.0f - 3.0f * kNoteWidth) / 2.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kBarHeight = 1.0f / 4.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight) / 2.0f;

      Path path;
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight - kBarWidth,
                        2.0f * kNoteWidth + kBarWidth, kBarWidth);

      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + 2.0f * kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + 2.0f * kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path menu() {
      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      path.addLineSegment(Line<float>(0.2f, 0.3f, 0.8f, 0.3f), 0.05f);
      path.addLineSegment(Line<float>(0.2f, 0.5f, 0.8f, 0.5f), 0.05f);
      path.addLineSegment(Line<float>(0.2f, 0.7f, 0.8f, 0.7f), 0.05f);
      return path;
    }

    static Path menu(int width) {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacing = 0.2f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing = width * kSpacing;

      float center = (line_width % 2) * 0.5f + (width / 2);
      Path path;
      path.addLineSegment(Line<float>(buffer, center - spacing, width - buffer, center - spacing), line_width);
      path.addLineSegment(Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(Line<float>(buffer, center + spacing, width - buffer, center + spacing), line_width);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path plus(int width) {
      static constexpr float kBuffer = 0.35f;
      static constexpr float kLineWidth = 0.04f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);

      float center = (line_width % 2) * 0.5f + (width / 2);
      Path path;
      path.addLineSegment(Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(Line<float>(center, buffer, center, width - buffer), line_width);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path plusOutline() {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.15f;

      float start = kBuffer;
      float end = 1.0f - kBuffer;
      float close = 0.5f - kLineWidth * 0.5f;
      float far = 0.5f + kLineWidth * 0.5f;

      Path path;
      path.startNewSubPath(start, close);
      path.lineTo(start, far);
      path.lineTo(close, far);
      path.lineTo(close, end);
      path.lineTo(far, end);
      path.lineTo(far, far);
      path.lineTo(end, far);
      path.lineTo(end, close);
      path.lineTo(far, close);
      path.lineTo(far, start);
      path.lineTo(close, start);
      path.lineTo(close, close);
      path.lineTo(start, close);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path save(int width) {
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacingX = 0.3f;
      static constexpr float kSpacingY = 0.25f;
      static constexpr float kArrowMovement = 0.14f;
      static constexpr float kBoxWrap = 0.07f;

      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing_x = width * kSpacingX;
      int spacing_y = width * kSpacingY;
      float movement = width * kArrowMovement / 2.0f;
      float wrap = width * kBoxWrap;

      float center = (line_width % 2) * 0.5f + (width / 2);

      Path outline;
      outline.startNewSubPath(center, center - spacing_y);
      outline.lineTo(center, center + movement);

      outline.startNewSubPath(center - 2 * movement, center - movement);
      outline.lineTo(center, center + movement);
      outline.lineTo(center + 2 * movement, center - movement);

      outline.startNewSubPath(center - spacing_x + wrap, center);
      outline.lineTo(center - spacing_x, center);
      outline.lineTo(center - spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center);
      outline.lineTo(center + spacing_x - wrap, center);

      Path path;
      PathStrokeType stroke(line_width, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static Path downTriangle() {
      Path path;

      path.startNewSubPath(0.33f, 0.4f);
      path.lineTo(0.66f, 0.4f);
      path.lineTo(0.5f, 0.6f);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path upTriangle() {
      Path path;

      path.startNewSubPath(0.33f, 0.6f);
      path.lineTo(0.66f, 0.6f);
      path.lineTo(0.5f, 0.4f);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path exitX() {
      Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      Path path;
      PathStrokeType stroke(0.03f, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path thickX() {
      Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      Path path;
      PathStrokeType stroke(0.1f, PathStrokeType::curved, PathStrokeType::butt);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path star() {
      Path path;
      path.addStar(Point<float>(0.5f, 0.5f), 5, 0.2f, 0.4f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path keyboard() {
      Path path;
      path.addLineSegment(Line<float>(0.2f, 0.2f, 0.2f, 0.2f), 0.01f);
      path.addLineSegment(Line<float>(0.8f, 0.8f, 0.8f, 0.8f), 0.01f);

      path.startNewSubPath(0.24f, 0.35f);
      path.lineTo(0.24f, 0.65f);
      path.lineTo(0.41f, 0.65f);
      path.lineTo(0.41f, 0.5f);
      path.lineTo(0.35f, 0.5f);
      path.lineTo(0.35f, 0.35f);
      path.closeSubPath();

      path.startNewSubPath(0.48f, 0.65f);
      path.lineTo(0.48f, 0.35f);
      path.lineTo(0.52f, 0.35f);
      path.lineTo(0.52f, 0.5f);
      path.lineTo(0.58f, 0.5f);
      path.lineTo(0.58f, 0.65f);
      path.lineTo(0.42f, 0.65f);
      path.lineTo(0.42f, 0.5f);
      path.lineTo(0.48f, 0.5f);
      path.closeSubPath();

      path.startNewSubPath(0.65f, 0.35f);
      path.lineTo(0.76f, 0.35f);
      path.lineTo(0.76f, 0.65f);
      path.lineTo(0.59f, 0.65f);
      path.lineTo(0.59f, 0.5f);
      path.lineTo(0.65f, 0.5f);
      path.lineTo(0.65f, 0.35f);
      path.closeSubPath();

      return path;
    }

    static Path keyboardBordered() {
      Path board = keyboard();

      board.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.01f);
      board.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.01f);
      return board;
    }

    static Path fullKeyboard() {
      Path path;
      path.startNewSubPath(1, 0);
      path.lineTo(1, 2);
      path.lineTo(23, 2);
      path.lineTo(23, 1);
      path.lineTo(15, 1);
      path.lineTo(15, 0);
      path.closeSubPath();

      path.startNewSubPath(29, 0);
      path.lineTo(29, 1);
      path.lineTo(25, 1);
      path.lineTo(25, 2);
      path.lineTo(47, 2);
      path.lineTo(47, 1);
      path.lineTo(43, 1);
      path.lineTo(43, 0);
      path.closeSubPath();

      path.startNewSubPath(57, 0);
      path.lineTo(57, 1);
      path.lineTo(49, 1);
      path.lineTo(49, 2);
      path.lineTo(71, 2);
      path.lineTo(71, 0);
      path.closeSubPath();

      path.startNewSubPath(73, 0);
      path.lineTo(73, 2);
      path.lineTo(95, 2);
      path.lineTo(95, 1);
      path.lineTo(83, 1);
      path.lineTo(83, 0);
      path.closeSubPath();

      path.startNewSubPath(99, 0);
      path.lineTo(99, 1);
      path.lineTo(97, 1);
      path.lineTo(97, 2);
      path.lineTo(119, 2);
      path.lineTo(119, 1);
      path.lineTo(112, 1);
      path.lineTo(112, 0);
      path.closeSubPath();

      path.startNewSubPath(128, 0);
      path.lineTo(128, 1);
      path.lineTo(121, 1);
      path.lineTo(121, 2);
      path.lineTo(143, 2);
      path.lineTo(143, 1);
      path.lineTo(141, 1);
      path.lineTo(141, 0);
      path.closeSubPath();

      path.startNewSubPath(157, 0);
      path.lineTo(157, 1);
      path.lineTo(145, 1);
      path.lineTo(145, 2);
      path.lineTo(167, 2);
      path.lineTo(167, 0);
      path.closeSubPath();

      return path;
    }

    static Path gear() {
      static constexpr float kRadius = 0.3f;
      static constexpr float kGearOuterRatio = 1.2f;
      static constexpr int kNumGearTeeth = 8;

      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float offset = 0.5f - kRadius;
      float diameter = 2.0f * kRadius;
      path.addPieSegment(offset, offset, diameter, diameter, 0.0f, 2.0f * vital::kPi, 0.5f);
      for (int i = 0; i < kNumGearTeeth; ++i) {
        float phase = 2.0f * i * vital::kPi / kNumGearTeeth;
        float x_offset = kRadius * cosf(phase);
        float y_offset = kRadius * sinf(phase);
        Line<float> line(0.5f + x_offset, 0.5f + y_offset,
                         0.5f + kGearOuterRatio * x_offset, 0.5f + kGearOuterRatio * y_offset);
        path.addLineSegment(line, 0.13f);
      }

      return path;
    }

    static Path magnifyingGlass() {
      static constexpr float kRadius = 0.22f;
      static constexpr float kWidthRatio = 0.3f;
      static constexpr float kGlassOffset = 0.2f;
      static constexpr float kSqrt2 = 1.41421356237f;

      Path path;
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float diameter = 2.0f * kRadius;
      path.addPieSegment(kGlassOffset, kGlassOffset, diameter, diameter, 0.0f, 2.0f * vital::kPi, 1.0f - kWidthRatio);

      float line_width = kWidthRatio * kRadius;
      float line_start = kGlassOffset + kSqrt2 * kRadius + line_width / 2.0f;
      path.addLineSegment(Line<float>(line_start, line_start, 1.0f - kGlassOffset, 1.0f - kGlassOffset), line_width);

      return path;
    }

    static Path save() {
      Path outline;
      outline.startNewSubPath(0.5f, 0.25f);
      outline.lineTo(0.5f, 0.6f);
      outline.startNewSubPath(0.35f, 0.45f);
      outline.lineTo(0.5f, 0.6f);
      outline.lineTo(0.65f, 0.45f);

      outline.startNewSubPath(0.25f, 0.5f);
      outline.lineTo(0.2f, 0.5f);
      outline.lineTo(0.2f, 0.75f);
      outline.lineTo(0.8f, 0.75f);
      outline.lineTo(0.8f, 0.5f);
      outline.lineTo(0.75f, 0.5f);

      Path path;
      PathStrokeType stroke(0.05f, PathStrokeType::curved, PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path loop() {
      Path outline;
      outline.startNewSubPath(0.68f, 0.3f);
      outline.lineTo(0.85f, 0.3f);
      outline.lineTo(0.85f, 0.7f);
      outline.lineTo(0.15f, 0.7f);
      outline.lineTo(0.15f, 0.3f);
      outline.lineTo(0.61f, 0.3f);
      PathStrokeType outer_stroke(0.12f, PathStrokeType::curved, PathStrokeType::rounded);

      Path path;
      outer_stroke.createStrokeWithArrowheads(path, outline, 0.0f, 0.0f, 0.4f, 0.26f);
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path bounce() {
      Path left_outline, right_outline;
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::butt);
      Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      Path path;
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path bipolar() {
      Path left_outline, right_outline;
      left_outline.startNewSubPath(0.3f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.7f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::rounded);
      Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      Path path;
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path shuffle() {
      Path over;
      over.startNewSubPath(0.1f, 0.7f);
      over.lineTo(0.25f, 0.7f);
      over.lineTo(0.55f, 0.3f);
      over.lineTo(0.95f, 0.3f);

      Path under_first;
      under_first.startNewSubPath(0.1f, 0.3f);
      under_first.lineTo(0.25f, 0.3f);
      under_first.lineTo(0.325f, 0.4f);

      Path under_second;
      under_second.startNewSubPath(0.475f, 0.6f);
      under_second.lineTo(0.55f, 0.7f);
      under_second.lineTo(0.95f, 0.7f);

      PathStrokeType stroke(0.12f, PathStrokeType::curved, PathStrokeType::butt);
      Path over_path;
      stroke.createStrokeWithArrowheads(over_path, over, 0.0f, 0.0f, 0.35f, 0.26f);
      Path under_path_first;
      stroke.createStrokedPath(under_path_first, under_first);
      Path under_path_second;
      stroke.createStrokeWithArrowheads(under_path_second, under_second, 0.0f, 0.0f, 0.35f, 0.26f);

      Path path;
      path.addPath(over_path);
      path.addPath(under_path_first);
      path.addPath(under_path_second);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path pencil() {
      static constexpr float kPencilDimension = 0.14f;
      static constexpr float kEraserLength = 0.6f * kPencilDimension;
      static constexpr float kSeparatorWidth = 0.15f * kPencilDimension;
      static constexpr float kPencilRemoval = kEraserLength + kSeparatorWidth;
      static constexpr float kBorder = 0.22f;

      Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kPencilDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kPencilRemoval, kBorder + kPencilDimension + kPencilRemoval);
      path.lineTo(1.0f - kBorder - kPencilRemoval - kPencilDimension, kBorder + kPencilRemoval);
      path.lineTo(kBorder, 1.0f - kBorder - kPencilDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kPencilDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kPencilDimension);
      path.lineTo(1.0f - kBorder - kEraserLength, kBorder + kPencilDimension + kEraserLength);
      path.lineTo(1.0f - kBorder - kEraserLength - kPencilDimension, kBorder + kEraserLength);
      path.closeSubPath();
      
      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path halfSinCurve() {
      static constexpr float kBorder = 0.15f;
      static constexpr float kLineWidth = 0.1f;
      static constexpr float kEndpointStrokeWidth = 0.08f;
      static constexpr float kEndpointRadius = 0.09f;
      static constexpr int kNumCurvePoints = 16;
      static constexpr float kFullRadians = vital::kPi * 2.0f;
      static constexpr float kBumpIn = kEndpointRadius;
      static constexpr float kAdjustXIn = kBumpIn + kEndpointRadius / 2.0f;

      Path curve;
      float start_x = kBorder + kAdjustXIn;
      float start_y = 1.0f - kBorder - kBumpIn;
      float end_x = 1.0f - kBorder - kAdjustXIn;
      float end_y = kBorder + kBumpIn;

      curve.startNewSubPath(start_x, end_x);
      for (int i = 0; i < kNumCurvePoints; ++i) {
        float t = (1.0f + i) / kNumCurvePoints;
        float x = t * end_x + (1.0f - t) * start_x;
        float y_t = sinf((t - 0.5f) * vital::kPi) * 0.5f + 0.5f;
        float y = y_t * end_y + (1.0f - y_t) * start_y;
        curve.lineTo(x, y);
      }

      Path path;
      PathStrokeType line_stroke(kLineWidth, PathStrokeType::curved, PathStrokeType::butt);
      PathStrokeType endpoint_stroke(kEndpointStrokeWidth, PathStrokeType::curved, PathStrokeType::butt);
      line_stroke.createStrokedPath(path, curve);

      Path arc;
      arc.addCentredArc(end_x + kBumpIn, end_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      arc.addCentredArc(start_x - kBumpIn, start_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      Path end_points;
      endpoint_stroke.createStrokedPath(end_points, arc);
      path.addPath(end_points);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path ellipses() {
      Path path;
      path.addEllipse(0.1f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.7f, 0.4f, 0.2f, 0.2f);

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path paintBrush() {
      static constexpr float kBrushWideDimension = 0.3f;
      static constexpr float kBrushHandleDimension = 0.08f;
      static constexpr float kBrushLength = 0.6f * kBrushWideDimension;
      static constexpr float kSeparatorWidth = 0.15f * kBrushWideDimension;
      static constexpr float kConnectionDistance = kBrushLength + kSeparatorWidth;
      static constexpr float kHandleDistance = kConnectionDistance + kBrushWideDimension * 0.8f;
      static constexpr float kBorder = 0.15f;

      Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kBrushHandleDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kHandleDistance, kBorder + kBrushHandleDimension + kHandleDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance, kBorder + kBrushWideDimension + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance - kBrushWideDimension, kBorder + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kHandleDistance - kBrushHandleDimension, kBorder + kHandleDistance);
      path.lineTo(kBorder, 1.0f - kBorder - kBrushHandleDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kBrushWideDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kBrushWideDimension);
      path.lineTo(1.0f - kBorder - kBrushLength, kBorder + kBrushWideDimension + kBrushLength);
      path.lineTo(1.0f - kBorder - kBrushLength - kBrushWideDimension, kBorder + kBrushLength);
      path.closeSubPath();

      path.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static Path createFilterStroke(const Path& outline, float line_thickness = 0.1f) {
      PathStrokeType stroke(line_thickness, PathStrokeType::curved, PathStrokeType::rounded);

      Path path;
      stroke.createStrokedPath(path, outline);
      return path;
    }

    static Path lowPass(float line_thickness = 0.1f) {
      Path outline;
      outline.startNewSubPath(1.0f, 0.8f);
      outline.lineTo(0.7f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(0.0f, 0.5f);
      Path path = createFilterStroke(outline, line_thickness);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path highPass() {
      Path outline;
      outline.startNewSubPath(0.0f, 0.8f);
      outline.lineTo(0.3f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(1.0f, 0.5f);
      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path bandPass() {
      static constexpr float kMiddle = 3.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kMiddle);
      outline.lineTo(0.5f, 0.25f);
      outline.lineTo(2.0f / 3.0f, kMiddle);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static Path leftArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f - kArrowAmount);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static Path rightArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f - kArrowAmount);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static Path phaser1() {
      Path outline;
      outline.startNewSubPath(0.0f, 0.5f);
      outline.lineTo(1.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f / 2.0f, 1.0f / 4.0f);
      outline.lineTo(2.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f, 0.5f);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static Path phaser3() {
      static constexpr int kNumHumps = 5;

      static const float kUp = 1.0f / 4.0f;
      static const float kDown = 3.0f / 4.0f;

      float delta = 1.0f / (2 * kNumHumps + 2);
      Path outline;
      outline.startNewSubPath(0.0f, 0.5f);

      float position = 0.0f;
      for (int i = 0; i < kNumHumps; ++i) {
        position += delta;
        outline.lineTo(position, kDown);
        position += delta;
        outline.lineTo(position, kUp);
      }
      position += delta;
      outline.lineTo(position, kDown);
      outline.lineTo(1.0f, 0.5f);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static Path notch() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kTop);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(1.0f / 2.0f, kBottom);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kTop);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static Path narrowBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kBottom);
      outline.lineTo(0.5f, kTop);
      outline.lineTo(2.0f / 3.0f, kBottom);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static Path wideBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kBottom);

      Path path = createFilterStroke(outline);
      path.addLineSegment(Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }
};

