return function(ctx)
  local go = assert(ctx.go, "Caddy needs the pinned native Go cross-toolchain")
  local function go_command(proxy, arguments)
    local command = { "env", "GOROOT=" .. go, "GOTOOLCHAIN=local", "GOOS=pyxis",
      "GOARCH=amd64", "GOAMD64=v1", "CGO_ENABLED=0", "GOSUMDB=off",
      "GOPROXY=" .. proxy, "GOMODCACHE=" .. ctx.build .. "/modcache",
      "GOCACHE=" .. ctx.build .. "/gocache", go .. "/bin/go", "-C", ctx.source }
    for _, argument in ipairs(arguments) do
      table.insert(command, argument)
    end
    ctx.run(command)
  end
  local function source_bytes(name)
    local file = assert(io.open(ctx.source .. "/" .. name, "rb"))
    local bytes = assert(file:read("*a"))
    assert(file:close())
    return bytes
  end
  local original = { ["go.mod"] = source_bytes("go.mod"),
    ["go.sum"] = source_bytes("go.sum") }
  -- go.sum authenticates dependencies before they are copied into this private
  -- vendor tree. Vendoring must not change the pinned module graph or checksums.
  go_command("https://repo.internal/repository/go-proxy/", { "mod", "vendor" })
  for name, bytes in pairs(original) do
    assert(source_bytes(name) == bytes, "vendoring changed pinned " .. name)
  end
  ctx.run({ "git", "-C", ctx.source, "apply", "--whitespace=error-all", "--",
    ctx.recipe .. "/" .. ctx.metadata.vendor_patch })
  local tags = "nobbolt,nobadger,nomysql,nopgx,nosshagentkms"
  go_command("off", { "build", "-mod=vendor", "-p=8", "-trimpath",
    "-tags=" .. tags, "-ldflags=-s -w -X github.com/caddyserver/caddy/v2.CustomVersion=v" ..
      ctx.metadata.version .. "-pyxis", "-o", ctx.build .. "/caddy.pxe", "./cmd/caddy" })
  local outputs = ctx.metadata.outputs
  local function install(source, output)
    ctx.run({ "install", "-D", "-m", "644", source, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/caddy.pxe", outputs.executable)
  install(ctx.recipe .. "/manifest.json", outputs.manifest)
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.port_notice)
  install(go .. "/share/pyxis-go/LICENSE", outputs.go_license)
  install(go .. "/share/pyxis-go/revision", outputs.go_revision)
  install(go .. "/PATENTS", outputs.go_patents)
  install(ctx.recipe .. "/patches/0001-pyxis-signal-selection.patch", outputs.signal_patch)
  install(ctx.recipe .. "/" .. ctx.metadata.vendor_patch, outputs.dependency_patch)
  ctx.run({ "python3", ctx.recipe .. "/stage-notices.py", ctx.source, ctx.stage,
    go, ctx.build .. "/gocache", tags, ctx.metadata.version })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/caddy" })
  local source = assert(io.open(ctx.stage .. "/" .. outputs.source, "w"))
  assert(source:write("source=", ctx.metadata.source.url, "\ncommit=",
    ctx.metadata.source.commit, "\nversion=", ctx.metadata.version,
    "\narchive_sha256=", ctx.metadata.source.archive.sha256,
    "\nmodule_proxy=https://repo.internal/repository/go-proxy/\n",
    "dependency_pins=upstream go.sum; compiled module sums in app/metadata/caddy/modules.txt\n",
    "tags=", tags, "\nlink=-s -w; Go pclntab retained\n",
    "patches=0001-pyxis-signal-selection.patch,0002-pyxis-dependencies.patch\n",
    "profile=foreground HTTP and supplied-certificate TLS; admin, ACME and HTTP3 disabled in examples\n"))
  assert(source:close())
end
