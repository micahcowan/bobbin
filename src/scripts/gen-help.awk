#!/usr/bin/awk -f

#   scripts/gen-help.awk
#
#   Copyright (c) 2023 Micah John Cowan.
#   This code is licensed under the MIT license.
#   See the accompanying LICENSE file for details.

function o(s) {
    print "    \"" s "\\n\"";
}

BEGIN {
    STARTED=0
    print "// This file was auto-generated from the README.md,"
    print "// as part of the build process. Any changes here may"
    print "// be overwritten!"
    print
    print "// this file is read by config.c."
    print
    print "static const char help_text[] = "
}

1 {
    OUTPUT = "";
}

/^#/ {
    if (IN_SYNOPSIS) {
        o("")
        o("See " DOCDIR "/README.md for much, much more detail.")
    }
    IN_SYNOPSIS = 0;
}

IN_SYNOPSIS && !/^$/ {
    gsub(/\*\*/,"")
    gsub(/\\/,"")
    gsub(/&lt;/,"<")
    gsub(/<br[^>]*>/,"");

    # Deal with variable params (from *var* -> VAR)
    while (i = match($0,/\*/)) {
        j = i + match(substr($0,i+1),/\*/);
        if (j == i) break;
        s = substr($0,i,j-i+1);
        t = s
        gsub(/\*/,"",t);
        t = toupper(t);
        $0 = substr($0,1,i-1) t substr($0,j+1)
    }
    o($0)
}

/^### Synopsis/ {
    IN_SYNOPSIS = 1;
}

/^<!--START-OPTIONS-->/ {
    STARTED = 1;
}

NEED_DESC && !/^$/ {
    NEED_DESC = 0;
    gsub(/\*\*/,"");
    gsub(/\\\]/,"]");
    gsub(/\\\[/,"[");
    OUTPUT = "     " $0;
}

STARTED && /^#### / {
    sub(/^#### /,"")
    gsub("\"","")
    $0 = toupper($0)
    o("")
    o($0)
}

STARTED && /^##### / {
    NEED_DESC = 1;
    sub(/^##### /,"");
    while(match($0,/\*[^*][^*]*\*/)) {
        a = ""
        if (RSTART != 0) {
            a = substr($0,1,RSTART-1)
        }
        b = toupper(substr($0,RSTART+1,RLENGTH-2))
        c = ""
        if (RSTART+RLENGTH < length($0)) {
            c = substr($0,RSTART+RLENGTH)
        }
        $0 = a b c
    }
    gsub(/\\\[/, "[");
    gsub(/\\\]/, "]");
    OUTPUT = "  " $0;
}

OUTPUT {
    o(OUTPUT);
}

/^<!--END-OPTIONS-->/ {
    print ";"
    exit(0);
}

