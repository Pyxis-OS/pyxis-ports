# Public CA certificates

The recipe installs curl's Mozilla-derived **2026-09-25** PEM snapshot:
188,900 bytes, 121 certificates, SHA-256
`a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505`.
The runner fetches the [dated snapshot](https://curl.se/ca/cacert-2026-09-25.pem)
and verifies its checksum before staging the unchanged file. No host trust store,
target libraries or locally generated certificate data are used.

`share/ca-certificates` contains `cacert.pem`, `cacert.pem.sha256` and
`source.txt`. `share/licenses/ca-certificates` contains MPL-2.0 and the provenance
notice. The PEM itself retains upstream's header and certificate labels.
Its header records `mk-ca-bundle.pl` version 1.33 and the Mozilla source-data
SHA-256; the provenance file preserves both.

The [upstream conversion documentation](https://curl.se/docs/caextract.html)
states that the bundle omits Mozilla's additional trust-store constraints.
This PEM export does not implement the complete browser trust policy.

Updates are manual: review certificate changes in a new dated snapshot, verify
its published checksum, update this recipe's pin/provenance, rebuild the image
and restart HTTPS providers. There is no boot-time download or automatic trust
update. Configuring and parsing trust belong to the HTTPS provider.
