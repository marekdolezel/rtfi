#!/bin/bash

function alt_check_nios2_environment {
    command nios2-elf-ar &> /dev/null
    if [ $? -ne 1 ]; then
        echo "Please run /opt/18.1/nios2eds/nios2_command_shell.sh before this script"
        exit 1
    fi
}

function alt_check_fpga_isconnected {
    # Check that the fpga is connected to the computer
    USB_BLASTER=$(jtagconfig | cut -d' ' -f2 | grep "USB-Blaster")

    if [ $USB_BLASTER = "USB-Blaster" ]; then
        echo "Detected USB Blaster []"
    else
        echo "Please connect the FPGA to the computer"
        exit 3
    fi
}

function alt_program_fpga {
        (nios2-configure-sof ${1} &>/dev/null ) &
}