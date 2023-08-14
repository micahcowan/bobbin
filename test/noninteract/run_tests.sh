#!/bin/sh

main() {
    status=0
    for test in "$TESTDIR"/*.t; do
        # Set up execution dir
        rundir=${test##*/}
        mkdir -p "$rundir"
        if test -e "$test"/run -a ! -e "$rundir"/run; then
            for file in "$test"/*; do
                fn=${test##*/}
                if test "$fn" != "run"; then
                    ( set -x; cp "$fn" "$rundir" )
                fi
            done
        fi
        (
            cd "$rundir"
            printf '%s' "TEST $rundir: "

            # Run the test
            sh ./run >output 2>&1
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
        status=$?
    done
    exit "$status"
}

main "$@"
