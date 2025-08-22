cmake_minimum_required(VERSION 3.10)
project(ip_fragment_reassembly)

SET(SOURCE_FILE 
    main.c
    handler.c
    ipv4-fragment.c
    ipv4-reassembly.c
)
aux_source_directory(rbtree2 SOURCE_FILE)

add_executable(${PROJECT_NAME} ${SOURCE_FILE})
target_link_libraries(${PROJECT_NAME} pthread pcap)