# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
cmake_minimum_required(VERSION 3.20)

project(vaeg LANGUAGES C CXX)

option(VAEG_ENABLE_TESTS "Enable vaeg tests" OFF)
option(VAEG_WERROR "Treat warnings as errors" OFF)
option(VAEG_WINDOWS_CONSOLE "Keep a console window for Windows SDL2 builds" OFF)
option(VAEG_FETCH_SDL2 "Fetch pinned SDL2 instead of using system SDL2" OFF)
option(VAEG_STATIC_SDL2 "Link SDL2 statically" OFF)
option(VAEG_ENABLE_LIBRASHADER
    "Enable optional librashader OpenGL CRT presentation" OFF)
option(VAEG_ENABLE_ARCHIVE_DROP
    "Enable archive disk-image drag and drop when LibArchive is available" ON)
option(VAEG_FETCH_LIBARCHIVE
    "Fetch pinned static archive dependencies instead of using system LibArchive" OFF)
option(VAEG_Z80_COMPAT_INTEGRATION_TRACE
    "Compile deterministic subsystem compatibility/FDD integration tracing" OFF)
option(VAEG_UPD9002_PERF_DIAGNOSTIC
    "Compile disabled-by-default uPD9002 runtime performance diagnostics" OFF)
set(VAEG_ZEX_ARTIFACT_DIR "" CACHE PATH
    "Offline directory containing the five hash-verified ZEX artifacts")

set(VAEG_BUILD_DATE "unknown")
set(VAEG_BUILD_COMMIT "unknown")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(VAEG_GIT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/generated/git-empty-config")
file(WRITE "${VAEG_GIT_CONFIG}" "")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "GIT_CONFIG_GLOBAL=${VAEG_GIT_CONFIG}"
            "GIT_CONFIG_SYSTEM=${VAEG_GIT_CONFIG}"
            "${GIT_EXECUTABLE}" show -s --format=%H HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VAEG_BUILD_COMMIT_RESULT
        OUTPUT_VARIABLE VAEG_BUILD_COMMIT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "GIT_CONFIG_GLOBAL=${VAEG_GIT_CONFIG}"
            "GIT_CONFIG_SYSTEM=${VAEG_GIT_CONFIG}"
            "TZ=Asia/Tokyo"
            "${GIT_EXECUTABLE}" show -s
            --date=format-local:%Y%m%d --format=%cd HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VAEG_BUILD_DATE_RESULT
        OUTPUT_VARIABLE VAEG_BUILD_DATE_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(VAEG_BUILD_COMMIT_RESULT EQUAL 0
            AND VAEG_BUILD_COMMIT_OUTPUT MATCHES "^[0-9a-f]+$")
        set(VAEG_BUILD_COMMIT "${VAEG_BUILD_COMMIT_OUTPUT}")
    endif()
    if(VAEG_BUILD_DATE_RESULT EQUAL 0
            AND VAEG_BUILD_DATE_OUTPUT MATCHES "^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$")
        set(VAEG_BUILD_DATE "${VAEG_BUILD_DATE_OUTPUT}")
    endif()
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vaeg_build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_build_info.h"
    @ONLY)

if(VAEG_ENABLE_TESTS AND NOT VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    message(FATAL_ERROR
        "VAEG_ENABLE_TESTS requires VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON; "
        "the trace-equivalence test cannot use an uninstrumented executable")
endif()

if(VAEG_FETCH_SDL2)
    include(FetchContent)
    if(VAEG_STATIC_SDL2)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    endif()
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        URL "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz"
        URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL2)
    if(VAEG_STATIC_SDL2)
        if(NOT TARGET SDL2::SDL2-static)
            message(FATAL_ERROR "Fetched SDL2 did not provide SDL2::SDL2-static")
        endif()
        set(VAEG_SDL2_LIBS SDL2::SDL2-static)
    else()
        set(VAEG_SDL2_LIBS SDL2::SDL2)
    endif()
else()
    find_package(SDL2 QUIET CONFIG)
    if(VAEG_STATIC_SDL2 AND TARGET SDL2::SDL2-static)
        set(VAEG_SDL2_LIBS SDL2::SDL2-static)
    elseif(VAEG_STATIC_SDL2)
        message(FATAL_ERROR
            "VAEG_STATIC_SDL2 requires SDL2::SDL2-static; use VAEG_FETCH_SDL2=ON")
    elseif(TARGET SDL2::SDL2)
        set(VAEG_SDL2_LIBS SDL2::SDL2)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        set(VAEG_SDL2_LIBS PkgConfig::SDL2)
    endif()
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VAEG_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/machine"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpu/upd9002"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpu"
    "${CMAKE_CURRENT_SOURCE_DIR}/io"
    "${CMAKE_CURRENT_SOURCE_DIR}/cbus"
    "${CMAKE_CURRENT_SOURCE_DIR}/bios"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/vermouth"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/getsnd"
    "${CMAKE_CURRENT_SOURCE_DIR}/vram"
    "${CMAKE_CURRENT_SOURCE_DIR}/fdd"
    "${CMAKE_CURRENT_SOURCE_DIR}/font"
    "${CMAKE_CURRENT_SOURCE_DIR}/memoryva"
    "${CMAKE_CURRENT_SOURCE_DIR}/io"
    "${CMAKE_CURRENT_SOURCE_DIR}/generic")

set(VAEG_COMMON_SOURCES
    common/strres.c
    common/milstr.c
    common/_memory.c
    common/textfile.c
    common/profile.c
    common/rect.c
    common/lstarray.c
    common/bmpdata.c
    common/codecnv.c
    common/parts.c
    common/resize.c
    generic/cmjasts.c
    generic/cmver.c
    generic/np2info.c)

set(VAEG_CORE_SOURCES
    cpu/upd9002/upd9002_core.c
	cpu/upd9002/upd9002_diagnostic.c
	diagnostics/upd9002_debug.c
    cpu/upd9002/upd9002_state.c
    cpu/upd9002/upd9002_mn.c
    cpu/upd9002/upd9002_ea.c
    cpu/upd9002/upd9002_8x.c
    cpu/upd9002/upd9002_sf.c
    cpu/upd9002/upd9002_f6.c
    cpu/upd9002/upd9002_fe.c
    cpu/upd9002/upd9002_rp.c
    cpu/upd9002/memory.c
    cpu/upd9002/dmap.c
    io/iocore.c
    io/bmsio.c
    io/dmac.c
    io/emsio.c
    io/fdc.c
    io/hostfat.c
    io/mouseif.c
    io/np2sysp.c
    io/pic.c
    io/pit.c
    io/serial.c
    io/upd4990.c
    cbus/cbuscore.c
    cbus/sasiio.c
    cbus/scsiio.c
    cbus/scsicmd.c
    cbus/mpu98ii.c
    sound/vermouth/midiout.c
    sound/vermouth/midimod.c
    sound/vermouth/midinst.c
    sound/vermouth/midvoice.c
    sound/vermouth/midtable.c
    sound/getsnd/getsnd.c
    sound/getsnd/getwave.c
    sound/getsnd/getsmix.c
    bios/sxsibios.c
    sound/sound.c
    sound/sndcsec.c
    sound/fmboard.c
    sound/fmtimer.c
    sound/beepc.c
    sound/beepg.c
    sound/opngenc.c
    sound/opngeng.c
    sound/psggenc.c
    sound/psggeng.c
    sound/rhythmc.c
    sound/adpcmc.c
    sound/adpcmg.c
    sound/soundrom.c
    fdd/diskdrv.c
    fdd/newdisk.c
    fdd/fddfile.c
    fdd/fdd_xdf.c
    fdd/fdd_d88.c
    fdd/fdd_mtr.c
    fdd/sxsi.c
    font/font.c
    font/fontdata.c
    font/fontpc88.c
    font/fontpc98.c
    font/fontv98.c
    font/fontmake.c
    machine/pccore.c
    machine/nevent.c
    machine/calendar.c
    machine/timing.c
    machine/keystat.c
    machine/statsave.c
    machine/debugsub.c)

