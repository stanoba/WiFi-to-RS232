"""
Unit tests for PylonHttpClient using httpx MockTransport.
Ensures mock responses are parsed properly and safety errors are raised before network requests.
"""

import pytest
import httpx
from pylon_mcp.client import PylonHttpClient, PylonConnectionError, PylonApiError
from pylon_mcp.config import DeviceConfig
from pylon_mcp.safety import SafetyError
from tests.mock_data import MOCK_DATA_US3000C, MOCK_STATUS_ACTIVE


def mock_handler(request: httpx.Request):
    url = str(request.url)
    
    # Check Bearer auth if provided in request
    auth = request.headers.get("Authorization")
    if "secret_dev" in url and auth != "Bearer valid_token":
        return httpx.Response(401, json={"error": "Unauthorized"})

    if url.endswith("/api/data"):
        return httpx.Response(200, json=MOCK_DATA_US3000C)
    elif url.endswith("/api/status"):
        return httpx.Response(200, json=MOCK_STATUS_ACTIVE)
    elif url.endswith("/metrics"):
        return httpx.Response(200, text="# HELP pylontech_soc_percent\npylontech_soc_percent 86\n")
    elif "/cmd?cmd=pwr" in url:
        return httpx.Response(200, text="@\r\npwr\r\nCommand OK\r\n$$")
    elif url.endswith("/404"):
        return httpx.Response(404, text="Not Found")
        
    return httpx.Response(200, json={"status": "ok"})


@pytest.fixture
def mock_client(monkeypatch):
    client = PylonHttpClient()
    # Replace client._get_client with mock transport
    transport = httpx.MockTransport(mock_handler)
    
    async def mock_safe_request(device, method, path, params=None, json_data=None):
        from pylon_mcp.safety import validate_http_request
        validate_http_request(path, method)
        
        headers = {}
        if device.bearer_token:
            headers["Authorization"] = f"Bearer {device.bearer_token}"
            
        async with httpx.AsyncClient(transport=transport, base_url=device.clean_url()) as ac:
            resp = await ac.request(method, path, headers=headers, params=params, json=json_data)
            resp.raise_for_status()
            return resp

    monkeypatch.setattr(client, "safe_request", mock_safe_request)
    return client


@pytest.mark.asyncio
async def test_client_get_data(mock_client):
    dev = DeviceConfig(name="test", url="http://192.168.1.150")
    data = await mock_client.get_data(dev)
    assert data["stack"]["model"] == "US3000C"
    assert data["stack"]["soc"] == 86
    assert len(data["modules"]) == 2


@pytest.mark.asyncio
async def test_client_model_caching(mock_client):
    dev = DeviceConfig(name="test_c", url="http://192.168.1.150")
    profile = await mock_client.get_model_profile(dev)
    assert profile.name == "US3000C"
    assert profile.supports_soh_cmd is True
    # Cached
    profile2 = await mock_client.get_model_profile(dev)
    assert profile2 is profile


@pytest.mark.asyncio
async def test_client_blocked_endpoint_raises(mock_client):
    dev = DeviceConfig(name="test", url="http://192.168.1.150")
    with pytest.raises(SafetyError):
        await mock_client.safe_request(dev, "GET", "/reboot")
        
    with pytest.raises(SafetyError):
        await mock_client.safe_request(dev, "POST", "/update")


@pytest.mark.asyncio
async def test_client_send_command_allowed(mock_client):
    dev = DeviceConfig(name="test", url="http://192.168.1.150")
    res = await mock_client.send_command(dev, "pwr")
    assert "Command OK" in res
