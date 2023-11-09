#!/bin/bash

function create_demo_results_dir {
    local RESULTS_DIR=${1}
    local APPLICATION_NAME=${2}

    if [ -z ${RESULTS_DIR} ] || [ -z ${APPLICATION_NAME} ]
    then
        return -1
    fi

    if [ ! -d ${RESULTS_DIR}/${APPLICATION_NAME} ]
    then
        mkdir -p ${RESULTS_DIR}/${APPLICATION_NAME}
        return $?
    fi
}

function compile_rtfi_tool {
    local SOFTWARE_DIR="$1"
    local BUILD_DIR_NAME="$2"

    if [ -z "$SOFTWARE_DIR" ] || [ -z "$BUILD_DIR_NAME" ]
    then
        return -1
    fi

    pushd "$SOFTWARE_DIR" &>/dev/null
    if [ -d "$BUILD_DIR_NAME" ]
    then
        test ! -z "$BUILD_DIR_NAME" && rm -rf "$BUILD_DIR_NAME" 
    fi

    popd &>/dev/null

}

function compile_demo_app {
    local SOFTWARE_DIR="$1"
    local APPLICATION_NAME="$2"
    local RESULTS_DIR="$3"

    if [ -z "$SOFTWARE_DIR" ] || [ -z "$APPLICATION_NAME" ] || [ -z "$RESULTS_DIR" ]
    then
        return -1
    fi

    pushd "$SOFTWARE_DIR"/"$APPLICATION_NAME" &> /dev/null
    make &> "$RESULTS_DIR"/"$APPLICATION_NAME"/make_output
    MAKE_RET=$?
    popd &> /dev/null
    
    return "$MAKE_RET"
}

function check_sorting_output {
    local TERMINAL_OUTPUT_FILE="$1"

    if [ -z "$TERMINAL_OUTPUT_FILE" ]
    then
        return -1
    fi

    # Store excerpt of terminal output with sorted array to variable

    # grep -n prints matched line in this format 'line_number: line content'
    # cut -d':' -f1 creates multiline variable with line number on each line
    #   first line contains line number of first <-- in ${TERMINAL_OUTPUT_FILE}
    #   second line contains line number of second <-- in ${TERMINAL_OUTPUT_FILE}
    local array_cut_lines="$(cat ${TERMINAL_OUTPUT_FILE} | grep -n "<--" | cut -d':' -f1)"
    if [ -z "$array_cut_lines" ]; then
        return 2 # could not match "<--"
    fi
    local start_cut=$(echo "${array_cut_lines}" | sed '1q;d') # store line number of first <-- to variable ${start_cut}
    local end_cut=$(echo "${array_cut_lines}" | sed '2q;d')   # store line number of second <-- to variable ${start_cut}
    if [ -z "$start_cut" ] || [ -z "$end_cut" ]
    then
        return 2;
    fi
    local start_cut=$((start_cut+1)) # exclude first <-- from output excerpt
    local end_cut=$((end_cut-1))    # exclude second <-- from output excerpt

    local terminal_output_excerpt="$(sed -n ${start_cut}','${end_cut}'p' ${TERMINAL_OUTPUT_FILE})"
    local terminal_output_excerpt_sorted="$(echo "${terminal_output_excerpt}" | sort -n)"

    if [ $(echo "$terminal_output_excerpt" | wc -l) -eq 1 ]
    then
        return 2; # no output
    fi

    local unique_lines=$(echo "$terminal_output_excerpt" | uniq -c | wc -l | cut -d' ' -f1)
    if [ "$unique_lines" -lt 10 ]
    then
        return 3
    fi 
    
    diff <(echo "$terminal_output_excerpt") <(echo "$terminal_output_excerpt_sorted") &> /dev/null
    
    return $?
}