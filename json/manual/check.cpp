#include <nlohmann/json.hpp>
#include <cstdio>
#include <sstream>

int main()
{
  using nlohmann::json;
  auto value = json::parse(R"({"answer":42,"items":[true,"native"]})");
  if (value.at("answer").get<int>() != 42 || !value.at("items")[0].get<bool>()) return 1;
  std::stringstream stream;
  stream << value;
  json restored;
  stream >> restored;
  if (restored != value || json::from_cbor(json::to_cbor(value)) != value) return 2;
  bool rejected = false;
  try { (void)json::parse("{"); } catch (const json::parse_error &) { rejected = true; }
  if (!rejected) return 3;
  std::puts("json: parse/stream/CBOR/error checks passed");
  return 0;
}
