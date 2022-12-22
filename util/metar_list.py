#!python3
import avwx

FILTER_COUNTRIES = ["CA"]
FILTER_STATES = ["ON"]

stations = avwx.station.station_list(reporting=True)
for s in stations:
    m = avwx.Metar(s)
    if m.station.country in FILTER_COUNTRIES and \
            m.station.state in FILTER_STATES:
        if m.update():
            print(f'"{m.station.icao}",')
