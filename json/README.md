# JSON for Modern C++

Pinned nlohmann-json 3.12.0 headers, CMake target nlohmann_json::nlohmann_json
and nlohmann_json.pc. json-dev depends on libcxx-dev. All headers and the MIT
notice are installed; no SoH code or assets enter this reusable package.

The local detection patch respects libc++'s configured filesystem availability.
Pyxis has no std::filesystem; it does not synthesize path conversions. Normal
JSON parsing, serialization, exceptions, streams, CBOR and MessagePack remain
upstream implementations. No additional JSON ABI-changing defines are selected.
The manual consumer builds against the installed prefix and checks valid and
invalid input, typed fields, round trips and stream I/O in QEMU.
