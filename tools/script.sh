#!/bin/bash

. tools/quartus_env.sh 

STRUCTS_VARS_REGEX=${1}
NUMOF_EXPERIMENTS_PER_DEMO=${2}
echo $STRUCTS_VARS_REGEX
RTFI_EXEC=${PWD}/build/RunTimeFaultInjector
RTFI_DIR=${PWD}
RTFI_ARG_EXPERIMENT_ID=0
RTFI_ARG_TCPPORT=20082
RTFI_ARG_HOSTIP=0

RTFI_RESULTS_DIR=${PWD}/results_dir
CONTROLC=0

SCRIPT_SH_PID=$$
SCRIPT_SH_START_TIME=$(date +%s)

function catch_sigusr1 {
    echo "Received SIGURS1. Something is wrong. Terminating.."
    kill -INT 0
    wait
    exit 
}

function catch_ctrl_c {
    echo "^C has been pressed. Discard the results of the experiment above"
    pkill nios2-terminal
    pkill nios2-download
    exit

}

trap catch_sigusr1 USR1
trap catch_ctrl_c INT

command nios2-elf-ar &> /dev/null
if [ $? -ne 1 ]; then
    echo "Please run /opt/18.1/nios2eds/nios2_command_shell.sh before this script"
    exit 1
fi

