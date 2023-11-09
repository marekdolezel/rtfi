#!/bin/bash
# test simple fault types 
UCOS_TEST_ROOT_DIR="/home/ubuntu/de0_nano_quartus_project/software_tests"

ELF_BINARY1_DIR=${UCOS_TEST_ROOT_DIR}"/stuckat_initialval0"
ELF_BINARY2_DIR=${UCOS_TEST_ROOT_DIR}"/stuckat_initialval100"

OPTION_ELF_BINARY1=${ELF_BINARY1_DIR}"/stuckat.elf"
OPTION_ELF_BINARY2=${ELF_BINARY2_DIR}"/stuckat.elf"

ADDR=""
OPTION_TCPPORT="20081"
RTFI_DIR=${HOME}/runtimefaultinjector
FIM_DIR=${RTFI_DIR}/FaultInjectionMechanism
INCLUDE=${RTFI_DIR}/include
SUT=fim.elf

INJ_TYPE_FLIP_ID=0
INJ_TYPE_SET_ZERO_ID=1
INJ_TYPE_SET_ONE_ID=2

INJ_TYPE_FLIP_NAME="flip_bit"
INJ_TYPE_SET_ONE_NAME="setone_bit"
INJ_TYPE_SET_ZERO_NAME="setzero_bit"


gcc -c -I ${INCLUDE} ${FIM_DIR}/fault_injection_mechanism.c  -DDEBUG
gcc -c -I ${INCLUDE} ${FIM_DIR}/rsp.c   -DDEBUG
gcc -c -I ${INCLUDE} ${FIM_DIR}/storage.c   -DDEBUG
gcc -c -I ${INCLUDE} fim__test.c

gcc *.o -o ${SUT}
DATE_NOW=$(date +%d_%m_%Y_%H%m%S)

# Make sure terminal is not running, just kill it
pkill nios2-terminal

function make_bin {
    pushd ${1} &>/dev/null
    make &>/dev/null
    popd &>/dev/null
}

function make_clean_bin {
    pushd ${1} &>/dev/null
    make clean &>/dev/null
    popd &>/dev/null
}

function get_addr {
    pushd ${1} &>/dev/null
    ADDR=$(nm stuckat.elf -S | grep stuckatVar | cut -d' ' -f1)
    echo "addr="${ADDR}
    popd &>/dev/null
}

function run_test {
    BIT_IDX=${1}
    INJ_TYPE_ID=${2}
    TEST_ID=${4}
    INJ_TYPE_NAME=""
    PLATFORM_BINARY_DIR=${3}

    get_addr ${PLATFORM_BINARY_DIR}

    if [ ${INJ_TYPE_ID} -eq ${INJ_TYPE_FLIP_ID} ]; then INJ_TYPE_NAME=${INJ_TYPE_FLIP_NAME}; fi
    if [ ${INJ_TYPE_ID} -eq ${INJ_TYPE_SET_ZERO_ID} ]; then INJ_TYPE_NAME=${INJ_TYPE_SET_ZERO_NAME}; fi
    if [ ${INJ_TYPE_ID} -eq ${INJ_TYPE_SET_ONE_ID} ]; then INJ_TYPE_NAME=${INJ_TYPE_SET_ONE_NAME}; fi
 
    echo "FIM TEST"${TEST_ID} "params are below:"
    echo -e "\tdate="${DATE_NOW}
    echo -e "\tport="${OPTION_TCPPORT}
    echo -e "\tfault_name="${INJ_TYPE_NAME}
    echo -e "\tbit="${BIT_IDX}
    echo -e "\tbinary_dir="${PLATFORM_BINARY_DIR}
    echo -e "\tbinary="${PLATFORM_BINARY_DIR}/stuckat.elf
    echo -e "\taddr="${ADDR}

    BIN=${PLATFORM_BINARY_DIR}/"stuckat.elf"
    nios2-download ${BIN} --tcpport=${OPTION_TCPPORT} &> nios2_download_test${TEST_ID}_${INJ_TYPE_NAME}${BIT_IDX}_${DATE_NOW}.log &

    nios2-terminal &> nios2_terminal_test${TEST_ID}_${INJ_TYPE_NAME}${BIT_IDX}_${DATE_NOW}.log &
    TERMINAL_PID=$!

    ./${SUT} ${OPTION_TCPPORT} ${INJ_TYPE_ID} ${BIT_IDX} ${ADDR} &> rtfi_test${TEST_ID}_${DATE_NOW}.log

    kill -SIGINT ${TERMINAL_PID}
    sleep 4

    diff a_nios2_terminal_test${TEST_ID}_${INJ_TYPE_NAME}${BIT_IDX}.ref nios2_terminal_test${TEST_ID}_${INJ_TYPE_NAME}${BIT_IDX}_${DATE_NOW}.log

    echo -e "\tTEST"${TEST_ID}" result(O=OK, 1=FAIL)" $?
}

make_bin ${ELF_BINARY1_DIR}
make_bin ${ELF_BINARY2_DIR}


#           bit_id       inj_type_id            binary for target       test_id
run_test    3           ${INJ_TYPE_SET_ONE_ID}  ${ELF_BINARY1_DIR}   1
run_test    31          ${INJ_TYPE_SET_ONE_ID}  ${ELF_BINARY1_DIR}   2
run_test    2           ${INJ_TYPE_SET_ZERO_ID} ${ELF_BINARY1_DIR}   3
run_test    31          ${INJ_TYPE_FLIP_ID}     ${ELF_BINARY1_DIR}   4
run_test    10          ${INJ_TYPE_FLIP_ID}     ${ELF_BINARY1_DIR}   5

run_test    3           ${INJ_TYPE_SET_ONE_ID}  ${ELF_BINARY2_DIR}   6
run_test    31          ${INJ_TYPE_SET_ONE_ID}  ${ELF_BINARY2_DIR}   7
run_test    2           ${INJ_TYPE_SET_ZERO_ID} ${ELF_BINARY2_DIR}   8
run_test    31          ${INJ_TYPE_FLIP_ID}     ${ELF_BINARY2_DIR}   9
run_test    10          ${INJ_TYPE_FLIP_ID}     ${ELF_BINARY2_DIR}   10

make_clean_bin ${ELF_BINARY1_DIR}
make_clean_bin ${ELF_BINARY2_DIR}
