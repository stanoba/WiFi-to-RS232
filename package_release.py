#!/usr/bin/env python3
"""
WiFi-to-RS232 (Pylon Smart Monitor & BMS Emulator) Multi-Platform Release Packaging Script
Builds and packages firmware binaries for all 3 supported ESP32 platforms:
- Wemos D1 Mini 32 (Standard ESP32 Dual-Core)
- LOLIN S2 Mini (ESP32-S2 Single-Core + PSRAM)
- ESP32-C3 Super Mini (ESP32-C3 RISC-V)

Generated Release Binaries:
- pylon-smart-monitor-v<VERSION>-<ENV>-firmware.bin (OTA update)
- pylon-smart-monitor-v<VERSION>-<ENV>-factory-0x0.bin (Complete 0x0000 Factory Web Flash)
- pylon-bms-emulator-v<VERSION>-<ENV>-firmware.bin (OTA update)
- pylon-bms-emulator-v<VERSION>-<ENV>-factory-0x0.bin (Complete 0x0000 Factory Web Flash)
- pylon-v<VERSION>-checksums.txt (SHA-256 hashes)
- Automatically updates webflasher/firmware/ and manifest*.json with multi-chip support.
"""

import sys
import os
import glob
import json
import shutil
import hashlib
import subprocess

TARGET_ENVIRONMENTS = [
    {
        "env": "wemos_d1_mini32",
        "chip": "esp32",
        "chip_family": "ESP32",
        "bootloader_offset": "0x1000",
        "flash_mode": "dio",
        "flash_freq": "40m",
        "flash_size": "4MB"
    },
    {
        "env": "lolin_s2_mini",
        "chip": "esp32s2",
        "chip_family": "ESP32-S2",
        "bootloader_offset": "0x1000",
        "flash_mode": "dio",
        "flash_freq": "40m",
        "flash_size": "4MB"
    },
    {
        "env": "esp32_c3_super_mini",
        "chip": "esp32c3",
        "chip_family": "ESP32-C3",
        "bootloader_offset": "0x0000",
        "flash_mode": "dio",
        "flash_freq": "40m",
        "flash_size": "4MB"
    }
]

def get_firmware_version():
    config_path = os.path.join(os.path.dirname(__file__), "software", "include", "Config.h")
    if os.path.exists(config_path):
        with open(config_path, "r", encoding="utf-8") as f:
            for line in f:
                if "#define FIRMWARE_VERSION" in line:
                    parts = line.split()
                    if len(parts) >= 3:
                        return parts[2].strip('"')
    return "1.0.0"

def find_boot_app0():
    home_dir = os.path.expanduser("~")
    candidates = glob.glob(os.path.join(home_dir, ".platformio", "packages", "**", "boot_app0.bin"), recursive=True)
    if candidates:
        return candidates[0]
    return None

