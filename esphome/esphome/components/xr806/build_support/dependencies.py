"""Pinned external dependencies for the native ESPHome XR806 platform."""

from __future__ import annotations

import fcntl
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

CACHE_ROOT = Path.home() / ".cache/esphome-xr806/dependencies"

DEPENDENCIES = {
    "xr806_sdk": {
        "url": "git@github.com:bavci1988/xr806-sdk.git",
        "commit": "fe411b7e81abb4a34be252a4a6277010ac669042",
        "required": (
            "gcc.mk",
            ".config",
            "project/common/board/xr806_wxu/board_config.c",
            "project/demo/trv16_port/gcc/Makefile",
            "lib/xradio_v3/libwlan.a",
            "bin/xradio_v3/wlan_fw.bin",
        ),
    },
    "arduinojson": {
        "url": "https://github.com/bblanchon/ArduinoJson.git",
        "commit": "77771d3c07668e01d8f52acb03910c1110bb373f",
        "required": (
            "src/ArduinoJson.h",
            "LICENSE.txt",
        ),
    },
    # Version-pinned native ESPHome API Noise crypto dependencies.
    "noise_c": {
        "url": "https://github.com/esphome-libs/noise-c.git",
        "commit": "62c2fdca7a874ea898761147bb062ca1cde9f36d",  # v0.1.31
        "required": ("CMakeLists.txt", "include/noise/protocol.h"),
    },
    "libsodium": {
        "url": "https://github.com/esphome-libs/libsodium.git",
        "commit": "95c5e60d991b3f735619da8a307951c0483ef567",  # 1.10021.12
        "submodules": True,
        "required": (
            "CMakeLists.txt",
            "patches/apply.sh",
            "libsodium/src/libsodium/include/sodium.h",
        ),
    },
}


def _git(*args: str, cwd: Path | None = None) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def _verify(path: Path, name: str, expected_commit: str | None = None) -> None:
    """Verify the cached Git revision and required files."""
    info = DEPENDENCIES[name]

    if not (path / ".git").exists():
        raise RuntimeError(f"Invalid dependency cache (missing Git metadata): {path}")

    revision = _git("rev-parse", "HEAD", cwd=path)

    if revision != (expected_commit or info["commit"]):
        raise RuntimeError(f"Unexpected {name} revision: {revision}")

    changes = _git(
        "status",
        "--porcelain",
        "--untracked-files=all",
        cwd=path,
    )

    if changes:
        raise RuntimeError(f"Cached {name} sources have local modifications: {path}")

    for relative in info["required"]:
        if not (path / relative).is_file():
            raise RuntimeError(f"Missing {name} dependency file: {relative}")


def _verify_files(path: Path, name: str) -> None:
    """Verify only the required files exist.

    Used for an SDK that is versioned by a parent repository (no standalone
    .git), so the Git-revision and clean-worktree checks do not apply.
    """
    info = DEPENDENCIES[name]
    for relative in info["required"]:
        if not (path / relative).is_file():
            raise RuntimeError(f"Missing {name} dependency file: {relative}")


def _colocated_sdk() -> Path | None:
    """Return the SDK bundled alongside this ESPHome fork, if present.

    In the consolidated Tuya2ESP-OTA_WXU layout the SDK lives at ``<repo>/sdk``,
    a sibling of the ESPHome fork at ``<repo>/esphome``. It is versioned as part
    of that repository (no standalone ``.git``), so it is used directly without
    a clone or Git-revision check. When the ESPHome fork is used standalone this
    path does not exist and the external clone is used instead.
    """
    sdk = Path(__file__).resolve().parents[5] / "sdk"
    return sdk if sdk.is_dir() else None


def sdk_revision_token(sdk_path: Path) -> str:
    """Return a cache key for the SDK sources at ``sdk_path``.

    The caller copies the SDK into an isolated build workspace and reuses that
    copy only while this token is unchanged. An external clone is immutable, so
    its Git HEAD is used. A co-located SDK (versioned by the parent repository,
    no standalone ``.git``) can be edited in place, so a signature over the file
    tree is used instead and the workspace refreshes whenever a file changes.
    """
    if (sdk_path / ".git").is_dir():
        return _git("rev-parse", "HEAD", cwd=sdk_path)

    digest = hashlib.sha1()
    for path in sorted(sdk_path.rglob("*")):
        if path.is_file():
            stat = path.stat()
            relative = path.relative_to(sdk_path).as_posix()
            digest.update(f"{relative}\0{stat.st_size}\0{int(stat.st_mtime)}\0".encode())
    return "colocated-" + digest.hexdigest()[:16]