set(VAEG_Z80_COMPAT_PRODUCTION_SOURCES
    cpu/upd780/upd780_disasm.cpp
    cpu/z80_compat_cpu.cpp
    cpu/z80_compat_state.cpp
    cpu/upd9002_upd70008.cpp)
message(STATUS "Production uPD70008-compatible core: shared Z80 compatibility backend")

set(VAEG_VA_SOURCES
    bios/romva.c
    memoryva/gvramva.c
    memoryva/memoryva.c
    ${VAEG_Z80_COMPAT_PRODUCTION_SOURCES}
    io/bkupmemva.c
    io/boardsb2.c
    io/cgromva.c
    io/fdsubsys.c
    io/gactrlva.c
    io/i8255.c
    io/memctrlva.c
    io/mouseifva.c
    io/sgp.c
    io/subsystem.cpp
    io/subsystemif.c
    io/subsystemmx.c
    io/sysportva.c
    io/tsp.c
    io/upd9002_regs.c
    io/va91.c
    io/videova.c
    vram/makegrphva.c
    vram/makesprva.c
    vram/maketextva.c
    vram/scrndrawva.c
    vram/sdrawva.c)

set(VAEG_SDL2_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_sdlrenderer2.cpp
    external/ymfm/src/ymfm_adpcm.cpp
    external/ymfm/src/ymfm_opn.cpp
    external/ymfm/src/ymfm_ssg.cpp
    sdl2/np2.c
    sdl2/headless_input.c
    sdl2/debug_harness.c
    sdl2/g75_screen.c
    sdl2/cliopts.c
    sdl2/screenshot.c
    sdl2/dosio.c
    sdl2/ini.c
    sdl2/taskmng.c
    sdl2/pacing.c
    sdl2/framedisp.c
    sdl2/viewport.c
    sdl2/appicon.c
    sdl2/splash.c
    sdl2/sysmng.c
    sdl2/timemng.c
    sdl2/trace.c
    sdl2/scrnmng.c
    sdl2/sdlkbd.c
    sdl2/kbdinject.c
    sdl2/kbdmap.c
    sdl2/kbdpaste.c
    sdl2/romankana.c
    sdl2/inputmng.c
    sdl2/dropmedia.cpp
    sdl2/hostfat_snapshot.cpp
    sdl2/hostfat_manager.cpp
    sdl2/romcheck.c
    sdl2/selftest.c
    sdl2/soundmng.c
    sdl2/soundopts.c
    sdl2/ymfmbridge.cpp
    sdl2/mousemng.c
    sdl2/mousestate.c
    sdl2/joymng.c
    sdl2/commng.c
    sdl2/fontmng.c
    sdl2/gui/gui.cpp)
if(VAEG_ENABLE_LIBRASHADER)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/librashader/librashader_loader.cpp
        sdl2/librashader/librashader_gl_bridge.cpp)
endif()
if(VAEG_ENABLE_TESTS)
    list(APPEND VAEG_SDL2_SOURCES
        tests/idp/m69_status_composition.c
        tests/upd780/subsystem_integration.cpp
        tests/upd9002/direct_harness.c
        tests/upd9002/brkem_upd70008.c
		tests/upd9002/dispatch_normalization.c
        tests/upd9002/flags_materialization.c
        tests/upd9002/iret_restoration.c
        tests/upd9002/m65a_ff7.c
        tests/upd9002/m65b_bound.c
        tests/upd9002/m65c_f72.c
        tests/upd9002/m65d_ff6.c
        tests/upd9002/m65e_tail10.c
        tests/upd9002/m96d_nop.c
        tests/upd9002/m68_segmented_memory.c
        tests/upd9002/m70_prefix_string.c
        tests/upd9002/m64_semantics.c
        tests/upd9002/mov_imm_register.c
        tests/upd9002/semantics_bundle.c
        tests/upd9002/fixtures.c
		tests/upd9002/rep0f_diagnostic_stop.c
        tests/upd9002/state_scenario.c
        tests/upd9002/statsave_boundary.c
        tests/upd9002/ssts_worker.c)
endif()

