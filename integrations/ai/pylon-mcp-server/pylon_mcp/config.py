"""
Multi-device configuration management for Pylontech FastMCP Server.
Supports YAML configuration with multiple monitors and emulators.
"""

import os
from pathlib import Path
from typing import Dict, List, Optional
from pydantic import BaseModel, Field
import yaml


class DeviceConfig(BaseModel):
    name: str
    type: str = "monitor"             # "monitor" or "emulator"
    url: str                          # e.g. "http://192.168.1.150"
    bearer_token: Optional[str] = ""  # Bearer token for Authorization header
    description: Optional[str] = ""

    def clean_url(self) -> str:
        return self.url.rstrip("/")


class DefaultsConfig(BaseModel):
    default_device: Optional[str] = None
    timeout_seconds: float = 10.0
    connect_timeout_seconds: float = 3.0


class PylonConfig(BaseModel):
    devices: Dict[str, DeviceConfig] = Field(default_factory=dict)
    defaults: DefaultsConfig = Field(default_factory=DefaultsConfig)

    def get_device(self, name: Optional[str] = None) -> DeviceConfig:
        """
        Resolves device config by name, falling back to default_device or the first registered device.
        """
        if not self.devices:
            # Fallback to local emulator default if no devices configured
            return DeviceConfig(
                name="default",
                type="monitor",
                url="http://192.168.1.150",
                bearer_token="",
                description="Default Fallback Monitor"
            )

        target_name = name
        if not target_name or target_name.lower() in ("default", ""):
            target_name = self.defaults.default_device

        if target_name and target_name in self.devices:
            return self.devices[target_name]

        # If not found by name, pick the first configured device
        first_key = next(iter(self.devices))
        return self.devices[first_key]

    def list_devices(self) -> List[DeviceConfig]:
        return list(self.devices.values())


def load_config(config_path: Optional[str] = None) -> PylonConfig:
    """
    Loads YAML configuration from path, env var, or default locations.
    """
    candidates = []
    
    if config_path:
        candidates.append(Path(config_path))
    
    if "PYLON_MCP_CONFIG" in os.environ:
        candidates.append(Path(os.environ["PYLON_MCP_CONFIG"]))

    # Current working directory
    candidates.append(Path("config.yaml"))
    candidates.append(Path("config.yml"))

    # Directory of the server package
    server_dir = Path(__file__).resolve().parent.parent
    candidates.append(server_dir / "config.yaml")
    candidates.append(server_dir / "config.example.yaml")

    for candidate in candidates:
        if candidate.is_file():
            try:
                with open(candidate, "r", encoding="utf-8") as f:
                    data = yaml.safe_load(f) or {}
                
                devices_dict: Dict[str, DeviceConfig] = {}
                raw_devs = data.get("devices", [])
                
                if isinstance(raw_devs, list):
                    for d in raw_devs:
                        cfg = DeviceConfig(**d)
                        devices_dict[cfg.name] = cfg
                elif isinstance(raw_devs, dict):
                    for k, v in raw_devs.items():
                        v["name"] = k
                        cfg = DeviceConfig(**v)
                        devices_dict[cfg.name] = cfg

                raw_defaults = data.get("defaults", {})
                defaults_cfg = DefaultsConfig(**raw_defaults)
                
                return PylonConfig(devices=devices_dict, defaults=defaults_cfg)
            except Exception as e:
                print(f"Warning: Failed to load config from {candidate}: {e}")

    # Return empty config with defaults if no file found
    return PylonConfig()
