#!/bin/bash

OUTPUT_FILE=${1}
cat ${1} | grep  took | cut -d':' -f2 | awk  '{sum+=$1} END{print "AVG is",sum/NR,"sec"}'