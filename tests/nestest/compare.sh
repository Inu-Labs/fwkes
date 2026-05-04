#!/bin/bash

# usage: ./compare.sh file1 file2
if [ "$#" -ne 2 ]; then
    echo "usage: $0 file1 file2"
    exit 1
fi

file1="$1"
file2="$2"

if [ ! -f "$file1" ] || [ ! -f "$file2" ]; then
    echo "Error: one or both files do not exist."
    exit 1
fi

tmp1=$(mktemp)
tmp2=$(mktemp)

sed -E 's/\x1B\[[0-9;]*[A-Za-z]//g' "$file1" | tr -d '\000-\011\013-\037\177' > "$tmp1"
sed -E 's/\x1B\[[0-9;]*[A-Za-z]//g' "$file2" | tr -d '\000-\011\013-\037\177' > "$tmp2"

exec 3<"$tmp1"
exec 4<"$tmp2"

line_number=1
while true; do
    read -r line1 <&3
    read -r line2 <&4

    if [ -z "$line1" ] || [ -z "$line2" ]; then
        break
    fi

    if [ "$line1" != "$line2" ]; then
        echo -e "\033[1m\033[93m$line_number:\033[0m"
        echo -e "\033[31m-$line1\033[0m"
        echo -e "\033[32m+$line2\033[0m"
        echo
    fi

    ((line_number++))
done

exec 3<&-
exec 4<&-

rm "$tmp1" "$tmp2"
