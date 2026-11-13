#!/bin/bash
set -euo pipefail
base=/etc/s6-overlay
perm="$base/scripts/init-permissions_up"
# Refuse to silently patch a different upstream layout.
grep -q '^chmown vdr vdr 775 664 "/srv/vdr"' "$perm"
sed -i '\|^chmown vdr vdr 775 664 "/srv/vdr"|d; \|^chmown vdr vdr 775 664 "/vdr/timeshift"|d' "$perm"
# Preserve the s6 dependency graph, replace these actions with no-ops.
for action in init-inet_connection/up init-updpkg/up init-updpkg/down local-packages/up; do
    test -f "$base/s6-rc.d/$action"
    printf '#!/command/execlineb -P\ns6-true\n' > "$base/s6-rc.d/$action"
done
# Remove the optional epgsearch template migration download from startup.
prepare="$base/scripts/vdr-prepare_up"
grep -q '^# epgsearch$' "$prepare"
grep -q '^# streamdev$' "$prepare"
awk '/^# epgsearch$/{skip=1} /^# streamdev$/{skip=0} !skip' "$prepare" > "$prepare.new"
cat "$prepare.new" > "$prepare"
rm "$prepare.new"
bash -n "$perm" "$prepare"
