#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <map>
#include "credentials.h"
#include "airports.h"

#ifndef __CREDENTIALS_H__
// Define these in include/credentials.h, so they don't get uploaded to github for the world to see.
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_AVWX_TOKEN "YOUR_AVWX_TOKEN"
#endif

#ifndef __AIRPORTS_H__
// Define these in include/airports.h, so they're specific to your setup.
char airportMapping[][5] = {
    "KJFK"};
#endif

#define BAUD_RATE 115200
#define INTERVAL_MILLIS 300000 // every five minutes

// JSON Filter doc.
StaticJsonDocument<200> filter;

#define PIN D2
#define NUM_AIRPORTS (sizeof airportMapping / sizeof airportMapping[0])

Adafruit_NeoPixel pixels(NUM_AIRPORTS, PIN, NEO_GRB + NEO_KHZ800);

int lastUpdate = 0;

int led_brightness = 50;

std::map<std::string, uint32_t> rulemap = {
    {"VFR", pixels.Color(0, led_brightness, 0)},               // green
    {"IFR", pixels.Color(led_brightness, 0, 0)},               // red
    {"MVFR", pixels.Color(0, 0, led_brightness)},              // blue
    {"LIFR", pixels.Color(led_brightness, 0, led_brightness)}, // magenta
    {"NONE", 0}};                                              // off

/**
 * A lightweight wrapper around the rules -> color map, that will return the "NONE" value for an unsupported flight rule.
 * @param flight_rule - A string representing an FAA flight rule. One of: "VFR", "IFR", "MVFR", "LIFR", or the special value, "NONE".
 * @return a NeoPixel color (as uint32_t)
 */
uint32_t get_color(const char *flight_rule)
{
    uint32_t retval = rulemap["NONE"];
    if (rulemap.find(flight_rule) != rulemap.end())
    {
        retval = rulemap[flight_rule];
    }
    return retval;
}
/**
 * Fetch the weather for a given ICAO airport code from AVWX, and set the relevant NeoPixel index to the correct color.
 */
void fetchWeather(char *code, int pixel)
{
    // Example URL (for CYYZ): http://avwx.rest/api/metar/CYYZ?format=json&onfail=cache&token=MY_TOKEN
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        String icao = code;
        String url = "http://avwx.rest/api/metar/" + icao + "?format=json&onfail=cache&token=" + SECRET_AVWX_TOKEN;

        // fetch the API Response
        http.begin(client, url);
        int httpResponseCode = http.GET();
        uint32_t rgb = 0; // RGB color for NeoPixel

        if (httpResponseCode > 0)
        {
            String json = http.getString();
            // Serial.println(json);

            // parse the JSON
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
            }
            else
            {
                // Print the result
                // serializeJsonPretty(doc, Serial);

                const char *flight_rules = doc["flight_rules"];
                Serial.printf("%s:%s\n", code, flight_rules);
                // Serial.println();

                // Get appropriate color for the flight rule.
                rgb = get_color(flight_rules);
            }
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();

        // Set the pixel to the color
        pixels.setPixelColor(pixel, rgb);

        pixels.show(); // Send the updated pixel colors to the hardware.
    }
}

void setup()
{
    // initialize JSON filter

    filter["flight_rules"] = true;

    // Initialize Serial.
    Serial.begin(BAUD_RATE);

    // Initialize Wifi
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    Serial.println();
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("success!");
    Serial.print("IP Address is: ");
    Serial.println(WiFi.localIP());

    // Initialize Indicator LED
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    unsigned long t = millis();
    if (t - lastUpdate > INTERVAL_MILLIS || lastUpdate == 0)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            // update the LED
            for (uint16_t i = 0; i < NUM_AIRPORTS; i++)
            {
                // digitalWrite(LED_BUILTIN, HIGH);
                fetchWeather(airportMapping[i], i);
                digitalWrite(LED_BUILTIN, LOW);
                delay(10);
            }
            Serial.println("=========");
            lastUpdate = millis();
        }
        else
        {
            Serial.println("WiFi disconnected.");
            lastUpdate = millis();
        }
    }
}