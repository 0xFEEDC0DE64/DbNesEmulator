find_package(Qt5Core CONFIG REQUIRED)

set(HEADERS
    emusettings.h
    inputprovider.h
    nescorelib_global.h
    nesemulator.h
    rom.h
    soundhighpassfilter.h
    soundlowpassfilter.h
    boards/bandai.h
    boards/board.h
    boards/ffe.h
    boards/mmc2.h
    boards/namcot106.h
    emu/apudmc.h
    emu/apu.h
    emu/apunos.h
    emu/apusq1.h
    emu/apusq2.h
    emu/aputrl.h
    emu/cpu.h
    emu/dma.h
    emu/interrupts.h
    emu/memory.h
    emu/ports.h
    emu/ppu.h
    enums/chrarea.h
    enums/emuregion.h
    enums/mirroring.h
    enums/ntarea.h
    enums/prgarea.h
    mappers/mapper000.h
    mappers/mapper001.h
    mappers/mapper002.h
    mappers/mapper003.h
    mappers/mapper004.h
)

set(SOURCES
    nesemulator.cpp
    rom.cpp
    soundhighpassfilter.cpp
    soundlowpassfilter.cpp
    boards/bandai.cpp
    boards/board.cpp
    boards/ffe.cpp
    boards/mmc2.cpp
    boards/namcot106.cpp
    emu/apu.cpp
    emu/apudmc.cpp
    emu/apunos.cpp
    emu/apusq1.cpp
    emu/apusq2.cpp
    emu/aputrl.cpp
    emu/cpu.cpp
    emu/dma.cpp
    emu/interrupts.cpp
    emu/memory.cpp
    emu/ports.cpp
    emu/ppu.cpp
    mappers/mapper000.cpp
    mappers/mapper001.cpp
    mappers/mapper002.cpp
    mappers/mapper003.cpp
    mappers/mapper004.cpp
)

add_library(nescorelib ${HEADERS} ${SOURCES})

target_compile_definitions(nescorelib PRIVATE NESCORELIB_LIBRARY)

target_link_libraries(nescorelib Qt5::Core dbcorelib)

target_include_directories(nescorelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
