# file: <项目根-目录>/src/CMakeLists.txt
FILE(GLOB_RECURSE app_sources
    "*.cpp"
    "module_actuators/*.cpp"
    "module_ai/*.cpp"
    "module_mqtt/*.cpp"
    "app_mesh_manager/*.cpp"
    "module_wifi/*.cpp"
    
)

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS .
                                    app_mesh_manager
                                    module_ai
                                    module_mqtt
                                    module_wifi
                                    # 确保这一行存在且正确无误
    EMBED_TXTFILES "module_ai/digicert_global_root_g2.pem"
     PRIV_REQUIRES 
        "arduino"

)