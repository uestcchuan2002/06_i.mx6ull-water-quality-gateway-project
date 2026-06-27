#!/bin/sh
# Download SQLite3 amalgamation for static cross-compilation
SQLITE_VERSION="3450100"
SQLITE_URL="https://www.sqlite.org/2024/sqlite-amalgamation-${SQLITE_VERSION}.zip"
DEST_DIR="third_party/sqlite3"

set -e

mkdir -p "$DEST_DIR"

if [ ! -f "$DEST_DIR/sqlite3.c" ] || [ ! -f "$DEST_DIR/sqlite3.h" ]; then
    echo "Downloading SQLite3 amalgamation ${SQLITE_VERSION}..."
    if command -v wget >/dev/null 2>&1; then
        wget -q "$SQLITE_URL" -O /tmp/sqlite3.zip
    elif command -v curl >/dev/null 2>&1; then
        curl -sL "$SQLITE_URL" -o /tmp/sqlite3.zip
    else
        echo "ERROR: wget or curl required" >&2
        exit 1
    fi

    echo "Extracting..."
    python3 -c "
import zipfile, os, shutil
with zipfile.ZipFile('/tmp/sqlite3.zip') as z:
    z.extractall('/tmp/sqlite3_extract')
src_dir = [d for d in os.listdir('/tmp/sqlite3_extract') if d.startswith('sqlite-amalgamation')][0]
srcdir = os.path.join('/tmp/sqlite3_extract', src_dir)
for f in ['sqlite3.c', 'sqlite3.h']:
    shutil.copy(os.path.join(srcdir, f), '$DEST_DIR')
" && rm -rf /tmp/sqlite3.zip /tmp/sqlite3_extract

    echo "Done: $DEST_DIR/sqlite3.c, $DEST_DIR/sqlite3.h"
else
    echo "SQLite3 sources already present in $DEST_DIR"
fi
