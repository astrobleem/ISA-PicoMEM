cmake_minimum_required(VERSION 3.16)

#PMBIOS object Library
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pmbios.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND ${CMAKE_LINKER} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/pmbios.o pmbios.bin
    #places in data
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/pmbios.bin
)

add_custom_target(pmbiosobj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pmbios.o)
add_library(pmbios OBJECT IMPORTED GLOBAL)
add_dependencies(pmbios pmbiosobj)
set_target_properties(pmbios PROPERTIES IMPORTED_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/pmbios.o")

#PM2BIOS object Library
set(PM2_ROM_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
if (PM2_PMR1)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(PM2_ROM_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pmr1")
    file(MAKE_DIRECTORY "${PM2_ROM_DIRECTORY}")
    add_custom_command(
        OUTPUT ${PM2_ROM_DIRECTORY}/pm2bios.bin
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/add_pmr_descriptor.py
            ${CMAKE_CURRENT_LIST_DIR}/pm2bios.bin ${PM2_ROM_DIRECTORY}/pm2bios.bin
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/pm2bios.bin
            ${CMAKE_CURRENT_LIST_DIR}/add_pmr_descriptor.py
        VERBATIM
    )
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pm2bios.o
    WORKING_DIRECTORY ${PM2_ROM_DIRECTORY}
    COMMAND ${CMAKE_LINKER} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/pm2bios.o pm2bios.bin
    #places in data
    DEPENDS ${PM2_ROM_DIRECTORY}/pm2bios.bin
)

add_custom_target(pm2biosobj ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pm2bios.o)
add_library(pm2bios OBJECT IMPORTED GLOBAL)
add_dependencies(pm2bios pm2biosobj)
set_target_properties(pm2bios PROPERTIES IMPORTED_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/pm2bios.o")