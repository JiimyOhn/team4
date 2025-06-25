#pragma hdrstop
#include "SbsDecoder.h"
#include "TimeFunctions.h"
#include <System.SysUtils.hpp>
#include <string.h>

#define SBS_MESSAGE_TYPE      0
#define SBS_TRANSMISSION_TYPE 1
#define SBS_SESSION_ID        2
#define SBS_AIRCRAFT_ID       3
#define SBS_HEX_INDENT        4
#define SBS_FLIGHT_ID         5
#define SBS_DATE_GENERATED    6
#define SBS_TIME_GENERATED    7
#define SBS_DATE_LOGGED       8
#define SBS_TIME_LOGGED       9
#define SBS_CALLSIGN          10
#define SBS_ALTITUDE          11
#define SBS_GROUND_SPEED      12
#define SBS_TRACK_HEADING     13
#define SBS_LATITUDE          14
#define SBS_LONGITUDE         15
#define SBS_VERTICAL_RATE     16
#define SBS_SQUAWK            17
#define SBS_ALERT             18
#define SBS_EMERGENCY         19
#define SBS_SBI               20
#define SBS_IS_ON_GROUND      21

static int hexDigitVal(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char* strsep(char **stringp, const char *delim) {
    char *start = *stringp;
    if (start == NULL) return NULL;
    char *ptr = strpbrk(start, delim);
    if (ptr == NULL) {
        *stringp = NULL;
        return start;
    }
    *ptr = '\0';
    *stringp = ptr + 1;
    return start;
}

ParsedAircraftData DecodeSbsMessage(const char* msg) {
    ParsedAircraftData result;
    char* mutableMsg = new char[strlen(msg) + 1];
    strcpy(mutableMsg, msg);
    char* toFree = mutableMsg;

    char *SBS_Fields[22] = {0};
    for (int i = 0; i < 22; i++) {
        SBS_Fields[i] = strsep(&mutableMsg, ",");
        if (!SBS_Fields[i]) break;
    }

    if (!SBS_Fields[SBS_MESSAGE_TYPE] || stricmp(SBS_Fields[SBS_MESSAGE_TYPE], "MSG") != 0 || !SBS_Fields[SBS_HEX_INDENT]) {
        delete[] toFree;
        return result;
    }

    char *icaoStr = SBS_Fields[SBS_HEX_INDENT];
    if (strlen(icaoStr) < 6) {
        delete[] toFree;
        return result;
    }

    unsigned int addr = 0;
    unsigned char *chars = (unsigned char *)&addr;
    for (int j = 0; j < 6; j += 2) {
        int high = hexDigitVal(icaoStr[j]);
        int low = hexDigitVal(icaoStr[j + 1]);
        if (high == -1 || low == -1) {
             delete[] toFree;
             return result;
        }
        chars[2 - j / 2] = (high << 4) | low;
    }

    result.IsValid = true;
    result.ICAO = addr;
    result.Timestamp = GetCurrentTimeInMsec();

    if (SBS_Fields[SBS_CALLSIGN] && strlen(SBS_Fields[SBS_CALLSIGN]) > 0) {
        result.hasFlightNum = true;
        strncpy(result.FlightNum, SBS_Fields[SBS_CALLSIGN], 8);
        result.FlightNum[8] = '\0';
    }
    if (SBS_Fields[SBS_ALTITUDE] && strlen(SBS_Fields[SBS_ALTITUDE]) > 0) {
        result.hasAltitude = true;
        result.Altitude = StrToFloatDef(SBS_Fields[SBS_ALTITUDE], 0);
    }
    if (SBS_Fields[SBS_GROUND_SPEED] && strlen(SBS_Fields[SBS_GROUND_SPEED]) > 0) {
        result.hasSpeed = true;
        result.Speed = StrToFloatDef(SBS_Fields[SBS_GROUND_SPEED], 0);
    }
    if (SBS_Fields[SBS_TRACK_HEADING] && strlen(SBS_Fields[SBS_TRACK_HEADING]) > 0) {
        result.hasHeading = true;
        result.Heading = StrToFloatDef(SBS_Fields[SBS_TRACK_HEADING], 0);
    }
    if (SBS_Fields[SBS_LATITUDE] && strlen(SBS_Fields[SBS_LATITUDE]) > 0 &&
        SBS_Fields[SBS_LONGITUDE] && strlen(SBS_Fields[SBS_LONGITUDE]) > 0) {
        double lat = StrToFloatDef(SBS_Fields[SBS_LATITUDE], 999);
        double lon = StrToFloatDef(SBS_Fields[SBS_LONGITUDE], 999);
        if (lat <= 90.0 && lat >= -90.0 && lon <= 180.0 && lon >= -180.0) {
            result.hasPosition = true;
            result.Latitude = lat;
            result.Longitude = lon;
        }
    }
    if (SBS_Fields[SBS_VERTICAL_RATE] && strlen(SBS_Fields[SBS_VERTICAL_RATE]) > 0) {
        result.hasVerticalRate = true;
        result.VerticalRate = StrToFloatDef(SBS_Fields[SBS_VERTICAL_RATE], 0);
    }

    delete[] toFree;
    return result;
}