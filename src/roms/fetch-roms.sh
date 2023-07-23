#!/bin/sh

set -e -u -C

# Requires wget and sha256sum be installed

# To use original FTP source instead of HTTPS mirror,
# swap 'ftp://ftp.apple.asimov.net/pub/apple_II'
# for 'https://mirrors.apple2.org.za/ftp.apple.asimov.net'

ROMFILES='
apple2plus.rom
https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/apple.rom
378ba00c86a64cca49cedaca7de8d5d351983ebc295d9d11e0752febfc346249

apple2.rom
https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/apple2o.rom
68d9db6bb4c305d40c3fa89fa0f2d7b7f71516a9431e3138859f23bc5bddb2d1
'
status=0

main() {
    trap how_to_fetch EXIT

    # Find a fetcher
    if which curl >/dev/null 2>&1; then
        fprog=curl
    elif which wget >/dev/null 2>&1; then
        fprog=wget
    else
        echo >&2 "Can't find a program (curl or wget) to download ROMs!"
        exit 1
    fi

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

how_to_fetch() {
    exec >&2
    echo
    echo "!!ATTENTION!!"
    echo "Failed to fetch all ROMs, but you can still download by hand"
    echo "and place in this directory. Note that the expected filenames"
    echo "are not necessarily the ones from the download URLs."
    echo "Please be sure to rename the files as shown, for bobbin"
    echo "to function correctly."
    echo

    i=0
    for field in $ROMFILES; do
        case "$i" in
            0)
                fname=$field
                i=1
                ;;
            1)
                printf '%s\n    -> %s\n\n' "$field" "$fname"
                i=2
                ;;
            2)
                i=0
                ;;
        esac
    done
    exit 1
}

file_present() {
    file=$1; shift
    sum=$1; shift
    if test "x$(../sha256-verify "$file" 2>/dev/null)" = "x$sum"; then
        warn "ROM $file already present, skipping."
        return 0
    else
        ret=$?
        warn "ROM $file not present, fetching..."
        return $ret
    fi
}

fetcher() {
    retval=0
    echo >&2 "Fetching with $fprog..."
    case "$fprog" in
        wget)
            # Avoid exiting due to set -e
            if ! wget -nv -O tmprom "$1"; then
                retval=$?
            fi
            ;;
        curl)
            # Avoid exiting due to set -e
            if ! curl --no-progress-meter -o tmprom "$1"; then
                retval=$?
            fi
            ;;
    esac
}

fetch_file() {
    file=$1; shift
    url=$1; shift
    sum=$1; shift

    rm -f tmprom
    if ! fetcher "$url"; then
        warn "*** DOWNLOAD FAILED: $file ***"
        status=1
        return 0;
    elif test "x$(../sha256-verify tmprom)" != "x$sum"; then
        warn "*** BAD CHECKSUM: $file - DISCARDING ***"
        rm -f tmprom
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
if test 0 -eq "$status"; then
    trap - EXIT
fi
exit "$status"
