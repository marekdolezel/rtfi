#!/bin/bash

. tools/quartus_env.sh 

RTFI_ARG_TCPPORT=20081
demonstration=bubble_sort

# Compile the demo application
pushd ${NIOS2_SOFTWARE_DIR}/${demonstration} > /dev/null
echo -ne "\tRunning make for demonstration ${demonstration} ... "
make &> /dev/null
if [ $? != 0 ]; then
    echo -e "err"
    echo "make has failed (demo app does not compile) ... quiting"
    exit 4
else
    echo -ne "OK\n"
fi

# Load the program to FPGA, leaving CPU in paused state 
echo -ne "\t2) Loading program to FPGA in the background ... " 
(nios2-download ${demonstration}.elf &> /dev/null) & 
NIOS2_DOWNLOAD_PID=$!
popd > /dev/null

wait ${NIOS2_DOWNLOAD_PID}
NIOS2_DOWNLOAD_RET=$?
if [ ${NIOS2_DOWNLOAD_RET} != 0 ]; then
    echo "err ${NIOS2_DOWNLOAD_RET}"
else
    echo "OK ${NIOS2_DOWNLOAD_RET}"
fi

 # Starting nios2-gdb-server, leaving CPU in paused state
echo -e "\t3 Starting nios2-gdb-server on port ${RTFI_ARG_TCPPORT} ..." 
(nios2-gdb-server --stop --tcpport ${RTFI_ARG_TCPPORT} --tcptimeout 15 ) &
NIOS2_GDB_SERVER_PID=$!

sleep 2;
gdb -x gdb_exec