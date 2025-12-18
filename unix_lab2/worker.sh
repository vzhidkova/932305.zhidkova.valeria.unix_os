#!/bin/sh

SHARED_DIR="/shared"
LOCK_FILE="$SHARED_DIR/.lock"


CONTAINER_ID="$(cat /proc/sys/kernel/random/uuid)"


COUNTER=1

mkdir -p "$SHARED_DIR"
touch "$LOCK_FILE"

while true
do
    exec 9>"$LOCK_FILE"
    flock 9

    i=1
    while :; do
        NAME=$(printf "%03d" "$i")
        FILE="$SHARED_DIR/$NAME"
        if [ ! -e "$FILE" ]; then
            echo "container=$CONTAINER_ID file=$COUNTER" > "$FILE"
            CREATED_FILE="$FILE"
            break
        fi
        i=$((i + 1))
    done

    flock -u 9
    exec 9>&-


    sleep 1
    rm -f "$CREATED_FILE"
    sleep 1

    COUNTER=$((COUNTER + 1))
done
