find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Gamepad CONFIG REQUIRED)

set(HEADERS
    gamepadinput.h
    nesguilib_global.h
)

set(SOURCES
    gamepadinput.cpp
)

add_library(nesguilib ${HEADERS} ${SOURCES})

target_compile_definitions(nesguilib PRIVATE NESGUILIB_LIBRARY)

target_link_libraries(nesguilib Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Gamepad nescorelib)

target_include_directories(nesguilib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
