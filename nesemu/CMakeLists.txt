find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Multimedia CONFIG REQUIRED)
find_package(Qt5Gamepad CONFIG REQUIRED)

set(HEADERS
)

set(SOURCES
    main.cpp
)

add_executable(nesemu ${HEADERS} ${SOURCES})

target_link_libraries(nesemu Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Gamepad Qt5::Multimedia dbcorelib dbguilib nescorelib nesguilib)
