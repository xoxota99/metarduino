# metarduino
ESP8266/Arduino project, similar to METARMaps (https://metarmaps.com/)

This implementation uses the Wemos D1 Mini Pro (16MB), and NeoPixels. Each LED is mapped to a specific ICAO airport code. Once per minute, we make a call to an AVWX endpoint, download relevant METAR data, and update the LEDs. simple, really. Don't know what all the fuss is about.

You can also do this on Raspberry Pi, with a ten-line python script, but since I would have to sell a kidney to buy an RPi right now, I figured I'd do this instead.

