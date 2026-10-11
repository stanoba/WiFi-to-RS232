"""
Asynchronous HTTP client for Pylon Smart Monitor & BMS Emulator REST API.
Enforces safety checks before any request and manages model profile caching.
"""

from typing import Any, Dict, Optional
import httpx
from .config import DeviceConfig
from .models import ModelProfile, detect_model_profile, UNKNOWN_PROFILE
from .safety import validate_http_request, validate_rs232_command, SafetyError


class PylonConnectionError(Exception):
    """Raised when the target ESP32 monitor or emulator cannot be reached."""
    pass


class PylonApiError(Exception):
    """Raised when the ESP32 returns an HTTP error status code or unexpected payload."""
    pass


class PylonHttpClient:
    def __init__(self, timeout: float = 10.0, connect_timeout: float = 3.0):
        self.timeout = httpx.Timeout(timeout, connect=connect_timeout)
        self._model_cache: Dict[str, ModelProfile] = {}

    async def _get_client(self) -> httpx.AsyncClient:
        return httpx.AsyncClient(timeout=self.timeout)

    async def safe_request(
        self,
        device: DeviceConfig,
        method: str,
        path: str,
        params: Optional[Dict[str, Any]] = None,
        json_data: Optional[Dict[str, Any]] = None,
    ) -> httpx.Response:
        """
        Executes an HTTP request after passing through central safety checks.
        """
        # 1. Enforce endpoint safety
        validate_http_request(path, method)

        # 2. Build headers
        headers = {
            "User-Agent": "Pylon-FastMCP/1.0",
            "Accept": "application/json, text/plain, */*",
        }
        if device.bearer_token:
            headers["Authorization"] = f"Bearer {device.bearer_token.strip()}"

        url = f"{device.clean_url()}{path}"

        try:
            async with httpx.AsyncClient(timeout=self.timeout) as client:
                response = await client.request(
                    method=method.upper(),
                    url=url,
                    headers=headers,
                    params=params,
                    json=json_data,
                )
                response.raise_for_status()
                return response
        except httpx.ConnectError as e:
            raise PylonConnectionError(
                f"Failed to connect to device '{device.name}' at {device.clean_url()}. "
                "Verify the device IP address, WiFi connection, and power supply."
            ) from e
        except httpx.TimeoutException as e:
            raise PylonConnectionError(
                f"Connection to device '{device.name}' timed out after {self.timeout.read}s. "
                "The ESP32 might be performing a long serial scrape or WiFi signal is weak."
            ) from e
        except httpx.HTTPStatusError as e:
            status = e.response.status_code
            if status == 401:
                raise PylonApiError(
                    f"Authentication failed on device '{device.name}' (401 Unauthorized). "
                    "Check the configured Bearer token."
                ) from e
            elif status == 404:
                raise PylonApiError(
                    f"Endpoint '{path}' not found on device '{device.name}' (404 Not Found)."
                ) from e
            raise PylonApiError(f"HTTP {status} from device '{device.name}': {e.response.text}") from e
        except Exception as e:
            raise PylonApiError(f"Unexpected error communicating with '{device.name}': {e}") from e

    async def get_model_profile(self, device: DeviceConfig) -> ModelProfile:
        """
        Retrieves and caches the detected battery model profile for the device.
        """
        if device.name in self._model_cache:
            return self._model_cache[device.name]

        try:
            data = await self.get_data(device)
            detected_str = None
            if isinstance(data, dict):
                stack = data.get("stack", {})
                detected_str = stack.get("model")
                if not detected_str:
                    modules = data.get("modules", [])
                    if modules and isinstance(modules[0], dict):
                        detected_str = modules[0].get("device")

            profile = detect_model_profile(detected_str)
            if profile.model != UNKNOWN_PROFILE.model:
                self._model_cache[device.name] = profile
            return profile
        except Exception:
            return UNKNOWN_PROFILE

    async def get_status(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/api/status")
        return resp.json()

    async def get_data(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/api/data")
        return resp.json()

    async def get_module(self, device: DeviceConfig, module_id: int) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/api/module", params={"m": module_id})
        return resp.json()

    async def get_history(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/api/history")
        return resp.json()

    async def get_peers(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/api/peers")
        return resp.json()

    async def get_metrics(self, device: DeviceConfig) -> str:
        resp = await self.safe_request(device, "GET", "/metrics")
        return resp.text

    async def get_console_log(self, device: DeviceConfig) -> str:
        resp = await self.safe_request(device, "GET", "/log/raw")
        return resp.text

    async def poll_now(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/poll_now")
        try:
            return resp.json()
        except Exception:
            return {"status": "ok", "message": "Poll triggered successfully"}

    async def toggle_pause(self, device: DeviceConfig) -> Dict[str, Any]:
        resp = await self.safe_request(device, "GET", "/toggle_pause")
        try:
            return resp.json()
        except Exception:
            return {"status": "ok", "message": "Polling pause state toggled"}

    async def send_command(self, device: DeviceConfig, raw_cmd: str) -> str:
        """
        Validates the RS232 command against the safety allowlist and model capability matrix,
        then transmits it via /cmd to the device's FIFO queue.
        """
        profile = await self.get_model_profile(device)
        validated_cmd = validate_rs232_command(raw_cmd, profile)

        resp = await self.safe_request(device, "GET", "/cmd", params={"cmd": validated_cmd})
        return resp.text
