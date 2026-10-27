# JSON for Modern C++

Pinned nlohmann-json 3.12.0 headers, CMake target nlohmann_json::nlohmann_json
and nlohmann_json.pc. json-dev depends on libcxx-dev. All headers and the MIT
notice are installed; no SoH code or assets enter this reusable package.

The headers are unpatched and use the SDK's native std::filesystem support.
JSON parsing, serialization, exceptions, streams, CBOR and MessagePack remain
upstream implementations. No additional JSON ABI-changing defines are selected.
The manual consumer builds against the installed prefix and checks valid and
invalid input, typed fields, round trips and stream I/O in QEMU.
