return {
  source = {
    url = "https://github.com/caddyserver/caddy",
    commit = "72dd0fb067f6d7826c7f79907670ba4a713bfe37",
    archive = {
      url = "https://proxy.golang.org/github.com/caddyserver/caddy/v2/@v/v2.11.7.zip",
      mirror = "https://repo.internal/repository/go-proxy/github.com/caddyserver/caddy/v2/@v/v2.11.7.zip",
      sha256 = "c5b3a3304999625d751b2d2284cb037b9530bf563d1a62b720a7aa6765b0a5d9",
      format = "zip",
      directory = "github.com/caddyserver/caddy/v2@v2.11.7",
    },
  },
  version = "2.11.7",
  license = "Apache-2.0; linked dependency notices are staged separately",
  dependencies = {
    host = { "curl", "sha256sum", "python3", "git", "install" },
    pyxis = {},
  },
  patches = { "patches/0001-pyxis-signal-selection.patch" },
  -- Applied after go.sum-checked vendoring; vendor sources never escape this work.
  vendor_patch = "patches/0002-pyxis-dependencies.patch",
  outputs = {
    executable = "bin/caddy.pxb/app/bin/caddy.pxe",
    manifest = "bin/caddy.pxb/manifest.json",
    license = "bin/caddy.pxb/app/metadata/caddy/LICENSE",
    port_notice = "bin/caddy.pxb/app/metadata/caddy/PORT-NOTICE",
    dependency_notices = "bin/caddy.pxb/app/metadata/dependencies/NOTICES.txt",
    go_license = "bin/caddy.pxb/app/metadata/go/LICENSE",
    go_patents = "bin/caddy.pxb/app/metadata/go/PATENTS",
    signal_patch = "bin/caddy.pxb/app/metadata/caddy/0001-pyxis-signal-selection.patch",
    dependency_patch = "bin/caddy.pxb/app/metadata/caddy/0002-pyxis-dependencies.patch",
    go_revision = "bin/caddy.pxb/app/metadata/go/revision",
    modules = "bin/caddy.pxb/app/metadata/caddy/modules.txt",
    source = "share/caddy/source.txt",
  },
}
