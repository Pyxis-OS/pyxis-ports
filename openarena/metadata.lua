local packs = {
  { name = "pak0.pk3", sha256 = "5bf50ed038fe8098f32e2e9bec7184a763ae2030c8feccef1e8448f6bd7d027f" },
  { name = "pak1-maps.pk3", sha256 = "f8723e7629ac33f94ba025dc11de820c9ec8671c9d4d92ba2b6ba63c7fe74237" },
  { name = "pak2-players-mature.pk3", sha256 = "b54e8e6cefdad8b7d3ceb4d10669e6393f205e007116538bd45c19bfdcb80bd8" },
  { name = "pak2-players.pk3", sha256 = "5021d10cc90ff29158c1750120b8fe0f036c1f20f030a86803313367ca6e3757" },
  { name = "pak4-textures.pk3", sha256 = "aa0a8f59fbb0847e9160f05f2e357bcb1acb66b2b95eb5b68f6302ce9ed78e3c" },
  { name = "pak5-TA.pk3", sha256 = "17f6657d9e59e1afc2333474483786e9b2f9f530855d3bb36428780781fe54aa" },
  { name = "pak6-misc.pk3", sha256 = "71ea57f8bfba366086ca8ee21d7d5e82d90ab0c16fbe66dc5a8f7636f8762687" },
  { name = "pak6-patch085.pk3", sha256 = "d310556375fab08af683e7ea17db319e08d2d11d0ef72972a48e62021aabf239" },
  { name = "pak6-patch088.pk3", sha256 = "c1e1c98f088eb9cb6236f5ca5c0303fa6c9f378e6a235491c3cff3b7d52dc5ed" },
}

local notices = { "COPYING", "CREDITS", "README", "readme_085.txt", "readme_088.txt" }
local outputs = {
  notice = "share/licenses/openarena/PORT-NOTICE",
  provenance = "share/openarena/source.txt",
}
for _, pack in ipairs(packs) do
  outputs[pack.name] = "share/openarena/baseoa/" .. pack.name
end
for _, name in ipairs(notices) do
  outputs[name] = "share/licenses/openarena/" .. name
end

return {
  source = {
    url = "https://openarena.ws/",
    archive = {
      url = "https://download.tuxfamily.org/openarena/rel/088/openarena-0.8.8.zip",
      mirror = "https://repo.internal/repository/raw-tuxfamily/openarena/rel/088/openarena-0.8.8.zip",
      sha256 = "5a8faf7f5b51f351b0a1618c06b6b98a5f1a6758f1d39818de2c87df2a0bac4a",
      format = "zip",
      directory = "openarena-0.8.8",
    },
  },
  version = "0.8.8",
  license = "GPL-2.0-only AND GPL-2.0-or-later AND BSD-3-Clause; see PORT-NOTICE",
  dependencies = {
    host = { "curl", "sha256sum", "python3", "install" },
    pyxis = {},
  },
  patches = {},
  packs = packs,
  notices = notices,
  outputs = outputs,
}
