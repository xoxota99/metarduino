#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Adafruit_NeoPixel.h>
#include <TinyXML.h>
#include <map>
#include <string>
#include "credentials.h"
#include "airportdata.h" //airport data.

#ifndef __CREDENTIALS_H__
// Define these in include/credentials.h, so they don't get uploaded to github for the world to see.
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_AVWX_TOKEN "YOUR_AVWX_TOKEN"
#endif

#ifndef __AIRPORTDATA_H__
// ICAO airport codes, in order of their respecive NeoPixel in the strip.
// Define these in include/airports.h, so they're specific to your setup.
char *airport_codes[] = {
    "KJFK",
    "KLAX",
    "KDFW"};
#endif

#define READ_TIMEOUT 5 // Cancel query if no data received (seconds)
#define BAUD_RATE 115200
#define INTERVAL_MILLIS 300000 // every five minutes

#define PIN D2

Adafruit_NeoPixel *pixels;
const int AIRPORT_COUNT = sizeof(airport_codes) / sizeof(airport_codes[0]);
int pixel_state[AIRPORT_COUNT];

int lastUpdate = 0;
int led_brightness = 50;

std::map<std::string, uint32_t>
    rulemap = {{"VFR", pixels->Color(0, led_brightness, 0)},               // green
               {"IFR", pixels->Color(led_brightness, 0, 0)},               // red
               {"MVFR", pixels->Color(0, 0, led_brightness)},              // blue
               {"LIFR", pixels->Color(led_brightness, 0, led_brightness)}, // magenta
               {"NONE", 0}};                                               // off

// During XML processing, the icao code of the station we are currently processing.
char *processing_station_id = "";

TinyXML xml;
uint8_t buffer[150]; // For XML decoding

// LED index to ICAO code mapping. Essentially, the ordering of the array
// is the ordering of the NeoPixels strip representing the airports.
char airport_param_string[AIRPORT_COUNT * 5] = ""; // = "CTCK,CTNK,CTRA,CWAJ,CWCI,CWCJ,CWCO,CWDV,CWGD,CWGH,CWGL,CWKK,CWLS,CWMZ,CWNC,CWNZ,CWPC,CWQP,CWRK,CWWB,CWWZ,CXBI,CXCA,CXDI,CXEA,CXET,CXHA,CXHM,CXKA,CXKE,CXKI,CXPC,CXTO,CXVN,CXZC,CYAM,CYAT,CYCK,CYER,CYGK,CYHD,CYHM,CYKF,CYKP,CYLD,CYLH,CYMO,CYOO,CYOW,CYPO,CYPQ,CYQA,CYQG,CYQK,CYQT,CYRL,CYSB,CYSN,CYSP,CYTL,CYTR,CYTS,CYTZ,CYVV,CYXL,CYXR,CYXU,CYXZ,CYYB,CYYU,CYYW,CYYZ,CYZE,CYZR,CZEL,CZMD,CZSJ,CZTB";

/**
 * Given a map of items, where the key is the ICAO code of the airport,
 * construct the airport parameter string to use in the url for fetching METAR data.
 */
void construct_airport_param_str(char *codes[], char *output)
{
    Serial.println("construct_airport_param_str");

    for (int i = 0; i < AIRPORT_COUNT; i++)
    {
        strcat(output, codes[i]);
        if (i < AIRPORT_COUNT - 1)
            strcat(output, ",");
    }
    // Serial.printf("%s\n", output);
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

int icao_lookup(char *code)
{
    for (int i = 0; i < AIRPORT_COUNT; i++)
    {
        if (strcmp(code, airport_codes[i]))
        {
            return i;
        }
    }
    return -1;
}

void XML_callback(uint8_t statusflags, char *tagName,
                  uint16_t tagNameLen, char *data, uint16_t dataLen)
{
    if (dataLen > 0 && tagNameLen > 0 && (statusflags && STATUS_ATTR_TEXT))
    {
        if (!strcmp(tagName, "/response/data/METAR/station_id"))
        {
            processing_station_id = data;
            Serial.printf("\n%s", data);
            pixel_state[icao_lookup(data)] = get_color("NONE"); // for METARs that do not include Flight rules.
        }

        if (!strcmp(tagName, "/response/data/METAR/flight_category"))
        {
            pixel_state[icao_lookup(processing_station_id)] = get_color(data);
            Serial.printf("=%s", data);
        }
    }
}

/**
 * Update all NeoPixels with the currently heald flight rules data.
 */
void refresh()
{
    Serial.println("Refreshing pixels.");
    for (int i = 0; i < AIRPORT_COUNT; i++)
    {
        uint32_t rgb = pixel_state[i];

        // Set the pixel to the color
        pixels->setPixelColor(i, rgb);

        pixels->show(); // Send the updated pixel colors to the hardware.
    }
}

int setup_wifi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("WiFi connecting..");

        WiFi.begin(SECRET_SSID, SECRET_PASS);
        // Wait up to 1 minute for connection...
        int c = 0;

        for (c = 0; (c < 60) && (WiFi.status() != WL_CONNECTED); c++)
        {
            Serial.print('.');
            delay(500);
        }

        if (WiFi.status() != WL_CONNECTED)
        { // If it didn't connect within 1 min
            Serial.println("Wifi init failed.");
            return 0;
        }
        Serial.println("success!");
        Serial.print("IP Address is: ");
        Serial.println(WiFi.localIP());
        delay(1000); // Pause before hitting it with queries & stuff
    }
    return 1;
}

