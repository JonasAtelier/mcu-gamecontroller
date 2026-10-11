# ESP-IDF only. stm-forge never reads this file: it compiles every .c under
# src/, chip folders included, puts src/ on the include path of this
# library's sources and adds include/ -- which is why each port file, and
# gamecontroller_bt.c, is empty on the other platform. Here the chip files
# are listed one by one, and PRIV_INCLUDE_DIRS "src" lets a chip folder
# reach gamecontroller_priv.h by plain name.
#
# The core is always built, even with CONFIG_GAMECONTROLLER off; each pad's
# parser only when its Kconfig option is on, and the Bluetooth glue only
# with Bluedroid and the transport its pads need. Only SRCS depends on
# Kconfig -- never REQUIRES, which IDF resolves before sdkconfig: bt and
# esp_hid are always required, as esp-ps-controller did, and build to
# little without Bluetooth on.
set(srcs "src/gamecontroller.c" "src/gamecontroller_log.c"
         "src/gamecontroller_port_esp.c" "src/ps_reports.c")

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
