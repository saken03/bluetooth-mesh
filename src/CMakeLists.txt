file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mm"
)

add_executable(meshnode ${SOURCES})

target_include_directories(meshnode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Optional Linux BlueZ Bluetooth support (ignored on macOS)
if (USE_BLUETOOTH)
    find_library(BLUETOOTH_LIB bluetooth)
    if (NOT BLUETOOTH_LIB)
        message(FATAL_ERROR "libbluetooth not found")
    endif()
    target_compile_definitions(meshnode PUBLIC USE_BLUETOOTH)
    target_link_libraries(meshnode PUBLIC ${BLUETOOTH_LIB})
endif()

find_package(Threads REQUIRED)
target_link_libraries(meshnode PUBLIC Threads::Threads)

# macOS BLE support
if(APPLE)
    target_link_libraries(meshnode PUBLIC
        "-framework CoreBluetooth"
        "-framework Foundation"
    )
endif()