set(VAEG_SPLASH_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_splash_data.c")
add_custom_command(
    OUTPUT "${VAEG_SPLASH_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.bmp"
        "-DOUTPUT=${VAEG_SPLASH_DATA}"
        "-DSYMBOL=vaeg_splash_bmp"
        "-DSOURCE_LABEL=assets/vaeg.bmp"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.bmp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_SPLASH_DATA}")

set(VAEG_GUI_FONT_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_gui_font_data.c")
add_custom_command(
    OUTPUT "${VAEG_GUI_FONT_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSansJP-Regular.ttf"
        "-DOUTPUT=${VAEG_GUI_FONT_DATA}"
        "-DSYMBOL=vaeg_gui_font_ttf"
        "-DSOURCE_LABEL=assets/NotoSansJP-Regular.ttf"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSansJP-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_GUI_FONT_DATA}")

set(VAEG_APP_ICON_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_app_icon_data.c")
add_custom_command(
    OUTPUT "${VAEG_APP_ICON_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.ico"
        "-DOUTPUT=${VAEG_APP_ICON_DATA}"
        "-DSYMBOL=vaeg_app_icon_ico"
        "-DSOURCE_LABEL=assets/vaeg.ico"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_APP_ICON_DATA}")

find_program(VAEG_NASM_EXECUTABLE NAMES nasm)
set(VAEG_HOSTFAT_SYS "${CMAKE_CURRENT_BINARY_DIR}/guest/hostfat.sys")
set(VAEG_R2FPROBE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/r2fprobe.com")
set(VAEG_SGPSPRITE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgpdemo.com")
set(VAEG_SGPWIRE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgpwire.com")
set(VAEG_SGP256_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgp256.com")
if(VAEG_NASM_EXECUTABLE)
    add_custom_command(
        OUTPUT "${VAEG_HOSTFAT_SYS}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_HOSTFAT_SYS}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/hostfat.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/hostfat.asm"
        VERBATIM)
    add_custom_target(hostfat_sys ALL DEPENDS "${VAEG_HOSTFAT_SYS}")
    add_custom_command(
        OUTPUT "${VAEG_R2FPROBE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_R2FPROBE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/r2fprobe.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/r2fprobe.asm"
        VERBATIM)
    add_custom_target(r2fprobe_com ALL DEPENDS "${VAEG_R2FPROBE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGPSPRITE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGPSPRITE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-pseudo-sprite/sgp_sprite.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-pseudo-sprite/sgp_sprite.asm"
        VERBATIM)
    add_custom_target(sgpsprite_com ALL DEPENDS "${VAEG_SGPSPRITE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGPWIRE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGPWIRE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe.asm"
        VERBATIM)
    add_custom_target(sgpwire_com ALL DEPENDS "${VAEG_SGPWIRE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGP256_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGP256_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe256.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe256.asm"
        VERBATIM)
    add_custom_target(sgp256_com ALL DEPENDS "${VAEG_SGP256_COM}")
    message(STATUS "HOSTFAT.SYS: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "R2FPROBE.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGPDEMO.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGPWIRE.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGP256.COM: enabled (${VAEG_NASM_EXECUTABLE})")
else()
    message(STATUS "HOSTFAT.SYS: NASM not found; guest-driver target disabled")
    message(STATUS "R2FPROBE.COM: NASM not found; guest-probe target disabled")
    message(STATUS "SGPDEMO.COM: NASM not found; guest-demo target disabled")
    message(STATUS "SGPWIRE.COM: NASM not found; guest-demo target disabled")
    message(STATUS "SGP256.COM: NASM not found; guest-demo target disabled")
endif()

if(WIN32)
    enable_language(RC)
    list(APPEND VAEG_SDL2_SOURCES sdl2/vaeg.rc)
endif()

add_library(vaeg_common STATIC ${VAEG_COMMON_SOURCES})
target_include_directories(vaeg_common PUBLIC ${VAEG_INCLUDE_DIRS})
target_compile_options(vaeg_common PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_common PRIVATE -Werror)
endif()

if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    list(APPEND VAEG_CORE_SOURCES cpu/upd9002/upd9002_trace.c)
endif()
if(VAEG_UPD9002_PERF_DIAGNOSTIC)
    list(APPEND VAEG_CORE_SOURCES cpu/upd9002/upd9002_perf.c)
endif()

add_library(vaeg_core STATIC ${VAEG_CORE_SOURCES})
target_include_directories(vaeg_core PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_core PUBLIC vaeg_common)
target_compile_definitions(vaeg_core PUBLIC
    SUPPORT_V30ORIGINAL)
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_core PRIVATE
        VAEG_UPD9002_M42_TESTING=1
		VAEG_UPD9002_M46_TESTING=1
        VAEG_UPD9002_SSTS_TESTING=1)
endif()
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_core PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_UPD9002_PERF_DIAGNOSTIC)
    target_compile_definitions(vaeg_core PUBLIC
        VAEG_UPD9002_PERF_DIAGNOSTIC=1)
endif()
target_compile_options(vaeg_core PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_core PRIVATE -Werror)
endif()

add_library(vaeg_va STATIC ${VAEG_VA_SOURCES})
target_include_directories(vaeg_va PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_va PUBLIC vaeg_core vaeg_common)
target_compile_options(vaeg_va PRIVATE -Wall -fsigned-char)
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_va PRIVATE VAEG_UPD9002_M76_TESTING=1)
endif()
target_include_directories(vaeg_va PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
target_compile_definitions(vaeg_va PRIVATE
    Z80_DISABLE_BREAKPOINT
    Z80_DISABLE_DEBUG
    Z80_DISABLE_NESTCHECK
    Z80_NO_EXCEPTION
    Z80_UNSUPPORT_16BIT_PORT)
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_va PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_va PUBLIC
        VAEG_UPD780_INTEGRATION_TESTING=1)
endif()
if(VAEG_WERROR)
    target_compile_options(vaeg_va PRIVATE -Werror)
endif()

set(VAEG_SDL2_EXE_OPTIONS)
if(WIN32 AND NOT VAEG_WINDOWS_CONSOLE)
    list(APPEND VAEG_SDL2_EXE_OPTIONS WIN32)
endif()
add_executable(vaeg_sdl2 ${VAEG_SDL2_EXE_OPTIONS} ${VAEG_SDL2_SOURCES})
set_target_properties(vaeg_sdl2 PROPERTIES
    OUTPUT_NAME vaeg
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl2")
target_include_directories(vaeg_sdl2 PRIVATE
    ${VAEG_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ymfm/src")
if(VAEG_ENABLE_LIBRASHADER)
    find_package(OpenGL REQUIRED)
    target_include_directories(vaeg_sdl2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/librashader/include")
    target_compile_definitions(vaeg_sdl2 PRIVATE LIBRA_RUNTIME_OPENGL=1)
    target_link_libraries(vaeg_sdl2 PRIVATE OpenGL::GL)
endif()
target_link_libraries(vaeg_sdl2 PRIVATE vaeg_core vaeg_va vaeg_common
    vaeg_core vaeg_va ${VAEG_SDL2_LIBS})
if(VAEG_ENABLE_LIBRASHADER)
    target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_ENABLE_LIBRASHADER=1)
    message(STATUS "librashader CRT presentation: enabled")
else()
    message(STATUS "librashader CRT presentation: disabled")
endif()
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_sdl2 PRIVATE
        VAEG_UPD9002_M42_TESTING=1
        VAEG_UPD9002_M76_TESTING=1
        VAEG_UPD9002_M44_TESTING=1
		VAEG_UPD9002_M46_TESTING=1
        VAEG_UPD9002_M48_TESTING=1
        VAEG_UPD9002_M60A_TESTING=1
        VAEG_UPD9002_M60E_TESTING=1
        VAEG_UPD9002_M61_TESTING=1
        VAEG_UPD9002_M62_TESTING=1
        VAEG_UPD9002_M64_TESTING=1
        VAEG_UPD9002_M65A_TESTING=1
        VAEG_UPD9002_M65B_TESTING=1
        VAEG_UPD9002_M65C_TESTING=1
        VAEG_UPD9002_M65D_TESTING=1
        VAEG_UPD9002_M65E_TESTING=1
        VAEG_UPD9002_M96D_TESTING=1
        VAEG_UPD9002_M68_TESTING=1
        VAEG_UPD9002_M70_TESTING=1
        VAEG_IDP_M69_TESTING=1
        VAEG_UPD9002_SSTS_TESTING=1
        "VAEG_UPD9002_HARNESS_MANIFEST_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/harness_manifest.csv\""
        "VAEG_UPD9002_FIXTURE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/state_fixtures_m42.txt\"")
endif()
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_sdl2 PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_ENABLE_ARCHIVE_DROP)
    if(MINGW OR VAEG_FETCH_LIBARCHIVE)
        include(FetchContent)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_zlib
            URL "https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz"
            URL_HASH SHA256=38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(vaeg_zlib)
        configure_file("${vaeg_zlib_SOURCE_DIR}/zlib.h"
            "${vaeg_zlib_BINARY_DIR}/zlib.h" COPYONLY)

        set(XZ_NLS OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_XZ OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_XZDEC OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_LZMADEC OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_LZMAINFO OFF CACHE BOOL "" FORCE)
        set(XZ_DOC OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_xz
            URL "https://github.com/tukaani-project/xz/releases/download/v5.8.3/xz-5.8.3.tar.xz"
            URL_HASH SHA256=fff1ffcf2b0da84d308a14de513a1aa23d4e9aa3464d17e64b9714bfdd0bbfb6
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(vaeg_xz)

        set(ZLIB_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
        set(ZLIB_INCLUDE_DIR "${vaeg_zlib_BINARY_DIR}" CACHE PATH "" FORCE)
        if(WIN32)
            set(VAEG_ZLIB_STATIC "${vaeg_zlib_BINARY_DIR}/libzlibstatic.a")
        else()
            set(VAEG_ZLIB_STATIC "${vaeg_zlib_BINARY_DIR}/libz.a")
        endif()
        set(ZLIB_LIBRARY "${VAEG_ZLIB_STATIC}" CACHE FILEPATH "" FORCE)
        set(LIBLZMA_INCLUDE_DIR "${vaeg_xz_SOURCE_DIR}/src/liblzma/api"
            CACHE PATH "" FORCE)
        set(LIBLZMA_LIBRARY "${vaeg_xz_BINARY_DIR}/liblzma.a"
            CACHE FILEPATH "" FORCE)
        # FindLibLZMA cannot link-test a same-configure FetchContent target.
        # The pinned liblzma target exports these API functions once built.
        set(LIBLZMA_HAS_AUTO_DECODER TRUE CACHE BOOL "" FORCE)
        set(LIBLZMA_HAS_EASY_ENCODER TRUE CACHE BOOL "" FORCE)
        set(LIBLZMA_HAS_LZMA_PRESET TRUE CACHE BOOL "" FORCE)
        set(LZMA_API_STATIC TRUE CACHE BOOL "" FORCE)
        set(WITHOUT_LZMA_API_STATIC FALSE CACHE BOOL "" FORCE)

        set(ENABLE_WERROR OFF CACHE BOOL "" FORCE)
        set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
        set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
        set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
        set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
        set(ENABLE_UNZIP OFF CACHE BOOL "" FORCE)
        set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
        set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
        set(ENABLE_NETTLE OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBB2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZ4 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZO OFF CACHE BOOL "" FORCE)
        set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
        set(ENABLE_BZip2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_EXPAT OFF CACHE BOOL "" FORCE)
        set(ENABLE_WIN32_XMLLITE OFF CACHE BOOL "" FORCE)
        set(ENABLE_PCREPOSIX OFF CACHE BOOL "" FORCE)
        set(ENABLE_PCRE2POSIX OFF CACHE BOOL "" FORCE)
        set(ENABLE_XATTR OFF CACHE BOOL "" FORCE)
        set(ENABLE_ACL OFF CACHE BOOL "" FORCE)
        set(ENABLE_ICONV OFF CACHE BOOL "" FORCE)
        set(ENABLE_CNG OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBGCC OFF CACHE BOOL "" FORCE)
        set(POSIX_REGEX_LIB NONE CACHE STRING "" FORCE)
        set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
        set(ENABLE_LZMA ON CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_libarchive
            URL "https://github.com/libarchive/libarchive/releases/download/v3.8.7/libarchive-3.8.7.tar.xz"
            URL_HASH SHA256=d3a8ba457ae25c27c84fd2830a2efdcc5b1d40bf585d4eb0d35f47e99e5d4774
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        # LibArchive's Unix type probes expect the platform's C extensions.
        # Keep vaeg's own C targets on strict C99 outside this dependency.
        set(VAEG_SAVED_CMAKE_C_EXTENSIONS ${CMAKE_C_EXTENSIONS})
        set(CMAKE_C_EXTENSIONS ON)
        FetchContent_MakeAvailable(vaeg_libarchive)
        set(CMAKE_C_EXTENSIONS ${VAEG_SAVED_CMAKE_C_EXTENSIONS})
        add_dependencies(archive_static zlibstatic liblzma)
        target_compile_definitions(vaeg_sdl2 PRIVATE
            VAEG_HAVE_LIBARCHIVE LIBARCHIVE_STATIC)
        target_link_libraries(vaeg_sdl2 PRIVATE archive_static)
        message(STATUS
            "Archive disk-image drop: enabled (static fetched LibArchive 3.8.7)")
    else()
        find_package(LibArchive QUIET)
        if(LibArchive_FOUND)
            target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_HAVE_LIBARCHIVE)
            if(TARGET LibArchive::LibArchive)
                target_link_libraries(vaeg_sdl2 PRIVATE LibArchive::LibArchive)
            else()
                target_include_directories(vaeg_sdl2 PRIVATE ${LibArchive_INCLUDE_DIRS})
                target_link_libraries(vaeg_sdl2 PRIVATE ${LibArchive_LIBRARIES})
            endif()
            message(STATUS "Archive disk-image drop: enabled (${LibArchive_VERSION})")
        else()
            message(STATUS "Archive disk-image drop: disabled (LibArchive not found)")
        endif()
    endif()
else()
    message(STATUS "Archive disk-image drop: disabled by VAEG_ENABLE_ARCHIVE_DROP")
endif()
if(MINGW)
    target_link_options(vaeg_sdl2 PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(vaeg_sdl2 PRIVATE winpthread)
endif()
if(UNIX)
    target_link_libraries(vaeg_sdl2 PRIVATE m)
endif()
if(VAEG_ENABLE_LIBRASHADER)
    target_link_libraries(vaeg_sdl2 PRIVATE ${CMAKE_DL_LIBS})
endif()
if(WIN32 AND VAEG_FETCH_SDL2 AND NOT VAEG_STATIC_SDL2)
    add_custom_command(TARGET vaeg_sdl2 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:vaeg_sdl2>")
endif()
target_compile_options(vaeg_sdl2 PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_sdl2 PRIVATE -Werror)
endif()

if(VAEG_ENABLE_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(VAEG_SSTS_V20_ROOT "" CACHE PATH
        "Verified external SingleStepTests V20 checkout for uPD9002 comparison")

    if(TARGET hostfat_sys)
        add_test(NAME vaeg_hostfat_driver
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/check_driver.py"
                --input "${VAEG_HOSTFAT_SYS}")
        set_tests_properties(vaeg_hostfat_driver PROPERTIES
            LABELS "romless;hostfat;pcengine")
    endif()

    if(TARGET r2fprobe_com)
        add_test(NAME vaeg_pcengine_redirector_probe
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/check_probe.py"
                --input "${VAEG_R2FPROBE_COM}")
        set_tests_properties(vaeg_pcengine_redirector_probe PROPERTIES
            LABELS "romless;hostfs;pcengine")
    endif()

    add_test(NAME vaeg_upd9002_dispatch_graph
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_dispatch_graph PROPERTIES
        LABELS "romless;upd9002;dispatch")

    add_test(NAME vaeg_upd9002_dispatch_generator_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(vaeg_upd9002_dispatch_generator_selftest PROPERTIES
        LABELS "romless;upd9002;dispatch")

    add_test(NAME vaeg_upd9002_native_invariant
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_native_invariant.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_native_invariant PROPERTIES
        LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_dispatch_normalization_static
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_dispatch_normalization.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_dispatch_normalization_static PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_dispatch_normalization
		COMMAND vaeg_sdl2 --upd9002-m46-dispatch-qa)
    set_tests_properties(vaeg_upd9002_dispatch_normalization PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_rep0f_diagnostic_stop
		COMMAND vaeg_sdl2 --upd9002-m48-rep0f-diagnostic)
    set_tests_properties(vaeg_upd9002_rep0f_diagnostic_stop PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_flags_materialization
		COMMAND vaeg_sdl2 --upd9002-m60a-flags-materialization)
    set_tests_properties(vaeg_upd9002_flags_materialization PROPERTIES
		LABELS "romless;upd9002;flags;semantics")

    add_test(NAME vaeg_upd9002_iret_restoration
		COMMAND vaeg_sdl2 --upd9002-m60e-iret-restoration)
    set_tests_properties(vaeg_upd9002_iret_restoration PROPERTIES
		LABELS "romless;upd9002;flags;iret;semantics")

    add_test(NAME vaeg_upd9002_brkem_upd70008
        COMMAND vaeg_sdl2 --upd9002-brkem-upd70008)
    set_tests_properties(vaeg_upd9002_brkem_upd70008 PROPERTIES
        LABELS "romless;upd9002;upd70008;compatibility;transitions")

    add_test(NAME vaeg_upd9002_mov_imm_register
		COMMAND vaeg_sdl2 --upd9002-m61-mov-imm-register)
    set_tests_properties(vaeg_upd9002_mov_imm_register PROPERTIES
		LABELS "romless;upd9002;mov;semantics")

    add_test(NAME vaeg_upd9002_semantics_bundle
		COMMAND vaeg_sdl2 --upd9002-m62-semantics-bundle)
    set_tests_properties(vaeg_upd9002_semantics_bundle PROPERTIES
		LABELS "romless;upd9002;m62;semantics")

    add_test(NAME vaeg_upd9002_m64_semantics
		COMMAND vaeg_sdl2 --upd9002-m64-semantics)
    set_tests_properties(vaeg_upd9002_m64_semantics PROPERTIES
		LABELS "romless;upd9002;m64;semantics")

    add_test(NAME vaeg_upd9002_m65a_ff7
		COMMAND vaeg_sdl2 --upd9002-m65a-ff7)
    set_tests_properties(vaeg_upd9002_m65a_ff7 PROPERTIES
		LABELS "romless;upd9002;m65a;semantics")

    add_test(NAME vaeg_upd9002_m65b_bound
		COMMAND vaeg_sdl2 --upd9002-m65b-bound)
    set_tests_properties(vaeg_upd9002_m65b_bound PROPERTIES
		LABELS "romless;upd9002;m65b;semantics")

    add_test(NAME vaeg_upd9002_m65c_f72
		COMMAND vaeg_sdl2 --upd9002-m65c-f72)
    set_tests_properties(vaeg_upd9002_m65c_f72 PROPERTIES
		LABELS "romless;upd9002;m65c;semantics")

    add_test(NAME vaeg_upd9002_m65d_ff6
		COMMAND vaeg_sdl2 --upd9002-m65d-ff6)
    set_tests_properties(vaeg_upd9002_m65d_ff6 PROPERTIES
		LABELS "romless;upd9002;m65d;semantics")
    add_test(NAME vaeg_upd9002_m65e_tail10
		COMMAND vaeg_sdl2 --upd9002-m65e-tail10)
    set_tests_properties(vaeg_upd9002_m65e_tail10 PROPERTIES
		LABELS "romless;upd9002;m65e;semantics")

    add_test(NAME vaeg_upd9002_m96d_nop
		COMMAND vaeg_sdl2 --upd9002-m96d-nop)
    set_tests_properties(vaeg_upd9002_m96d_nop PROPERTIES
		LABELS "romless;upd9002;m96d;semantics")

    add_test(NAME vaeg_upd9002_m68_segmented_memory
		COMMAND vaeg_sdl2 --upd9002-m68-segmented-memory)
    set_tests_properties(vaeg_upd9002_m68_segmented_memory PROPERTIES
		LABELS "romless;upd9002;m68;memory;semantics")

    add_test(NAME vaeg_idp_m69_status_composition
		COMMAND vaeg_sdl2 --idp-m69-status-composition)
    set_tests_properties(vaeg_idp_m69_status_composition PROPERTIES
		LABELS "romless;idp;tsp;m69")

    add_test(NAME vaeg_m75_scsi_controller
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_scsi_controller.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_m75_scsi_controller PROPERTIES
		LABELS "romless;scsi;m75")

    add_test(NAME vaeg_m75_transfer_info
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_transfer_info.py"
            --selftest)
    set_tests_properties(vaeg_m75_transfer_info PROPERTIES
        LABELS "romless;scsi;m75;transfer-info")

    add_test(NAME vaeg_m75_fat_inspection
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/test_inspect_vaeg_fat.py"
            -v)
    set_tests_properties(vaeg_m75_fat_inspection PROPERTIES
        LABELS "romless;scsi;m75;fat")

    add_test(NAME vaeg_m75_storage_regression_fixtures
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_storage_regression.py"
            --selftest)
    set_tests_properties(vaeg_m75_storage_regression_fixtures PROPERTIES
        LABELS "romless;sasi;hostfat;m75;storage")

    add_test(NAME vaeg_upd9002_m70_prefix_string
		COMMAND vaeg_sdl2 --upd9002-m70-prefix-string)
    set_tests_properties(vaeg_upd9002_m70_prefix_string PROPERTIES
		LABELS "romless;upd9002;m70;semantics")

    add_test(NAME vaeg_upd9002_rep0f_transition_selftest
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(vaeg_upd9002_rep0f_transition_selftest PROPERTIES
		LABELS "romless;upd9002;dispatch;state;ssts")

    add_test(NAME vaeg_upd9002_rep0f_transition_static
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_rep0f_transition_static PROPERTIES
		LABELS "romless;upd9002;dispatch;state;ssts")

    add_test(NAME vaeg_upd9002_protected_deletion
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_protected_deletion.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_protected_deletion PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_protected_deletion_selftest
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_protected_deletion.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(
		vaeg_upd9002_protected_deletion_selftest PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_rename
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rename.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_rename PROPERTIES
		LABELS "romless;upd9002;repository")

    add_executable(vaeg_upd9002_abi tests/upd9002/abi.c)
    target_include_directories(vaeg_upd9002_abi PRIVATE ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_abi PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_abi PRIVATE -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_abi
        COMMAND vaeg_upd9002_abi
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/abi_g41.txt")
    set_tests_properties(vaeg_upd9002_abi PROPERTIES
        LABELS "romless;upd9002;state")

    add_executable(vaeg_upd9002_state_boundary
        tests/upd9002/state_boundary.c
        cpu/upd9002/upd9002_state.c)
    target_include_directories(vaeg_upd9002_state_boundary PRIVATE
        ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_state_boundary PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_state_boundary PRIVATE
        -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_state_boundary
        COMMAND vaeg_upd9002_state_boundary)
    set_tests_properties(vaeg_upd9002_state_boundary PROPERTIES
        LABELS "romless;upd9002;state")

    add_executable(vaeg_upd9002_state_payload_probe
        tests/upd9002/state_payload_probe.c
        cpu/upd9002/upd9002_state.c)
    target_include_directories(vaeg_upd9002_state_payload_probe PRIVATE
        ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_state_payload_probe PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_state_payload_probe PRIVATE
        -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_state_payload_probe
        COMMAND vaeg_upd9002_state_payload_probe
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/state_fixtures_m42.txt")
    set_tests_properties(vaeg_upd9002_state_payload_probe PROPERTIES
        LABELS "romless;upd9002;state")

    add_test(NAME vaeg_upd9002_m66a_state_verify
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m66_state.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" verify-m66a)
    set_tests_properties(vaeg_upd9002_m66a_state_verify PROPERTIES
        LABELS "romless;upd9002;state")

    add_test(NAME vaeg_upd9002_trace_equivalence
        COMMAND "${CMAKE_COMMAND}"
            "-DVAEG_EXECUTABLE=$<TARGET_FILE:vaeg_sdl2>"
            "-DTRACE_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/trace_baseline.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/run_trace_equivalence.cmake")
    set_tests_properties(vaeg_upd9002_trace_equivalence PROPERTIES
        # This invokes the complete selftest three times.  The M55 64 MiB
        # snapshot identity checks are intentionally expensive under ASan.
        LABELS "romless;upd9002;trace;harness" TIMEOUT 420)

    if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
        add_test(NAME vaeg_m74_debug_harness
            COMMAND "${CMAKE_COMMAND}"
                "-DVAEG_EXECUTABLE=$<TARGET_FILE:vaeg_sdl2>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/m74/run_debug_harness.cmake")
        set_tests_properties(vaeg_m74_debug_harness PROPERTIES
            LABELS "romless;upd9002;trace;harness;m74" TIMEOUT 60)
    endif()

    add_test(NAME vaeg_upd9002_ssts_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
            selftest)
    set_tests_properties(vaeg_upd9002_ssts_selftest PROPERTIES
        LABELS "romless;upd9002;ssts")

    add_test(NAME vaeg_upd9002_ssts_ratchet_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts_ratchet.py"
            selftest)
    set_tests_properties(vaeg_upd9002_ssts_ratchet_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;ratchet")

    add_test(NAME vaeg_upd9002_ssts_ratchet_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts_ratchet.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_ssts_ratchet_static PROPERTIES
        LABELS "romless;upd9002;ssts;ratchet;repository")

    add_test(NAME vaeg_upd9002_m60a_evidence_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60a_evidence.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m60a_evidence_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;ratchet;flags")

    add_test(NAME vaeg_upd9002_m60a_evidence_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60a_evidence.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --predecessor-sha
            "e7f2325bc81310532091a8ca82914030fdb8b6ba")
    set_tests_properties(vaeg_upd9002_m60a_evidence_static PROPERTIES
        LABELS "romless;upd9002;ssts;ratchet;flags;repository")

    add_test(NAME vaeg_upd9002_m60b_authority_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60b_authority.py"
            selftest
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m60b_authority_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;ratchet;authority;target-policy")

    add_test(NAME vaeg_upd9002_m60b_authority_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60b_authority.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --protected-evidence-only)
    set_tests_properties(vaeg_upd9002_m60b_authority_static PROPERTIES
        LABELS
            "romless;upd9002;ssts;ratchet;authority;target-policy;repository")

    add_test(NAME vaeg_upd9002_m60c_erratum_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60c_erratum.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m60c_erratum_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;authority;documentation")

    add_test(NAME vaeg_upd9002_m60c_erratum_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60c_erratum.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m60c_erratum_static PROPERTIES
        LABELS "romless;upd9002;ssts;authority;documentation;repository")

    add_test(NAME vaeg_upd9002_m60c_audit_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60c_audit.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m60c_audit_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;authority;fpu;target-policy")

    add_test(NAME vaeg_upd9002_m60c_audit_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60c_audit.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --protected-evidence-only)
    set_tests_properties(vaeg_upd9002_m60c_audit_static PROPERTIES
        LABELS
            "romless;upd9002;ssts;authority;fpu;target-policy;repository")

    add_test(NAME vaeg_upd9002_m60d_frame_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60d_frame_audit.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m60d_frame_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;interrupt-frame")

    add_test(NAME vaeg_upd9002_m60d_frame_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60d_frame_audit.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --protected-evidence-only)
    set_tests_properties(vaeg_upd9002_m60d_frame_static PROPERTIES
        LABELS
            "romless;upd9002;ssts;evidence;interrupt-frame;repository")

    add_test(NAME vaeg_upd9002_m60e_iret_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60e_iret.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m60e_iret_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;iret;flags")

    add_test(NAME vaeg_upd9002_m60e_iret_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m60e_iret.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --protected-evidence-only)
    set_tests_properties(vaeg_upd9002_m60e_iret_static PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;iret;flags;repository")

    add_test(NAME vaeg_upd9002_m61_mov_imm_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m61_mov_imm.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m61_mov_imm_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;mov;semantics")

    add_test(NAME vaeg_upd9002_m61_mov_imm_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m61_mov_imm.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m61_mov_imm_static PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;mov;semantics;repository")

    add_test(NAME vaeg_upd9002_m62_bundle_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m62_bundle.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m62_bundle_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;m62;semantics")

    add_test(NAME vaeg_upd9002_m62_bundle_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m62_bundle.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m62_bundle_static PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;m62;semantics;repository")

    add_test(NAME vaeg_upd9002_m64_expanded_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m64_expanded.py"
            selftest)
    set_tests_properties(vaeg_upd9002_m64_expanded_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;m64;semantics;target-policy")

    add_test(NAME vaeg_upd9002_m64_expanded_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m64_expanded.py"
            verify-static
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m64_expanded_static PROPERTIES
        LABELS
            "romless;upd9002;ssts;evidence;m64;semantics;target-policy;repository")

    add_test(NAME vaeg_upd9002_m70_prefix_string_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m70_prefix_string.py"
            selftest
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m70_prefix_string_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence;m70;semantics;target-policy")

    add_test(NAME vaeg_upd9002_m70_prefix_string_static
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m70_prefix_string.py"
            verify
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_m70_prefix_string_static PROPERTIES
        LABELS
            "romless;upd9002;ssts;evidence;m70;semantics;target-policy;repository")

    add_test(NAME vaeg_upd9002_ssts_evidence_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_semantics_evidence.py"
            selftest)
    set_tests_properties(vaeg_upd9002_ssts_evidence_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;evidence")

    if(EXISTS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/ssts/evidence/g59/manifest.json")
        add_test(NAME vaeg_upd9002_ssts_evidence_static
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_semantics_evidence.py"
                verify-static
                --root "${CMAKE_CURRENT_SOURCE_DIR}")
        set_tests_properties(vaeg_upd9002_ssts_evidence_static PROPERTIES
            LABELS "romless;upd9002;ssts;evidence;repository")
    endif()

    add_test(NAME vaeg_milestone_id_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/milestone_ids.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --selftest --discover --audit)
    set_tests_properties(vaeg_milestone_id_selftest PROPERTIES
        LABELS "romless;repository;milestone")

    add_test(NAME vaeg_upd9002_ssts_worker_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
            worker-selftest
            --worker "$<TARGET_FILE:vaeg_sdl2>")
    set_tests_properties(vaeg_upd9002_ssts_worker_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;harness" TIMEOUT 60)

    foreach(profile ci full)
        add_test(NAME vaeg_upd9002_ssts_${profile}_baseline
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
                report
                --summary
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/ssts/baseline/v20_native_${profile}.json")
        set_tests_properties(vaeg_upd9002_ssts_${profile}_baseline PROPERTIES
            LABELS "romless;upd9002;ssts;baseline")
    endforeach()

    if(VAEG_SSTS_V20_ROOT)
        execute_process(
            COMMAND git rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE VAEG_SSTS_GIT_RESULT
            OUTPUT_VARIABLE VAEG_SSTS_EVALUATED_SHA
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(LENGTH "${VAEG_SSTS_EVALUATED_SHA}"
            VAEG_SSTS_EVALUATED_SHA_LENGTH)
        if(NOT VAEG_SSTS_GIT_RESULT EQUAL 0
                OR NOT VAEG_SSTS_EVALUATED_SHA MATCHES "^[0-9a-f]+$"
                OR NOT VAEG_SSTS_EVALUATED_SHA_LENGTH EQUAL 40)
            message(FATAL_ERROR
                "A precise Git HEAD is required for the SST ratchet")
        endif()
        add_test(NAME vaeg_upd9002_ssts_ci_external
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_g65m_ci.py"
                --root "${CMAKE_CURRENT_SOURCE_DIR}"
                --dataset-root "${VAEG_SSTS_V20_ROOT}"
                --worker "$<TARGET_FILE:vaeg_sdl2>"
                --evaluated-sha "${VAEG_SSTS_EVALUATED_SHA}"
                --output-root "${CMAKE_CURRENT_BINARY_DIR}/ssts")
        set_tests_properties(vaeg_upd9002_ssts_ci_external PROPERTIES
            LABELS "romless;upd9002;ssts;external;ratchet"
            TIMEOUT 1800)
    else()
        add_test(NAME vaeg_upd9002_ssts_ci_external
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
                external-skip
                --reason "VAEG_SSTS_V20_ROOT is not configured")
        set_tests_properties(vaeg_upd9002_ssts_ci_external PROPERTIES
            LABELS "romless;upd9002;ssts;external"
            SKIP_RETURN_CODE 77)
    endif()

    function(vaeg_configure_z80_compat_standalone_target target)
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
        target_compile_options(${target} PRIVATE -Wall -fsigned-char)
        if(VAEG_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endfunction()

    set(VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE tests/z80_compat/header_compile.cpp)
    add_executable(vaeg_z80_compat_header_default
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_default)

    add_executable(vaeg_z80_compat_header_no_functional
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_no_functional)
    target_compile_definitions(vaeg_z80_compat_header_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    add_executable(vaeg_z80_compat_header_debug_default
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_debug_default)
    target_compile_definitions(vaeg_z80_compat_header_debug_default PRIVATE
        VAEG_Z80_COMPAT_REQUIRE_DEBUG)

    add_executable(vaeg_z80_compat_header_release
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_release)
    target_compile_definitions(vaeg_z80_compat_header_release PRIVATE
        VAEG_Z80_COMPAT_REQUIRE_RELEASE
        Z80_DISABLE_DEBUG
        Z80_DISABLE_BREAKPOINT
        Z80_DISABLE_NESTCHECK
        Z80_NO_EXCEPTION)

    add_executable(vaeg_z80_compat_interrupt_default
        external/suzukiplan-z80/test/test-interrupt-extension.cpp)
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_interrupt_default)

    add_executable(vaeg_z80_compat_interrupt_no_functional
        external/suzukiplan-z80/test/test-interrupt-extension.cpp)
    vaeg_configure_z80_compat_standalone_target(
        vaeg_z80_compat_interrupt_no_functional)
    target_compile_definitions(vaeg_z80_compat_interrupt_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    function(vaeg_add_z80_compat_wrapper_library target)
        add_library(${target} STATIC
            cpu/z80_compat_cpu.cpp
            cpu/z80_compat_state.cpp)
        target_include_directories(${target}
            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
        target_compile_features(${target} PUBLIC cxx_std_17)
        target_compile_options(${target} PRIVATE -Wall -fsigned-char)
        target_compile_definitions(${target} PRIVATE
            Z80_DISABLE_BREAKPOINT
            Z80_DISABLE_DEBUG
            Z80_DISABLE_NESTCHECK
            Z80_NO_EXCEPTION
            Z80_UNSUPPORT_16BIT_PORT)
        if(VAEG_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endfunction()

    vaeg_add_z80_compat_wrapper_library(vaeg_z80_compat_wrapper)
    vaeg_add_z80_compat_wrapper_library(vaeg_z80_compat_wrapper_no_functional)
    target_compile_definitions(vaeg_z80_compat_wrapper_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    add_executable(vaeg_z80_compat_wrapper_default tests/z80_compat/wrapper.cpp)
    target_link_libraries(vaeg_z80_compat_wrapper_default PRIVATE vaeg_z80_compat_wrapper)
    target_compile_options(vaeg_z80_compat_wrapper_default PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_wrapper_no_functional_test
        tests/z80_compat/wrapper.cpp)
    target_link_libraries(vaeg_z80_compat_wrapper_no_functional_test PRIVATE
        vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_wrapper_no_functional_test PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_wrapper_zex_runner
        tests/z80_compat/wrapper_zex_runner.cpp)
    target_link_libraries(vaeg_z80_compat_wrapper_zex_runner PRIVATE
        vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_wrapper_zex_runner PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_upd780_disasm
        cpu/upd780/upd780_disasm.cpp
        tests/upd780/disasm.cpp)
    target_include_directories(vaeg_upd780_disasm PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(vaeg_upd780_disasm PRIVATE cxx_std_17)
    target_compile_definitions(vaeg_upd780_disasm PRIVATE
        "VAEG_UPD780_DISASM_GOLDEN_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd780/disasm_golden.txt\"")
    target_compile_options(vaeg_upd780_disasm PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_upd780_disasm PRIVATE -Werror)
    endif()

    add_executable(vaeg_z80_compat_zex_runner tests/z80_compat/zex_runner.cpp)
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_zex_runner)
    target_compile_definitions(vaeg_z80_compat_zex_runner PRIVATE
        Z80_CALLBACK_PER_INSTRUCTION
        Z80_DISABLE_DEBUG
        Z80_DISABLE_NESTCHECK
        Z80_NO_FUNCTIONAL
        Z80_UNSUPPORT_16BIT_PORT)

    add_executable(vaeg_z80_compat_legacy_contract
        tests/z80_compat/legacy_contract.cpp)
    target_link_libraries(vaeg_z80_compat_legacy_contract PRIVATE
        vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_legacy_contract PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_z80_compat_legacy_contract PRIVATE -Werror)
    endif()

    add_library(vaeg_z80_compat_trace_common STATIC
        tests/z80_compat/differential/trace_common.cpp)
    target_include_directories(vaeg_z80_compat_trace_common PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential")
    target_compile_features(vaeg_z80_compat_trace_common PUBLIC cxx_std_17)
    target_compile_options(vaeg_z80_compat_trace_common PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_reference
        tests/z80_compat/differential/reference_trace.cpp)
    target_link_libraries(vaeg_z80_compat_trace_reference PRIVATE
        vaeg_z80_compat_trace_common vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_trace_reference PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_repeat
        tests/z80_compat/differential/new_trace.cpp)
    target_link_libraries(vaeg_z80_compat_trace_repeat PRIVATE
        vaeg_z80_compat_trace_common vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_trace_repeat PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_compare
        tests/z80_compat/differential/trace_compare.cpp)
    target_include_directories(vaeg_z80_compat_trace_compare PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential")
    target_compile_features(vaeg_z80_compat_trace_compare PRIVATE cxx_std_17)
    target_compile_options(vaeg_z80_compat_trace_compare PRIVATE
        -Wall -fsigned-char)

    if(VAEG_WERROR)
        foreach(target vaeg_z80_compat_trace_common vaeg_z80_compat_trace_reference
                       vaeg_z80_compat_trace_repeat
                       vaeg_z80_compat_trace_compare)
            target_compile_options(${target} PRIVATE -Werror)
        endforeach()
    endif()

    add_test(NAME vaeg_z80_compat_legacy_contract
        COMMAND vaeg_z80_compat_legacy_contract)
    add_test(NAME vaeg_z80_compat_header_default
        COMMAND vaeg_z80_compat_header_default)
    add_test(NAME vaeg_z80_compat_header_no_functional
        COMMAND vaeg_z80_compat_header_no_functional)
    add_test(NAME vaeg_z80_compat_header_debug_default
        COMMAND vaeg_z80_compat_header_debug_default)
    add_test(NAME vaeg_z80_compat_header_release
        COMMAND vaeg_z80_compat_header_release)
    add_test(NAME vaeg_z80_compat_interrupt_default
        COMMAND vaeg_z80_compat_interrupt_default)
    add_test(NAME vaeg_z80_compat_interrupt_no_functional
        COMMAND vaeg_z80_compat_interrupt_no_functional)
    add_test(NAME vaeg_z80_compat_wrapper_default
        COMMAND vaeg_z80_compat_wrapper_default)
    add_test(NAME vaeg_z80_compat_wrapper_no_functional
        COMMAND vaeg_z80_compat_wrapper_no_functional_test)
    add_test(NAME vaeg_upd780_disasm COMMAND vaeg_upd780_disasm)
    set_tests_properties(vaeg_upd780_disasm PROPERTIES
        LABELS "romless;upd780;disasm" TIMEOUT 120)
    foreach(regression_suite directed state generated convergence)
        add_test(NAME vaeg_z80_compat_regression_${regression_suite}
            COMMAND "${CMAKE_COMMAND}"
                "-DREFERENCE_RUNNER=$<TARGET_FILE:vaeg_z80_compat_trace_reference>"
                "-DREPEAT_RUNNER=$<TARGET_FILE:vaeg_z80_compat_trace_repeat>"
                "-DCOMPARATOR=$<TARGET_FILE:vaeg_z80_compat_trace_compare>"
                "-DSUITE=${regression_suite}"
                "-DCASES=128"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/compat-regression"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential/run_differential.cmake")
        set_tests_properties(vaeg_z80_compat_regression_${regression_suite}
            PROPERTIES LABELS "romless;compatibility;regression" TIMEOUT 120)
    endforeach()
    if(VAEG_ZEX_ARTIFACT_DIR)
        get_filename_component(VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE
            "${VAEG_ZEX_ARTIFACT_DIR}" ABSOLUTE)
        function(vaeg_verify_zex_artifact filename expected_sha256)
            set(artifact
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/${filename}")
            if(NOT EXISTS "${artifact}")
                message(FATAL_ERROR "Required ZEX artifact is absent: ${artifact}")
            endif()
            file(SHA256 "${artifact}" actual_sha256)
            if(NOT actual_sha256 STREQUAL expected_sha256)
                message(FATAL_ERROR
                    "ZEX artifact hash mismatch for ${artifact}: "
                    "expected ${expected_sha256}, got ${actual_sha256}")
            endif()
        endfunction()
        vaeg_verify_zex_artifact(zexdoc.cim
            b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90)
        vaeg_verify_zex_artifact(zexall.cim
            fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0)
        vaeg_verify_zex_artifact(zexdoc.src
            0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3)
        vaeg_verify_zex_artifact(zexall.src
            a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c)
        vaeg_verify_zex_artifact(zex-license.txt
            e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052)

        add_test(NAME vaeg_z80_compat_zexdoc
            COMMAND vaeg_z80_compat_zex_runner
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/zexdoc.cim")
        add_test(NAME vaeg_z80_compat_zexall
            COMMAND vaeg_z80_compat_zex_runner
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/zexall.cim")
        add_test(NAME vaeg_z80_compat_wrapper_zexdoc
            COMMAND vaeg_z80_compat_wrapper_zex_runner
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/zexdoc.cim")
        add_test(NAME vaeg_z80_compat_wrapper_zexall
            COMMAND vaeg_z80_compat_wrapper_zex_runner
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/zexall.cim")
        set_tests_properties(vaeg_z80_compat_zexdoc vaeg_z80_compat_zexall PROPERTIES
            LABELS "romless;compatibility;zex"
            TIMEOUT 900)
        set_tests_properties(
            vaeg_z80_compat_wrapper_zexdoc vaeg_z80_compat_wrapper_zexall PROPERTIES
            LABELS "romless;compatibility;zex;wrapper"
            TIMEOUT 900)
    endif()
    add_test(NAME vaeg_romless_tests COMMAND vaeg_sdl2 --selftest)
    set_tests_properties(vaeg_romless_tests PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        RESOURCE_LOCK vaeg_sxsi_image_selftest)
    add_test(NAME vaeg_m75_transfer_info_compiled COMMAND vaeg_sdl2 --selftest)
    set_tests_properties(vaeg_m75_transfer_info_compiled PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        LABELS "m75;scsi;compiled"
        RESOURCE_LOCK vaeg_sxsi_image_selftest)
endif()
