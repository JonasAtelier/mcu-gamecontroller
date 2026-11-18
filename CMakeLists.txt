# Read by ESP-IDF (and so esp-forge) as a component, and by any other CMake
# project that add_subdirectory()s this repo, such as a CubeMX-generated STM32
# one. stm-forge never reads it: it compiles every .c under src/, chip folders
# included, puts src/ on the include path of this library's sources and adds
# include/ -- which is why each port file, and gamecontroller_bt.c, is empty
# on the other platform. ESP_PLATFORM tells the two apart; IDF sets it in its
# early requirements pass as well as in the build.

if(NOT ESP_PLATFORM)
    # A static library `gamecontroller`. Every .c under src/, as stm-forge
    # does: a pad's parser compiles to nothing unless its
    # CONFIG_GAMECONTROLLER_<PAD> is 1, and the Bluetooth glue always does
    # here, so there is no source list here to keep in step with the IDF one
    # below.
    file(GLOB_RECURSE gamecontroller_sources CONFIGURE_DEPENDS src/*.c)
    add_library(gamecontroller STATIC ${gamecontroller_sources})
    # include/ for the app; src/ only for this library's own sources, so a
    # pad folder reaches gamecontroller_priv.h by plain name and no private
    # header is ever on the app's include path.
    target_include_directories(gamecontroller PUBLIC include PRIVATE src)

    # The options are the Kconfig bools, read from Kconfig so a new pad needs
    # no line here: a true CMake variable CONFIG_GAMECONTROLLER_<PAD> (or
    # CONFIG_GAMECONTROLLER_DEBUG) becomes CONFIG_GAMECONTROLLER_<PAD>=1, what
    # stm-forge passes. Only bools: an int option cannot be 1-or-absent, and
    # GAMECONTROLLER_MAX_PADS, the one int, only sizes the Bluetooth glue.
    # Each `config` line in Kconfig is followed directly by its type line,
    # which is what the match relies on. PUBLIC, because gamecontroller_log.h
    # reads CONFIG_GAMECONTROLLER_DEBUG in the app's sources.
    file(READ Kconfig gamecontroller_kconfig)
    string(REGEX MATCHALL "config GAMECONTROLLER_[A-Z0-9_]+[ \t\r\n]+bool"
           gamecontroller_options "${gamecontroller_kconfig}")
    list(TRANSFORM gamecontroller_options REPLACE
         "config (GAMECONTROLLER_[A-Z0-9_]+)[ \t\r\n]+bool" "CONFIG_\\1")
    set(gamecontroller_pads_on "")
    foreach(option IN LISTS gamecontroller_options)
        if(${option})
            target_compile_definitions(gamecontroller PUBLIC ${option}=1)
            if(NOT option STREQUAL "CONFIG_GAMECONTROLLER_DEBUG")
                list(APPEND gamecontroller_pads_on ${option})
            endif()
        endif()
    endforeach()
    if(NOT gamecontroller_pads_on)
        message(WARNING "gamecontroller: no pad is selected; "
                        "set(CONFIG_GAMECONTROLLER_<PAD> ON) before add_subdirectory()")
    endif()

    # The HAL is the app's. MCU_HAL_TARGET names a target carrying its include
    # dirs and defines (USE_HAL_DRIVER, the device, the hal_conf.h directory)
    # -- stm32cubemx in a CubeMX-generated project; MCU_DEVICE_HEADER is the
    # family header the library includes as CMSIS_device_header, which
    # CubeMX does not define. Both are shared by every mcu-* library, so an
    # app sets them once. Left unset, the app provides them its own way.
    #
    # Only the target's include dirs and defines are copied, the target is
    # never linked: CubeMX up to 6.13 also hangs main.c, the HAL and the
    # startup file on stm32cubemx as INTERFACE sources, which linking would
    # compile into libgamecontroller.a. A misspelt name fails at generate
    # time.
    if(MCU_HAL_TARGET)
        target_include_directories(gamecontroller PRIVATE
            $<TARGET_PROPERTY:${MCU_HAL_TARGET},INTERFACE_INCLUDE_DIRECTORIES>)
        target_compile_definitions(gamecontroller PRIVATE
            $<TARGET_PROPERTY:${MCU_HAL_TARGET},INTERFACE_COMPILE_DEFINITIONS>)
    endif()
    if(MCU_DEVICE_HEADER)
        target_compile_definitions(gamecontroller PUBLIC
            "CMSIS_device_header=\"${MCU_DEVICE_HEADER}\"")
    endif()
    return()
endif()

# ESP-IDF: the chip files are listed one by one, and PRIV_INCLUDE_DIRS "src"
# lets a chip folder reach gamecontroller_priv.h by plain name.
#
# The core is always built, even with CONFIG_GAMECONTROLLER off; each pad's
# parser only when its Kconfig option is on, and the Bluetooth glue only
# with Bluedroid and the transport its pads need. Only SRCS depends on
# Kconfig -- never REQUIRES, which IDF resolves before sdkconfig: bt and
# esp_hid are always required, as esp-ps-controller did, and build to
# little without Bluetooth on.
set(srcs "src/gamecontroller.c" "src/gamecontroller_log.c"
         "src/gamecontroller_port_esp.c" "src/ps_reports.c"
         "src/gamecontroller_bt.c")

if(CONFIG_GAMECONTROLLER AND NOT CONFIG_GAMECONTROLLER_DS4 AND
   NOT CONFIG_GAMECONTROLLER_DS5 AND NOT CONFIG_GAMECONTROLLER_XBOX)
    message(WARNING "gamecontroller: CONFIG_GAMECONTROLLER is on but no pad is selected; "
                    "set CONFIG_GAMECONTROLLER_<PAD>=y (esp-forge: GAMECONTROLLER_<PAD> = y)")
endif()

if(CONFIG_GAMECONTROLLER_DS4)
    list(APPEND srcs "src/ds4/ds4.c")
endif()
if(CONFIG_GAMECONTROLLER_DS5)
    list(APPEND srcs "src/ds5/ds5.c")
endif()
if(CONFIG_GAMECONTROLLER_XBOX)
    list(APPEND srcs "src/xbox/xbox.c")
endif()

# The pads' Bluetooth needs options a component cannot set; say which are
# missing rather than leave the app with link errors.
if((CONFIG_GAMECONTROLLER_DS4 OR CONFIG_GAMECONTROLLER_DS5) AND
   NOT (CONFIG_BT_BLUEDROID_ENABLED AND CONFIG_BT_CLASSIC_ENABLED AND
        CONFIG_BT_HID_HOST_ENABLED))
    message(WARNING "gamecontroller: DS4/DS5 need CONFIG_BT_BLUEDROID_ENABLED, "
                    "CONFIG_BT_CLASSIC_ENABLED and CONFIG_BT_HID_HOST_ENABLED; "
                    "only their report parsers are built")
endif()
if(CONFIG_GAMECONTROLLER_XBOX AND
   NOT (CONFIG_BT_BLUEDROID_ENABLED AND CONFIG_BT_BLE_ENABLED))
    message(WARNING "gamecontroller: Xbox needs CONFIG_BT_BLUEDROID_ENABLED and "
                    "CONFIG_BT_BLE_ENABLED; only its report parser is built")
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "src"
                       REQUIRES driver bt esp_hid)
