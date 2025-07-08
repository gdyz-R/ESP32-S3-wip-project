# file: <项目根-目录>/src/CMakeLists.txt
FILE(GLOB_RECURSE app_sources
    "*.cpp"
    "module_ai/*.cpp"
    "module_mqtt/*.cpp"
    "module_wifi/*.cpp"
    "espnow_manager/*.cpp"
)

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS .
                                    module_ai
                                    module_mqtt
                                    espnow_manager
                                    module_wifi
                                    # 确保这一行存在且正确无误
    EMBED_TXTFILES "module_ai/digicert_global_root_g2.pem"
    REQUIRES nvs_flash wifi_provisioning esp_wifi esp_event esp_netif mqtt json
)