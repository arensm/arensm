#!/usr/bin/env bash

set -euo pipefail

API_URL="https://de1.api.radio-browser.info/json/stations/search"
SEARCH_TERM="${1:-wdr}"
OUTPUT_FILE="${2:-favorites-${SEARCH_TERM}.opml}"
OFFSET="${OFFSET:-10}"
LIMIT="${LIMIT:-10}"

usage() {
    cat <<'EOF'
Verwendung:
  ./LMS-fav.sh [Suchbegriff] [Ausgabedatei]

Beispiele:
  ./LMS-fav.sh wdr
  ./LMS-fav.sh "radio bob" favorites.opml

Optional lassen sich OFFSET und LIMIT als Umgebungsvariablen setzen:
  OFFSET=0 LIMIT=100 ./LMS-fav.sh wdr favorites.opml
EOF
}

if [[ "${SEARCH_TERM}" == "-h" || "${SEARCH_TERM}" == "--help" ]]; then
    usage
    exit 0
fi

for command in curl jq; do
    if ! command -v "${command}" >/dev/null 2>&1; then
        echo "Fehler: '${command}' ist nicht installiert." >&2
        exit 1
    fi
done

if ! [[ "${OFFSET}" =~ ^[0-9]+$ && "${LIMIT}" =~ ^[1-9][0-9]*$ ]]; then
    echo "Fehler: OFFSET muss >= 0 und LIMIT muss > 0 sein." >&2
    exit 1
fi

tmp_json="$(mktemp)"
tmp_opml="$(mktemp)"
trap 'rm -f "${tmp_json}" "${tmp_opml}"' EXIT

echo "Suche nach '${SEARCH_TERM}' ..."
curl --fail --silent --show-error --location --get \
    --data-urlencode "offset=${OFFSET}" \
    --data-urlencode "limit=${LIMIT}" \
    --data-urlencode "name=${SEARCH_TERM}" \
    --data-urlencode "hidebroken=true" \
    --data-urlencode "order=clickcount" \
    --data-urlencode "reverse=true" \
    "${API_URL}" >"${tmp_json}"

if ! jq -e 'type == "array"' "${tmp_json}" >/dev/null; then
    echo "Fehler: Die Radio-Browser-API lieferte keine Senderliste." >&2
    exit 1
fi

station_count="$(jq '[.[]
    | select(.name != null and .name != "" and .url != null and .url != "")
    | select(((.codec // "") | ascii_upcase) == "MP3" or ((.codec // "") | ascii_upcase) == "OGG")
    | select((.url | ascii_downcase | test("\\.m3u8?($|[?#])")) | not)
] | length' "${tmp_json}")"

{
    printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>'
    printf '%s\n' '<opml version="1.1">'
    printf '  <head>\n'
    printf '    <title>Radio-Browser: %s</title>\n' "$(jq -Rnr --arg value "${SEARCH_TERM}" '$value | @html')"
    printf '    <expansionState></expansionState>\n'
    printf '  </head>\n'
    printf '  <body>\n'
    jq -r '.[]
        | select(.name != null and .name != "" and .url != null and .url != "")
        | select(((.codec // "") | ascii_upcase) == "MP3" or ((.codec // "") | ascii_upcase) == "OGG")
        | select((.url | ascii_downcase | test("\\.m3u8?($|[?#])")) | not)
        | "    <outline URL=\"\(.url | @html)\" text=\"\(.name | @html)\" type=\"audio\" />"' \
        "${tmp_json}"
    printf '  </body>\n'
    printf '%s\n' '</opml>'
} >"${tmp_opml}"

mv "${tmp_opml}" "${OUTPUT_FILE}"
echo "Fertig: ${station_count} Sender wurden nach '${OUTPUT_FILE}' geschrieben."
