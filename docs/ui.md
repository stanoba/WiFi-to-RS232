# Web Interface Guide

Visual overview of the **WiFi-to-RS232 Pylon Smart Monitor** web interface — all pages accessible via `http://<device-ip>/` or `http://pylon-smart.local/`.

---

## Navigation Bar & Polling Indicator

The persistent header across all web pages provides immediate visual feedback of the BMS communication state:

- **🟢 Green `● Polling` (Idle):** Battery telemetry is fresh; the serial interface is idle waiting for the next scheduled cycle.
- **🟡 Amber Blinking `● Polling` (Active):** An active RS232 scrape transaction (`pwr`, `bat`, `stat`, `info`) is executing in the background. The smooth opacity animation confirms real-time serial activity without layout shifting.
- **🔴 Red `⏸ Paused`:** BMS serial polling is paused by the user (via `/toggle_pause` or quick action).
- **Universal Synchronization:** Powered by the ultra-lightweight `/api/status` endpoint (or live AJAX payloads on `/` and `/module`), ensuring the indicator state stays synchronized in real time regardless of which page is currently open (`/`, `/module`, `/log`, `/settings`, `/wifi`, `/update`).

---

## Dashboard (`/`)

Live battery telemetry overview organized into modular Title Case sections with thematic emojis:
- **6×2 Analytics Grid** — 12 telemetry cards: Average SOC, Total Current, Highest/Lowest Cell Voltage (both with consistent blue accent styling), Highest/Lowest Temperature, Voltage Spread (ΔV), Stack SOH, Stack Voltage, Total Power, Average Cell Voltage, Cell Standard Deviation. All cards update dynamically via AJAX without full page reloads.
- **📈 24-Hour Telemetry History** — Zero-dependency HTML5 Canvas chart with 1h/6h/12h/24h range selector, area gradient fill for SOC (%), red line curve and unified tooltip indicator for Current (A) with auto-scaling right Y-axis. Voltage was streamlined out to minimize memory footprint.
- **🔋 Battery Modules Detail** — Module ID, Device Model, Voltage, Current, SOC, Spread, Pack Temp, MOSFET Temp, Base State, Barcode.
- **⚙️ System Status & Diagnostics** — Symmetrical 3-column live grid: Hostname, IP, WiFi signal & SSID, battery link status, active units, MQTT status, uptime, free RAM & fragmentation, CPU load & temperature, reset reason, chip model & flash size.
- **🌐 Pylon Smart Monitors on Network** — Automatic background mDNS discovery card that displays live peer devices detected on the LAN, including hostname, IP address, battery model, connected module count, firmware version, and one-click navigation links.

| Light Theme | Dark Theme |
|:---:|:---:|
| ![Dashboard — Light](../assets/ui-dashboard.png) | ![Dashboard — Dark](../assets/ui-dashboard-dark.png) |

---

## 19" Rack Battery Cell Matrix (`/module?m=N`)

Physical 19" chassis visualization for a single battery module:
- **Cell Bargraphs** — 15 series cells displayed as vertical fill bars.
- **Live Values** — SOC % above 3-decimal cell voltage in high-contrast white.
- **Dynamic Thermal-to-Turquoise Gradient** — Fill color transitions from cell temperature color at the top (cyan <18°C → green 18–25°C → lime 26–32°C → amber 33–40°C → red >40°C) to the turquoise brand base, with inline thermal legend.
- **Detail Cards** — Health & Cycles (SOH count/status), Thermal Telemetry (with ΔT row), Hardware Spec (board version, firmware, release date).
- **BMS Cumulative Event Table** — Lifetime protection trigger counts: COC, DOC, SC, Bat OV, Bat LV, Bat UV, COT, DOT.

| Light Theme | Dark Theme |
|:---:|:---:|
| ![Module Detail — Light](../assets/ui-module-detail.png) | ![Module Detail — Dark](../assets/ui-module-detail-dark.png) |

---

## Live RS232 Console (`/log`)

Real-time RS232 terminal log:
- Transmitted commands displayed as `TX >>`, received responses as `RX <<`, each with synchronized real-world timestamp `[YYYY-MM-DD HH:MM:SS]`.
- **Action Toolbar** — `⚡ Poll Now`, auto-refresh toggle, `☀️ Light Theme` toggle, `🗑️ Clear Log`.
- **Quick Command Buttons** — `stat`, `info`, `bat`, `pwr`, `soh`/`euro`.
- **Custom Command Input** — Send arbitrary Pylontech console commands directly.

| Light Theme | Dark Theme |
|:---:|:---:|
| ![Console Log — Light](../assets/ui-console-log.png) | ![Console Log — Dark](../assets/ui-console-log-dark.png) |

---

## Settings (`/settings`)

System configuration page:
- **Network** — Static IP, Subnet Mask, Default Gateway, Primary/Secondary DNS.
- **Time Synchronization** — NTP server configuration, 22-city timezone selector with DST, 24h/12h format toggle, sync status badge, last sync timestamp, on-demand sync button.
- **Security** — HTTP Basic Auth credentials for Web UI, Bearer Token for REST API.
- **Polling** — Fast poll interval (default 60s), Slow poll interval (default 300s).
- **Home Assistant MQTT** — Broker IP, port, credentials, topic prefix.
- **System Actions** — Reboot, WiFi reconfigure, factory reset.

| Light Theme | Dark Theme |
|:---:|:---:|
| ![Settings — Light](../assets/ui-settings.png) | ![Settings — Dark](../assets/ui-settings-dark.png) |

---

## AP / Captive Portal Mode

When no WiFi is configured or the network is unavailable, the device creates an open access point:

- **SSID:** `Pylon-Smart-XXXX` (last 4 chars of MAC)
- **Password:** none (open network)
- **IP:** `192.168.4.1`
- Captive portal automatically redirects to the full Dashboard.
- Battery polling triggers automatically when a client connects.
- Useful for portable field diagnostics without a home network.

---

## OTA Firmware Update (`/update`)

Browser-based over-the-air update:
1. Open `http://<device-ip>/update`.
2. Click **Choose File** and select `firmware.bin` from `.pio/build/<env>/firmware.bin`.
3. Click **Flash Firmware** — the device uploads, verifies, flashes, and reboots automatically.

> [!TIP]
> PlatformIO network OTA is also supported: `pio run -t upload --upload-port <device-ip>`
