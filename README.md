# WiFi-to-RS232 Pylon Smart Monitor

Hardware and firmware solution for monitoring Pylontech LiFePO4 battery stacks (**US3000C**, **US3000D**) via their RJ45 RS232 console port and exposing metrics over WiFi in **Prometheus format**, **Home Assistant MQTT**, and an interactive **Web Dashboard**.

---

## Features

- **Battery Support:** Tested with **US3000C** and **US3000D** modules (15-cell configuration).
- **Auto Model & Stack Detection:** Identifies battery model via `info` command and automatically detects Master stacks and active module count (up to 16 units) via `pwr`.
- **Intelligent Dual-Cadence Polling:**
  - **Fast Polling (default 60s):** Queries dynamic telemetry (`pwr` stack totals and `bat` per-cell voltages, temperatures, and balancing flags) in under 1 second.
  - **Slow Polling (default 300s):** Queries static/slow telemetry (`stat` cycle counts & alarms, `info` firmware & barcodes, `soh`/`euro` metrics).
  - Configurable directly from the Web UI (`/settings`).
- **Interactive Web Dashboard** with 24-Hour Canvas chart, 12-card analytics grid, modules table, and live system status.
- **19" Rack Battery Visualization** — cell bargraphs with dynamic thermal gradient and BMS alarm counters.
- **Time Synchronization & Timezones (NTP)** with 22 worldwide cities and automatic DST.
- **Network & Static IP Configuration** directly from the Web UI.
- **Home Assistant MQTT Integration** with Auto-Discovery, device isolation via MAC suffixes, and 3-decimal display precision for cell voltages.
- **REST JSON API** — live telemetry, historical time-series, and ESP32 system diagnostics.
- **Prometheus Exporter & Grafana Dashboard** — full battery and ESP32 system metrics at `/metrics` with a ready-to-import [Grafana dashboard](integrations/grafana/pylontech-smart-monitor-dashboard.json).
- **Security** — optional HTTP Basic Auth and Bearer Token for REST API.
- **Standalone AP Mode** — portable field diagnostics without a home network.
- **Live RS232 Console** — raw `TX >>` / `RX <<` log with quick command buttons.
- **mDNS Peer Discovery** — automatically discovers other Pylon Smart Monitors on the network with module counts and live links.
- **Over-The-Air (OTA) Updates** — browser upload or PlatformIO network OTA.
- **Hardware Status LEDs** — WiFi (`CONN`) and Serial (`SER`) activity indicators.

---

## Web Interface

![Dashboard — Light Theme](assets/ui-dashboard.png)
 
![Dashboard — Dark Theme](assets/ui-dashboard-dark.png)

> [!TIP]
> Full web interface description with all screenshots: **[`docs/ui.md`](docs/ui.md)**

---

## Getting Started

### 1. Build and Flash Firmware

Prerequisites: [PlatformIO Core](https://platformio.org/).

**Wemos D1 Mini ESP32** (default):
```bash
cd software
pio run -e wemos_d1_mini32 -t upload
```

**LOLIN S2 Mini (ESP32-S2):**
```bash
cd software
# On first flash, hold the '0' button, press RST, release '0' to enter bootloader mode
pio run -e lolin_s2_mini -t upload
```

**ESP32-C3 Super Mini (RISC-V):**
```bash
cd software
# On first flash, hold the 'BOOT' button, plug in USB-C, release 'BOOT' to enter bootloader mode
pio run -e esp32_c3_super_mini -t upload
```

To monitor debug serial output:
```bash
pio device monitor -b 115200
```

### 2. First Boot & WiFi Setup

1. Power on the device.
2. Connect to the open WiFi access point **`Pylon-Smart-XXXX`** (no password required).
3. The captive portal opens automatically (or navigate to `http://192.168.4.1/`).
4. Open `/settings`, select your SSID, enter password, and save.

### 3. Web Navigation

| URL | Description |
|:---|:---|
| `http://<device-ip>/` | Main Dashboard |
| `http://<device-ip>/module?m=N` | Module Rack Detail |
| `http://<device-ip>/log` | Live RS232 Console |
| `http://<device-ip>/settings` | Settings & Integrations |
| `http://<device-ip>/wifi` | WiFi Setup Portal |
| `http://<device-ip>/metrics` | Prometheus Metrics |
| `http://<device-ip>/api/data` | REST JSON API (live telemetry & diagnostics) |
| `http://<device-ip>/api/module` | REST JSON API (module detail) |
| `http://<device-ip>/api/history` | REST JSON API (24h telemetry samples) |
| `http://<device-ip>/api/peers` | REST JSON API (discovered peer monitors) |
| `http://<device-ip>/update` | OTA Firmware Update |
| `http://pylon-smart.local/` | mDNS hostname (same as above) |

---

## Documentation

| Document | Description |
|:---|:---|
| **[`docs/hardware.md`](docs/hardware.md)** | PCB design, pinout, UART mapping, RS232 interface |
| **[`docs/ui.md`](docs/ui.md)** | Web interface guide with screenshots |
| **[`docs/api.md`](docs/api.md)** | REST API reference, endpoint schemas, integration examples |
| **[`docs/prometheus.md`](docs/prometheus.md)** | Prometheus metrics reference and Grafana PromQL recipes |
| **[`integrations/grafana/`](integrations/grafana/pylontech-smart-monitor-dashboard.json)** | Ready-to-import Grafana dashboard template for Prometheus |
| **[`software/README.md`](software/README.md)** | Firmware build, configuration, and OTA update guide |

---

## Disclaimer

> [!CAUTION]
> **Use at Your Own Risk:** This project, including all firmware, hardware schematics, PCB designs, documentation, and tools, is provided on an **"AS IS"** basis without warranty of any kind, either express or implied, including but not limited to the implied warranties of merchantability, fitness for a particular purpose, or non-infringement.
>
> In no event shall the authors, maintainers, or contributors be held liable for any direct, indirect, incidental, special, exemplary, punitive, or consequential damages (including, but not limited to, loss of data, loss of profits, equipment damage, battery degradation, electrical fires, property damage, or personal injury) arising in any way out of the use, modification, assembly, or installation of this hardware or software, even if advised of the possibility of such damage.
>
> Working with high-capacity lithium battery energy storage systems (LiFePO4) involves high electrical currents and inherent fire or shock hazards. You are solely responsible for verifying wiring, correct voltage polarity, proper fusing, and compliance with local electrical safety regulations before connecting any custom hardware to your battery or inverter.
>
> **Trademark Notice:** This is an independent open-source project and is not affiliated with, endorsed by, sponsored by, or associated with Pylon Technologies Co., Ltd. "Pylontech", "Pylon", and associated model designations are trademarks or registered trademarks of their respective holders.

---

## License

This project is licensed under the GNU General Public License v3.0 — see the [LICENSE](LICENSE) file for full details.
