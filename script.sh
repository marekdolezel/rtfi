#!/bin/bash

. tools/quartus_env.sh 
. tools/altera_common_functions.sh
. tools/helper_functions.sh

RTFI_WITH_VALGRIND_FLG=0
RTFI_BUILD_DIR_NAME=build
RTFI_EXEC_NAME=RunTimeFaultInjector
RTFI_EXEC="$PWD"/"$RTFI_BUILD_DIR_NAME"/"$RTFI_EXEC_NAME"
RTFI_DIR="$PWD"
RTFI_ARG_EXPERIMENT_ID=0
RTFI_ARG_TCPPORT=20083
RTFI_ARG_HOSTIP=0
RTFI_DONOT_RUN=0

CONTROLC=0

SCRIPT_SH_PID=$$
SCRIPT_SH_START_TIME=$(date +%s)

# Signal handlers
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

# Check arguments

function print_usage {
    echo "Usage: ./script.sh options"
    echo "  -n numofexperiments"
    echo "  [-v] with valgrind"
    echo "  [-s] RTFI with sanitizer flags"
    echo "  -D resultsdir"
    echo "  -r 'egrep regex'"
    echo "Example: ./script.sh -n 100 -r 'OSRdyTbl|OS_Running$' -D results_dir_take2"
}

N_FLG=0
D_FLG=0
R_FLG=0
S_FLG=0

while getopts "n:vD:r:Xs" opt
do
    case ${opt} in
        n) NUMOF_EXPERIMENTS_PER_DEMO=$OPTARG; N_FLG=1 ;;
        v) RTFI_WITH_VALGRIND_FLG=1 ;;
        D) RTFI_RESULTS_DIR="$PWD"/"$OPTARG"; D_FLG=1 ;;
        r) STRUCTS_VARS_REGEX=$OPTARG; R_FLG=1 ;;
        X) RTFI_DONOT_RUN=1 ;;
        s) S_FLG=1 ;;
        \?) print_usage ; exit 1; >&2 /dev/null;;
    esac
done

if [ "$N_FLG" -eq 0 ] || [ "$D_FLG" -eq 0 ] || [ "$R_FLG" -eq 0 ] 
then
    echo "Required options are missing"
    print_usage
    exit 1
fi

if [ "$S_FLG" -eq 1 ];
then 
    echo "Running RTFI with sanitizer glags"
    RTFI_BUILD_DIR_NAME=sanitizer
    RTFI_EXEC_NAME=RunTimeFaultInjector
    RTFI_EXEC="$PWD"/"$RTFI_BUILD_DIR_NAME"/"$RTFI_EXEC_NAME"
fi


EXPERIMENT_RESULTS_CVS="$RTFI_RESULTS_DIR"/results.csv

alt_check_nios2_environment

# alt_program_fpga ${QUARTUS_SOF_FILE}


# Build RTFI 

if [ ! -d "$RTFI_BUILD_DIR_NAME" ]
then
    mkdir "$RTFI_BUILD_DIR_NAME" 
fi
if [ -d "$RTFI_BUILD_DIR_NAME" ]
then
    pushd "$RTFI_BUILD_DIR_NAME" &> /dev/null
    make &>/dev/null 
    if [ $? != 0 ]
    then
        echo "Error: RTFI make has failed"
        exit 1
    fi
    popd > /dev/null
else
    echo "Could not create ${RTFI_BUILD_DIR_NAME}"
    exit 1
fi


if [ ! -d "$RTFI_RESULTS_DIR" ]
then
    echo "${RTFI_RESULTS_DIR} does not exist"
    mkdir -p "$RTFI_RESULTS_DIR"
fi

# First csv line

echo "experiment_id, application_name, rtfi_is_array_sorted, demo_state, additional_time,\
terminal_output, experiment_duration, addr, length, name,\
bit_idx, inj_type, fault_inj_time, fault_duration time " >> "$EXPERIMENT_RESULTS_CVS"


