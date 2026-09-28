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
| `esp32_cpu_freq_mhz` | Gauge | MHz | CPU clock frequency |

#### Heap Memory
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `esp32_heap_free_bytes` | Gauge | B | Current free heap |
| `esp32_heap_total_bytes` | Gauge | B | Total heap size |
| `esp32_heap_min_free_bytes` | Gauge | B | Historical minimum free heap (watermark since boot) |
| `esp32_heap_max_alloc_bytes` | Gauge | B | Largest single contiguous free block |
| `esp32_heap_fragmentation_percent` | Gauge | % | Heap fragmentation: `100 − (max_alloc / free × 100)` |

#### WiFi Signal
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `esp32_wifi_rssi_dbm` | Gauge | dBm | WiFi received signal strength indicator |
| `esp32_wifi_signal_percent` | Gauge | % | Mapped signal quality (0–100%) |
| `esp32_wifi_channel` | Gauge | — | WiFi channel number |

#### Storage
| Metric | Type | Unit | Description |
|:---|:---:|:---|:---|
| `esp32_flash_size_bytes` | Gauge | B | Total flash chip capacity |
| `esp32_sketch_size_bytes` | Gauge | B | Compiled firmware binary size |

---

## 3. Example Prometheus Metrics Output

```text
# HELP pylontech_scrape_success Whether the last serial scrape was successful
# TYPE pylontech_scrape_success gauge
pylontech_scrape_success 1

# HELP pylontech_scrape_duration_seconds Duration of serial scrape in seconds
# TYPE pylontech_scrape_duration_seconds gauge
pylontech_scrape_duration_seconds 0.842

# HELP pylontech_modules_detected Number of active battery modules
# TYPE pylontech_modules_detected gauge
pylontech_modules_detected 1

# HELP pylontech_module_info Battery module metadata
# TYPE pylontech_module_info gauge
pylontech_module_info{module="6",model="US3000C",barcode="PY240518C7K81942",board="H",main_soft="2.8",soft="2.8"} 1

# HELP pylontech_voltage_millivolts Module overall voltage
# TYPE pylontech_voltage_millivolts gauge
pylontech_voltage_millivolts{module="6"} 49820

# HELP pylontech_current_milliamps Module overall current
# TYPE pylontech_current_milliamps gauge
pylontech_current_milliamps{module="6"} -1420

# HELP pylontech_soc_percent Module State of Charge percent
# TYPE pylontech_soc_percent gauge
pylontech_soc_percent{module="6"} 62

# HELP pylontech_cell_voltage_millivolts Individual cell voltage in millivolts
# TYPE pylontech_cell_voltage_millivolts gauge
pylontech_cell_voltage_millivolts{module="6",cell="0"} 3321
pylontech_cell_voltage_millivolts{module="6",cell="1"} 3322
...
```

---

## 4. Useful PromQL Query Examples for Grafana

| Query Purpose | PromQL Expression |
|:---|:---|
| **Pack Voltage (V)** | `pylontech_voltage_millivolts / 1000` |
| **Total Stack Current (A)** | `sum(pylontech_current_milliamps) / 1000` |
| **Total Stack Power (W)** | `(pylontech_voltage_millivolts / 1000) * (pylontech_current_milliamps / 1000)` |
| **Cell Voltage Delta (mV)** | `max(pylontech_cell_voltage_millivolts) by (module) - min(pylontech_cell_voltage_millivolts) by (module)` |
| **BMS Temperature (°C)** | `pylontech_temperature_millicelsius / 1000` |
| **Active Balancing Cells** | `sum(pylontech_cell_balance) by (module)` |
| **Cumulative Discharged Capacity (Ah)** | `pylontech_discharged_capacity_mah_total / 1000` |
| **ESP32 Free Heap (KB)** | `esp32_heap_free_bytes / 1024` |
| **ESP32 Heap Fragmentation (%)** | `esp32_heap_fragmentation_percent` |
| **ESP32 CPU Load (%)** | `esp32_cpu_usage_percent` |
| **ESP32 Chip Temperature (°C)** | `esp32_cpu_temperature_celsius` |
| **ESP32 WiFi Signal (%)** | `esp32_wifi_signal_percent` |
| **ESP32 Uptime (hours)** | `esp32_uptime_seconds / 3600` |
