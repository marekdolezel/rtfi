#!/bin/bash

RTFI_DIR=${HOME}/runtimefaultinjector
FIM_DIR=${RTFI_DIR}/FaultInjectionMechanism
INCLUDE=${RTFI_DIR}/include
SUT=test

gcc -c -I ${INCLUDE} ${FIM_DIR}/fault_injection_mechanism.c  -DDEBUG
gcc -c -I ${INCLUDE} ${FIM_DIR}/rsp.c   -DDEBUG
gcc -c -I ${INCLUDE} ${FIM_DIR}/storage.c   -DDEBUG
gcc -c -I ${INCLUDE} test_compute_new_addr.c

gcc *.o -o ${SUT}
./test