def ensure_dependency(name: str) -> Path:
    """Ensure a pinned dependency is present in the shared cache."""
    if name not in DEPENDENCIES:
        raise ValueError(f"Unknown XR806 dependency: {name}")

    info = DEPENDENCIES[name]

    # HA Device Builder: allow a read-only, pre-provisioned SDK checkout.
    # Keep revision, required-file and clean-worktree verification enabled.
    if name == "xr806_sdk":
        override = os.environ.get("XR806_SDK_SOURCE_DIR", "").strip()
        if override:
            source = Path(override).expanduser().resolve(strict=True)
            # Only explicitly pre-provisioned SDKs may use a non-default SHA.
            # CI supplies its checked-out GitHub event SHA, preserving exact
            # revision and clean-worktree verification for each build.
            expected_commit = os.environ.get("XR806_SDK_EXPECTED_COMMIT", "").strip()
            if expected_commit and (
                len(expected_commit) != 40
                or any(c not in "0123456789abcdef" for c in expected_commit)
            ):
                raise RuntimeError(
                    "Invalid XR806_SDK_EXPECTED_COMMIT: expected full lowercase Git SHA"
                )
            _verify(source, name, expected_commit or None)
            print(f"[XR806] Verified external SDK checkout: {source}", flush=True)
            return source

        # Consolidated layout: use the SDK bundled next to this fork directly.
        # It is versioned by the parent repository, so there is no clone and no
        # Git-revision check; only the required files are verified.
        colocated = _colocated_sdk()
        if colocated is not None:
            _verify_files(colocated, name)
            print(f"[XR806] Using in-repo SDK: {colocated}", flush=True)
            return colocated

    parent = CACHE_ROOT / name
    destination = parent / info["commit"]

    parent.mkdir(parents=True, exist_ok=True)

    # A permanent lock file coordinates independent build processes.
    # Do not unlink this file: waiters may still hold its inode.
    lock_path = parent / ".download.lock"

    with lock_path.open("a+b") as lock:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)

        try:
            # Check again after obtaining the lock: another process
            # might have completed the download while we waited.
            if destination.exists() or destination.is_symlink():
                try:
                    _verify(destination, name)
                except (
                    OSError,
                    RuntimeError,
                    subprocess.CalledProcessError,
                ) as err:
                    raise RuntimeError(
                        f"Invalid XR806 dependency cache: {destination}. "
                        "Move the damaged directory aside before retrying."
                    ) from err

                print(f"[XR806] Cache hit: {name}", flush=True)
                return destination

            temporary = Path(
                tempfile.mkdtemp(
                    prefix=f".{name}-",
                    dir=parent,
                )
            )

            try:
                print(f"[XR806] Downloading: {name}", flush=True)

                subprocess.run(
                    [
                        "git",
                        "clone",
                        "--quiet",
                        "--no-checkout",
                        info["url"],
                        str(temporary),
                    ],
                    check=True,
                )

                _git(
                    "checkout",
                    "--detach",
                    "--quiet",
                    info["commit"],
                    cwd=temporary,
                )

                if info.get("submodules"):
                    subprocess.run(
                        ["git", "submodule", "update", "--init", "--recursive"],
                        cwd=temporary,
                        check=True,
                    )

                _verify(temporary, name)

                # Atomic publication of a fully verified checkout.
                temporary.rename(destination)

            finally:
                if temporary.exists():
                    shutil.rmtree(temporary)

            print(f"[XR806] Cached: {name}", flush=True)
            return destination

        finally:
            fcntl.flock(lock.fileno(), fcntl.LOCK_UN)


def resolve_dependencies() -> tuple[Path, Path]:
    """Return the verified SDK and ArduinoJson source directories."""
    sdk = ensure_dependency("xr806_sdk")
    arduinojson = ensure_dependency("arduinojson")

    return sdk, arduinojson / "src"
