"""
FastMCP Server implementation for Pylontech Battery Monitor & BMS Emulator.
Provides typed, validated tools for AI Agents to inspect telemetry, health, and diagnostics.
"""

from typing import Any, Dict, List, Optional
from mcp.server.fastmcp import FastMCP
from .config import load_config, PylonConfig, DeviceConfig
from .client import PylonHttpClient, PylonConnectionError, PylonApiError
from .safety import SafetyError

# Initialize FastMCP Server
mcp = FastMCP(
    "pylon-monitor",
    instructions="Pylontech LiFePO4 battery monitoring and diagnostics via WiFi-to-RS232 bridge or BMS emulator"
)

# Global configuration and HTTP client
_config: PylonConfig = load_config()
_client: PylonHttpClient = PylonHttpClient(
    timeout=_config.defaults.timeout_seconds,
    connect_timeout=_config.defaults.connect_timeout_seconds
)


def _resolve(device_name: Optional[str]) -> DeviceConfig:
    return _config.get_device(device_name)


# ==============================================================================
# Device Discovery & Status Tools
# ==============================================================================

@mcp.tool()
def pylon_list_devices() -> List[Dict[str, str]]:
    """
    Lists all configured Pylontech monitor and emulator devices.
    Returns names, device types, URLs, and descriptions.
    """
    devs = _config.list_devices()
    return [
        {
            "name": d.name,
            "type": d.type,
            "url": d.clean_url(),
            "description": d.description or ""
        }
        for d in devs
    ]


@mcp.tool()
async def pylon_get_status(device: str = "default") -> Dict[str, Any]:
    """
    Retrieves the micro polling and pause state of the Pylontech BMS bridge.
    Useful to check whether a background RS232 scrape transaction is currently in progress.
    """
    target = _resolve(device)
    try:
        data = await _client.get_status(target)
        return {"device": target.name, "status": data}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


# ==============================================================================
# Primary Telemetry & Diagnostic Tools
# ==============================================================================

@mcp.tool()
async def pylon_get_data(device: str = "default") -> Dict[str, Any]:
    """
    Retrieves comprehensive live telemetry for the battery stack.
    Includes:
    - Stack summary (Voltage, Current, Power, Average SOC, SOH, worst cell voltage spread, module count)
    - Module list (Device model, SOC, voltage, current, temperatures, base state, barcode)
    - Per-cell voltages and balancing flags
    - ESP32 controller diagnostics (Free heap, CPU load, WiFi RSSI, chip temperature, uptime)
    
    This is the primary tool to call for overall battery health and operational status.
    """
    target = _resolve(device)
    try:
        data = await _client.get_data(target)
        return {"device": target.name, "data": data}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


@mcp.tool()
async def pylon_get_module_detail(module_id: int, device: str = "default") -> Dict[str, Any]:
    """
    Retrieves in-depth telemetry for a specific battery module index (0 to 15).
    Includes:
    - Individual 15 (or 16) cell voltages in mV and balancing indicators
    - Temperatures (Cell pack and MOSFET temperature where available)
    - Lifetime protection and cycle statistics
    - Hardware information (Board version, firmware revision, barcode)
    """
    target = _resolve(device)
    try:
        data = await _client.get_module(target, module_id)
        return {"device": target.name, "module_id": module_id, "data": data}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "module_id": module_id, "error": str(e)}


@mcp.tool()
async def pylon_get_history(device: str = "default") -> Dict[str, Any]:
    """
    Retrieves rolling 24-hour historical telemetry samples (1-minute intervals).
    Contains timestamps, battery SOC (%) and Current (A) history for trend and discharge analysis.
    """
    target = _resolve(device)
    try:
        data = await _client.get_history(target)
        return {"device": target.name, "history": data}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


@mcp.tool()
async def pylon_get_peers(device: str = "default") -> Dict[str, Any]:
    """
    Discovers other Pylon Smart Monitors present on the local network via mDNS.
    Returns hostnames, IP addresses, battery models, module counts, and firmware versions.
    """
    target = _resolve(device)
    try:
        data = await _client.get_peers(target)
        return {"device": target.name, "peers": data}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


@mcp.tool()
async def pylon_get_metrics(device: str = "default") -> str:
    """
    Retrieves raw Prometheus / OpenMetrics formatted telemetry directly from /metrics.
    Includes all battery metrics and ESP32 controller metrics.
    """
    target = _resolve(device)
    try:
        return await _client.get_metrics(target)
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return f"# Error retrieving metrics for {target.name}: {e}"


@mcp.tool()
async def pylon_get_console_log(device: str = "default") -> str:
    """
    Retrieves recent raw RS232 communication traffic (TX >> and RX <<) from the rolling console log buffer.
    """
    target = _resolve(device)
    try:
        return await _client.get_console_log(target)
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return f"Error retrieving console log for {target.name}: {e}"


# ==============================================================================
# Control & RS232 Interaction Tools (With Strict Safety Guardrails)
# ==============================================================================

@mcp.tool()
async def pylon_poll_now(device: str = "default") -> Dict[str, Any]:
    """
    Triggers an immediate RS232 telemetry polling cycle without waiting for the scheduled timer.
    """
    target = _resolve(device)
    try:
        res = await _client.poll_now(target)
        return {"device": target.name, "result": res}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


@mcp.tool()
async def pylon_toggle_pause(device: str = "default") -> Dict[str, Any]:
    """
    Toggles the background scheduled RS232 polling between active and paused.
    Pausing is useful before running manual console commands to avoid command collisions.
    """
    target = _resolve(device)
    try:
        res = await _client.toggle_pause(target)
        return {"device": target.name, "result": res}
    except (PylonConnectionError, PylonApiError, SafetyError) as e:
        return {"device": target.name, "error": str(e)}


@mcp.tool()
async def pylon_send_command(command: str, device: str = "default") -> str:
    """
    Safely sends a raw Pylontech console command via the asynchronous FIFO queue.
    
    SAFETY RESTRICTIONS:
    - Only safe query commands are allowed: 'pwr', 'bat', 'stat', 'info', 'soh', 'euro', 'help'.
    - Destructive or system-altering commands (shutdown, update, reboot, reset) are STRICTLY BLOCKED.
    - Model awareness: Rejects commands that are unsupported on specific battery models
      (e.g., 'stat <n>' and 'soh' are rejected on US3000D).
    """
    target = _resolve(device)
    try:
        return await _client.send_command(target, command)
    except SafetyError as se:
        return f"⚠️ Safety validation error: {se}"
    except (PylonConnectionError, PylonApiError) as e:
        return f"Error executing command on {target.name}: {e}"


def main():
    mcp.run()


if __name__ == "__main__":
    main()
