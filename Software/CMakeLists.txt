# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 4.2)

# # 1. Detect if the ROM path is missing
# if(NOT DEFINED ENV{ESP_ROM_ELF_DIR})
#     message(STATUS "Fixing missing ESP_ROM_ELF_DIR...")
    
#     # 2. Derive the path from the IDF_PATH which should be set
#     if(DEFINED ENV{IDF_PATH})
#         set(ENV{ESP_ROM_ELF_DIR} "$ENV{IDF_PATH}/components/esp_rom/rom")
#     else()
#         # 3. Fallback: If even IDF_PATH is missing, find where this CMake file is
#         # and look for the standard Espressif location
#         set(ENV{ESP_ROM_ELF_DIR} "$ENV{HOME}/.espressif/v6.0/esp-idf/components/esp_rom/rom")
#     endif()
# endif()

set(ENV{IDF_PATH} /home/TX_Jerry/.espressif/v6.0/esp-idf)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Software)