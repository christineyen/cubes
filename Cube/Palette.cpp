#include "FastLED.h"       // FastLED library. Please use the latest development version.
#include "Palette.h"       // FastLED library. Please use the latest development version.
const CRGB PLT_PINKS[4] = { CRGB::DeepPink, CRGB::HotPink, CRGB::LightPink, CRGB::Pink  };
const CRGB PLT_REDS[4] = { CRGB::DarkRed, CRGB::IndianRed, CRGB::Red, CRGB::OrangeRed };
const CRGB PLT_ORANGES[4] = { CRGB::DarkOrange, CRGB::Orange, CRGB::Coral, CRGB::LightSalmon };
const CRGB PLT_YELLOWS[4] = { CRGB::Goldenrod, CRGB::Gold, CRGB::Yellow, CRGB::Khaki };
const CRGB PLT_GREENS[4] = { CRGB::DarkGreen, CRGB::ForestGreen, CRGB::Green, CRGB::LightGreen };
const CRGB PLT_TEALS[4] = { CRGB::LightSeaGreen, CRGB::Turquoise, CRGB::MediumAquamarine, CRGB::Aquamarine };
const CRGB PLT_LT_BLUES[4] = { CRGB::CornflowerBlue, CRGB::DodgerBlue, CRGB::DeepSkyBlue, CRGB::LightSkyBlue };
const CRGB PLT_DK_BLUES[4] = { CRGB::SteelBlue, CRGB::RoyalBlue, CRGB::Blue, CRGB::LightSteelBlue };
const CRGB PLT_PURPLES[4] = { CRGB::DarkViolet, CRGB::Amethyst, CRGB::MediumPurple, CRGB::Plum };
const CRGB PLT_WHITES[4] = { CRGB::Silver, CRGB::Gainsboro, CRGB::WhiteSmoke, CRGB::White };
const CRGB COLORS[10][4] = {
  PLT_PINKS,
  PLT_REDS,
  PLT_ORANGES,
  PLT_YELLOWS,
  PLT_GREENS,
  PLT_TEALS,
  PLT_LT_BLUES,
  PLT_DK_BLUES,
  PLT_PURPLES,
  PLT_WHITES
};
