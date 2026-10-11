"""
Safety rules, HTTP endpoint blocklist, and RS232 Command Allowlist.
Includes model-aware validation to prevent invalid commands on specific hardware models (e.g. Model D vs Model C).
"""

import re
from typing import Optional
from .models import ModelProfile, UNKNOWN_PROFILE


class SafetyError(Exception):
    """Raised when an operation violates MCP safety guidelines or hardware capability rules."""
    pass


# Endpoints that must NEVER be called by the MCP server to prevent service disruption,
# loss of network configuration, bricking, or inverter trip.
BLOCKED_HTTP_PATHS = frozenset({
    "/update",          # OTA firmware flashing
    "/reboot",          # ESP32 system reboot
    "/factory_reset",   # Reset all NVS settings to factory defaults
    "/forget_wifi",     # Erase WiFi credentials (drops remote connection)
    "/wifi",            # Interactive captive portal WiFi reconfiguration
})

# Endpoints that are read-only (GET allowed, POST/PUT/DELETE blocked)
READ_ONLY_HTTP_PATHS = frozenset({
    "/settings",        # Settings page (viewing is safe, modifying via MCP is blocked)
})

# Strict Allowlist of permitted Pylontech console commands
ALLOWED_RS232_COMMANDS = frozenset({
    "pwr",      # Stack power telemetry
    "bat",      # Per-cell voltages, temperatures, states
    "stat",     # Lifetime protection & cycle counters
    "info",     # Barcodes, serial numbers, firmware versions
    "soh",      # State of Health degradation matrix
    "euro",     # European efficiency & throughput statistics (Model D)
    "help",     # Help & command directory
})

# Commands permitted to take a single numeric module index (e.g. "bat 1", "pwr 1", "stat 2", "info 1")
ALLOWED_WITH_NUMERIC_ARG = frozenset({
    "bat",
    "pwr",
    "stat",
    "info",
})


def validate_http_request(path: str, method: str = "GET") -> None:
    """
    Validates an HTTP path and method against safety blocklists.
    Raises SafetyError if access is prohibited.
    """
    clean_path = path.split("?")[0].rstrip("/").lower()
    
    for blocked in BLOCKED_HTTP_PATHS:
        if clean_path == blocked.rstrip("/").lower() or clean_path.startswith(blocked.rstrip("/").lower() + "/"):
            raise SafetyError(
                f"Endpoint '{path}' is strictly blocked in the MCP server for safety. "
                "Actions like firmware updates, reboots, and network resets must be performed directly in the Web UI."
            )

    for read_only in READ_ONLY_HTTP_PATHS:
        if clean_path == read_only.rstrip("/").lower():
            if method.upper() != "GET":
                raise SafetyError(
                    f"HTTP method '{method}' is prohibited on '{path}'. Only 'GET' (read-only inspection) is permitted."
                )


def validate_rs232_command(raw_cmd: str, profile: Optional[ModelProfile] = None) -> str:
    """
    Validates and sanitizes a raw RS232 console command.
    Enforces the command allowlist and hardware model capability rules (Model C vs Model D).
    
    Raises SafetyError if the command is disallowed or incompatible with the detected battery model.
    """
    if not raw_cmd or not raw_cmd.strip():
        raise SafetyError("Command cannot be empty.")

    # Guard against command injection / chaining delimiters
    if re.search(r"[;&|`$<>\r\n]", raw_cmd):
        raise SafetyError("Command contains illegal characters or delimiters.")

    tokens = raw_cmd.strip().split()
    base_cmd = tokens[0].lower()

    # 1. Base command must be in allowlist
    if base_cmd not in ALLOWED_RS232_COMMANDS:
        raise SafetyError(
            f"Command '{base_cmd}' is not allowed via MCP. "
            f"Permitted console commands: {', '.join(sorted(ALLOWED_RS232_COMMANDS))}."
        )

    # 2. Argument validation
    arg: Optional[str] = None
    if len(tokens) > 1:
        if base_cmd not in ALLOWED_WITH_NUMERIC_ARG:
            allowed_with_args = ", ".join(f"'{c} <n>'" for c in sorted(ALLOWED_WITH_NUMERIC_ARG))
            raise SafetyError(f"Command '{base_cmd}' does not accept arguments. Only {allowed_with_args} accept a module ID.")
        
        arg = tokens[1]
        if not arg.isdigit():
            raise SafetyError(f"Argument for '{base_cmd}' must be a numeric module index (e.g. '{base_cmd} 1'). Received: '{arg}'.")
        
        if len(tokens) > 2:
            raise SafetyError(f"Too many arguments for '{base_cmd}'. Expected at most one numeric module index.")

    # 3. Model-aware compatibility checks
    if profile and profile.model != UNKNOWN_PROFILE.model:
        # Check 'euro' command compatibility
        if base_cmd == "euro" and not profile.supports_euro:
            raise SafetyError(
                f"Command 'euro' is not supported on model '{profile.name}'. "
                "Model D (US3000D) supports 'euro', whereas Model C (US3000C) uses 'soh'. "
                "Use high-level tool 'pylon_get_data' instead."
            )

        # Check 'soh' command compatibility
        if base_cmd == "soh" and not profile.supports_soh_cmd:
            raise SafetyError(
                f"Command 'soh' is not supported on model '{profile.name}'. "
                "US3000D does not implement 'soh' (it provides 'euro' stats). "
                "Use high-level tool 'pylon_get_data' instead."
            )

        # Check 'stat <n>' slave query compatibility
        if base_cmd == "stat" and arg is not None and not profile.slave_supports_stat:
            raise SafetyError(
                f"Command 'stat {arg}' is not supported on model '{profile.name}'. "
                "Model D (US3000D) only supports 'stat' without arguments for the whole stack. "
                "To inspect individual module statistics, use high-level tool 'pylon_get_module' or 'pylon_get_data'."
            )

        # Check 'info <n>' slave query compatibility
        if base_cmd == "info" and arg is not None and not profile.slave_supports_info:
            raise SafetyError(
                f"Command 'info {arg}' is not supported on model '{profile.name}'. "
                "Model D (US3000D) does not support slave info queries. Use 'info' without arguments."
            )

    return " ".join(tokens).lower()
