# Prometheus Metrics & Grafana Monitoring

This document details the Prometheus exporter integration for the **WiFi-to-RS232 Pylontech Battery Monitor**, including scraper configuration, complete metric schemas, label descriptions, raw output examples, and PromQL query recipes for Grafana dashboards.

---

## 1. Scraper Configuration (`prometheus.yml`)

The ESP32 firmware serves metrics in standard Prometheus text exposition format (`version=0.0.4`) on HTTP port 80 at `/metrics`.

Add the following scrape configuration to your `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'pylontech'
    scrape_interval: 60s
    metrics_path: /metrics
    static_configs:
      - targets: ['<device-ip>:80']   # e.g. '192.168.1.150:80' or 'pylon-smart.local:80'
```

> [!TIP]
> A scrape interval of `60s` matches the firmware's fast polling cadence for battery voltage, current, and cell balance states.

---

## 2. Exported Metrics Reference

### Exporter & Scrape Telemetry
| Metric | Type | Description |
|:---|:---:|:---|
| `pylontech_scrape_success` | Gauge | `1` if the last serial BMS communication succeeded, `0` if timed out or failed |
| `pylontech_scrape_duration_seconds` | Gauge | Duration of the last serial query cycle in seconds (typically < 1.0s) |
| `pylontech_firmware_info` | Gauge | Firmware version metadata (`version`, `build_date`, `build_time`), always `1` |
| `pylontech_modules_detected` | Gauge | Total number of active battery modules detected in the stack |

### Module Hardware Metadata
| Metric | Type | Description & Labels |
|:---|:---:|:---|
| `pylontech_module_info` | Gauge | Battery module hardware info (`module`, `model`, `barcode`, `board`, `main_soft`, `soft`), value is always `1` |

### Battery Pack Power & Thermal Telemetry (Fast Polling - 1 min)
| Metric | Type | Unit | Description |
|:---|:---:|:---:|:---|
| `pylontech_voltage_millivolts` | Gauge | mV | Overall module terminal voltage |
| `pylontech_current_milliamps` | Gauge | mA | Overall module current (positive = charging, negative = discharging) |
| `pylontech_temperature_millicelsius` | Gauge | m°C | Battery cell pack internal temperature (divide by 1000 for °C) |
| `pylontech_mosfet_temperature_millicelsius` | Gauge | m°C | BMS power switching MOSFET temperature (divide by 1000 for °C) |
| `pylontech_soc_percent` | Gauge | % | Module State of Charge (0 – 100%) |

### Individual Cell Telemetry (Fast Polling - 1 min)
| Metric | Type | Labels | Description |
|:---|:---:|:---|:---|
| `pylontech_cell_voltage_millivolts` | Gauge | `module`, `cell` | Individual cell voltage in millivolts (15 cells per module) |
| `pylontech_cell_balance` | Gauge | `module`, `cell` | `1` if cell balancing is currently active, `0` otherwise |
| `pylontech_cell_soh_count` | Gauge | `module`, `cell` | Individual cell SOH degradation count (supported on US3000C via `soh` command) |

### Battery Health & Lifetime Statistics (Slow Polling - 5 min)
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `pylontech_soh_percent` | Gauge | % | Module State of Health (0 – 100%) |
| `pylontech_soh_times_total` | Counter | events | Cumulative SOH degradation event count from `stat` command |
| `pylontech_cycle_times_total` | Counter | cycles | Total cumulative charge/discharge cycles recorded by the BMS |
| `pylontech_discharged_capacity_mah_total` | Counter | mAh | Cumulative discharged capacity throughput in mAh |

### BMS Cumulative Alarm & Protection Counters (Slow Polling - 5 min)
| Metric | Type | Labels | Description |
|:---|:---:|:---|:---|
| `pylontech_alarm_events_total` | Counter | `module`, `event` | Cumulative count of protection triggers recorded in BMS memory |

**Alarm Event Types (`event` label):**
- `coc` – Charge Over-Current protection trigger
- `doc` – Discharge Over-Current protection trigger
- `sc` – Short Circuit protection trigger
- `bat_ov` – Battery Over-Voltage protection trigger
- `bat_lv` – Battery Low-Voltage warning trigger
- `bat_uv` – Battery Under-Voltage cut-off trigger
- `cot` – Charge Over-Temperature protection trigger
- `dot` – Discharge Over-Temperature protection trigger

