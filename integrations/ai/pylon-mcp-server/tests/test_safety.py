"""
Unit tests for safety guardrails: endpoint blocklist and RS232 command allowlist.
"""

import pytest
from pylon_mcp.safety import (
    validate_http_request,
    validate_rs232_command,
    SafetyError,
    BLOCKED_HTTP_PATHS,
    ALLOWED_RS232_COMMANDS
)


def test_blocked_http_endpoints():
    """Verify that all dangerous endpoints raise SafetyError."""
    for path in BLOCKED_HTTP_PATHS:
        with pytest.raises(SafetyError):
            validate_http_request(path, method="GET")
        with pytest.raises(SafetyError):
            validate_http_request(path, method="POST")


def test_subpath_blocked():
    """Verify subpaths of dangerous endpoints are also blocked."""
    with pytest.raises(SafetyError):
        validate_http_request("/update/firmware", method="POST")
    with pytest.raises(SafetyError):
        validate_http_request("/reboot?now=1", method="GET")


def test_read_only_endpoint_protection():
    """Verify write requests to read-only paths raise SafetyError."""
    # GET is allowed
    validate_http_request("/settings", method="GET")
    
    # POST, PUT, DELETE must fail
    with pytest.raises(SafetyError):
        validate_http_request("/settings", method="POST")
    with pytest.raises(SafetyError):
        validate_http_request("/settings", method="PUT")


def test_allowed_http_endpoints():
    """Verify standard telemetry and query endpoints are permitted."""
    allowed = ["/api/data", "/api/module?m=0", "/api/status", "/api/history", "/api/peers", "/metrics", "/log/raw", "/poll_now", "/toggle_pause"]
    for path in allowed:
        validate_http_request(path, method="GET")


def test_allowed_rs232_commands():
    """Verify all permitted query commands pass validation."""
    for cmd in ALLOWED_RS232_COMMANDS:
        res = validate_rs232_command(cmd)
        assert res == cmd


def test_allowed_commands_with_numeric_arg():
    """Verify 'bat <n>' and 'stat <n>' accept numeric module index."""
    assert validate_rs232_command("bat 1") == "bat 1"
    assert validate_rs232_command("bat 15") == "bat 15"
    assert validate_rs232_command("stat 2") == "stat 2"


def test_disallowed_command_args():
    """Verify non-numeric arguments or commands that don't take args raise SafetyError."""
    with pytest.raises(SafetyError):
        validate_rs232_command("bat abc")
    
    with pytest.raises(SafetyError):
        validate_rs232_command("pwr 1")  # pwr takes no arguments
        
    with pytest.raises(SafetyError):
        validate_rs232_command("info 2")  # info takes no arguments via MCP


def test_forbidden_rs232_commands():
    """Verify destructive, shutdown, reset or unknown commands are rejected."""
    forbidden = [
        "shut",
        "shutdown",
        "reboot",
        "reset",
        "format",
        "sleep",
        "wake",
        "pwroff",
        "config",
        "write",
        "debug",
        "test",
    ]
    for cmd in forbidden:
        with pytest.raises(SafetyError):
            validate_rs232_command(cmd)


def test_command_injection_prevention():
    """Verify chaining and shell metacharacters are strictly blocked."""
    injections = [
        "bat 1; reboot",
        "pwr && shutdown",
        "info | grep US",
        "stat`reboot`",
        "bat 1\nshutdown",
    ]
    for bad in injections:
        with pytest.raises(SafetyError):
            validate_rs232_command(bad)
