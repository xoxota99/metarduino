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
char airportMapping[][5] = {
    "KJFK"};
#endif

#define BAUD_RATE 115200
#define INTERVAL_MILLIS 300000 // every five minutes

// JSON Filter doc.
StaticJsonDocument<200> filter;

#define PIN D2
#define BRIGHTNESS 50
#define NUM_AIRPORTS (sizeof airportMapping / sizeof airportMapping[0])

Adafruit_NeoPixel pixels(NUM_AIRPORTS, PIN, NEO_GRB + NEO_KHZ800);

int lastUpdate = 0;

std::map<std::string, uint32_t> rulemap = {
    {"VFR", pixels.Color(0, BRIGHTNESS, 0)},           // green
    {"IFR", pixels.Color(BRIGHTNESS, 0, 0)},           // red
    {"MVFR", pixels.Color(0, 0, BRIGHTNESS)},          // blue
    {"LIFR", pixels.Color(BRIGHTNESS, 0, BRIGHTNESS)}, // magenta
    {"NONE", 0}};                                      // off

uint32_t get_color(const char *input)
{
    if (rulemap.find(input) != rulemap.end())
    {
        return rulemap[input];
    }
    return 0;
}

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
        uint32_t c = 0;

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
                c = get_color(flight_rules);
            }
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();

        // Set the pixel to the color
        pixels.setPixelColor(pixel, c);

        pixels.show(); // Send the updated pixel colors to the hardware.
    }
}

void setup()
{
    // initialize JSON filter

    filter["flight_rules"] = true;

    // run once.
    Serial.begin(BAUD_RATE);
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
            lastUpdate = millis();
        }
        else
        {
            Serial.println("WiFi disconnected.");
            lastUpdate = millis();
        }
    }
}