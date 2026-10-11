# Pylontech FastMCP Server

High-performance, safety-first Model Context Protocol (FastMCP) server connecting AI coding and home automation agents directly to Pylontech LiFePO4 battery banks monitored via the **WiFi-to-RS232 ESP32 Bridge** (or **BMS Emulator**).

---

## ⚡ Key Capabilities

* **High-Level Telemetry:** Instant access to stack summary, 15/16 cell voltages, balancing status, thermal gradients, and 24-hour historical trends via JSON REST streaming.
* **Multi-Device Support:** Manage single or multiple monitors and emulators in a unified YAML configuration (`config.yaml`).
* **Strict Safety Guardrails:**
  * **Endpoint Blocklist:** Destructive actions (`/update`, `/reboot`, `/factory_reset`, `/forget_wifi`) are strictly blocked at the MCP level.
  * **Command Allowlist:** Only non-destructive queries (`pwr`, `bat`, `stat`, `info`, `soh`, `euro`, `help`) are permitted through `/cmd`.
* **Hardware Model Compatibility Matrix:**
  * Auto-detects **US3000C**, **US3000D**, **US2000C**, **UP5000**, and **Force-L1/L2**.
  * Enforces model-specific constraints (e.g. Model D does not support `stat <n>` or `soh`; Model C uses `soh` and does not support `euro`).
* **Prometheus & Diagnostics:** Direct extraction of `/metrics` and ESP32 controller diagnostics (Free heap, CPU load, WiFi RSSI, reset reason).

---

## 📦 Installation & Setup

### 1. Requirements
* Python 3.10+
* FastMCP (`pip install "mcp>=1.0.0" httpx pyyaml pydantic`)

### 2. Configure Devices
Copy `config.example.yaml` to `config.yaml`:

```bash
cp config.example.yaml config.yaml
```

Edit `config.yaml` with your monitor and emulator IP addresses:

```yaml
devices:
  - name: "monitor_garage"
    type: "monitor"
    url: "http://192.168.1.150"
    bearer_token: ""         # Optional: if API token is configured in /settings
    description: "Main Battery Bank (US3000C x4) - Garage"

  - name: "test_emulator"
    type: "emulator"
    url: "http://192.168.1.200"
    bearer_token: ""
    description: "Lab ESP32 BMS Physics & Rack Emulator"

defaults:
  default_device: "monitor_garage"
  timeout_seconds: 10.0
  connect_timeout_seconds: 3.0
```

---

## 🛠️ Available MCP Tools

| Tool | Parameters | Description |
|:---|:---|:---|
| `pylon_list_devices` | — | Lists all configured battery monitors and emulators. |
| `pylon_get_data` | `device: str = "default"` | **Primary tool:** Full live telemetry (stack totals, individual modules, cell voltages, balancing, temperatures, ESP32 stats). |
| `pylon_get_module_detail` | `module_id: int, device: str = "default"` | In-depth telemetry for a specific module (0–15). |
| `pylon_get_status` | `device: str = "default"` | Real-time RS232 polling state (`is_polling`, `is_paused`). |
| `pylon_get_history` | `device: str = "default"` | Rolling 24-hour historical telemetry (1-min SOC and Current samples). |
| `pylon_get_peers` | `device: str = "default"` | Discovered peer monitors on the local network via mDNS. |
| `pylon_get_metrics` | `device: str = "default"` | Raw Prometheus / OpenMetrics telemetry directly from `/metrics`. |
| `pylon_get_console_log` | `device: str = "default"` | Recent raw RS232 traffic buffer (`TX >>`, `RX <<`). |
| `pylon_poll_now` | `device: str = "default"` | Triggers an immediate serial scrape. |
| `pylon_toggle_pause` | `device: str = "default"` | Toggles background polling pause state. |
| `pylon_send_command` | `command: str, device: str = "default"` | Safely enqueues a safe console command (strictly validated against allowlist and hardware profile). |

---

## 🔒 Safety & Hardware Constraints

1. **No Destructive Actions:** Dangerous operations like OTA updates, ESP32 reboots, and network resets can NEVER be triggered via MCP.
2. **Model Awareness:**
   * **US3000D (Model D):** Rejects `soh` and `stat <n>` queries (Model D only supports stack `stat` and uses `euro` for efficiency stats).
   * **US3000C (Model C):** Rejects `euro` (uses `soh` for cell degradation matrix).

---

## 🤖 Connecting to AI Agents

### 1. Antigravity / Claude Desktop / Cursor

Add the following to your MCP settings file (e.g. `~/.gemini/antigravity/mcp_config.json`, `claude_desktop_config.json`, or `.cursor/mcp.json`):

```json
{
  "mcpServers": {
    "pylon-monitor": {
      "command": "python",
      "args": [
        "-m", "pylon_mcp.server"
      ],
      "cwd": "/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server",
      "env": {
        "PYLON_MCP_CONFIG": "/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server/config.yaml"
      }
    }
  }
}
```

### 2. OpenAI Codex / ChatGPT Desktop / OpenAI Agents SDK

For OpenAI clients supporting MCP tools (ChatGPT Desktop, Codex CLI, or custom OpenAI Assistants / Agents SDK):

**ChatGPT Desktop (`~/Library/Application Support/OpenAI/ChatGPT/mcp.json` or `%APPDATA%\OpenAI\ChatGPT\mcp.json`):**
```json
{
  "servers": {
    "pylon-monitor": {
      "command": "python",
      "args": [
        "-m",
        "pylon_mcp.server"
      ],
      "cwd": "/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server",
      "env": {
        "PYLON_MCP_CONFIG": "/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server/config.yaml"
      }
    }
  }
}
```

**OpenAI Codex CLI / Python Agent integration:**
```python
# Connect to pylon-mcp via stdio transport in Python:
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

server_params = StdioServerParameters(
    command="python",
    args=["-m", "pylon_mcp.server"],
    cwd="/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server",
    env={"PYLON_MCP_CONFIG": "/path/to/WiFi-to-RS232/integrations/ai/pylon-mcp-server/config.yaml"}
)

async with stdio_client(server_params) as (read, write):
    async with ClientSession(read, write) as session:
        await session.initialize()
        tools = await session.list_tools()
        result = await session.call_tool("pylon_get_data", arguments={"device": "default"})
        print(result.content)
```

---

## 🧪 Testing

Run native tests without connecting to any hardware:

```bash
python tests/run_tests.py
```
