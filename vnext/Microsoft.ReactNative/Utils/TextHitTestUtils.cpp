// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "TextHitTestUtils.h"

namespace Microsoft::ReactNative {

static bool IsPointAfterCharacter(
    const winrt::Point &point,
    const winrt::TextPointer &textPointer,
    winrt::Rect rect,
    double width,
    bool isRtl) {
  const auto bottom = rect.Y + rect.Height;

  auto right = width;
  if (!isRtl) {
    // The character Rect always has Width = 0, so we use the X-dimension of
    // the next character on the same line. If the next character is not on the
    // same line, we use the rightmost boundary of the TextBlock.
    //
    // The side-effect is that LTR text may have additional hit box space at
    // the end of a line, but it's better than the alternative of the hit box
    // excluding the last character on a line.
    const auto nextPointer = textPointer.GetPositionAtOffset(1, winrt::LogicalDirection::Forward);
    if (nextPointer != nullptr) {
      const auto nextRect = nextPointer.GetCharacterRect(winrt::LogicalDirection::Forward);
      if (rect.Y == nextRect.Y) {
        right = nextRect.X;
      }
    }
  }

  // The character is before the point if the Y-coordinate of the point is
  // below (greater than) the bottom of the character rect, or if the
  // Y-coordinate is below (greater than) the top of the character rect and
  // the X-coordinate is "after" the end of the character rect (greater than
  // the right side for LTR or less than the left for RTL):
  // ┌────> X
  // │ ┌───────────┐  ┌───────────┐
  // ↓ │     ┌─────┘  └─────┐     │
  // Y └─────┘ (x,y)  (x,y) └─────┘
  //        LTR            RTL
  const auto isAfterX = !isRtl ? point.X > right : point.X < rect.X;
  return point.Y > bottom || (point.Y > rect.Y && isAfterX);
}

static bool IsPointBeforeCharacter(
    const winrt::Point &point,
    const winrt::TextPointer &textPointer,
    winrt::Rect rect,
    double width,
    bool isRtl) {
  const auto bottom = rect.Y + rect.Height;

  auto right = width;
  if (isRtl) {
    // The character Rect always has Width = 0, so we use the X-dimension of
    // the previous character on the same line. If the previous character is
    // not on the same line, we use the rightmost boundary of the TextBlock.
    //
    // The side-effect is that RTL text may have additional valid hit box
    // space at the end of a line, but it's better than the alternative of the
    // hit box excluding the last character on a line.
    const auto prevPointer = textPointer.GetPositionAtOffset(-1, winrt::LogicalDirection::Forward);
    if (prevPointer != nullptr) {
      const auto prevRect = prevPointer.GetCharacterRect(winrt::LogicalDirection::Forward);
      if (rect.Y == prevRect.Y) {
        right = prevRect.X;
      }
    }
  }

  // The character is after the point if the Y-coordinate of the point is above
  // (less than) the top of the character rect, or if the Y-coordinate is above
  // (less than) the bottom of the character rect and the X-coordinate is
  // "before" the beginning of the character rect (less than the left side for
  // LTR or greater than the right for RTL):
  // ┌────> X
  // │ (x,y) ┌─────┐  ┌─────┐ (x,y)
  // ↓ ┌─────┘     │  │     └─────┐
  // Y └───────────┘  └───────────┘
  //        LTR            RTL
  const auto isBeforeX = !isRtl ? point.X < rect.X : point.X > right;
  return point.Y < rect.Y || (point.Y < bottom && isBeforeX);
}

static bool IsRTL(const winrt::TextPointer &textPointer) {
  auto currentPointer = textPointer.GetPositionAtOffset(1, winrt::LogicalDirection::Forward);
  auto currentRect = textPointer.GetCharacterRect(winrt::LogicalDirection::Forward);
  auto firstCharacterRect = currentRect;
  while (currentPointer != nullptr) {
    currentRect = currentPointer.GetCharacterRect(winrt::LogicalDirection::Forward);
    // If we haven't figured out the direction by the time we reach the end of
    // a line, try again for the next line.
    if (currentRect.Y != firstCharacterRect.Y) {
      firstCharacterRect = currentRect;
    } else if (currentRect.X != firstCharacterRect.X) {
      return currentRect.X < firstCharacterRect.X;
    }

    currentPointer = currentPointer.GetPositionAtOffset(1, winrt::LogicalDirection::Forward);
  }

  // Assume LTR if there are not enough characters to determine LTR vs. RTL.
  // This may occur for TextBlocks with a single character per line.
  return false;
}

static winrt::TextPointer GetPositionFromPointCore(
    const winrt::TextPointer &start,
    const winrt::TextPointer &end,
    const winrt::Point &targetPoint) {
  // Since characters in a TextBlock are sorted from top-left to bottom-right
  // (or top-right to bottom-left for RTL), we can use binary search to find
  // the character with bounds that container the pointer point.
  //
  // This algorithm currently makes the following assumptions:
  // 1. Characters on the same line have the same Rect::Y value
  // 2. Search space is over only LTR or only RTL characters
  const auto width = start.VisualParent().Width();
  const auto isRtl = IsRTL(start);
  auto textPointer = start;
  auto L = start.Offset();
  auto R = end.Offset();
  while (L <= R) {
    const auto m = /* floor */ (L + R) / 2;
    const auto relativeOffset = m - textPointer.Offset();
    textPointer = textPointer.GetPositionAtOffset(relativeOffset, winrt::LogicalDirection::Forward);
    const auto rect = textPointer.GetCharacterRect(winrt::LogicalDirection::Forward);
    if (IsPointAfterCharacter(targetPoint, textPointer, rect, width, isRtl) /* A[m] < T */) {
      L = m + 1;
    } else if (IsPointBeforeCharacter(targetPoint, textPointer, rect, width, isRtl) /* A[m] > T */) {
      R = m - 1;
    } else {
      return textPointer;
    }
  }

  return nullptr;
}

winrt::TextPointer TextHitTestUtils::GetPositionFromPoint(const winrt::TextBlock &textBlock, const winrt::Point &targetPoint) {
  return GetPositionFromPointCore(textBlock.ContentStart(), textBlock.ContentEnd(), targetPoint);
}

winrt::TextPointer TextHitTestUtils::GetPositionFromPoint(const winrt::Run &run, const winrt::Point &targetPoint) {
  const auto start = run.ContentStart();
  const auto end = run.ContentEnd();

  auto startRect = start.GetCharacterRect(xaml::Documents::LogicalDirection::Forward);
  auto endRect = end.GetCharacterRect(xaml::Documents::LogicalDirection::Forward);

  // For runs on the same line, we can use a simple bounding box test
  if (startRect.Y == endRect.Y) {
    // Swap rectangles in RTL scenarios.
    if (startRect.X > endRect.X) {
      const auto tempRect = startRect;
      startRect = endRect;
      endRect = tempRect;
    }

    if ((startRect.X <= targetPoint.X) && (endRect.X + endRect.Width >= targetPoint.X) &&
        (startRect.Y <= targetPoint.Y) && (endRect.Y + endRect.Height >= targetPoint.Y)) {
      return start;
    }

    return nullptr;
  }

  return GetPositionFromPointCore(start, end, targetPoint);
}

} // namespace Microsoft::ReactNative
