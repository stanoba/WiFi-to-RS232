# Pylontech BMS Console Emulator (ESP32)

Standalone ESP32 firmware (Wemos D1 Mini ESP32, LOLIN S2 Mini, ESP32-C3 Super Mini) that emulates the hardware RS232 console interface of Pylontech LiFePO4 battery stacks (**US2000C**, **US3000C**, **US3000D**, **US5000**, **UP5000**).

It enables full offline development, automated testing, and validation of monitoring software (**Pylon Smart Monitor**) without requiring a physical battery stack.

---

## ⚡ One-Click Web Installer (No Toolchain Required)

Flash the BMS Emulator directly from your web browser using Web Serial (Chrome, Edge, Brave, Opera):

👉 **[Launch Web Installer & Flasher (stanoba.github.io/WiFi-to-RS232)](https://stanoba.github.io/WiFi-to-RS232/)**

1. Connect your ESP32 via USB.
2. Select **Pylon BMS Emulator**.
3. Click **Connect & Flash**.

---

## 1. Hardware & Interconnect Cable (100% Reusability)

The emulator uses the **exact same hardware board** (MAX3232 PCB + ESP32) as the Pylon Smart Monitor.

Connecting the Smart Monitor board and the Emulator board is done via a standard **RJ45 Crossover / Null-Modem cable**:

```
┌─────────────────────────────────┐                 ┌─────────────────────────────────┐
│     Pylon Smart Monitor         │                 │      Pylon BMS Emulator         │
│     (MAX3232 PCB + ESP32)       │                 │     (Identical HW + ESP32)      │
│                                 │                 │                                 │
│   RJ45 Connector                │                 │   RJ45 Connector                │
│   Pin 3 (TX RS232)  ────────────┼─────────────────┼───► Pin 6 (RX RS232)            │
│   Pin 6 (RX RS232)  ◄───────────┼─────────────────┼───── Pin 3 (TX RS232)           │
│   Pin 8 (GND RS232) ────────────┼─────────────────┼──── Pin 8 (GND RS232)           │
└─────────────────────────────────┘                 └─────────────────────────────────┘
```

> **Note:** Pins 1, 2, 4, 5, and 7 are not connected (NC). The cable provides a safe, level-shifted ±12V RS232 link with a common ground reference.

---

## 2. Supported Models & Commands

The emulator faithfully replicates Pylontech CLI prompt syntax (`@ ... $$ pylon>`), table column headers, decimal formatting, and model-specific quirks:

| Command | US2000C | US3000C | US3000D | US5000 / UP5000 | Description |
|---|---|---|---|---|---|
| `pwr` | 18 columns | 18 columns | 22 columns (incl. sensor ID) | 18 columns | Stack & unit summary telemetry |
| `bat` | 15 cells | 15 cells | 15 cells (DTemp. & CTemp.) | 16 cells | Per-cell voltages, temperatures, states |
| `bat <n>` | Supported | Supported | Supported | Supported | Detailed telemetry for specific module |
| `stat` | Supported | Supported | Supported (stack only) | Supported | Lifetime protection and cycle statistics |
| `stat <n>`| Supported | Supported | *Unsupported* | Supported | Statistics for specific module |
| `info` | Supported | Supported | Supported | Supported | Barcodes, serial numbers, firmware versions |
| `soh` | Supported | Supported | *Unknown command* | Supported | Per-cell SOH degradation matrix |
| `euro` | *Unknown command* | *Unknown command* | Supported | *Unknown command* | European efficiency and energy throughput stats |
| `help` | Supported | Supported | Supported | Supported | List of available console commands |

---

## 3. Web Interface & Features

- **Interactive Dashboard (`/`)**:
  - **LiFePO4 Physics Engine:** Sliders for global State of Charge (0–100% SOC), current flow (+A charge / -A discharge / idle), ambient temperature, and cell imbalance spread (mV).
  - **Dynamic Solar & Inverter Simulation:** Realistic simulation of solar PV charging curves and household load profiles with cloud passing fluctuations and current micro-jitter.
  - **19" Server Rack Visualizer:** Proportional faceplate heights (2U for US2000C, 3U for US3000, 3.6U for US5000), interactive DIP switches, power switches, RJ45 ports, LED indicators (RUN/ALM), and 6-segment SOC bargraphs.
  - **Master Hierarchy Validation:** Auto-enforces generation rank (US5000 > US3000D > US3000C > US2000C) and firmware priority rules with one-click **Auto-Sort**.
  - **Scalability:** Add or remove modules dynamically (1 to 16 batteries in rack).
  - **mDNS Peer Discovery:** Automatically detects live Pylon Smart Monitors on the local network.
- **Module Detail & Fault Injection (`/module?id=N`)**:
  - Customize model type, serial numbers, firmware versions, individual cell voltages, temperatures, and balancing flags (`BAL`).
  - Configure lifetime protection counters (`stat`: OV/UV/COC/DOC/COT/DOT/BMIC/LifeAlarm) and model-specific stats (`soh` vs `euro`).
  - Real-time fault and alarm injection (Over-Voltage, Under-Voltage, Over-Current, Over-Temperature, BMIC Fault).
- **Serial Console (`/log`)**:
  - Full-screen terminal logging all incoming RS232 queries (`RX >>`) and outgoing responses (`TX <<`) with timestamp synchronization, log filtering, and quick command buttons.
- **Settings & API Security (`/settings`)**:
  - Network configuration (STA / AP), Hostname, NTP time synchronization with worldwide timezones.
  - **Bearer Token Security:** Protect `/api/...` endpoints with API tokens and an integrated token generator.

| Light Theme | Dark Theme |
|:---:|:---:|
| ![Emulator Dashboard — Light](../assets/ui-emulator-dashboard.png) | ![Emulator Dashboard — Dark](../assets/ui-emulator-dashboard-dark.png) |

---

## 4. Automated Unit Testing (Native C++)

The emulator includes high-speed native C++ unit tests (Unity framework) running locally on your host development machine:

```bash
pio test -d emulator -e native
```

**Test Coverage:**
* **LiFePO4 OCV Curve:** Accurate open-circuit voltage calculations across 0–100% SOC.
* **Physics & Balancing:** Internal resistance, MOSFET self-heating, cell spread factors, and automatic balancing under charge.
* **Master Hierarchy:** Generation rank checks, firmware precedence, and automatic sorting.
* **CLI Protocol Formatting:** Exact string formatting for `help`, `info`, `pwr` (18 vs 22 columns), `bat`, `stat`, `soh`, and `euro`.
* **Atomic NVS Persistence:** Verification of binary blob rack storage and module add/delete persistence across reboots.

---

## 5. Build and Flash Firmware

```bash
cd emulator
# Wemos D1 Mini ESP32 (default):
pio run -e wemos_d1_mini32 -t upload

# LOLIN S2 Mini (ESP32-S2):
pio run -e lolin_s2_mini -t upload

# ESP32-C3 Super Mini:
pio run -e esp32_c3_super_mini -t upload
```
