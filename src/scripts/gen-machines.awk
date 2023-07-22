#!/usr/bin/awk -f

function print_tags() {
    for (name in names_num) {
        tag = toupper(name) "_TAG"
        print "static const char * const " tag " = \"" name "\";"
    }
}

function print_aliases() {
    for (name in names_num) {
        alias = toupper(name) "_ALIASES"
        tag = toupper(name) "_TAG"
        ors = ORS
        ORS = ""
        print "static const StrArray " alias " = {" tag
        for (i=0; i < names_num[name]; ++i) {
            print ", \"" names[name,i] "\""
        }
        ORS = ors
        print ", NULL};"
    }
}

function get_name_list() {
    rem = $0
    i = 0
    first = ""
    for (x in list) delete list[x]
    while (match(rem, /`[^`]+`/)) {
        item = substr(rem, RSTART+1, RLENGTH-2)
        if (first == "") {
            first = item
        } else {
            names[first, i++] = item
        }
        list[i] = item
        rem = substr(rem, RSTART + RLENGTH + 1)
    }
    names_num[first] = i
}

BEGIN {
    STARTED=0
    print "// This file was auto-generated from the README.md,"
    print "// as part of the build process. Any changes here may"
    print "// be overwritten!"
    print
    print "// this file is read by config.c."
    print
}

/^<!--MACHINES-->/ {
    STARTED = 1;
}

STARTED && /^ - / {
    gsub(/\\/,"",$0)
    get_name_list()
}

/^<!--\/MACHINES-->/ {
    print_tags()
    print ""
    print_aliases()
    exit(0);
}

