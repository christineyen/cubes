// for use at http://htmlpreview.github.io/?https://github.com/T-vK/LedStripSimulator/blob/master/index.html
var PIN = 10; //for now the pin has to be 10
var NUM_LEDS = 60;
var BRIGHTNESS = 255;

const PINK = 0xfabebe;
const RED = 0xe6194b;
const ORANGE = 0xf58231;
const YELLOW = 0xffe119;
const GREEN = 0x3cb44b;
const TEAL = 0x008080;
const LT_BLUE = 0x46f0f0;
const DK_BLUE = 0x0082c8;
const PURPLE = 0x911eb4;
const WHITE = 0xffffff;
const COLOR_LIST = [
  PINK,
  RED,
  ORANGE,
  YELLOW,
  GREEN,
  TEAL,
  LT_BLUE,
  DK_BLUE,
  PURPLE,
  WHITE
];
const COLORS = COLOR_LIST.map((color) => {
  let arr = new Array(4);
  arr.fill(color);
  return arr;
});

var strip = new Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

const NODEID = 2;
var XMIT = [
  { nodeID: 0 },
  { nodeID: 3 },
];

var palette = [];

function NUM_NODES() {
  return XMIT.length;
}

function setup() {
  strip.setBrightness(BRIGHTNESS);
  strip.begin();

  const stretch = Math.floor(NUM_LEDS / (NUM_NODES() + 1));
  for (var i = 0; i < NUM_LEDS; i++) {
    if (Math.floor(i / stretch) < NUM_NODES()) {
      palette[i] = COLORS[XMIT[Math.floor(i / stretch)].nodeID][0];
    } else {
      palette[i] = COLORS[NODEID][0];
    }
  }
}

let startIndex = 0;

function loop() {
    fillLEDsFromPaletteColors(startIndex);
    strip.show();
    delay(500);
    startIndex++;
}

function fillLEDsFromPaletteColors(colorIndex) {
    for (var i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, palette[colorIndex%palette.length]);
      colorIndex += 1; // this is 3 in arduino-world because... the Palette is actually 256 items long
    }
}
