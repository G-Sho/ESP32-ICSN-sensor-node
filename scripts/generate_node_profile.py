import json
import os
from pathlib import Path

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")


def _project_option(name, default):
    try:
        value = env.GetProjectOption(name)
    except Exception:
        value = default
    if value is None:
        return default
    value = str(value).strip()
    return value if value else default


def _to_positive_int(value, field_name):
    if isinstance(value, bool):
        raise ValueError(f"{field_name} must be an integer")
    ivalue = int(value)
    if ivalue <= 0:
        raise ValueError(f"{field_name} must be > 0")
    return ivalue


def _load_profile(profile_path):
    with profile_path.open("r", encoding="utf-8") as f:
        profile = json.load(f)

    build = profile.get("build", {})
    runtime = profile.get("runtime", {})
    security = runtime.get("security", {})

    capacities = {
        "fib": _to_positive_int(build.get("fib_capacity"), "build.fib_capacity"),
        "pit": _to_positive_int(build.get("pit_capacity"), "build.pit_capacity"),
        "cs": _to_positive_int(build.get("cs_capacity"), "build.cs_capacity"),
        "rib": _to_positive_int(build.get("rib_capacity"), "build.rib_capacity"),
    }

    runtime_config = {
        "MAX_VIRTUAL_DEPTH": int(runtime.get("max_virtual_depth", 5)),
        "HOP_COUNT_THRESHOLD": int(runtime.get("hop_count_threshold", 10)),
        "PMK": str(security.get("pmk", "")),
        "LMK": str(security.get("lmk", "")),
        "peers": runtime.get("peers", []),
        "fib_init": runtime.get("fib_init", []),
    }

    return capacities, runtime_config


def _write_build_header(header_path, capacities):
    content = """#pragma once

#include <cstddef>

namespace BuildCapacity {{
constexpr size_t FIB_ENTRIES = {fib};
constexpr size_t PIT_ENTRIES = {pit};
constexpr size_t CS_ENTRIES = {cs};
constexpr size_t RIB_ENTRIES = {rib};
}} // namespace BuildCapacity
""".format(
        fib=capacities["fib"],
        pit=capacities["pit"],
        cs=capacities["cs"],
        rib=capacities["rib"],
    )
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(content, encoding="utf-8")


def _write_runtime_json(path, runtime_config):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(runtime_config, indent=2) + "\n", encoding="utf-8")


def main():
    project_dir = Path(env.subst("$PROJECT_DIR"))
    build_dir = Path(env.subst("$BUILD_DIR"))
    pioenv = env.subst("$PIOENV")

    env_profile = os.getenv("ICSN_NODE_PROFILE", "").strip()
    profile_name = env_profile or _project_option("custom_node_profile", "sensor")

    profile_path = project_dir / "node_profiles" / f"{profile_name}.json"
    if not profile_path.exists():
        raise FileNotFoundError(f"Node profile not found: {profile_path}")

    capacities, runtime_config = _load_profile(profile_path)

    generated_dir = build_dir / "generated"
    build_header = generated_dir / "BuildCapacity.hpp"
    runtime_json_preview = generated_dir / "config.json"

    _write_build_header(build_header, capacities)
    _write_runtime_json(runtime_json_preview, runtime_config)

    # Ensure the env-specific generated header is preferred over fallback headers.
    env.Prepend(CPPPATH=[str(generated_dir)])

    should_emit_data_config = (
        "uploadfs" in COMMAND_LINE_TARGETS
        or os.getenv("ICSN_EMIT_RUNTIME_CONFIG", "0").strip() == "1"
    )
    if should_emit_data_config:
        runtime_json = project_dir / "data" / "config.json"
        _write_runtime_json(runtime_json, runtime_config)

    print(
        "[node-profile] env={env_name} role={role} capacities=(fib={fib},pit={pit},cs={cs},rib={rib}) preview={preview}".format(
            env_name=pioenv,
            role=profile_name,
            fib=capacities["fib"],
            pit=capacities["pit"],
            cs=capacities["cs"],
            rib=capacities["rib"],
            preview=runtime_json_preview,
        )
    )


main()