pushd "$NIOS2_SOFTWARE_DIR" > /dev/null
for APPLICATION_NAME in $( ls ${NIOS2_SOFTWARE_DIR} )
do
    # Only demo apps are of interest, cannot run just BSP without payload (application)
    # Demo apps with .ignore suffix are skipped by this script 
    if [ "$APPLICATION_NAME" != "$NIOS2_BSP_NAME" ] && [[ "$APPLICATION_NAME" != *".ignore"* ]] && [ -d "$NIOS2_SOFTWARE_DIR"/"$APPLICATION_NAME" ]
    then
        echo "<-- Processing application: ${APPLICATION_NAME}"

        # Create directory for results for current demo application in $RTFI_RESULTS_DIR/$APPLICATION_NAME directory
        create_demo_results_dir "$RTFI_RESULTS_DIR" "$APPLICATION_NAME"
        RET=$?
        if [ "$RET" != 0 ]; then echo "Failed to create application directory for logs with error ${RET}"; exit 1; fi

        # Compile the demo application
        compile_demo_app "$NIOS2_SOFTWARE_DIR" "$APPLICATION_NAME" "$RTFI_RESULTS_DIR"
        RET=$?
        if [ "$RET" != 0 ]; then echo -e "\Failed to compile application with error ${RET}"; exit 1; fi

        pushd "$NIOS2_SOFTWARE_DIR"/"$APPLICATION_NAME" &> /dev/null

        for experiment in `seq 1 ${NUMOF_EXPERIMENTS_PER_DEMO}`
        do
            SELECTED_VARS="$(nm "${APPLICATION_NAME}".elf -S | grep -E ${STRUCTS_VARS_REGEX})"    # Each line in this variable represents C structure or variable information (addr, length, name)
            NUMOF_VARS=$(echo "$SELECTED_VARS" | wc -l | cut -d' ' -f1 )    # Count how many C variables/structures are available for injection
            #TODO ERROR HANDLING, WHEN NUMOF_VARS is 0

            VAR_ID=$(( ( RANDOM % $NUMOF_VARS )  + 1 ))                     # Select random line with a variable information (addr, length, name)
            
            VAR_NM_INFO=$(echo "$SELECTED_VARS" | sed ${VAR_ID}'q;d')       # Store selected line with variable information to VAR_NM_INFO 
            addr=$(echo ${VAR_NM_INFO} | cut -d' ' -f1)
            length=$(echo ${VAR_NM_INFO} | cut -d' ' -f2)
            name=$(echo ${VAR_NM_INFO} | cut -d' ' -f4)

            # Get array_addr, array_size_bytes ; demo_state_addr, demo_state_size_bytes
            ARRAY_NM_INFO="$(nm "${APPLICATION_NAME}".elf -S | grep -E  array1$)"
            array_addr=$(echo ${ARRAY_NM_INFO} | cut -d' ' -f1)
            array_size_bytes=$(echo ${ARRAY_NM_INFO} | cut -d' ' -f2)
            array_name=$(echo ${ARRAY_NM_INFO} | cut -d' ' -f4)

            DEMOSTATE_NM_INFO="$(nm "${APPLICATION_NAME}".elf -S | grep -E  demo_state$)"
            ds_addr=$(echo ${DEMOSTATE_NM_INFO} | cut -d' ' -f1)
            ds_size_bytes=$(echo ${DEMOSTATE_NM_INFO} | cut -d' ' -f2)
            ds_name=$(echo ${DEMOSTATE_NM_INFO} | cut -d' ' -f4)

            echo -e "\n\nExperiment ${RTFI_ARG_EXPERIMENT_ID} ($(date), application ${APPLICATION_NAME})"
            echo -e "\tYou have provided ${NUMOF_VARS} var(s)/structure(s) for the experiment, ${name} was selected"
            VARS="$(nm ${APPLICATION_NAME}.elf -S | grep -E ${STRUCTS_VARS_REGEX} | nl)"
            echo -e "$VARS" | ( TAB='\t\t' ; sed "s/^/$TAB/" )

            # Load the program to FPGA, leaving CPU in paused state 
            (nios2-download ${APPLICATION_NAME}.elf &> "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/nios2_download_${RTFI_ARG_EXPERIMENT_ID}) & 
            NIOS2_DOWNLOAD_PID=$!

            wait "$NIOS2_DOWNLOAD_PID"
            NIOS2_DOWNLOAD_RET=$?
            if [ "$NIOS2_DOWNLOAD_RET" != 0 ]
            then
                echo "Failed to load program to FPGA (error ${NIOS2_DOWNLOAD_RET})"
                exit 1
            fi

            # Starting nios2-gdb-server, leaving CPU in paused state
            (nios2-gdb-server --tcpport "$RTFI_ARG_TCPPORT"  &> "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/nios2_gdb_server_${RTFI_ARG_EXPERIMENT_ID}) &
            NIOS2_GDB_SERVER_PID=$!

            pushd "$RTFI_DIR" > /dev/null

            # Run RTFI
            sleep 1
            echo -ne "\t** Running FaultInjector "
            EXPERIMENT_START_TIME=$(date +%s)
            if [ "$RTFI_WITH_VALGRIND_FLG" -eq 1 ]
            then
                echo -ne "with valgrind ..."
                (valgrind\
                --leak-check=full \
                --show-leak-kinds=all \
                --track-origins=yes \
                --verbose \
                --log-file=valgrind_rtfi${RTFI_ARG_EXPERIMENT_ID}.txt \
                "$RTFI_EXEC" -p "$RTFI_ARG_TCPPORT" -h "$RTFI_ARG_HOSTIP"\
                -a "$addr" -l "$length"\
                -s\
                -e "$RTFI_ARG_EXPERIMENT_ID"\
                -w "$array_addr" -x "$array_size_bytes"\
                -t "$ds_addr" -z "$ds_size_bytes" &> "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/rtfi_run_${RTFI_ARG_EXPERIMENT_ID}) &
            else 
                echo -ne "no valgrind ..."
                (${RTFI_EXEC} -p ${RTFI_ARG_TCPPORT} -h ${RTFI_ARG_HOSTIP}\
                -a "$addr" -l "$length"\
                -s\
                -e "$RTFI_ARG_EXPERIMENT_ID"\
                -w "$array_addr" -x "$array_size_bytes"\
                -t "$ds_addr" -z "$ds_size_bytes" &> "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/rtfi_run_${RTFI_ARG_EXPERIMENT_ID}) &
            fi
            RTFI_PID=$!

            # Run terminal for target
            (nios2-terminal &> "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/nios2_terminal_${RTFI_ARG_EXPERIMENT_ID}) & 
            NIOS2_TERMINAL_PID=$!


            wait "$RTFI_PID"
            RTFI_RET=$?
            EXPERIMENT_END_TIME=$(date +%s)
            EXPERIMENT_DURATION=$((EXPERIMENT_END_TIME - EXPERIMENT_START_TIME))

            # Send ctrl-C to console 
            kill -s SIGINT "$NIOS2_TERMINAL_PID" &> /dev/null
            if [ $? != 0 ]
            then
                echo "\tWarning: Terminal is already running, or failed due to I/O error"
            fi

            if [ "$RTFI_RET" == 127 ]; then echo "RTFI binary does not exist"; exit 1; fi
            if [ "$RTFI_RET" == 134 ]; then echo "RTFI - coredumped"; exit 2; fi

            # Print RTFI result to the user
            if [ "$RTFI_RET" = 0 ]
            then
                echo "OK ${RTFI_RET}"
            else
                echo "err ${RTFI_RET}"
            fi

            RTFI_EXPERIMENT_PARAMS=$(cat "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/rtfi_run_${RTFI_ARG_EXPERIMENT_ID} | grep EXPERIMENT | cut -d';' -f2)
            RTFI_EXPERIMENT_PARAM_BIT_IDX=$(echo $RTFI_EXPERIMENT_PARAMS | cut -d',' -f1 | cut -d':' -f2)
            RTFI_EXPERIMENT_PARAM_INJ_TYPE=$(echo $RTFI_EXPERIMENT_PARAMS | cut -d',' -f2 | cut -d':' -f2)
            RTFI_EXPERIMENT_PARAM_FAULT_INJ_TIME=$(echo $RTFI_EXPERIMENT_PARAMS | cut -d',' -f3 | cut -d':' -f2)
            RTFI_EXPERIMENT_PARAM_FAULT_DURATION_TIME=$(echo $RTFI_EXPERIMENT_PARAMS | cut -d',' -f4 | cut -d':' -f2)
            # Print RTFI parameters
            echo -e "\t** Experiment params: ${RTFI_EXPERIMENT_PARAMS}"
            
            echo -e "\tResults: experiment ${RTFI_ARG_EXPERIMENT_ID} took ${EXPERIMENT_DURATION} secs"
            # This prints SORT: OK, or SORT: ERR; this decision is made based on target memory content
            RTFI_IS_ARRAY_SORTED=$(cat "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/rtfi_run_${RTFI_ARG_EXPERIMENT_ID} | grep SORT | cut -d':' -f2)
            DEMO_STATE=$(cat ${RTFI_RESULTS_DIR}/${APPLICATION_NAME}/rtfi_run_${RTFI_ARG_EXPERIMENT_ID} | grep DEMO_STATE | cut -d':' -f2)
            ADDITIONAL_TIME=$(cat ${RTFI_RESULTS_DIR}/${APPLICATION_NAME}/rtfi_run_${RTFI_ARG_EXPERIMENT_ID} | grep ADDITIONAL_TIME | cut -d':' -f2)

            echo -e "\t\t** RTFI is_array_sorted"
            echo -e "\t\t\t** RTFI is_array_sorted: ${RTFI_IS_ARRAY_SORTED}"
            echo -e "\t\t\t** RTFI is_array_sorted demo_state: ${DEMO_STATE}"
            echo -e "\t\t\t** RTFI is_array_sorted additional_time: ${ADDITIONAL_TIME}"
            
            # Check sorting again, this time check target console
            check_sorting_output "$RTFI_RESULTS_DIR"/"$APPLICATION_NAME"/nios2_terminal_${RTFI_ARG_EXPERIMENT_ID}
            RET=$?
            if [ "$RET" = -1 ]; then echo "Invalid arguments to check_sorting_output"; exit 1; fi

            # -1 invalid arguments
            #  0 output is correct (sorted)
            #  1 output is incorrect (not sorted)
            #  2 No output
            #  3 less than 10 unique lines 
            if [ "$RET" = 0 ]   # Output is correct (and sorted)
            then
                echo -e "\t\t** console output: CORRECT (sorted)"
                CHECK_TERMINAL_OUTPUT="CORRECT"
            elif [ "$RET" = 1 ] # Output is incorrect (and not sorted)
            then
                echo -e "\t\t** console output: INCORRECT (not sorted)"
                CHECK_TERMINAL_OUTPUT="INCORRECT"
            elif [ "$RET" = 2 ] # No output at all
            then
                echo -e "\t\t** console output: NO OUTPUT (missing output between cut lines)"
                CHECK_TERMINAL_OUTPUT="NO_OUTPUT"
            elif [ "$RET" = 3 ] # less than 10 unique lines
            then
                echo -e "\t\t** console output: INCORRECT (less than 10 unique lines)"
                CHECK_TERMINAL_OUTPUT="INCORRECT_NOTUNIQUE"
            fi
            
           

            popd > /dev/null # pop RTFI_DIR

            # print experiment information to csv file
            echo "${RTFI_ARG_EXPERIMENT_ID},${APPLICATION_NAME},${RTFI_IS_ARRAY_SORTED},${DEMO_STATE},${ADDITIONAL_TIME},\
${CHECK_TERMINAL_OUTPUT},${EXPERIMENT_DURATION},\
${addr},${length},${name},${RTFI_EXPERIMENT_PARAM_BIT_IDX},${RTFI_EXPERIMENT_PARAM_INJ_TYPE},\
${RTFI_EXPERIMENT_PARAM_FAULT_INJ_TIME},${RTFI_EXPERIMENT_PARAM_FAULT_DURATION_TIME}" >> "$EXPERIMENT_RESULTS_CVS"

            RTFI_ARG_EXPERIMENT_ID=$((RTFI_ARG_EXPERIMENT_ID+1))
        done # experiment 
       
        # Clean files after make
        make clean &> /dev/null
        echo "<-- end of application: ${APPLICATION_NAME}"

        popd &> /dev/null # pop application_dir
    fi
    # sleep 150
done # APPLICATION_NAME
popd &> /dev/null
