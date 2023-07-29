#!/usr/bin/awk -f

BEGIN {
    status = 0
}

/^static AryOfStr [A-Z0-9_]*_OPT_NAMES\[\] = [{].*[}];$/ {
    match($0, /^static AryOfStr */)
    start = RLENGTH
    match(substr($0, start), /_OPT_NAMES\[/)
    lng = RSTART + RLENGTH - 3
    # Record that this variable names ref exists, and should be defined
    names[substr($0, start+1, lng)] = 0
}

/^const OptInfo options\[\] = [{]$/ {
    IN_OPTIONS = 1
}

IN_OPTIONS && /^[}];$/ {
    IN_OPTIONS = 0
}

IN_OPTIONS &&
    /^  *[{] *[A-Z0-9_]*_OPT_NAMES *, *T_[A-Z0-9_]* *,/ {

    match($0, /^  *[{] */)
    start = RLENGTH
    match(substr($0, start), /_OPT_NAMES[ ,]/)
    lng = RSTART + RLENGTH - 3
    # Record that this variable name has in fact been used
    names[substr($0, start+1, lng)] = 1
}

END {
    print "" > "/dev/stderr"
    for (n in names) {
        if (!names[n]) {
            print "Option " n " has not been implemented!" > "/dev/stderr"
            status = 1
        }
    }
    print "" > "/dev/stderr"

    exit (status)
}
