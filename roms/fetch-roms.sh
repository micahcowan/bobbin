#!/bin/sh

set -e -u -C

# Requires wget and sha256sum be installed

# To use an HTTPS mirror instead, swap
# 'https://mirrors.apple2.org.za/ftp.apple.asimov.net/'
# for 'ftp://ftp.apple.asimov.net/pub/apple_II

ROMFILES='
apple2+.rom
ftp://ftp.apple.asimov.net/pub/apple_II/emulators/rom_images/apple.rom
378ba00c86a64cca49cedaca7de8d5d351983ebc295d9d11e0752febfc346249

apple2.rom
ftp://ftp.apple.asimov.net/pub/apple_II/emulators/rom_images/apple2o.rom
68d9db6bb4c305d40c3fa89fa0f2d7b7f71516a9431e3138859f23bc5bddb2d1
'
status=0

main() {
    i=0
    for field in $ROMFILES; do
        case "$i" in
            0)
                file=$field
                i=1
                ;;
            1)
                url=$field
                i=2
                ;;
            2)
                sha256=$field
                if ! file_present "$file" "$sha256"; then
                    fetch_file "$file" "$url" "$sha256"
                fi
                i=0
                ;;
        esac
    done
}

file_present() {
    file=$1; shift
    sum=$1; shift
    if test "x$(sha256sum "$file" | sed 's/ .*//')" = "x$sum"; then
        warn "ROM $file already present, skipping."
        return 0
    else
        ret=$?
        warn "ROM $file not present, fetching..."
        return $ret
    fi
}

fetch_file() {
    file=$1; shift
    url=$1; shift
    sum=$1; shift

    rm -f tmprom
    if ! wget -O tmprom "$url"; then
        warn "*** DOWNLOAD FAILED: $file ***"
        status=1
        return 0;
    elif test "x$(sha256sum tmprom | sed 's/ .*//')" != "x$sum"; then
        warn "*** BAD CHECKSUM: $file - DISCARDING ***"
        mv -f tmprom "$file"
        status=1
        return 0;
    else
        warn "*** FETCHED: $file ***"
        mv -f tmprom "$file"
    fi
}

warn() {
    printf >&2 '%s: %s\n' "$0" "$*"
}

main "$@"
exit "$status"
