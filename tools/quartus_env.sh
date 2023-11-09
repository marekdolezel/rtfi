#!/bin/bash

QUARTUS_PROJECT_DIR=${HOME}/de0_nano_quartus_project
QUARTUS_SOF_FILE=${QUARTUS_PROJECT_DIR}/DE0_NANO_SDRAM_Nios_Test_time_limited.sof
NIOS2_SOFTWARE_DIR=${QUARTUS_PROJECT_DIR}/software
NIOS2_BSP_DIR=${NIOS2_SOFTWARE_DIR}/bc_proj_ucos2_bsp
NIOS2_BSP_NAME=bc_proj_ucos2_bsp
UCOS_CONFIG_H=${NIOS2_BSP_DIR}/UCOSII/inc/os_cfg.h