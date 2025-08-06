# Test Main CMakeLists.txt
idf_component_register(
    SRCS 
        "test_main.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "unity"
        "csi_collector"
        "web_server"
        "mqtt_client"
        "ntp_sync"
        "ota_updater"
)