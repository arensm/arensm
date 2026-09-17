#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<'EOF'
Verwendung:
  ./LMS-merge.sh AUSGABE.opml EINGABE1.opml EINGABE2.opml [weitere ...]

Beispiel:
  ./LMS-merge.sh favorites-gesamt.opml favorites-wdr.opml favorites-ndr.opml

Auch Platzhalter sind möglich:
  ./LMS-merge.sh favorites-gesamt.opml favorites-*.opml
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if (( $# < 3 )); then
    echo "Fehler: Eine Ausgabedatei und mindestens zwei Eingabedateien werden benötigt." >&2
    usage >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "Fehler: 'python3' ist nicht installiert." >&2
    exit 1
fi

output_file="$1"
shift

python3 - "${output_file}" "$@" <<'PY'
import copy
import os
import sys
import tempfile
import xml.etree.ElementTree as ET


def local_name(tag):
    return tag.rsplit("}", 1)[-1]


output_file = os.path.abspath(sys.argv[1])
input_files = [os.path.abspath(path) for path in sys.argv[2:]]

if output_file in input_files:
    sys.exit("Fehler: Die Ausgabedatei darf nicht zugleich eine Eingabedatei sein.")

for path in input_files:
    if not os.path.isfile(path):
        sys.exit(f"Fehler: Eingabedatei nicht gefunden: {path}")

merged_body = ET.Element("body")
known_urls = set()
added = 0
duplicates = 0

for path in input_files:
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as error:
        sys.exit(f"Fehler: Ungültiges XML in '{path}': {error}")

    if local_name(root.tag).lower() != "opml":
        sys.exit(f"Fehler: '{path}' ist keine OPML-Datei.")

    body = next((node for node in root if local_name(node.tag).lower() == "body"), None)
    if body is None:
        sys.exit(f"Fehler: In '{path}' fehlt das OPML-Element <body>.")

    for outline in body:
        if local_name(outline.tag).lower() != "outline":
            continue

        stream_url = (outline.get("URL") or outline.get("url") or "").strip()
        if stream_url and stream_url in known_urls:
            duplicates += 1
            continue

        if stream_url:
            known_urls.add(stream_url)

        merged_body.append(copy.deepcopy(outline))
        added += 1

root = ET.Element("opml", {"version": "1.1"})
head = ET.SubElement(root, "head")
ET.SubElement(head, "title").text = "Zusammengeführte LMS-Favoriten"
ET.SubElement(head, "expansionState")
root.append(merged_body)

tree = ET.ElementTree(root)
if hasattr(ET, "indent"):
    ET.indent(tree, space="  ")

output_dir = os.path.dirname(output_file) or "."
if not os.path.isdir(output_dir):
    sys.exit(f"Fehler: Zielverzeichnis nicht gefunden: {output_dir}")

temporary_name = None
try:
    with tempfile.NamedTemporaryFile(
        mode="wb", prefix=".LMS-merge-", suffix=".opml", dir=output_dir, delete=False
    ) as temporary_file:
        temporary_name = temporary_file.name
        tree.write(temporary_file, encoding="utf-8", xml_declaration=True)
    os.replace(temporary_name, output_file)
except OSError as error:
    if temporary_name and os.path.exists(temporary_name):
        os.unlink(temporary_name)
    sys.exit(f"Fehler beim Schreiben von '{output_file}': {error}")

print(f"Fertig: {added} Favoriten aus {len(input_files)} Dateien zusammengeführt.")
print(f"Übersprungene doppelte Stream-URLs: {duplicates}")
print(f"Ausgabedatei: {output_file}")
PY

