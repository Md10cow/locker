#!/bin/bash

COUNT=10
RESULT_FILE_NAME="results.txt"

rm -f file.lck statistics $RESULT_FILE_NAME
make -s main

pids=()
for (( i=0; i<$COUNT; i++ ))
do
    ./main file &
    pids+=($!)
done

sleep 300

completed=0
for pid in ${pids[@]}
do
    kill -SIGINT $pid
    if [[ $? -eq 0 ]]
    then
        ((completed++))
    fi
done

echo "Expected - " $COUNT >>"$RESULT_FILE_NAME"
echo "Actual - " $completed >>"$RESULT_FILE_NAME"
cat statistics >> "$RESULT_FILE_NAME"
rm -f file.lck statistics
