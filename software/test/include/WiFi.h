#pragma once
#include "Arduino.h"

class MockWiFi {
public:
    String macAddress() {
        return "AA:BB:CC:DD:EE:FF";
    }
};

extern MockWiFi WiFi;
#if !defined(WIFI_MOCK_IMPL)
MockWiFi WiFi;
#endif
