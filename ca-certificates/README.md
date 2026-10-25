# Public CA certificates

The recipe installs curl's Mozilla-derived **2026-10-20** PEM snapshot (Mozilla
data as of Tue Oct 20 13:32:40 2026 GMT): 200,116 bytes, 118 certificates,
SHA-256 `796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe`.
The runner fetches the [dated snapshot](https://curl.se/ca/cacert-2026-10-20.pem)
and verifies its checksum before staging the unchanged file. No host trust store,
target libraries or locally generated certificate data are used.

`share/ca-certificates` contains `cacert.pem`, `cacert.pem.sha256` and
`source.txt`. `share/licenses/ca-certificates` contains MPL-2.0 and the provenance
notice. The PEM itself retains upstream's header and certificate labels.
Its header records `mk-ca-bundle.pl` version 1.35 and the Mozilla source-data
SHA-256; the provenance file preserves both. Version 1.35 also writes a
`# Fingerprint (SHA-256)` comment line before each certificate, so the file is
larger than the 2026-09-25 snapshot although it holds fewer certificates.

Compared with the 2026-09-25 snapshot (121 certificates), this one removes three
eMudhra roots that Mozilla distrusts, **emSign ECC Root CA - C3**, **emSign ECC
Root CA - G3** and **emSign Root CA - C1**, and adds none. **emSign Root CA - G1**
remains. The comparison used the certificate labels of the two PEM files.

The [upstream conversion documentation](https://curl.se/docs/caextract.html)
states that the bundle omits Mozilla's additional trust-store constraints.
This PEM export does not implement the complete browser trust policy.

Updates are manual: review certificate changes in a new dated snapshot, verify
its published checksum, update this recipe's pin/provenance, rebuild the image
and restart HTTPS providers. There is no boot-time download or automatic trust
update. Configuring and parsing trust belong to the HTTPS provider.
