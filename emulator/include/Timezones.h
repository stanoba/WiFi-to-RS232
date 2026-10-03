#pragma once
#include <Arduino.h>

struct TimezoneEntry {
    const char *city;
    const char *posix;
};

static const TimezoneEntry TIMEZONE_LIST[] = {
    {"Europe/Bratislava (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vienna (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Budapest (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw (UTC+1, CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/London (UTC+0, BST)", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Dublin (UTC+0, IST)", "GMT0IST,M3.5.0/1,M10.5.0"},
    {"Europe/Kyiv (UTC+2, EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Bucharest (UTC+2, EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Athens (UTC+2, EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Helsinki (UTC+2, EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"UTC (Universal Coordinated Time)", "UTC0"},
    {"America/New_York (UTC-5, EDT)", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Chicago (UTC-6, CDT)", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver (UTC-7, MDT)", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Los_Angeles (UTC-8, PDT)", "PST8PDT,M3.2.0,M11.1.0"},
    {"Asia/Dubai (UTC+4)", "GST-4"},
    {"Asia/Bangkok (UTC+7)", "ICT-7"},
    {"Asia/Singapore (UTC+8)", "SGT-8"},
    {"Asia/Tokyo (UTC+9)", "JST-9"},
    {"Australia/Sydney (UTC+10, AEDT)", "AEST-10AEDT,M10.1.0,M4.1.0/3"}
};

static const size_t TIMEZONE_COUNT = sizeof(TIMEZONE_LIST) / sizeof(TIMEZONE_LIST[0]);
