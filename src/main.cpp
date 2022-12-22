#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#ifndef __CREDENTIALS_H__
// Define these in include/credentials.h, so they don't get uploaded to github for the world to see.
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_AVWX_TOKEN "YOUR_AVWX_TOKEN"
#endif

#define BAUD_RATE 115200
#define INTERVAL_MILLIS 60000

// LED index to ICAO code mapping.
const char *airportMapping[] = {
    "CTCK",
    "CTNK",
    "CTRA",
    "CWAJ",
    "CWCI",
    "CWCJ",
    "CWCO",
    "CWDV",
    "CWGD",
    "CWGH",
    "CWGL",
    "CWKK",
    "CWLS",
    "CWMZ",
    "CWNC",
    "CWNZ",
    "CWPC",
    "CWQP",
    "CWRK",
    "CWWB",
    "CWWZ",
    "CXBI",
    "CXCA",
    "CXDI",
    "CXEA",
    "CXET",
    "CXHA",
    "CXHM",
    "CXKA",
    "CXKE",
    "CXKI",
    "CXPC",
    "CXTO",
    "CXVN",
    "CXZC",
    "CYAM",
    "CYAT",
    "CYCK",
    "CYER",
    "CYGK",
    "CYHD",
    "CYHM",
    "CYKF",
    "CYKP",
    "CYLD",
    "CYLH",
    "CYMO",
    "CYOO",
    "CYOW",
    "CYPO",
    "CYPQ",
    "CYQA",
    "CYQG",
    "CYQK",
    "CYQT",
    "CYRL",
    "CYSB",
    "CYSN",
    "CYSP",
    "CYTL",
    "CYTR",
    "CYTS",
    "CYTZ",
    "CYVV",
    "CYXL",
    "CYXR",
    "CYXU",
    "CYXZ",
    "CYYB",
    "CYYU",
    "CYYW",
    "CYYZ",
    "CYZE",
    "CYZR",
    "CZEL",
    "CZMD",
    "CZSJ",
    "CZTB"};

#define PIN D2
#define BRIGHTNESS 50
#define NUM_AIRPORTS (sizeof airportMapping / sizeof airportMapping[0])

Adafruit_NeoPixel pixels(NUM_AIRPORTS, PIN, NEO_GRB + NEO_KHZ800);

int lastUpdate = 0;

enum e_Rule
{
    VFR = 0,
    IFR = 1,
    MVFR = 2,
    LIFR = 3,
    NONE = 4
};

e_Rule resolveRule(char *input)
{
    if (strcmp(input, "VFR"))
    {
        return VFR;
    }
    else if (strcmp(input, "IFR"))
    {
        return IFR;
    }
    else if (strcmp(input, "MVFR"))
    {
        return MVFR;
    }
    else if (strcmp(input, "LIFR"))
    {
        return LIFR;
    }
    else
    {
        return NONE;
    }
}

void setup()
{
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

void fetchWeather(const char *code, int pixel)
{
    // Example URL (for CYYZ): http://avwx.rest/api/metar/CYYZ?format=json&onfail=cache&token=MY_TOKEN

    HTTPClient http;
    String icao = code;
    String url = "http://avwx.rest/api/metar/" + icao + "?format=json&onfail=cache&token=" + SECRET_AVWX_TOKEN;

    http.begin(url);
    int httpCode = http.GET();
    String json = http.getString();
    Serial.println(json);
    http.end();

    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());

        // Set the pixel to OFF.

        return;
    }

    char *flight_rules = doc["flight_rules"];

    Serial.printf("%s:%s", code, flight_rules);

    // Convert flight rule string to enum
    e_Rule rule = resolveRule(flight_rules);

    // Get appropriate color for the flight rule.

    // Set the pixel to the color
}

void loop()
{
    unsigned long t = millis();
    if (t - lastUpdate > INTERVAL_MILLIS)
    {
        // update the LED
        for (uint16_t i = 0; i < NUM_AIRPORTS; i++)
        {
            fetchWeather(airportMapping[i], i);
            delay(1000);
        }
        lastUpdate = millis();
    }
}