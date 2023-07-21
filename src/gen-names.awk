#!/usr/bin/awk -f

BEGIN {
    STARTED=0
    print "// This file was auto-generated from the README.md,"
    print "// as part of the build process. Any changes here may"
    print "// be overwritten!"
    print
    print "// this file is read by config.c."
    print
}

/^<!--START-OPTIONS-->/ {
    STARTED = 1;
}

STARTED && /^##### / {
    sub(/^##### /,"");
    sub(/ \*arg\*$/,"");

    # Extract a variable name.
    # From the first option, unless it's a single character.
    a = match($0, /-/);
    b = match($0, /,/);
    if (b == 0) b = match($0, /$/);
    if (b - a == 2) {
        o = b+1
        rest = substr($0,o);
        aa = o-1 + match(rest,/-/);
        # If it's a single character, try the next one.
        if (aa != o-1) {
            a = aa;
            rr = substr($0,aa);
            b = aa-1 + match(rr,/,/);
            if (b == aa-1) b = aa + match(rr,/$/);
        }
    }
    a = a-1 + match(substr($0,a), /[^-]/);
    name = toupper(substr($0,a,b-a));
    gsub(/-/,"_",name);
    gsub(/, /,"\", \"");
    sub (/^/,"\"");
    sub (/$/,"\"");
    gsub(/"--*/,"\"");
    print "static AryOfStr " name "_OPT_NAMES[] = {"  $0 ", NULL};";
}

/^<!--END-OPTIONS-->/ {
    exit(0);
}
