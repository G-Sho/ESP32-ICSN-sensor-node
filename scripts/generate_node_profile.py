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
        "fib_next_hops_per_entry": _to_positive_int(
            build.get("fib_next_hops_per_entry", 4),
            "build.fib_next_hops_per_entry",
        ),
        "pit_requesters_per_entry": _to_positive_int(
            build.get("pit_requesters_per_entry", 4),
            "build.pit_requesters_per_entry",
        ),
        "node_id_max_chars": _to_positive_int(
            build.get("node_id_max_chars", 17),
            "build.node_id_max_chars",
        ),
        "rib_next_hops_per_node": _to_positive_int(
            build.get("rib_next_hops_per_node", 4),
            "build.rib_next_hops_per_node",
        ),
    }

    memory_policy = {
        "cs_payload_memory": str(build.get("cs_payload_memory", "auto")).strip().lower(),
    }

    runtime_config = {
        "MAX_VIRTUAL_DEPTH": int(runtime.get("max_virtual_depth", 5)),
        "HOP_COUNT_THRESHOLD": int(runtime.get("hop_count_threshold", 10)),
        "PMK": str(security.get("pmk", "")),
        "LMK": str(security.get("lmk", "")),
        "peers": runtime.get("peers", []),
        "fib_init": runtime.get("fib_init", []),
    }

    return capacities, runtime_config, memory_policy


def _board_has_psram():
    board_cfg = env.BoardConfig()
    extra_flags = board_cfg.get("build.extra_flags", [])
    if isinstance(extra_flags, str):
        extra_flags = [extra_flags]

    for flag in extra_flags:
        if "BOARD_HAS_PSRAM" in str(flag):
            return True

    return False


def _resolve_memory_policy(memory_policy):
    raw = memory_policy.get("cs_payload_memory", "auto")
    valid = {"auto", "heap", "psram"}
    if raw not in valid:
        raise ValueError(
            "build.cs_payload_memory must be one of: auto, heap, psram"
        )

    if raw == "heap":
        return {"cs_payload_psram_preferred": False, "effective_mode": "heap"}

    if raw == "psram":
        return {"cs_payload_psram_preferred": True, "effective_mode": "psram"}

    use_psram = _board_has_psram()
    return {
        "cs_payload_psram_preferred": use_psram,
        "effective_mode": "psram" if use_psram else "heap",
    }


def _write_build_header(header_path, capacities, resolved_policy):
    content = """#pragma once

#include <cstddef>

namespace BuildCapacity {{
constexpr size_t FIB_ENTRIES = {fib};
constexpr size_t PIT_ENTRIES = {pit};
constexpr size_t CS_ENTRIES = {cs};
constexpr size_t RIB_ENTRIES = {rib};
constexpr size_t FIB_NEXT_HOPS_PER_ENTRY = {fib_next_hops_per_entry};
constexpr size_t PIT_REQUESTERS_PER_ENTRY = {pit_requesters_per_entry};
constexpr size_t NODE_ID_MAX_CHARS = {node_id_max_chars};
constexpr size_t RIB_NEXT_HOPS_PER_NODE = {rib_next_hops_per_node};
}} // namespace BuildCapacity

namespace BuildMemoryPolicy {{
constexpr bool CS_PAYLOAD_PSRAM_PREFERRED = {cs_payload_psram_preferred};
}} // namespace BuildMemoryPolicy
""".format(
        fib=capacities["fib"],
        pit=capacities["pit"],
        cs=capacities["cs"],
        rib=capacities["rib"],
        fib_next_hops_per_entry=capacities["fib_next_hops_per_entry"],
        pit_requesters_per_entry=capacities["pit_requesters_per_entry"],
        node_id_max_chars=capacities["node_id_max_chars"],
        rib_next_hops_per_node=capacities["rib_next_hops_per_node"],
        cs_payload_psram_preferred=(
            "true" if resolved_policy["cs_payload_psram_preferred"] else "false"
        ),
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

    capacities, runtime_config, memory_policy = _load_profile(profile_path)
    resolved_policy = _resolve_memory_policy(memory_policy)

    generated_dir = build_dir / "generated"
    build_header = generated_dir / "BuildCapacity.hpp"
    runtime_json_preview = generated_dir / "config.json"

    _write_build_header(build_header, capacities, resolved_policy)
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
        "[node-profile] env={env_name} role={role} capacities=(fib={fib},pit={pit},cs={cs},rib={rib}) cs_payload={cs_payload_mode} preview={preview}".format(
            env_name=pioenv,
            role=profile_name,
            fib=capacities["fib"],
            pit=capacities["pit"],
            cs=capacities["cs"],
            rib=capacities["rib"],
            cs_payload_mode=resolved_policy["effective_mode"],
            preview=runtime_json_preview,
        )
    )


main()
