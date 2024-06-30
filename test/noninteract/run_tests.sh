#!/bin/sh

main() {
    status=0
    for dir; do
        # Set up execution dir
        test="${TESTDIR}/${dir}"
        rundir="./testruns/${dir}"
        rm -fr "$rundir"
        mkdir -p "$rundir"

        # builddir != srcdir, so copy needed files
        for file in "$test"/*; do
            fn=${file##*/}
            if test "$fn" != "run"; then
                ( set -x; cp "$file" "$rundir" )
            fi
        done

        (
            cd "$rundir"
            printf '%s' "TEST $rundir: "

            # Copy indisk* to testdisk*
            for dsk in indisk*; do
                if test -e "$dsk"; then # guards against dsk='indisk*'
                    testdisk="testdisk${dsk#indisk}"
                    rm -f "$testdisk"
                    cp "$dsk" "$testdisk"
                    chmod +w "$testdisk"
                fi
            done

            # Run the test
            sh "$test"/run >output 2>&1
            stat=$?

            if test -e "$test/exstat"; then
                exstat=$(cat "$test"/exstat)
            else
                exstat=0
            fi

            if test -e "$test/expected"; then
                diff -u expected output
                diffstat=$?
            else
                diffstat=0
            fi

            if test "$stat" -eq "$exstat" -a "$diffstat" -eq 0
            then
                printf '[\033[1;32mPASS\033[m]\n'
                exit 0
            else
                printf '[\033[1;31mFAIL\033[m]\n'
                exit 1
            fi
        )
        newstat=$?
        if test "$newstat" -ne 0; then
            status=$newstat
        fi
    done
    exit "$status"
}

main "$@"
