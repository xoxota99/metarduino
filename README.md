# metarduino
ESP8266/Arduino project, similar to [METARMaps](https://metarmaps.com/), but GPLv3.

This implementation uses the [Wemos D1 Mini Pro](https://www.amazon.com/dp/B07G9HZ5LM) (16MB), and [Adafruit NeoPixels](https://www.adafruit.com/product/1938). Each LED is mapped to a specific [ICAO airport code](https://en.wikipedia.org/wiki/ICAO_airport_code). Once every five minutes, we make a call to an [AVWX endpoint](https://avwx.rest/), download the relevant METAR JSON document for each code, and update the LEDs.

Simple, really. Don't know what all the fuss is about.

This is a PlatformIO / VSCode project. If you use the Arduino IDE, you're on your own, but it should work with relatively little modification.

You can also do this on Raspberry Pi, with a ten-line python script, but since I would have to sell a kidney to buy an RPi right now, I figured I'd do this instead.

__Dependencies:__ 
* __Platform:__ "D1 Mini Pro"
* Adafruit NeoPixel 1.10.7 or greater
* bblanchon/ArduinoJson 6.19.4 or greater

__What I did:__ I took my local VNC (VFR Sectional in the US), poked a hole for each airport I care about that broadcasts a METAR, mount a NeoPixel in the hole, plugged them into the Wemos D1 (I used pin D2, but any digital IO pin should work), and drove that pin with the NeoPixel library.

__One Caveat:__ The Wemos is a 3V device, and NeoPixels need 5V, so you'll need a level shifter on the IO pin. You'll also want to provide the pixels with their own 5v supply, separate (or parallel) to the Wemos.

__Future Improvements:__ 
* It might be cool to add a dimmer knob, connected to the Analog input on the Wemos, to control the NeoPixel brightness.
* Parse weather information from other sources (such as [NemoWX](https://nemowx.com/)). AVWX has a [parser API](https://avwx.docs.apiary.io/#reference/0/parse-metar/parse-a-given-metar).
* Make a version that parses TAFs, with a dimmer knob that moves the display forward / backward in time?
* Open to other suggestions.