### Extended Model D "Euro" Statistics (US3000D / when supported)
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `pylontech_euro_energy_throughput_watthours_total` | Counter | Wh | Cumulative energy throughput across battery lifetime |
| `pylontech_euro_capacity_throughput_amperehours_total` | Counter | Ah | Cumulative capacity throughput across battery lifetime |
| `pylontech_euro_round_trip_efficiency` | Gauge | ‱ | Round-trip efficiency (basis 10,000 = 100.00%, e.g. 9650 = 96.50%) |

### ESP32 Controller Metrics

System health metrics from the ESP32 microcontroller itself. All metrics are prefixed `esp32_`.

#### Identity & Info
| Metric | Type | Labels | Description |
|:---|:---:|:---|:---|
| `esp32_system_info` | Gauge | `chip`, `revision`, `mac`, `ip`, `ssid` | Controller identification (always `1`) |

#### Runtime & Performance
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `esp32_uptime_seconds` | Counter | s | Seconds since last reboot |
| `esp32_cpu_usage_percent` | Gauge | % | CPU utilization (averaged over 500 ms window) |
| `esp32_cpu_temperature_celsius` | Gauge | °C | Internal chip temperature from built-in sensor |
| `esp32_cpu_frequency_mhz` | Gauge | MHz | CPU clock frequency |

#### Heap & PSRAM Memory
| Metric | Type | Unit | Description |
|:---|:---:|:---:|:---|
| `esp32_heap_free_bytes` | Gauge | B | Current free heap |
| `esp32_heap_total_bytes` | Gauge | B | Total heap size |
| `esp32_heap_min_free_bytes` | Gauge | B | Historical minimum free heap (watermark since boot) |
| `esp32_heap_max_alloc_bytes` | Gauge | B | Largest single contiguous free block |
| `esp32_heap_fragmentation_percent` | Gauge | % | Heap fragmentation: `100 - (max_alloc / free * 100)` |
| `esp32_psram_total_bytes` | Gauge | B | Total external PSRAM size (when PSRAM is present) |
| `esp32_psram_free_bytes` | Gauge | B | Free external PSRAM available |

#### Network & WiFi Connectivity
| Metric | Type | Unit | Description |
|:---|:---:|:---:|:---|
| `esp32_wifi_connected` | Gauge | 0/1 | Station mode connection state (`1` = connected, `0` = disconnected) |
| `esp32_wifi_rssi_dbm` | Gauge | dBm | WiFi received signal strength indicator |
| `esp32_wifi_signal_percent` | Gauge | % | Mapped signal quality (0-100%) |
| `esp32_wifi_channel` | Gauge | - | WiFi channel number |
| `esp32_wifi_ap_active` | Gauge | 0/1 | SoftAP mode state (`1` = active, `0` = inactive) |
| `esp32_wifi_ap_clients` | Gauge | clients | Number of clients currently connected to SoftAP |

#### NTP Time Synchronization & System Reset
| Metric | Type | Unit | Description |
|:---|:---:|:---:|:---|
| `esp32_reset_reason` | Gauge | - | Last CPU reset reason with `code` and `reason` labels (value always `1`) |
| `esp32_ntp_synced` | Gauge | 0/1 | NTP clock synchronization state (`1` = synced, `0` = unsynced) |
| `esp32_ntp_last_sync_timestamp` | Gauge | s | Unix epoch timestamp of last successful SNTP sync |

#### Storage
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `esp32_flash_size_bytes` | Gauge | B | Total flash chip capacity |
| `esp32_sketch_size_bytes` | Gauge | B | Compiled firmware binary size |
| `esp32_sketch_free_bytes` | Gauge | B | Free flash space available for OTA updates |

---

## 3. Ready-to-Use Grafana Dashboard

A complete, production-grade Grafana dashboard is included with this repository:
📁 **[`integrations/grafana/pylontech-smart-monitor-dashboard.json`](../integrations/grafana/pylontech-smart-monitor-dashboard.json)**

