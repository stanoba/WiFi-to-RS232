# REST API Reference & Integration Guide

The **WiFi-to-RS232 Pylon Smart Monitor** provides a lightweight, high-performance HTTP REST API designed for integration with custom home automation systems, Node-RED, Python scripts, Home Assistant, and external dashboards.

---

## 1. Authentication & Security

Authentication is configurable via the Web UI (`/settings`):

* **Disabled (Default):** API endpoints are publicly accessible on your local network.
* **Enabled (Bearer Token):** External clients must supply the pre-shared API Bearer Token using either:
  1. **HTTP Header (Recommended):**
     ```http
     Authorization: Bearer <your_api_token>
     ```
  2. **Query Parameter:**
     ```http
     GET /api/data?token=<your_api_token>
     ```

If authentication fails, the server responds with:
```json
{
  "error": "Unauthorized",
  "message": "Invalid or missing Bearer token"
}
```
HTTP Status: `401 Unauthorized`

---

## 2. API Endpoints Overview

| Endpoint | Method | Auth | Description |
|:---|:---:|:---:|:---|
| [`/api/data`](#get-apidata) | `GET` | Bearer | Comprehensive live telemetry (stack totals, individual modules, cell voltages, SOC, SOH, alarms) **and ESP32 system diagnostics** (`system` block; streamed in HTTP chunks) |
| [`/api/module`](#get-apimodule) | `GET` | Bearer | Single-module detail (cells, voltages, temperatures; streamed in HTTP chunks) |
| [`/api/history`](#get-apihistory) | `GET` | Bearer | Historical rolling 24-hour telemetry samples (current, SOC) for charting and time-series analysis (streamed in HTTP chunks) |
| [`/api/peers`](#get-apipeers) | `GET` | Bearer | Discovered peer Pylon Smart Monitors on the local network via mDNS |
| [`/sync_ntp`](#get-sync_ntp) | `GET` | Web/Token | Immediately triggers NTP time synchronization across redundant servers |
| [`/poll_now`](#get-poll_now) | `GET` | Web/Token | Forces an immediate serial scrape of all battery telemetry |
| [`/toggle_pause`](#get-toggle_pause) | `GET` | Web/Token | Toggles scheduled BMS polling between active and paused states |
| [`/cmd`](#get-cmd) | `GET` / `POST` | Web/Token | Enqueues a custom Pylontech console command into the asynchronous FIFO queue |
| [`/log/raw`](#get-lograw) | `GET` | Web/Token | Exports raw plain-text console log buffer |

---

## 3. Detailed Endpoint Reference

### `GET /api/data`

Returns complete real-time battery stack telemetry, module metadata, individual cell voltages, and thermal states.

#### Query Parameters:
* `token` *(optional)*: Bearer token (if not provided via `Authorization` header).

#### Example Request:
```bash
curl -s http://192.168.1.150/api/data \
  -H "Authorization: Bearer psm_9e29e00000000"
```

#### Example Response:
```json
{
  "stack": {
    "model": "US3000C",
    "modules_detected": 1,
    "voltage": 49.77,
    "current": -2.30,
    "power": -114.3,
    "soc": 86,
    "soh": 99,
    "scrape_success": true,
    "scrape_duration_ms": 1160,
    "last_scrape": 1790538015
  },
  "modules": [
    {
      "id": 6,
      "device": "US3000C",
      "voltage": 49.772,
      "current": -2.30,
      "soc": 86,
      "soh": 99,
      "soh_times": 0,
      "volt_spread_mv": 1,
      "temp_c": 25.9,
      "mos_temp_c": null,
      "base_state": "Dischg",
      "barcode": "HPTCR03261B00000",
      "release_date": "21-09-26",
      "cells": [
        {
          "cell": 0,
          "voltage_mv": 3318,
          "soc": 86,
          "temp_c": 23.6,
          "balance": false,
          "soh_count": 0,
          "soh_status": "Normal"
        },
        {
          "cell": 1,
          "voltage_mv": 3318,
          "soc": 86,
          "temp_c": 23.6,
          "balance": false,
          "soh_count": 0,
          "soh_status": "Normal"
        }
      ]
    }
  ],
  "system": {
    "uptime_s": 3842,
    "cpu_load_pct": 4,
    "cpu_temp_c": 47.2,
    "cpu_freq_mhz": 240,
    "chip_model": "ESP32",
    "chip_revision": 1,
    "heap_free": 198432,
    "heap_total": 327680,
    "heap_min_free": 172344,
    "heap_max_alloc": 131056,
    "heap_frag_pct": 8,
    "psram_free": 0,
    "psram_total": 0,
    "flash_size": 4194304,
    "sketch_size": 887984,
    "wifi_rssi": -62,
    "wifi_signal_pct": 76,
    "wifi_ssid": "HomeNet",
    "wifi_bssid": "AA:BB:CC:DD:EE:FF",
    "wifi_channel": 6,
    "ip": "192.168.1.150",
    "mac": "24:6F:28:AB:CD:EF",
    "reset_reason": "Power-on"
  }
}
```

#### Schema Fields:

##### `stack` Object

* `model` *(string)*: Verified battery model family (e.g. `"US3000C"`, `"US3000D"`).
* `modules_detected` *(integer)*: Total count of active modules detected in stack (1 to 16).
* `voltage` *(float)*: Total stack terminal voltage in Volts (V).
* `current` *(float)*: Total stack current in Amperes (A; positive = charging, negative = discharging).
* `power` *(float)*: Calculated instantaneous stack power in Watts (W = V × A).
* `soc` *(integer)*: Average stack State of Charge percentage (0 to 100%).
* `soh` *(integer|null)*: Stack State of Health percentage (0 to 100%, or `null` if not yet parsed).
* `scrape_success` *(boolean)*: `true` if the last serial transaction completed successfully.
* `scrape_duration_ms` *(integer)*: Duration of last scrape cycle in milliseconds.
* `last_scrape` *(integer)*: Unix epoch timestamp of the last scrape cycle.

##### `modules` Array
* `id` *(integer)*: Physical DIP switch / module index address (1 to 16).
* `device` *(string)*: Product device model name.
* `voltage` *(float)*: Module terminal voltage in Volts (V).
* `current` *(float)*: Module current in Amperes (A).
* `soc` *(integer)*: Module State of Charge percentage (%).
* `soh` *(integer|null)*: Module State of Health percentage (%).
* `soh_times` *(integer)*: Lifetime SOH degradation event count from `stat` command.
* `volt_spread_mv` *(integer)*: Worst-case cell voltage spread across this module ($V_{max} - V_{min}$) in millivolts.
* `temp_c` *(float)*: Internal cell pack temperature in degrees Celsius (°C).
* `mos_temp_c` *(float|null)*: BMS power MOSFET temperature in °C (`null` if unsupported by hardware).
* `base_state` *(string)*: Operating power state: `"Charge"`, `"Dischg"`, `"Idle"`, or `"Balance"`.
* `barcode` *(string)*: Factory serial barcode string.
* `release_date` *(string)*: Manufacturer board / firmware release date from `info` (e.g. `"21-09-26"`).
* `cells` *(array)*: Array of individual cell telemetry objects.

##### `cells` Object
* `cell` *(integer)*: 0-indexed cell position (0 to 14).
* `voltage_mv` *(integer)*: Cell voltage in millivolts (mV).
* `soc` *(integer)*: Individual cell State of Charge percentage (%).
* `temp_c` *(float)*: Cell temperature in degrees Celsius (°C).
* `balance` *(boolean)*: `true` if passive cell balancing resistor is actively discharging this cell.
* `soh_count` *(integer, optional)*: Individual cell degradation cycle count (US3000C).
* `soh_status` *(string, optional)*: Individual cell health status (e.g. `"Normal"`).

##### `system` Object

ESP32 controller diagnostics — always present in the response.

| Field | Type | Description |
|:---|:---:|:---|
| `uptime_s` | integer | Seconds elapsed since last reboot |
| `cpu_load_pct` | integer | CPU utilization % (averaged over 500 ms window) |
| `cpu_temp_c` | float | Internal chip temperature in °C |
| `cpu_freq_mhz` | integer | CPU clock frequency in MHz |
| `chip_model` | string | Chip family name (e.g. `"ESP32"`, `"ESP32-S2"`) |
| `chip_revision` | integer | Silicon revision number |
| `heap_free` | integer | Current free heap bytes |
| `heap_total` | integer | Total heap size bytes |
| `heap_min_free` | integer | Historical minimum free heap (watermark) |
| `heap_max_alloc` | integer | Largest contiguous free block available |
| `heap_frag_pct` | integer | Heap fragmentation percentage |
| `psram_free` | integer | Free PSRAM bytes (0 if not present) |
| `psram_total` | integer | Total PSRAM bytes (0 if not present) |
| `flash_size` | integer | Total flash chip size in bytes |
| `sketch_size` | integer | Compiled firmware size in bytes |
| `wifi_rssi` | integer | WiFi signal strength in dBm |
| `wifi_signal_pct` | integer | Signal quality percentage (0–100%) |
| `wifi_ssid` | string | Connected access point SSID |
| `wifi_bssid` | string | Access point MAC address |
| `wifi_channel` | integer | WiFi channel number |
| `ip` | string | Device IPv4 address |
| `mac` | string | Device WiFi MAC address |
| `reset_reason` | string | Human-readable last reset cause (e.g. `"Power-on"`, `"Watchdog"`) |
| `mqtt_enabled` | boolean | Whether MQTT client is enabled in settings |
| `mqtt_connected` | boolean | Current MQTT broker connection state |

---

### `GET /api/module`

Returns detailed telemetry for a single battery module including all cell-level data.

#### Query Parameters:
* `m` *(integer, required)*: Module index (1–16).
* `token` *(optional)*: Bearer token (if required).

#### Example Request:
```bash
curl -s "http://192.168.1.150/api/module?m=1"
```

---

### `GET /api/history`

Returns rolling historical telemetry samples stored in the ESP32 internal circular RAM ringbuffer. Samples are recorded automatically every 60 seconds.

#### Query Parameters:
* `range` *(integer, optional)*: Time window in seconds:
  * `3600` = Last 1 hour (up to 60 samples)
  * `21600` = Last 6 hours (up to 360 samples)
  * `43200` = Last 12 hours (up to 720 samples)
  * `86400` = Last 24 hours (default, up to 1440 samples)
* `token` *(optional)*: Bearer token (if required).

#### Example Request:
```bash
curl -s "http://192.168.1.150/api/history?range=3600"
```

#### Example Response:
```json
{
  "range": 3600,
  "count": 5,
  "samples": [
    {
      "t": 1790537772,
      "c": -2.18,
      "s": 87
    },
    {
      "t": 1790537832,
      "c": -2.11,
      "s": 87
    }
  ]
}
```

#### Schema Fields:
* `range` *(integer)*: Effective time range queried in seconds.
* `count` *(integer)*: Number of samples returned.
* `samples` *(array)*:
  * `t` *(integer)*: Unix epoch timestamp in seconds.
  * `c` *(float)*: Stack current in Amperes (A; positive = charging, negative = discharging).
  * `s` *(integer)*: Stack average SOC percentage (0 to 100%).

---

### `GET /api/peers`

Returns a list of other Pylon Smart Monitor devices discovered on the local area network using mDNS background discovery (`_pylon-smart._tcp`).

#### Example Request:
```bash
curl -s http://192.168.1.150/api/peers
```

#### Example Response:
```json
[
  {
    "name": "pylon-smart-0a12",
    "ip": "192.168.5.134",
    "port": 80,
    "model": "US3000C",
    "modules": 6,
    "ver": "1.2.0"
  },
  {
    "name": "pylon-smart-6d0a",
    "ip": "192.168.5.136",
    "port": 80,
    "model": "US3000C",
    "modules": 5,
    "ver": "1.2.0"
  }
]
```

#### Schema Fields:
* `name` *(string)*: Device mDNS hostname (without `.local`).
* `ip` *(string)*: IPv4 address of the peer device.
* `port` *(integer)*: HTTP Web Portal port (default `80`).
* `model` *(string)*: Pylontech battery model reported by the peer (e.g. `"US3000C"`).
* `modules` *(integer)*: Number of connected battery modules reported by the peer (e.g. `6`).
* `ver` *(string)*: Firmware version running on the peer device (e.g. `"1.2.0"`).

---

### `GET /sync_ntp`

Triggers an immediate re-synchronization with configured NTP servers (`pool.ntp.org`, `time.google.com`, `time.cloudflare.com`) to correct hardware RTC drift without rebooting.

#### Query Parameters:
* `redirect` *(string, optional)*: URL to redirect browser to after triggering sync (default: `/settings`).

#### Example Request:
```bash
curl -s -i "http://192.168.1.150/sync_ntp"
```

---

### `GET /poll_now`

Triggers an immediate fast and slow serial scrape of the connected Pylontech battery stack outside the scheduled polling cycle.

#### Query Parameters:
* `redirect` *(string, optional)*: URL to redirect after polling (default: `/`).

#### Example Request:
```bash
curl -s -i "http://192.168.1.150/poll_now"
```

---

### `GET /toggle_pause`

Toggles scheduled automatic BMS serial polling between active and paused states. Useful when performing manual console diagnostics or firmware maintenance.

#### Example Request:
```bash
curl -s -i "http://192.168.1.150/toggle_pause"
```

---

### `GET /cmd` / `POST /cmd`

Enqueues an arbitrary Pylontech console command into the asynchronous non-blocking FIFO queue. Commands are dispatched between scheduled poll cycles to prevent UART frame collisions.

> [!NOTE]
> **Safety Guardrail:** Destructive/dangerous commands (`shut`, `trst`, `rst`, `reset`, `update`, `boot`, `cali`, `calib`, `sleep`, `poweroff`, `format`, `erase`, `chg`, `dsg`) are blocked by default to prevent accidental shutdown or ADC corruption. Technicians can unlock full access by issuing `login debug` (and relock via `logout` or `exit`).

#### Query / Form Parameters:
* `c` *(string, required)*: Command string (e.g. `stat`, `info`, `bat`, `pwr`, `soh`, `euro`, `bat 1`).
* `ajax` *(string, optional)*: If set to `1`, responds with plain-text `"QUEUED"` instead of a 303 HTTP redirect.

#### Example Request:
```bash
curl -s "http://192.168.1.150/cmd?c=stat&ajax=1"
```

---

### `GET /log/raw`

Exports the current in-memory circular text buffer containing recent RS232 console communication (`TX >>` and `RX <<`).

#### Example Request:
```bash
curl -s "http://192.168.1.150/log/raw"
```

---

## 4. Integration Examples

### Python Telemetry Poller
```python
import requests

MONITOR_IP = "192.168.1.150"
API_TOKEN = "psm_9e29e00000000"

headers = {"Authorization": f"Bearer {API_TOKEN}"}
resp = requests.get(f"http://{MONITOR_IP}/api/data", headers=headers, timeout=5)

if resp.status_code == 200:
    data = resp.json()
    stack = data["stack"]
    print(f"Stack Voltage: {stack['voltage']} V | Current: {stack['current']} A | SOC: {stack['soc']}%")
    for mod in data["modules"]:
        print(f" Module {mod['id']} ({mod['device']}): {mod['voltage']} V, DeltaV: {mod['volt_spread_mv']} mV")
```

### Home Assistant REST Sensor (`configuration.yaml`)
```yaml
sensor:
  - platform: rest
    name: "Pylontech Battery Stack"
    resource: "http://192.168.1.150/api/data"
    headers:
      Authorization: "Bearer psm_9e29e00000000"
    scan_interval: 60
    json_attributes_path: "$.stack"
    json_attributes:
      - voltage
      - current
      - power
      - soc
      - soh
      - modules_detected
    value_template: "{{ value_json.stack.soc }}"
    unit_of_measurement: "%"
```

---

## 5. MQTT / Home Assistant Auto-Discovery

When MQTT is enabled in **Settings**, the monitor publishes Home Assistant
[MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
messages automatically. No manual `configuration.yaml` entries are required.

### Topic structure

| Topic | Payload | Retained | Description |
|:---|:---:|:---:|:---|
| `homeassistant/sensor/pylontech_<suffix>_<sensor>/config` | JSON | ✅ | HA discovery config (one per sensor, unique per device) |
| `<prefix>_<suffix>/state` | JSON | ❌ | Stack-level live state |
| `<prefix>_<suffix>/mod<N>/state` | JSON | ❌ | Per-module live state (one topic per module) |

> **Multi-Device Support:** `<suffix>` is the last 4 characters of the device WiFi MAC address (e.g. `b016`).
> Default prefix is `homeassistant/sensor/pylontech`. If two or more Pylon Smart Monitors connect to the same MQTT broker, each is registered as its own distinct device in Home Assistant with zero topic collision. Custom prefix can be set in Settings.

---

### Stack-level sensors (`<prefix>/state`)

| HA Entity | JSON key | Unit | Device Class |
|:---|:---|:---:|:---|
| Stack Voltage | `voltage` | V | `voltage` |
| Stack Current | `current` | A | `current` |
| Stack Power | `power` | W | `power` |
| Stack SOC | `soc` | % | `battery` |
| Stack SOH | `soh` | % | — |
| Active Modules | `modules` | — | — |

---

### Per-module sensors (`<prefix>/mod<N>/state`)

Each present module publishes its own JSON state message. The following sensors
are created per module:

#### Measurement sensors

| HA Entity | JSON key | Unit | Device Class |
|:---|:---|:---:|:---|
| Module N Voltage | `voltage` | V | `voltage` |
| Module N Current | `current` | A | `current` |
| Module N State of Charge | `soc` | % | `battery` |
| Module N Temperature | `temp` | °C | `temperature` |
| Module N MOSFET Temperature | `mos_temp` | °C | `temperature` |
| Module N Cell High Voltage | `cell_high_v` | V | `voltage` |
| Module N Cell Low Voltage | `cell_low_v` | V | `voltage` |
| Module N Cell High Temperature | `cell_high_t` | °C | `temperature` |
| Module N Cell Low Temperature | `cell_low_t` | °C | `temperature` |
| Module N Volt Spread | `vspread` | mV | — |
| Module N State of Health | `soh` | % | — |

#### Status / enum sensors

| HA Entity | JSON key | Example values |
|:---|:---|:---|
| Module N Status | `status` | `Charge`, `Dischg`, `Idle`, `Balance` |
| Module N Battery Voltage Status | `volt_status` | `Normal`, `High`, `Low` |
| Module N Current Status | `curr_status` | `Normal`, `High` |
| Module N Temperature Status | `temp_status` | `Normal`, `High`, `Low` |
| Module N Battery Temperature Status | `bat_t_status` | `Normal`, `High`, `Low` |
| Module N MOSFET Temperature Status | `mos_status` | `Normal`, `High` |

#### Master-only sensors

The following sensors are published **only for the active (master) module**
and only when the `euro` statistics have been received from the BMS:

| HA Entity | JSON key | Unit | Device Class |
|:---|:---:|:---:|:---|
| Module N Capacity Throughput | `cap_ah` | Ah | `energy_storage` |
| Module N Energy Throughput | `energy_wh` | Wh | `energy` |

---

### Example per-module state payload

```json
{
  "voltage": 49.772,
  "current": -2.30,
  "soc": 86,
  "temp": 25.9,
  "mos_temp": 27.4,
  "cell_high_v": 3.319,
  "cell_low_v": 3.318,
  "cell_high_t": 26.1,
  "cell_low_t": 24.8,
  "vspread": 1,
  "status": "Dischg",
  "volt_status": "Normal",
  "curr_status": "Normal",
  "temp_status": "Normal",
  "bat_t_status": "Normal",
  "mos_status": "Normal",
  "soh": 99,
  "cap_ah": 27671,
  "energy_wh": 138355
}
```
