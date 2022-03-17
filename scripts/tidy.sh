#!/usr/bin/env bash

if [[ ! -f compile_commands.json ]]
then
    echo "compile_commands.json file not found"
    exit 1
fi

OUTPUT_FILE="$1"

run-clang-tidy \
-style=file \
-fix \
-format \
-export-fixes "$OUTPUT_FILE"

exit 0
