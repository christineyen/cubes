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
const COLORS = [
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

var strip = new Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

const NODEID = 2;
var XMIT = [
  { nodeID: 7, ticks: 0 },
  { nodeID: 0, ticks: 0 },
  { nodeID: 3, ticks: 0 },
];

var palette = [];

function NUM_NODES() {
  return XMIT.length;
}

function setup() {
  strip.setBrightness(BRIGHTNESS);
  strip.begin();

  let numOtherNodes = 0;
  for (var i = 0; i < NUM_NODES(); i++) {
    if (XMIT[i].ticks > 0) {
      numOtherNodes++;
    }
  }

  let ledIdx = 0;
  let nodeIdx = 0;
  if (numOtherNodes > 0) {
    while (ledIdx < NUM_LEDS) {
      // just start off with the NODEID color
      if (nodeIdx % numOtherNodes == 0) {
        palette[ledIdx] = COLORS[NODEID];
        ledIdx++;
      }

      // find the next XMIT node with a legit number of ticks
      while (XMIT[nodeIdx % NUM_NODES()].ticks <= 0) {
        nodeIdx++;
      }

      // and add to the palette
      palette[ledIdx] = COLORS[XMIT[nodeIdx % NUM_NODES()].nodeID];
      ledIdx++;
      nodeIdx++;
    }
  } else {
    for (var i = 0; i < NUM_LEDS; i++) {
      palette[i] = COLORS[NODEID];
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
