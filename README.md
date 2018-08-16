# cubes

**TODO**:
- [ ] incorporate signal strength into the decay?
- [ ] actually introduce genome concept
- [ ] reconsider color palette?

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