### Dashboard Features:
1. **Dynamic Templating Variables:**
   - `$datasource` — Selects Prometheus or VictoriaMetrics data source.
   - `$instance` — Dynamically switches between multiple Pylon Smart Monitors on the network.
   - `$module` — Filters individual battery modules (1..16 or All).
2. **⚡ Battery Stack Key Performance Indicators:**
   - Stack Voltage, Total Current (colorized by charge/discharge), Total Power, Average SOC, Worst Cell Spread (ΔV in mV), Active Modules count, BMS Scrape Status, and ESP32 Free Heap.
3. **🔋 Battery Modules & Stack Telemetry:**
   - Stack Total Power & Current (dual Y-axis timeseries: W on left, A on right).
   - Module SOC (%) and Module Terminal Voltages (V).
   - Module & MOSFET Temperatures (°C with distinct dashed line styling for MOSFETs).
4. **🔬 Cell-Level Voltages, Balancing & Spread:**
   - Individual Cell Voltage Bar Gauge (15 cells per module with LiFePO4 safety thresholds).
   - Per-Module Cell Spread (ΔV in mV) timeseries for early detection of cell imbalance.
5. **📈 Battery Lifetime, Health & Events:**
   - SOH (%) degradation tracking, Battery Cycle Count, Lifetime Energy Throughput (kWh/Wh), Round-Trip Efficiency (%), and BMS Alarm & Protection Event counters.
6. **🛠️ Pylon Smart Monitor (ESP32) Diagnostics & Resources:**
   - CPU Load (%) & Internal Chip Temperature (°C) gauges.
   - Free Heap, Min Free Watermark & Max Alloc Block (memory leak & fragmentation tracker).
   - WiFi RSSI (dBm) & Link Quality (%).
   - RS232 Serial Scrape Duration (s) & Hardware/Battery Inventory Tables.

### How to Import into Grafana:
1. In Grafana, navigate to **Dashboards** → **New** → **Import**.
2. Upload [`integrations/grafana/pylontech-smart-monitor-dashboard.json`](../integrations/grafana/pylontech-smart-monitor-dashboard.json) or paste its JSON content.
3. Select your Prometheus data source when prompted.
4. Click **Import**.

---

## 4. Useful PromQL Query Examples

| Query Purpose | PromQL Expression |
|:---|:---|
| **Pack Voltage (V)** | `pylontech_voltage_millivolts / 1000` |
| **Total Stack Current (A)** | `sum(pylontech_current_milliamps) / 1000` |
| **Total Stack Power (W)** | `sum(pylontech_voltage_millivolts * pylontech_current_milliamps) / 1000000` |
| **Average SOC (%)** | `avg(pylontech_soc_percent)` |
| **Worst Cell Spread (mV)** | `max(pylontech_cell_voltage_millivolts) - min(pylontech_cell_voltage_millivolts)` |
| **Per-Module Cell Delta (mV)** | `max by (module) (pylontech_cell_voltage_millivolts) - min by (module) (pylontech_cell_voltage_millivolts)` |
| **Module Temperature (°C)** | `pylontech_temperature_millicelsius / 1000` |
| **MOSFET Temperature (°C)** | `pylontech_mosfet_temperature_millicelsius / 1000` |
| **Active Balancing Cells** | `sum by (module) (pylontech_cell_balance)` |
| **Energy Throughput (kWh)** | `pylontech_euro_energy_throughput_watthours_total / 1000` |
| **ESP32 Free Heap (KB)** | `esp32_heap_free_bytes / 1024` |
| **ESP32 Heap Fragmentation (%)** | `esp32_heap_fragmentation_percent` |
| **ESP32 CPU Load (%)** | `esp32_cpu_usage_percent` |
| **ESP32 Chip Temperature (°C)** | `esp32_cpu_temperature_celsius` |
| **ESP32 WiFi Signal (%)** | `esp32_wifi_signal_percent` |
| **RS232 Scrape Duration (s)** | `pylontech_scrape_duration_seconds` |
| **ESP32 Uptime (hours)** | `esp32_uptime_seconds / 3600` |