if [ $# -ne 2 ]; then
    echo "Please provide an argument with the names (egrep format) of global structures and/or variables to be injected with faults, and numof_experiments for demonstration"
    echo -e "\tan example: ./script.sh 'OSRdyTbl$|OS_Running$' 5"
    exit 2
fi

# Build RTFI 
if [ -d build ]; then
    echo "removing build directory"
    rm -rf build
fi
if [ ! -d build ]; then
    mkdir build 
    pushd build > /dev/null
    cmake -DCMAKE_BUILD_TYPE=Debug .. &>/dev/null
    make &>/dev/null 
    if [ $? != 0 ]; then
        echo "make RTFI ... err"
        exit 99
    else
        echo "make RTFI ... ok"
    fi
    popd > /dev/null
fi

if [ ! -d ${RTFI_RESULTS_DIR} ]; then
    echo "$RTFI_RESULTS_DIR does not exist"
    mkdir -p ${RTFI_RESULTS_DIR}
fi


# Check that the fpga is connected to the computer
USB_BLASTER=$(jtagconfig | cut -d' ' -f2 | grep "USB-Blaster")

if [ $USB_BLASTER = "USB-Blaster" ]; then
    echo "Detected USB Blaster []"
else
    echo "Please connect the FPGA to the computer"
    exit 3
fi

# Program the FPGA
pushd ${QUARTUS_PROJECT_DIR} > /dev/null
    # ${JTAG_FIX_SH}
    # TODO: how to run script from a background
    echo "REMOVE THIS: program fpga manually"
    # (nios2-configure-sof ${QUARTUS_SOF_FILE} &>/dev/null &)
    # sleep 5
popd > /dev/null

pushd $NIOS2_SOFTWARE_DIR > /dev/null
for demonstration in $( ls ${NIOS2_SOFTWARE_DIR} ); do
    demonstration=bubble_sort #REMOVE_LINE
    # Only demo apps are of interest, cannot run just BSP without payload (application)
    # Demo apps with .ignore suffix are skipped by this script 
    if [ ${demonstration} != ${NIOS2_BSP_NAME}  ] && [ ${demonstration} != *".ignore"* ] && [ -d ${NIOS2_SOFTWARE_DIR}/${demonstration} ]; then
        # demonstration="stuckat"
        echo "<-- Processing demonstration: ${demonstration}"

        # Create directory for results for current demo application
        if [ ! -d ${RTFI_RESULTS_DIR}/${demonstration} ]; then
            echo -e "\tCreating directory for results in ${RTFI_RESULTS_DIR}"
            mkdir -p ${RTFI_RESULTS_DIR}/${demonstration}
            if [ $? != 0 ]; then
                echo "Something is wrong"
                exit 2
            fi
        fi


        # Compile the demo application
        pushd ${NIOS2_SOFTWARE_DIR}/${demonstration} > /dev/null
        echo -ne "\tRunning make for demonstration ${demonstration} ... "
        make &> ${RTFI_RESULTS_DIR}/${demonstration}/make_output
        if [ $? != 0 ]; then
            echo -e "err"
            echo "make has failed (demo app does not compile) ... quiting"
            exit 4
        else
            echo -ne "OK\n"
        fi


        for experiment in `seq 1 ${NUMOF_EXPERIMENTS_PER_DEMO}`; do
            EXPERIMENT_START_TIME=$(date +%s)
            SELECTED_VARS="$(nm "${demonstration}".elf -S | grep -E ${STRUCTS_VARS_REGEX})"    # Each line in this variable represents C structure or variable information (addr, length, name)
            NUMOF_VARS=$(echo "$SELECTED_VARS" | wc -l | cut -d' ' -f1 )    # Count how many C variables/structures are available for injection
            #TODO ERROR HANDLING, WHEN NUMOF_VARS is 0

            VAR_ID=$(( ( RANDOM % $NUMOF_VARS )  + 1 ))                     # Select random line with a variable information (addr, length, name)
            VAR_NM_INFO=$(echo "$SELECTED_VARS" | sed ${VAR_ID}'q;d')       # Store selected line with variable information to VAR_NM_INFO 

            addr=$(echo ${VAR_NM_INFO} | cut -d' ' -f1)
            length=$(echo ${VAR_NM_INFO} | cut -d' ' -f2)
            name=$(echo ${VAR_NM_INFO} | cut -d' ' -f4)

            echo -e "\t"$(date)
            echo -e "\t1)  ${demonstration} EXPERIMENT(id: ${RTFI_ARG_EXPERIMENT_ID}) addr=${addr} length=${length} name=${name}"
            echo -e "\t\tYou have provided ${NUMOF_VARS} var(s)/structure(s) for the experiment, ${name} was selected"
            VARS="$(nm ${demonstration}.elf -S | grep -E $1 | nl)"
            echo -e "$VARS" | ( TAB='\t\t' ; sed "s/^/$TAB/" )

            # Load the program to FPGA, leaving CPU in paused state 
            echo -ne "\t2) Loading program to FPGA in the background ... " 
            (nios2-download ${demonstration}.elf &> ${RTFI_RESULTS_DIR}/${demonstration}/nios2_download_${RTFI_ARG_EXPERIMENT_ID}) & 
            NIOS2_DOWNLOAD_PID=$!

            wait ${NIOS2_DOWNLOAD_PID}
            NIOS2_DOWNLOAD_RET=$?
            if [ ${NIOS2_DOWNLOAD_RET} != 0 ]; then
                echo "err ${NIOS2_DOWNLOAD_RET}"
            else
                echo "OK ${NIOS2_DOWNLOAD_RET}"
            fi

            # Starting nios2-gdb-server, leaving CPU in paused state
            echo -e "\t3 Starting nios2-gdb-server on port ${RTFI_ARG_TCPPORT} ..." 
            (nios2-gdb-server --stop --tcpport ${RTFI_ARG_TCPPORT} --tcptimeout 8 &> ${RTFI_RESULTS_DIR}/${demonstration}/nios2_gdb_server_${RTFI_ARG_EXPERIMENT_ID}) &
            NIOS2_GDB_SERVER_PID=$!

            pushd ${RTFI_DIR} > /dev/null

            # Run RTFI
            echo -ne "\t4) Running FaultInjector ... "
            (${RTFI_EXEC} -p ${RTFI_ARG_TCPPORT} -h ${RTFI_ARG_HOSTIP} -a ${addr} -l ${length} -e ${RTFI_ARG_EXPERIMENT_ID} &> ${RTFI_RESULTS_DIR}/${demonstration}/rtfi_run_${RTFI_ARG_EXPERIMENT_ID}) &
            # (sleep 10 &)
            RTFI_PID=$!

            # Run terminal for target
            (nios2-terminal &> ${RTFI_RESULTS_DIR}/${demonstration}/nios2_terminal_${RTFI_ARG_EXPERIMENT_ID}) & 
            NIOS2_TERMINAL_PID=$!


            wait ${RTFI_PID}
            RTFI_RET=$?
            EXPERIMENT_END_TIME=$(date +%s)

            # Send ctrl-C to console 
            kill -s SIGINT ${NIOS2_TERMINAL_PID}
            if [ $? != 0 ]; then
                echo "Error: Terminal is already running?"
                exit
            fi

            # Print RTFI result to the user
            if [ ${RTFI_RET} = 1 ]; then
                echo "err ${RTFI_RET}"
            elif [ ${RTFI_RET} = 127 ]; then
                echo "RTFI binary does not exist ${RTFI_RET}"
            else
                echo "OK ${RTFI_RET}"
            fi

            # Print results
            echo -e "\t5) RTFI output:" $(cat ${RTFI_RESULTS_DIR}/${demonstration}/rtfi_run_${RTFI_ARG_EXPERIMENT_ID} | grep EXPERIMENT)
            echo -e "\t6) Fault Syndrome: NOT IMPLEMENTED"
            echo -e "\tAll 5 stages of experiment took: " $((EXPERIMENT_END_TIME - EXPERIMENT_START_TIME))            
            echo -e "\n\n"

            popd > /dev/null # pop RTFI_DIR


            RTFI_ARG_EXPERIMENT_ID=$((RTFI_ARG_EXPERIMENT_ID+1))
        done # experiment 
       
        # Clean files after make
        make clean &> /dev/null
        echo "<-- end of demonstration:" ${demonstration}

        popd > /dev/null # pop demonstration_dir
    fi

done # demonstration
popd > /dev/null

echo "Total time elapsed" $((   date +%s - SCRIPT_SH_START_TIME))