void setup_serial()
{
    // Initialize Serial.
    Serial.begin(BAUD_RATE);
    delay(1000);
}

void setup_status_led()
{
    // Initialize Indicator LED
    Serial.println("setup_status_led");
    pinMode(LED_BUILTIN, OUTPUT);
}

void setup_pixels()
{
    Serial.println("setup_pixels");
    pixels = new Adafruit_NeoPixel(AIRPORT_COUNT, PIN, NEO_GRB + NEO_KHZ800);
}

void setup_xml()
{
    Serial.println("setup_xml");
    // Initialize XML parser
    xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
}
/*******************/

void setup()
{
    setup_serial();

    setup_wifi();
    setup_status_led();
    setup_pixels();
    setup_xml();

    // Serial.println("Constructing Airport param string.");
    construct_airport_param_str(airport_codes, airport_param_string);

    // Serial.printf("airport_param_string = %s\n", airport_param_string);
}

void loop()
{
    unsigned long t = millis();
    if (t - lastUpdate > INTERVAL_MILLIS || lastUpdate == 0)
    {
        // initialize all flight rules to "NONE".
        Serial.println("Starting...");
        Serial.println("reset flight rules to NONE.");

        int rgb = get_color("NONE");

        for (int i = 0; i < AIRPORT_COUNT; i++)
        {
            pixel_state[i] = rgb;
        }

        // Serial.printf("Wifi Status = %d\n", (int)(WiFi.status()));
        if (WiFi.status() == WL_CONNECTED || setup_wifi())
        {
            // update the LEDs
            refresh(); // start by turning them all off. (flight_rules = "NONE")

            // setup SSL.
            std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
            client->setInsecure(); // Insecure SSL.

            // WiFiClient client;
            HTTPClient https;
            char url[1024] = "";
            strcat(url, "https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=5&mostRecentForEachStation=true&stationString=");
            strcat(url, airport_param_string);

            // fetch the API Response
            Serial.printf("Fetching: %s\n", url);
            https.begin(*client, url);
            int httpResponseCode = https.GET();

            if (httpResponseCode > 0)
            {
                xml.reset();
                t = millis(); // Start time
                int c;        // single char read from response.
                // const char *headers[1] = {"content-length"};
                // https.collectHeaders(headers, 1);
                // int content_length = atoi(https.header("content-length").c_str()); // length of response, from content-length HTTP header.
                int b = 0; // total bytes read

                // Serial.printf("Content Length = %d\n", content_length);
                while (client->connected())
                {
                    if (!(b++ & 0x40))
                    {
                        // refresh(); // Every 64 bytes, refresh pixels.
                    }

                    if (client->available())
                    {
                        c = client->read();
                        if (c >= 0)
                        {
                            // Serial.printf("%c", c);
                            xml.processChar(c);
                            t = millis(); // Reset timeout clock
                        }
                        else if ((millis() - t) >= (READ_TIMEOUT * 1000))
                        {
                            Serial.println("Timeout reading HTTP response, or response complete.");
                            break;
                        }
                    }
                }
                client->stop();
                refresh(); // refresh whatever data we have obtained.
            }
            else
            {
                Serial.print("HTTP Error code: ");
                Serial.println(httpResponseCode);
            } // if (httpResponseCode > 0)
            Serial.println("https.end");
            https.end();
        } // if (WiFi.status() == WL_CONNECTED || setup_wifi())

        Serial.println("digitalWrite(LED_BUILTIN, LOW);");
        digitalWrite(LED_BUILTIN, LOW);
        delay(10);

        Serial.println("=========");
        lastUpdate = millis();
    } // if (t - lastUpdate > INTERVAL_MILLIS || lastUpdate == 0)
} // void loop()
