#pragma once

#include "wlr_color.h"

#include <cstdint>

extern "C" {
#include <wlr/types/wlr_color_management_v1.h>
#include <wlr/types/wlr_output.h>
}

namespace umbriel {

  struct ColorLuminances {
    float min = 0.2F;
    float max = 80.0F;
    float reference = 80.0F;
  };

  struct ColorDescription {
    wlr_image_description_v1_data data{};
    wlr_color_primaries customPrimaries{};
    ColorLuminances luminances;
    float luminanceMultiplier = 1.0F;
    bool customLuminances = false;
    bool absoluteLuminance = false;
    bool requiresHdrOutput = false;
  };

  [[nodiscard]] ColorLuminances defaultColorLuminances(uint32_t transferFunction);
  [[nodiscard]] bool colorDescriptionsEqual(const ColorDescription& left, const ColorDescription& right);

  // HDR encodings can carry SDR content. A known content peak at or below
  // reference white does not request an automatic display mode switch unless
  // the client also supplies HDR mastering luminance.
  [[nodiscard]] bool colorDescriptionRequestsHdrOutput(const ColorDescription& description);

  // Brightest content light in cd/m², from MaxCLL and the mastering peak. The
  // smaller one wins when both are present, since MaxCLL above the mastering
  // display's peak is not meaningful. Placeholder values are ignored: the PQ
  // ceiling of 10000 cd/m², and a value that repeats the encoding's own
  // luminance range rather than describing the content. Zero means unknown.
  [[nodiscard]] float colorDescriptionContentPeak(const ColorDescription& description);

  // Automatic HDR offers a PQ encoding before scanout switches. Its target
  // volume still describes the current output, independently of that encoding.
  [[nodiscard]] ColorDescription outputColorDescription(
      const wlr_output_image_description* outputDescription, float effectiveSdrWhite, bool automaticHdrAvailable = false
  );

} // namespace umbriel
