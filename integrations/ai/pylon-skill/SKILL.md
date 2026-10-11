---
name: pylon-battery-diagnostics
description: Diagnose and analyze Pylontech LiFePO4 battery stacks (US2000C, US3000C, US3000D, US5000, UP5000) monitored via WiFi-to-RS232 bridge or BMS Emulator using the Pylon FastMCP server. Evaluates cell voltages, balancing, thermal gradients, model-specific capabilities, and BMS alarms.
---

# Pylontech Battery Diagnostics & Analysis Skill

Use this skill to systematically monitor, diagnose, and troubleshoot Pylontech LiFePO4 battery systems connected via the WiFi-to-RS232 ESP32 Bridge or BMS Emulator using the `pylon-monitor` FastMCP server.

---

## 🔍 Diagnostic Workflow

When a user asks for battery health, status, or troubleshooting:

1. **Discover Devices & Detect Model:**
   * Call `pylon_get_data(device="...")` (or `pylon_list_devices()` if device is unknown).
   * Identify the **Master Model** from `stack.model` (e.g. `US3000C`, `US3000D`, `UP5000`).
   * Check communication health: verify `stack.scrape_success == true` and `stack.is_polling`.

2. **Evaluate Core Telemetry:**
   * **State of Charge (SOC):** Stack average SOC % vs individual module SOC %.
   * **Current & Direction:** Positive current = Charging (solar/grid), Negative = Discharging (inverter load), 0A = Idle / Float.
   * **Terminal Voltage:** Total stack voltage (e.g. ~48V–52V for 15S LiFePO4).

3. **Analyze Cell-Level Health:**
   * Inspect individual cell voltages across all detected modules (`modules[i].cells`).
   * Calculate cell voltage delta ($\Delta V = V_{\max} - V_{\min}$) per module and across the entire rack.
   * Check for active cell balancing (`cell.balance == true`).

4. **Verify Thermal Distribution:**
   * Inspect pack internal temperatures (`temp_c`).
   * For **US3000D**, also check BMS power MOSFET temperatures (`mos_temp_c`).

5. **Review BMS Health & Protection Counters:**
   * For single module deep-dives, call `pylon_get_module_detail(module_id=N)`.
   * Check cumulative protection event counters (OV, UV, COC, DOC, SC, COT, DOT).

---

## 🔋 LiFePO4 Voltage & Health Thresholds

LiFePO4 chemistry has an extremely flat discharge curve between 20% and 80% SOC. Use these standard voltage thresholds for diagnosis:

| Condition | Cell Voltage (mV) | Pack Voltage (15S) | Pack Voltage (16S - UP5000) | Diagnostic Meaning |
|:---|:---:|:---:|:---:|:---|
| **Over-Voltage Cutoff** | > 3,650 mV | > 54.75 V | > 58.40 V | 🔴 Critical: BMS will trigger OV protection |
| **Full Charge / Absorption** | 3,450 – 3,550 mV | 51.75 – 53.25 V | 55.20 – 56.80 V | 🟢 Normal at 100% SOC during CV charge |
| **Float / Resting** | 3,330 – 3,380 mV | 49.95 – 50.70 V | 53.28 – 54.08 V | 🟢 Normal resting state (90–99% SOC) |
| **Nominal Working Range** | 3,250 – 3,330 mV | 48.75 – 49.95 V | 52.00 – 53.28 V | 🟢 Normal working plateau (20–85% SOC) |
| **Discharged Knee** | 3,100 – 3,200 mV | 46.50 – 48.00 V | 49.60 – 51.20 V | 🟡 Low: Approaching empty state (<15% SOC) |
| **Under-Voltage Cutoff** | < 2,800 mV | < 42.00 V | < 44.80 V | 🔴 Critical: BMS will disconnect to prevent cell damage |

### Cell Voltage Imbalance ($\Delta V$):
* **< 15 mV:** ⭐ Excellent balance. Typical for healthy, synchronized cells.
* **15 – 35 mV:** 🟢 Acceptable working spread under moderate load or high charge rate.
* **35 – 50 mV:** 🟡 Moderate spread. Monitor during next full charge cycle to see if balancing activates.
* **> 50 mV:** 🔴 Significant cell imbalance. Check if one weak cell is hitting the low cutoff early.

### Thermal Operating Windows:
* **Cell Temperature (`temp_c`):**
  * Normal: 18°C – 32°C.
  * Cold Warning: < 5°C (charging current should be restricted by BMS).
  * Hot Warning: > 40°C (requires rack ventilation inspection).
  * Critical High: > 50°C (BMS triggers COT protection).
* **MOSFET Temperature (`mos_temp_c` on US3000D):**
  * Normal: 25°C – 55°C.
  * Warning: > 65°C under continuous heavy discharge.

---

## ⚠️ Hardware Model Differences & Compatibility Matrix

Pylontech battery generations communicate differently over RS232:

| Capability | US3000C / US2000C | US3000D (Model D) | UP5000 / US5000 |
|:---|:---:|:---:|:---:|
| **Cell Count** | 15 cells | 15 cells | **16 cells** |
| **`pwr` Columns** | 18 columns | **22 columns** (Sensor IDs) | 18 columns |
| **`stat <n>` (Slave query)** | ✅ Supported | ❌ **Unsupported** (Stack only) | ✅ Supported |
| **`info <n>` (Slave query)** | ✅ Supported | ❌ **Unsupported** | ✅ Supported |
| **`soh` Command** | ✅ Supported | ❌ **Unsupported** | ✅ Supported |
| **`euro` Command** | ❌ Unsupported | ✅ **Supported** (Wh / Ah throughput) | ❌ Unsupported |
| **MOSFET Temp Sensor** | ❌ None | ✅ **Built-in** | ❌ None |

### Master Battery Precedence Rule:
In mixed racks (e.g. US3000D Master + US3000C Slaves), **the Master battery dictates the console protocol**:
* If Master is **US3000D**: Slave-indexed queries (`stat 1`, `info 1`) and `soh` will fail on the console.
* **Always rely on `pylon_get_data` and `pylon_get_module_detail`** — the ESP32 firmware handles this model detection and multi-cadence polling automatically.

---

## 🔒 Safety Rules for AI Agents

1. **NEVER attempt system-disrupting commands:**
   * Destructive commands (`shutdown`, `reboot`, `format`, `reset`) are blocked by the MCP server.
   * Do not instruct the user to execute raw shutdown commands if solar inverters or critical loads rely on the battery bank.
2. **Read-First Principle:**
   * Gather complete diagnostic data using `pylon_get_data` and `pylon_get_history` before drawing conclusions.
   * If manual RS232 queries are necessary, use `pylon_send_command` with safe queries (`pwr`, `bat`, `stat`, `info`).
