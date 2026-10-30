#!/usr/bin/env bash
# Copy the patched QMK core files and the VIA keymap into a QMK tree.
# Usage: QMK_HOME=~/qmk_firmware ./firmware/apply-core-patches.sh
set -euo pipefail

QMK_HOME="${QMK_HOME:-$HOME/qmk_firmware}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCHES="$REPO_ROOT/firmware/qmk-core-patches"
KEYMAP="$REPO_ROOT/firmware/keymaps/codex_micro_via_v1"
TARGET="$QMK_HOME/keyboards/doio/kb16/rev2/keymaps/codex_micro_via_v1"

if [ ! -d "$QMK_HOME" ]; then
    echo "找不到 QMK_HOME：$QMK_HOME；请先配置 QMK 源码路径。" >&2
    exit 1
fi

BASELINE=f5db962110615fd263c3ebf9f0402e7c2b08fbe4
CURRENT=$(git -C "$QMK_HOME" rev-parse HEAD)
if [ "$CURRENT" != "$BASELINE" ]; then
    echo "QMK 版本不匹配：$CURRENT；请使用单独的 $BASELINE 源码树构建，尚未复制文件。" >&2
    exit 1
fi

while IFS= read -r -d '' file; do
    rel="${file#"$PATCHES"/}"
    mkdir -p "$QMK_HOME/$(dirname "$rel")"
    cp "$file" "$QMK_HOME/$rel"
    echo "patched $rel"
done < <(find "$PATCHES" -type f \( -name '*.c' -o -name '*.h' \) -print0)

mkdir -p "$TARGET"
cp -R "$KEYMAP/." "$TARGET/"
echo "keymap installed at $TARGET"
echo
echo "Now run: qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1"
