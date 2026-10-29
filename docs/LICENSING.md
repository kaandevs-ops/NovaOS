# Licensing boundaries

At the author's request, original NovaOS code and documentation use **Apache-2.0**, matching the author's other repositories. The root LICENSE contains that license; NOTICE describes its scope. Existing per-file and upstream notices take precedence.

## Explicit exception

`src/pdf/nova_pdf_worker.c` already declares **GNU AGPL v3 or later** and includes MuPDF headers. That notice is preserved. The license text is in `LICENSES/AGPL-3.0-or-later.txt`. The upstream source URL and SHA-256 are in `third_party/mupdf/SOURCE.json`; the large source tarball is not bundled. No linked PDF executable or MuPDF library is distributed here. A future binary release must satisfy the applicable combined-work/source obligations; the root license does not erase them.

## Upstream materials

Read [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Included source archives retain their upstream headers and license texts. Font files have adjacent license files. Linux, Firefox and external runtime binaries are not bundled.

## Binary release gate

The older experimental VM is **not published as a Release**. Corresponding-source/relink materials, complete binary attribution, current Firefox distribution policy and image provenance must be resolved before that distribution. Publishing this source tree does not certify an old VM archive.

Primary references: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0), [MuPDF licensing](https://mupdf.readthedocs.io/en/latest/license.html), [GNU AGPL](https://www.gnu.org/licenses/agpl-3.0.html).
