#!/usr/bin/env bash

if [[ ! -f compile_commands.json ]]
then
    echo "compile_commands.json file not found"
    exit 1
fi

for arg in "$@"; do
    case "$arg" in
        --output=*)
            output_file=${arg#--output=}
            ;;

    esac
done

if [[ -z $output_file ]]
then
    echo "Output file not specified"
    exit 1
fi

run-clang-tidy \
-style=file \
-fix \
-format \
-export-fixes "$output_file"

if [[ -s "$output_file" ]]
then
    echo "Lint issues found:"
    cat "$output_file"
    exit 1
fi

exit 0
