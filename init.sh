#!/bin/bash

# capture CTRL+C, CTRL+Z and quit singles using the trap

trap 'echo The script is terminated via SIGINT; exit' SIGINT
trap 'echo The script is terminated via SIGQUIT; exit'  SIGQUIT
trap 'echo The script is terminated via SIGSTP; exit' SIGTSTP

echo "Script is running..."
while true
do
    sleep 1
done

