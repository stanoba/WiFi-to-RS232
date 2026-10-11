"""
Unit tests for model detection and hardware compatibility matrix (US3000C vs US3000D vs UP5000).
"""

import pytest
from pylon_mcp.models import detect_model_profile, BatteryModel
from pylon_mcp.safety import validate_rs232_command, SafetyError


def test_model_detection():
    p_3000c = detect_model_profile("US3000C")
    assert p_3000c.model == BatteryModel.US3000C
    assert p_3000c.supports_soh_cmd is True
    assert p_3000c.supports_euro is False
    assert p_3000c.slave_supports_stat is True

    p_3000d = detect_model_profile("US3000D")
    assert p_3000d.model == BatteryModel.US3000D
    assert p_3000d.supports_euro is True
    assert p_3000d.supports_soh_cmd is False
    assert p_3000d.slave_supports_stat is False

    p_5000 = detect_model_profile("UP5000")
    assert p_5000.model == BatteryModel.UP5000
    assert p_5000.default_cell_count == 16


def test_model_d_command_restrictions():
    """Verify US3000D hardware matrix: euro supported, soh & slave stat rejected."""
    profile_d = detect_model_profile("US3000D")

    # 1. 'euro' is valid on Model D
    assert validate_rs232_command("euro", profile_d) == "euro"

    # 2. 'stat' for stack is valid on Model D
    assert validate_rs232_command("stat", profile_d) == "stat"

    # 3. 'soh' is invalid on Model D
    with pytest.raises(SafetyError) as exc_info:
        validate_rs232_command("soh", profile_d)
    assert "is not supported on model 'US3000D'" in str(exc_info.value)

    # 4. 'stat <n>' is invalid on Model D (slave queries not supported)
    with pytest.raises(SafetyError) as exc_info:
        validate_rs232_command("stat 1", profile_d)
    assert "only supports 'stat' without arguments" in str(exc_info.value)


def test_model_c_command_restrictions():
    """Verify US3000C hardware matrix: soh & slave stat supported, euro rejected."""
    profile_c = detect_model_profile("US3000C")

    # 1. 'soh' is valid on Model C
    assert validate_rs232_command("soh", profile_c) == "soh"

    # 2. 'stat 1' is valid on Model C
    assert validate_rs232_command("stat 1", profile_c) == "stat 1"

    # 3. 'euro' is invalid on Model C
    with pytest.raises(SafetyError) as exc_info:
        validate_rs232_command("euro", profile_c)
    assert "is not supported on model 'US3000C'" in str(exc_info.value)
