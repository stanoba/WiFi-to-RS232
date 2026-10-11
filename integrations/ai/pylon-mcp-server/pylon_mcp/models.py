"""
Pylontech Battery Model Definitions and Hardware Capability Matrix.
Mirrors the firmware hardware registry in software/include/BatteryData.h.
"""

from enum import Enum
from typing import Optional, List
from pydantic import BaseModel


class BatteryModel(str, Enum):
    UNKNOWN = "UNKNOWN"
    US2000 = "US2000"
    US2000C = "US2000C"
    US3000 = "US3000"
    US3000C = "US3000C"
    US3000D = "US3000D"
    UP5000 = "UP5000"
    FORCE_L1 = "FORCE_L1"
    FORCE_L2 = "FORCE_L2"


class ModelProfile(BaseModel):
    model: BatteryModel
    name: str
    match_substrings: List[str]
    supports_euro: bool            # Supports 'euro' command (Model D)
    supports_soh_cmd: bool         # Supports 'soh' command (Model C, US2000C, UP5000)
    slave_supports_stat: bool      # Slave units accept 'stat <n>' (Model C=True, Model D=False)
    slave_supports_info: bool      # Slave units accept 'info <n>' (Model C=True, Model D=False)
    slave_supports_soh: bool       # Slave units accept 'soh <n>' (Model C=True, Model D=False)
    has_mosfet_temp: bool          # Native MOSFET temperature sensor in hardware
    default_cell_count: int        # Usually 15, UP5000 is 16


MODEL_REGISTRY: List[ModelProfile] = [
    ModelProfile(
        model=BatteryModel.US3000C,
        name="US3000C",
        match_substrings=["US3000C", "3000C"],
        supports_euro=False,
        supports_soh_cmd=True,
        slave_supports_stat=True,
        slave_supports_info=True,
        slave_supports_soh=True,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.US3000D,
        name="US3000D",
        match_substrings=["US3000D", "3000D"],
        supports_euro=True,
        supports_soh_cmd=False,
        slave_supports_stat=False,
        slave_supports_info=False,
        slave_supports_soh=False,
        has_mosfet_temp=True,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.US2000C,
        name="US2000C",
        match_substrings=["US2000C", "2000C"],
        supports_euro=False,
        supports_soh_cmd=True,
        slave_supports_stat=True,
        slave_supports_info=True,
        slave_supports_soh=True,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.US2000,
        name="US2000",
        match_substrings=["US2000", "2000B", "2000"],
        supports_euro=False,
        supports_soh_cmd=False,
        slave_supports_stat=True,
        slave_supports_info=True,
        slave_supports_soh=False,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.US3000,
        name="US3000",
        match_substrings=["US3000", "3000B"],
        supports_euro=False,
        supports_soh_cmd=False,
        slave_supports_stat=True,
        slave_supports_info=True,
        slave_supports_soh=False,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.UP5000,
        name="UP5000",
        match_substrings=["UP5000", "5000"],
        supports_euro=False,
        supports_soh_cmd=True,
        slave_supports_stat=True,
        slave_supports_info=True,
        slave_supports_soh=True,
        has_mosfet_temp=False,
        default_cell_count=16
    ),
    ModelProfile(
        model=BatteryModel.FORCE_L1,
        name="Force-L1",
        match_substrings=["Force-L1", "FORCEL1"],
        supports_euro=False,
        supports_soh_cmd=True,
        slave_supports_stat=False,
        slave_supports_info=False,
        slave_supports_soh=False,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
    ModelProfile(
        model=BatteryModel.FORCE_L2,
        name="Force-L2",
        match_substrings=["Force-L2", "FORCEL2"],
        supports_euro=False,
        supports_soh_cmd=True,
        slave_supports_stat=False,
        slave_supports_info=False,
        slave_supports_soh=False,
        has_mosfet_temp=False,
        default_cell_count=15
    ),
]

UNKNOWN_PROFILE = ModelProfile(
    model=BatteryModel.UNKNOWN,
    name="Unknown",
    match_substrings=[],
    supports_euro=False,
    supports_soh_cmd=False,
    slave_supports_stat=False,
    slave_supports_info=False,
    slave_supports_soh=False,
    has_mosfet_temp=False,
    default_cell_count=15
)


def detect_model_profile(device_name_or_info: Optional[str]) -> ModelProfile:
    """
    Detects the model profile based on device name or 'info' output substring.
    """
    if not device_name_or_info:
        return UNKNOWN_PROFILE
    
    clean_str = device_name_or_info.strip().upper()
    for profile in MODEL_REGISTRY:
        for sub in profile.match_substrings:
            if sub.upper() in clean_str:
                return profile
                
    return UNKNOWN_PROFILE
