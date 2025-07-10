# file: <项目根-目录>/src/CMakeLists.txt

# 1. 自动查找源文件
# 递归地查找当前目录(src)及其所有子目录下的所有 .cpp 和 .c 文件。
# 这样就不再需要手动列出每个子目录了。
FILE(GLOB_RECURSE app_sources
    "*.c"
    "*.cpp"
)

# 2. 自动发现所有子目录并将它们添加到 INCLUDE_DIRS
# 首先，获取 src 目录下的所有子项（文件和目录）
FILE(GLOB subdirectories "*")

# 初始化一个列表，用于存放所有需要包含的目录路径。
# 默认包含当前目录 "."，这样 #include "config.h" 这样的引用才能工作。
SET(include_dirs .)

# 遍历所有子项
FOREACH(dir ${subdirectories})
    # 判断该项是否是一个目录（并且不是隐藏目录，如 .vscode）
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir} AND NOT dir MATCHES "^\\..*")
        # 如果是目录，则将其添加到 include_dirs 列表中
        LIST(APPEND include_dirs ${dir})
    ENDIF()
ENDFOREACH()

# 3. 注册组件
# 使用上面动态生成的源文件列表和包含目录列表。
idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS ${include_dirs}
                       # 保留需要嵌入的文件
                       EMBED_TXTFILES "module_ai/digicert_global_root_g2.pem"
                       # 保留所有必需的组件依赖
                       REQUIRES nvs_flash wifi_provisioning esp_wifi esp_event esp_netif mqtt json
)