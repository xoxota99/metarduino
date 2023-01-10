#ifndef __AIRPORTSTRUCT_H__
#define __AIRPORTSTRUCT_H_

#include <string>
#include <strings.h>

struct airport_t
{
public:
    airport_t() : icao(""), led_index(0), flight_rules("") {}
    airport_t(std::string p_icao, int p_led_index, std::string p_flight_rules) : icao(p_icao), led_index(p_led_index), flight_rules(p_flight_rules) {}

    std::string icao;
    int led_index;
    std::string flight_rules;
};
#endif