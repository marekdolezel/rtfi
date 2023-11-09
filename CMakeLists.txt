cmake_minimum_required(VERSION 3.10)



project(RunTimeFaultInjector)

set(SOURCES
    # json-parser/json.c

    src/main.c

    # FaultInjector/json_process.c
    src/FaultInjectionMechanism/fault_injection_mechanism.c
    src/FaultInjectionMechanism/storage.c
    src/FaultInjectionMechanism/rsp.c

    src/FaultManager/fault_manager.c
    src/main.c

    src/SyndromeDetection/syndrome_detection.c
)

add_executable(${PROJECT_NAME} ${SOURCES})
add_definitions(-DSFMT)
add_definitions(-DDEBUG)
add_definitions(-DSFMT_MEXP=19937)
target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
            
)

target_link_libraries(${PROJECT_NAME} m)
target_link_libraries(${PROJECT_NAME} SFMT)