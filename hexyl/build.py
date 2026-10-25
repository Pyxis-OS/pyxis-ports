#!/usr/bin/env python3
"""Build pinned Hexyl with a supplied native Rust std prefix and SDK."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tomllib

TARGET = "x86_64-unknown-pyxis"
PROXY = "sparse+https://repo.internal/repository/cargo-proxy/"


def digest(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def main():
    source, terminal, build, stage, rust, sdk, cross, recipe = map(Path, sys.argv[1:])
    receipt_path = rust / "share/pyxis-rust/fork-installation.json"
    receipt = json.loads(receipt_path.read_text())
    if receipt["target"] != TARGET:
        raise ValueError("Rust prefix receipt has the wrong target")
    linker = rust / "share/pyxis-rust/pyxis-link.py"
    required = [rust / "bin/rustc", rust / "bin/cargo", linker,
                linker.with_name("link-flags.mk")]
    required += sorted((rust / "lib/rustlib" / TARGET / "lib").glob("*.rlib"))
    if not any(path.name.startswith("libstd-") for path in required):
        raise ValueError("Rust prefix lacks native std")
    for path in required:
        relative = str(path.relative_to(rust))
        if digest(path) != receipt["files"].get(relative):
            raise ValueError("Rust prefix differs from its receipt: " + relative)

    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(("CARGO_", "RUST"))}
    environment.update({
        "RUSTC": str(rust / "bin/rustc"),
        "CARGO_HOME": str(build / "cargo-home"),
        "CARGO_TARGET_DIR": str(build / "target"),
        "CARGO_TARGET_X86_64_UNKNOWN_PYXIS_LINKER": str(linker),
        "CARGO_TARGET_X86_64_UNKNOWN_PYXIS_RUSTFLAGS": "-Cembed-bitcode=no -Clto=off",
        "CARGO_PROFILE_RELEASE_LTO": "false",
        "CARGO_PROFILE_RELEASE_PANIC": "abort",
        "CARGO_PROFILE_RELEASE_DEBUG": "1",
        "PYXIS_RUST_SDK": str(sdk),
        "PYXIS_RUST_DEBUG_DIR": str(build / "debug-elf"),
        "CROSS_COMPILE": str(cross),
    })
    version = subprocess.check_output([str(rust / "bin/rustc"), "-Vv"],
                                      env=environment, text=True)
    if "commit-hash: " + receipt["rustc_commit"] not in version:
        raise ValueError("Installed rustc identity differs from its receipt")
    config = source / ".cargo/config.toml"
    config.parent.mkdir(exist_ok=True)
    config.write_text('[source.crates-io]\nreplace-with = "pyxis-cache"\n'
                      '[source.pyxis-cache]\nregistry = "' + PROXY + '"\n')
    cargo = str(rust / "bin/cargo")
    lock_bytes = (source / "Cargo.lock").read_bytes()
    vendor = build / "vendor"
    vendor_config = subprocess.check_output(
        [cargo, "vendor", "--locked", "--respect-source-config", str(vendor)],
        cwd=source, env=environment, text=True)
    if (source / "Cargo.lock").read_bytes() != lock_bytes:
        raise ValueError("Vendoring changed the patched lock graph")
    config.write_text(vendor_config)
    environment["CARGO_NET_OFFLINE"] = "true"
    subprocess.run([cargo, "build", "--frozen", "--release", "--bin", "hexyl",
                    "--target", TARGET, "-j16"], cwd=source, env=environment, check=True)
    if (source / "Cargo.lock").read_bytes() != lock_bytes:
        raise ValueError("Target build changed the patched lock graph")

    def install(path, destination):
        output = stage / destination
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, output)
        output.chmod(0o644)

    notices = Path("share/licenses/hexyl")
    install(build / "target" / TARGET / "release/hexyl", "bin/hexyl.pxe")
    for name in ("LICENSE-MIT", "LICENSE-APACHE"):
        install(source / name, notices / name)
    install(recipe / "PORT-NOTICE", notices / "PORT-NOTICE")
    install(source / "src/colors.rs", notices / "embedded-tables.rs")
    install(source / "Cargo.lock", notices / "Cargo.lock")
    install(recipe / "upstream-Cargo.lock", notices / "upstream-Cargo.lock")
    for path in (recipe / "patches").glob("*.patch"):
        install(path, notices / "patches" / path.name)
    for name in ("fork-toolchain.json", "fork-installation.json"):
        install(rust / "share/pyxis-rust" / name, notices / "rust" / name)
    shutil.copytree(rust / "share/doc/rust", stage / notices / "rust/notices")
    shutil.copytree(sdk / "share/licenses", stage / notices / "sdk")
    install(sdk / "manifest.txt", notices / "sdk-manifest.txt")

    packages = []
    for directory in sorted(vendor.iterdir()):
        manifest = directory / "Cargo.toml"
        if not manifest.is_file():
            continue
        package = tomllib.loads(manifest.read_text())["package"]
        checksum = json.loads((directory / ".cargo-checksum.json").read_text())["package"]
        packages.append({"name": package["name"], "version": package["version"],
                         "license": package.get("license"),
                         "license_file": package.get("license-file"), "sha256": checksum,
                         "repository": package.get("repository"),
                         "source": "vendored registry crate; full source/notices in sources.tar.xz"})
    packages.append({"name": "terminal_size", "version": "0.4.0",
                     "license": "MIT OR Apache-2.0",
                     "sha256": "4f599bd7ca042cfdf8f4512b277c02ba102247820f9d9d4a9f521f496751a6ef",
                     "patch": "0003-terminal-size-native-pyxis.patch"})
    (stage / notices / "dependencies.json").write_text(json.dumps(packages, indent=2) + "\n")
    with tarfile.open(stage / notices / "sources.tar.xz", "w:xz") as archive:
        def without_git(info):
            return None if ".git" in Path(info.name).parts else info
        archive.add(source, arcname="hexyl", filter=without_git)
        archive.add(terminal, arcname="terminal-size", filter=without_git)
        archive.add(vendor, arcname="vendor", filter=without_git)
    provenance = {
        "upstream": "https://github.com/sharkdp/hexyl", "version": "0.17.0",
        "commit": "8eb6d4771ce1ec7af65d06bd335457783b77d557",
        "archive_sha256": "72fa17397ad187eec6b295d02c7caabbb209a6e0d5706187b8a599bd5df8615e",
        "registry": PROXY, "rustc_commit": receipt["rustc_commit"],
        "rust_receipt_sha256": digest(receipt_path),
        "sdk_manifest_sha256": digest(sdk / "manifest.txt"),
        "lock_sha256": digest(source / "Cargo.lock"),
        "profile": "panic=abort; lto=false; embed-bitcode=no; debug=1; explicit --character-table=ascii",
        "link_adapter": "supplied Rust prefix; SDK native C link contract",
    }
    output = stage / "share/hexyl/source.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(provenance, indent=2) + "\n")


if __name__ == "__main__":
    main()
