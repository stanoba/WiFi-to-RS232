"""
Test runner using Python standard library unittest to verify safety and models.
"""

import sys
import unittest
import asyncio
from pathlib import Path

# Add package directory to sys.path
pkg_dir = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(pkg_dir))

from pylon_mcp.models import detect_model_profile, BatteryModel, UNKNOWN_PROFILE
from pylon_mcp.safety import (
    validate_http_request,
    validate_rs232_command,
    SafetyError,
    BLOCKED_HTTP_PATHS,
    ALLOWED_RS232_COMMANDS
)
from pylon_mcp.config import load_config, DeviceConfig
from pylon_mcp.client import PylonHttpClient
import httpx
from tests.mock_data import MOCK_DATA_US3000C, MOCK_STATUS_ACTIVE


class TestPylonSafety(unittest.TestCase):
    def test_blocked_http_endpoints(self):
        for path in BLOCKED_HTTP_PATHS:
            with self.assertRaises(SafetyError):
                validate_http_request(path, method="GET")
            with self.assertRaises(SafetyError):
                validate_http_request(path, method="POST")

    def test_subpath_blocked(self):
        with self.assertRaises(SafetyError):
            validate_http_request("/update/firmware", method="POST")
        with self.assertRaises(SafetyError):
            validate_http_request("/reboot?now=1", method="GET")

    def test_read_only_protection(self):
        validate_http_request("/settings", method="GET")
        with self.assertRaises(SafetyError):
            validate_http_request("/settings", method="POST")

    def test_allowed_http_endpoints(self):
        allowed = ["/api/data", "/api/module?m=0", "/api/status", "/api/history", "/api/peers", "/metrics", "/log/raw", "/poll_now", "/toggle_pause"]
        for path in allowed:
            validate_http_request(path, method="GET")

    def test_allowed_rs232_commands(self):
        for cmd in ALLOWED_RS232_COMMANDS:
            self.assertEqual(validate_rs232_command(cmd), cmd)

    def test_commands_with_numeric_arg(self):
        self.assertEqual(validate_rs232_command("bat 1"), "bat 1")
        self.assertEqual(validate_rs232_command("pwr 1"), "pwr 1")
        self.assertEqual(validate_rs232_command("stat 2"), "stat 2")
        self.assertEqual(validate_rs232_command("info 1"), "info 1")

    def test_disallowed_command_args(self):
        with self.assertRaises(SafetyError):
            validate_rs232_command("bat abc")
        with self.assertRaises(SafetyError):
            validate_rs232_command("euro 1")

    def test_forbidden_rs232_commands(self):
        forbidden = ["shutdown", "reboot", "format", "reset", "sleep", "shut", "pwroff"]
        for cmd in forbidden:
            with self.assertRaises(SafetyError):
                validate_rs232_command(cmd)

    def test_command_injection(self):
        injections = ["bat 1; reboot", "pwr && shutdown", "info | grep US", "bat 1\nshutdown"]
        for bad in injections:
            with self.assertRaises(SafetyError):
                validate_rs232_command(bad)


class TestPylonModels(unittest.TestCase):
    def test_model_detection(self):
        p_c = detect_model_profile("US3000C")
        self.assertEqual(p_c.model, BatteryModel.US3000C)
        self.assertTrue(p_c.supports_soh_cmd)
        self.assertFalse(p_c.supports_euro)
        self.assertTrue(p_c.slave_supports_stat)

        p_d = detect_model_profile("US3000D")
        self.assertEqual(p_d.model, BatteryModel.US3000D)
        self.assertTrue(p_d.supports_euro)
        self.assertFalse(p_d.supports_soh_cmd)
        self.assertFalse(p_d.slave_supports_stat)

    def test_model_d_restrictions(self):
        p_d = detect_model_profile("US3000D")
        self.assertEqual(validate_rs232_command("euro", p_d), "euro")
        self.assertEqual(validate_rs232_command("stat", p_d), "stat")
        
        with self.assertRaises(SafetyError) as ctx:
            validate_rs232_command("soh", p_d)
        self.assertIn("is not supported on model 'US3000D'", str(ctx.exception))

        with self.assertRaises(SafetyError) as ctx:
            validate_rs232_command("stat 1", p_d)
        self.assertIn("only supports 'stat' without arguments", str(ctx.exception))

    def test_model_c_restrictions(self):
        p_c = detect_model_profile("US3000C")
        self.assertEqual(validate_rs232_command("soh", p_c), "soh")
        self.assertEqual(validate_rs232_command("stat 1", p_c), "stat 1")

        with self.assertRaises(SafetyError) as ctx:
            validate_rs232_command("euro", p_c)
        self.assertIn("is not supported on model 'US3000C'", str(ctx.exception))


class TestPylonHttpClientMock(unittest.TestCase):
    def test_client_with_mock_transport(self):
        def handler(request: httpx.Request):
            url = str(request.url)
            if url.endswith("/api/data"):
                return httpx.Response(200, json=MOCK_DATA_US3000C)
            elif url.endswith("/api/status"):
                return httpx.Response(200, json=MOCK_STATUS_ACTIVE)
            elif "/cmd?cmd=pwr" in url:
                return httpx.Response(200, text="@\r\npwr\r\nCommand OK\r\n$$")
            return httpx.Response(200, json={"status": "ok"})

        transport = httpx.MockTransport(handler)

        async def run_async():
            client = PylonHttpClient()
            dev = DeviceConfig(name="test", url="http://192.168.1.150")
            
            # Monkeypatch safe_request with local mock transport
            async def mock_safe_req(device, method, path, params=None, json_data=None):
                validate_http_request(path, method)
                async with httpx.AsyncClient(transport=transport, base_url=device.clean_url()) as ac:
                    resp = await ac.request(method, path, params=params, json=json_data)
                    resp.raise_for_status()
                    return resp
            
            client.safe_request = mock_safe_req
            
            data = await client.get_data(dev)
            self.assertEqual(data["stack"]["model"], "US3000C")
            self.assertEqual(data["stack"]["soc"], 86)
            
            # Check model profile auto detection & cache
            profile = await client.get_model_profile(dev)
            self.assertEqual(profile.name, "US3000C")
            
            # Test send command
            cmd_out = await client.send_command(dev, "pwr")
            self.assertIn("Command OK", cmd_out)
            
            # Test safety on client
            with self.assertRaises(SafetyError):
                await client.safe_request(dev, "GET", "/reboot")

        asyncio.run(run_async())


if __name__ == "__main__":
    unittest.main(verbosity=2)
