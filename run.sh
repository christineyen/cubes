set -e

APP_JAVA_CONTENTS=/Applications/Arduino.app/Contents/Java
BUILD_PATH=/Users/cyen/src/arduino/build
#SKETCH=$(sed 's/\/$//' <<< $1)

PORT1=cu.usbserial-AK05BGOF
PORT2=cu.usbserial-A504WYJQ

BAUD=57600

run_sketch() { # takes three args: SKETCH, NODEID, PORT
  local SKETCH=$1
  local NODEID=$2
  local PORT=$3
  arduino-builder -compile \
    -hardware $APP_JAVA_CONTENTS/hardware \
    -tools $APP_JAVA_CONTENTS/tools-builder \
    -tools $APP_JAVA_CONTENTS/hardware/tools/avr \
    -built-in-libraries $APP_JAVA_CONTENTS/libraries \
    -libraries /Users/cyen/Documents/Arduino/libraries \
    -fqbn=arduino:avr:pro:cpu=16MHzatmega328 \
    -build-path=$BUILD_PATH \
    -prefs=build.extra_flags=-DNODEID=$NODEID \
    $SKETCH/$SKETCH.ino

  $APP_JAVA_CONTENTS/hardware/tools/avr/bin/avrdude \
    -C $APP_JAVA_CONTENTS/hardware/tools/avr/etc/avrdude.conf \
    -v \
    -patmega328p \
    -carduino \
    -P /dev/$PORT \
    -b$BAUD \
    -D \
    -Uflash:w:$BUILD_PATH/$SKETCH.ino.hex:i
}

echo "======= Building Node... on $PORT1"
run_sketch "Node" 4 $PORT1

echo "======= Building Gateway... on $PORT2"
run_sketch "Gateway" 3 $PORT2


screen -dmS sender /dev/$PORT1 57600
screen -dmS receiver /dev/$PORT2 57600
echo "reattach screens 'sender' or 'receiver'"
