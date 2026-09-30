# Hardware Design & Pinout

PCB design, component selection, and pin mapping for the **WiFi-to-RS232 Pylon Smart Monitor**.

---

## PCB

| PCB Top View | PCB Bottom View |
|:---:|:---:|
| ![PCB Top View](../assets/pcb-top.png) | ![PCB Bottom View](../assets/pcb-bottom.png) |

Fabrication Gerber files, drill files, and BOM are available in [`hardware/WemosSerial_2022-06-10.zip`](../hardware/WemosSerial_2022-06-10.zip).

---

## Supported Boards

| Board | PlatformIO Env | Chip | Cores | Recommendation |
|:---|:---|:---|:---|:---|
| **Wemos D1 Mini ESP32** | `wemos_d1_mini32` | ESP32 | Dual-core 240 MHz | **Recommended** (Best stability) |
| LOLIN S2 Mini | `lolin_s2_mini` | ESP32-S2 | Single-core 240 MHz | Alternative (Native USB-C, PSRAM) |
| ESP32-C3 Super Mini | `esp32_c3_super_mini` | ESP32-C3 | Single-core RISC-V 160 MHz | Ultra-compact |

> [!TIP]
> **Recommended Board:** The **Wemos D1 Mini ESP32** is the strongly recommended hardware platform. Its dual-core architecture allows the WiFi/IP network stack, HTTP web server, and mDNS responder to run on Core 0 while dedicated UART serial polling and FreeRTOS tasks execute on Core 1 without resource contention, resulting in significantly higher stability and responsiveness compared to single-core alternatives.

---

## Wemos D1 Mini ESP32 Socket Pinout

| Socket Pin | Function | ESP32 GPIO | Description |
|:---|:---|:---|:---|
| **D8** | UART2 TX | `GPIO5` | RS232 console TX to MAX3232 driver |
| **D7** | UART2 RX | `GPIO23` | RS232 console RX from MAX3232 driver (with internal pull-up) |
| **D3** | WiFi LED | `GPIO17` | `CONN` indicator LED (Active-LOW, VCC → Resistor → LED → GPIO) |
| **D4** | Serial LED | `GPIO16` | `SER` activity LED (Active-LOW, VCC → Resistor → LED → GPIO) |
| **5V / GND** | Power | `5V` / `GND` | Powered from 5V DC Jack or ESP32 USB |

> [!NOTE]
> Only the **outer 16 pins** of the Wemos D1 Mini ESP32 are plugged into the board socket. Hardware UART2 on `GPIO5` (TX) and `GPIO23` (RX) eliminates any interference with the onboard USB-to-UART bridge.

---

## LOLIN S2 Mini (ESP32-S2) Pin Mapping

The firmware also supports the **LOLIN S2 Mini** as a drop-in alternative module:

| Function | ESP32-S2 GPIO | Description |
|:---|:---|:---|
| UART1 TX | `GPIO12` | RS232 console TX to MAX3232 driver |
| UART1 RX | `GPIO11` | RS232 console RX from MAX3232 driver |
| WiFi LED | `GPIO18` | `CONN` indicator LED (Active-LOW) |
| Serial LED | `GPIO16` | `SER` activity LED (Active-LOW) |

> [!NOTE]
> The S2 Mini uses native USB CDC (no USB-to-UART chip). Hold the `0` button, press `RST`, then release `0` to enter bootloader mode for first-time USB flashing.

---

## ESP32-C3 Super Mini Pin Mapping

The firmware supports the ultra-compact **ESP32-C3 Super Mini** (RISC-V single-core @ 160MHz):

| Function | ESP32-C3 GPIO | Description |
|:---|:---|:---|
| UART1 TX | `GPIO4` | RS232 console TX to MAX3232 driver |
| UART1 RX | `GPIO5` | RS232 console RX from MAX3232 driver |
| WiFi LED | `GPIO8` | On-board blue indicator LED (Active-LOW) |
| Serial LED | `GPIO3` | `SER` activity indicator LED (Active-LOW) |

> [!NOTE]
> The ESP32-C3 features an internal hardware USB Serial/JTAG controller. Both the USB-C port (`Serial`) and hardware UART0 on pins 20/21 (`Serial0`) output debug logs simultaneously.

---

## UART Assignment Summary

| Board | Architecture | Pylontech UART | Object | TX GPIO | RX GPIO | Debug Console |
|:---|:---|:---|:---|:---|:---|:---|
| Wemos D1 Mini ESP32 | Xtensa Dual-Core | UART2 | `Serial2` | GPIO5 (D8) | GPIO23 (D7) | `Serial` (UART0 via onboard USB bridge) |
| LOLIN S2 Mini (ESP32-S2) | Xtensa Single-Core | UART1 | `Serial1` | GPIO12 | GPIO11 | `Serial` (USB-C CDC) + `Serial0` (pins 39/37) |
| ESP32-C3 Super Mini | RISC-V Single-Core | UART1 | `Serial1` | GPIO4 | GPIO5 | `Serial` (USB-C JTAG/CDC) + `Serial0` (pins 21/20) |

---

## RS232 Interface & Wiring

The Pylontech batteries expose a standard RS232 serial console via an **RJ45 modular port** (not Ethernet).

> [!WARNING]
> **DO NOT connect ESP32 GPIO pins directly to the Pylontech Console port!**
> The Pylontech RJ45 Console port uses standard true RS232 signaling with voltage levels up to $\pm 12\text{ V}$. Connecting ESP32 GPIO pins directly (which only tolerate 3.3V TTL logic) will **permanently destroy the ESP32 microcontroller**.
> 
> An RS232 transceiver (e.g. Maxim **MAX3232** with charge-pump capacitors or our custom hardware shield) **must always be used** to safely translate between RS232 ($\pm 12\text{ V}$) and 3.3V TTL UART logic.

### Pylontech RJ45 Console Pinout

| RJ45 Pin | Pylontech Signal | Direction | Connection to MAX3232 |
|:---|:---|:---|:---|
| **Pin 3** | Pylontech Console **TX** | Output $\rightarrow$ | MAX3232 **`R1IN`** (Pin 13) |
| **Pin 6** | Pylontech Console **RX** | Input $\leftarrow$ | MAX3232 **`T1OUT`** (Pin 14) |
| **Pin 8** | **GND** | Common Ground | MAX3232 / ESP32 **`GND`** |
| *Pins 1, 2, 4, 5, 7* | *NC* | — | Not connected |

### Communication Parameters

- **Baud Rate:** `115200`
- **Data Bits:** `8`
- **Stop Bits:** `1`
- **Parity:** None
- **Flow Control:** None

### Tested Battery Models

- Pylontech **US3000C** — 15-cell LiFePO4 (48V / 74Ah)
- Pylontech **US3000D** — 15-cell LiFePO4 (48V / 74Ah)
- Pylontech **US2000C** / **US2000** (compatible)
