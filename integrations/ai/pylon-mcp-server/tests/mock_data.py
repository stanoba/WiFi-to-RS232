"""
Mock response payloads for testing pylon-mcp-server without physical hardware.
"""

MOCK_STATUS_ACTIVE = {
    "is_polling": False,
    "is_paused": False
}

MOCK_DATA_US3000C = {
    "stack": {
        "model": "US3000C",
        "modules_detected": 2,
        "is_polling": False,
        "is_paused": False,
        "voltage": 49.77,
        "current": -2.30,
        "power": -114.47,
        "soc": 86,
        "soh": 99,
        "scrape_success": True,
        "scrape_duration_ms": 940,
        "last_scrape": 1790538000
    },
    "modules": [
        {
            "id": 1,
            "device": "US3000C",
            "voltage": 49.77,
            "current": -1.15,
            "soc": 86,
            "soh": 99,
            "soh_times": 0,
            "volt_spread_mv": 3,
            "temp_c": 24.5,
            "mos_temp_c": None,
            "base_state": "Dischg",
            "barcode": "HPTCR03261B00001",
            "cells": [
                {"cell": i, "voltage_mv": 3318 + (i % 3), "balance": False}
                for i in range(15)
            ]
        },
        {
            "id": 2,
            "device": "US3000C",
            "voltage": 49.77,
            "current": -1.15,
            "soc": 86,
            "soh": 99,
            "soh_times": 0,
            "volt_spread_mv": 4,
            "temp_c": 24.2,
            "mos_temp_c": None,
            "base_state": "Dischg",
            "barcode": "HPTCR03261B00002",
            "cells": [
                {"cell": i, "voltage_mv": 3317 + (i % 4), "balance": False}
                for i in range(15)
            ]
        }
    ],
    "system": {
        "heap_free": 182400,
        "cpu_usage": 12,
        "wifi_rssi": -58,
        "uptime": 86400
    }
}

MOCK_DATA_US3000D = {
    "stack": {
        "model": "US3000D",
        "modules_detected": 1,
        "is_polling": False,
        "is_paused": False,
        "voltage": 50.12,
        "current": 5.40,
        "power": 270.65,
        "soc": 92,
        "soh": 100,
        "scrape_success": True,
        "scrape_duration_ms": 820,
        "last_scrape": 1790538050
    },
    "modules": [
        {
            "id": 1,
            "device": "US3000D",
            "voltage": 50.12,
            "current": 5.40,
            "soc": 92,
            "soh": 100,
            "soh_times": 0,
            "volt_spread_mv": 2,
            "temp_c": 25.1,
            "mos_temp_c": 26.8,
            "base_state": "Charge",
            "barcode": "HPTDR03261B00001",
            "cells": [
                {"cell": i, "voltage_mv": 3341 + (i % 2), "balance": (i == 4)}
                for i in range(15)
            ]
        }
    ],
    "system": {
        "heap_free": 179200,
        "cpu_usage": 15,
        "wifi_rssi": -62,
        "uptime": 12000
    }
}
