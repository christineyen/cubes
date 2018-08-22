# cubes

**TODO**:
- [x] incorporate signal strength into the decay?
- [x] add some thing so that only other Cubes are considered valid
- [ ] actually introduce genome concept
- [ ] reconsider color palette?
- [ ] it'd be nice if the palette adding/removing a color would be less abrupt
- [ ] rolling average?

## random shit around adding LEDs

For APAs (the default setup, for 4 ins)
```
void setup() {
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
}
```

Make APAs go really fast
```
void setup() {
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
}
```

For Neopixels (3 ins) <---- THIS IS THE ONE WE'RE DEALING WITH
```
void setup() {
   FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}
```

## what we've learned about measuring RSSI

at `0` power:
- ~1ft: -30
- ~2ft: -38 ? (antennae position is very sensitive)
- ~3ft: -33
- ~6ft: -46
- ~10ft: -56