def sha256_file(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def update_manifest(manifest_path, version, proj_prefix):
    builds = []
    for target in TARGET_ENVIRONMENTS:
        env = target["env"]
        chip_family = target["chip_family"]
        builds.append({
            "chipFamily": chip_family,
            "parts": [
                {
                    "path": f"firmware/{proj_prefix}-{env}-factory-0x0.bin",
                    "offset": 0
                }
            ]
        })

    name_str = "Pylon Smart Monitor (WiFi-to-RS232 Telemetry Bridge)" if "monitor" in proj_prefix else "Pylon BMS Emulator (Hardware Simulator & Rack Physics)"

    mdata = {
        "name": name_str,
        "version": version,
        "new_install_prompt_erase": True,
        "builds": builds
    }

    try:
        with open(manifest_path, "w", encoding="utf-8") as mf:
            json.dump(mdata, mf, indent=2)
        print(f"Updated {os.path.basename(manifest_path)} (v{version}) with {len(builds)} chip families ({', '.join(t['chip_family'] for t in TARGET_ENVIRONMENTS)})")
    except Exception as e:
        print(f"Warning: Failed to write {manifest_path}: {e}")

def package_target(proj_dir, proj_prefix, target, version, dist_dir, webflasher_dir, boot_app0):
    env_name = target["env"]
    chip = target["chip"]
    boot_offset = target["bootloader_offset"]
    f_mode = target["flash_mode"]
    f_freq = target["flash_freq"]
    f_size = target["flash_size"]

    build_dir = os.path.join(proj_dir, ".pio", "build", env_name)
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    firmware = os.path.join(build_dir, "firmware.bin")

    if not os.path.exists(firmware):
        print(f"Skipping [{proj_prefix} / {env_name}]: {firmware} not found.")
        return []

    ota_dest = os.path.join(dist_dir, f"{proj_prefix}-v{version}-{env_name}-firmware.bin")
    factory_dest = os.path.join(dist_dir, f"{proj_prefix}-v{version}-{env_name}-factory-0x0.bin")

    # 1. Copy OTA binary
    shutil.copyfile(firmware, ota_dest)
    print(f"[{proj_prefix}/{env_name}] Created OTA binary: {os.path.basename(ota_dest)}")

    packaged_files = [ota_dest]

    # 2. Merge factory binary
    if os.path.exists(bootloader) and os.path.exists(partitions) and boot_app0 and os.path.exists(boot_app0):
        merge_cmd = [
            sys.executable, "-m", "esptool",
            "--chip", chip,
            "merge-bin",
            "-o", factory_dest,
            "--flash-mode", f_mode,
            "--flash-freq", f_freq,
            "--flash-size", f_size,
            boot_offset, bootloader,
            "0x8000", partitions,
            "0xe000", boot_app0,
            "0x10000", firmware
        ]
        print(f"[{proj_prefix}/{env_name}] Merging factory binary with esptool (--chip {chip})...")
        res = subprocess.run(merge_cmd, capture_output=True, text=True)
        if res.returncode == 0:
            print(f"[{proj_prefix}/{env_name}] Created Factory binary: {os.path.basename(factory_dest)}")
            packaged_files.append(factory_dest)

            # Copy to webflasher/firmware/
            webflasher_fw_dir = os.path.join(webflasher_dir, "firmware")
            os.makedirs(webflasher_fw_dir, exist_ok=True)
            wf_factory = os.path.join(webflasher_fw_dir, f"{proj_prefix}-{env_name}-factory-0x0.bin")
            wf_ota = os.path.join(webflasher_fw_dir, f"{proj_prefix}-{env_name}-firmware.bin")
            shutil.copyfile(factory_dest, wf_factory)
            shutil.copyfile(ota_dest, wf_ota)

            # Also create standard alias for default wemos_d1_mini32
            if env_name == "wemos_d1_mini32":
                shutil.copyfile(factory_dest, os.path.join(webflasher_fw_dir, f"{proj_prefix}-factory-0x0.bin"))
                shutil.copyfile(ota_dest, os.path.join(webflasher_fw_dir, f"{proj_prefix}-firmware.bin"))

            print(f"[{proj_prefix}/{env_name}] Updated Web Flasher binaries.")
        else:
            print(f"[{proj_prefix}/{env_name}] Warning: merge-bin failed: {res.stderr}")
    else:
        print(f"[{proj_prefix}/{env_name}] Warning: Missing bootloader, partitions, or boot_app0.bin.")

    return packaged_files

def main():
    version = sys.argv[1] if len(sys.argv) > 1 else get_firmware_version()
    if version.startswith("v"):
        version = version[1:]

    root_dir = os.path.dirname(os.path.abspath(__file__))
    dist_dir = os.path.join(root_dir, "dist")
    webflasher_dir = os.path.join(root_dir, "webflasher")

    os.makedirs(dist_dir, exist_ok=True)
    os.makedirs(webflasher_dir, exist_ok=True)

    boot_app0 = find_boot_app0()
    if not boot_app0:
        print("Warning: boot_app0.bin not found automatically in PlatformIO packages.")

    all_artifacts = []

    # 1. Package Software (Pylon Smart Monitor) for all environments
    soft_dir = os.path.join(root_dir, "software")
    if os.path.exists(soft_dir):
        for target in TARGET_ENVIRONMENTS:
            all_artifacts += package_target(soft_dir, "pylon-smart-monitor", target, version, dist_dir, webflasher_dir, boot_app0)

    # 2. Package Emulator (Pylon BMS Emulator) for all environments
    emu_dir = os.path.join(root_dir, "emulator")
    if os.path.exists(emu_dir):
        for target in TARGET_ENVIRONMENTS:
            all_artifacts += package_target(emu_dir, "pylon-bms-emulator", target, version, dist_dir, webflasher_dir, boot_app0)

    # 3. Update Web Flasher manifests with multi-platform builds
    update_manifest(os.path.join(webflasher_dir, "manifest.json"), version, "pylon-smart-monitor")
    update_manifest(os.path.join(webflasher_dir, "manifest-monitor.json"), version, "pylon-smart-monitor")
    update_manifest(os.path.join(webflasher_dir, "manifest-emulator.json"), version, "pylon-bms-emulator")

    # 4. Generate SHA256 Checksums
    checksums_dest = os.path.join(dist_dir, f"pylon-v{version}-checksums.txt")
    checksum_lines = []
    for f in all_artifacts:
        if os.path.exists(f):
            fname = os.path.basename(f)
            chash = sha256_file(f)
            checksum_lines.append(f"{chash}  {fname}")

    with open(checksums_dest, "w", encoding="utf-8") as f:
        f.write("\n".join(checksum_lines) + "\n")

    print(f"\n=======================================================")
    print(f"Created SHA256 checksums: {os.path.basename(checksums_dest)}")
    print(f"Total packaged binaries: {len(all_artifacts)}")
    print(f"All release artifacts ready in: {dist_dir}")
    print(f"Web Flasher ready in: {webflasher_dir}")
    print(f"=======================================================")

if __name__ == "__main__":
    main()
