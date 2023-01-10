#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <map>
#include <string>
#include <strings.h>
#include <TinyXML.h>
#include "credentials.h"
#include "airportdata.h" //airport data.

#ifndef __CREDENTIALS_H__
// Define these in include/credentials.h, so they don't get uploaded to github for the world to see.
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_AVWX_TOKEN "YOUR_AVWX_TOKEN"
#endif

#ifndef __AIRPORTDATA_H__
#include "airportstruct.h" //airport struct
// Define these in include/airports.h, so they're specific to your setup.
std::map<std::string, airport_t> airportMapping = {
    {"KJFK", {"KJFK", 0, "NONE"}}}
#endif

#define READ_TIMEOUT 15 // Cancel query if no data received (seconds)
#define BAUD_RATE 115200
#define INTERVAL_MILLIS 300000 // every five minutes

#define PIN D2
#define NUM_AIRPORTS (sizeof airportMapping / sizeof airportMapping[0])

Adafruit_NeoPixel pixels(NUM_AIRPORTS, PIN, NEO_GRB + NEO_KHZ800);

int lastUpdate = 0;

int led_brightness = 50;

std::map<std::string, uint32_t>
    rulemap = {{"VFR", pixels.Color(0, led_brightness, 0)},               // green
               {"IFR", pixels.Color(led_brightness, 0, 0)},               // red
               {"MVFR", pixels.Color(0, 0, led_brightness)},              // blue
               {"LIFR", pixels.Color(led_brightness, 0, led_brightness)}, // magenta
               {"NONE", 0}};                                              // off

// During XML processing, the icao code of the station we are currently processing.
std::string processing_station_id = "";

TinyXML xml;
uint8_t buffer[150]; // For XML decoding

// LED index to ICAO code mapping. Essentially, the ordering of the array
// is the ordering of the NeoPixels strip representing the airports.
std::string airportParamString = ""; // = "CTCK,CTNK,CTRA,CWAJ,CWCI,CWCJ,CWCO,CWDV,CWGD,CWGH,CWGL,CWKK,CWLS,CWMZ,CWNC,CWNZ,CWPC,CWQP,CWRK,CWWB,CWWZ,CXBI,CXCA,CXDI,CXEA,CXET,CXHA,CXHM,CXKA,CXKE,CXKI,CXPC,CXTO,CXVN,CXZC,CYAM,CYAT,CYCK,CYER,CYGK,CYHD,CYHM,CYKF,CYKP,CYLD,CYLH,CYMO,CYOO,CYOW,CYPO,CYPQ,CYQA,CYQG,CYQK,CYQT,CYRL,CYSB,CYSN,CYSP,CYTL,CYTR,CYTS,CYTZ,CYVV,CYXL,CYXR,CYXU,CYXZ,CYYB,CYYU,CYYW,CYYZ,CYZE,CYZR,CZEL,CZMD,CZSJ,CZTB";

/**
 * Given a map of items, where the key is the ICAO code of the airport,
 * construct the airport parameter string to use int he url for fetching METAR data.
 */
std::string construct_airport_param_str(std::map<std::string, airport_t> airportMapping)
{
    std::string retval = "";
    for (std::map<std::string, airport_t>::iterator it = airportMapping.begin(); it != airportMapping.end(); it++)
    {
        retval += it->first + ",";
    }
    retval.pop_back(); // remove trailing comma.
    return retval;
}

/**
 * A lightweight wrapper around the rules -> color map, that will return the "NONE" value for an unsupported flight rule.
 * @param flight_rule - A string representing an FAA flight rule. One of: "VFR", "IFR", "MVFR", "LIFR", or the special value, "NONE".
 * @return a NeoPixel color (as uint32_t)
 */
uint32_t get_color(std::string flight_rule)
{
    uint32_t retval = rulemap["NONE"];
    if (rulemap.find(flight_rule) != rulemap.end())
    {
        retval = rulemap[flight_rule];
    }
    return retval;
}

void XML_callback(uint8_t statusflags, char *tagName,
                  uint16_t tagNameLen, char *data, uint16_t dataLen)
{
    if ((statusflags & STATUS_ATTR_TEXT) && !strcasecmp(tagName, "station_id"))
    {
        processing_station_id = data;
        Serial.print(data);
        airportMapping[data].flight_rules = "NONE";
    }

    if ((statusflags && STATUS_ATTR_TEXT) && !strcasecmp(tagName, "flight_category"))
    {
        airportMapping[processing_station_id].flight_rules = data;
        Serial.print("=");
        Serial.printf(data);
        Serial.println();
    }
}

/*******************/

void setup()
{
    // Initialize Serial.
    Serial.begin(BAUD_RATE);

    construct_airport_param_str(airportMapping);

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

    // Initialize XML parser
    xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
}

/**
 * Update all NeoPixels with the currently heald flight rules data.
 */
void refresh()
{
    for (std::map<std::string, airport_t>::iterator it = airportMapping.begin(); it != airportMapping.end(); it++)
    {
        uint32_t c = get_color(it->second.flight_rules);
        int ledidx = it->second.led_index;

        // Set the LED at idx to color c.
    }
}

void loop()
{
    unsigned long t = millis();
    if (t - lastUpdate > INTERVAL_MILLIS || lastUpdate == 0)
    {
        // initialize all flight rules to "NONE".
        for (std::map<std::string, airport_t>::iterator it = airportMapping.begin(); it != airportMapping.end(); it++)
        {
            it->second.flight_rules = "NONE";
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.print("WiFi connecting..");
            WiFi.begin(SECRET_SSID, SECRET_PASS);
            // Wait up to 1 minute for connection...
            int c = 0;

            for (c = 0; (c < 60) && (WiFi.status() != WL_CONNECTED); c++)
            {
                Serial.write('.');
                delay(1000);
            }

            if (WiFi.status() != WL_CONNECTED)
            { // If it didn't connect within 1 min
                Serial.println("Failed. Will retry...");
                return;
            }
            Serial.println("OK!");
            delay(1000); // Pause before hitting it with queries & stuff
        }
        else
        {
            // update the LEDs
            refresh(); // start by turning them all off. (flight_rules = "NONE")

            WiFiClient client;
            HTTPClient http;
            std::string url = "https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=5&mostRecentForEachStation=true&stationString=" + airportParamString;
            // fetch the API Response
            http.begin(client, url.c_str());
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0)
            {
                xml.reset();
                t = millis();  // Start time
                int c;         // single char read from response.
                uint8_t b = 0; // total bytes read

                while (client.connected())
                {
                    if (!(b++ & 0x40))
                        refresh(); // Every 64 bytes, handle displays
                    if ((c = client.read()) >= 0)
                    {
                        xml.processChar(c);
                        t = millis(); // Reset timeout clock
                    }
                    else if ((millis() - t) >= (READ_TIMEOUT * 1000))
                    {
                        Serial.println("---Timeout---");
                        break;
                    }
                }
            }
            else
            {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
            http.end();
        }

        digitalWrite(LED_BUILTIN, LOW);
        delay(10);

        Serial.println("=========");
        lastUpdate = millis();
    }
    else
    {
        Serial.println("WiFi disconnected.");
        lastUpdate = millis();
    }
}
