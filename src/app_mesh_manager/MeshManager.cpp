#include "MeshManager.h"
#include "../config.h" // 引用全局配置文件

MeshManager::MeshManager() : _dataCallback(nullptr) {
    // 构造函数，可以在这里进行一些默认初始化
}

void MeshManager::init() {
    // 设置调试信息（可选）
    // _mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    // 使用 config.h 中的配置初始化 Mesh
    _mesh.init(MESH_NETWORK_SSID, MESH_NETWORK_PASSWORD, &_userScheduler, MESH_PORT);
    
    // --- 注册内部回调函数 ---
    // 使用 std::bind 将类成员函数绑定到 C-style 的回调接口
    // 或者使用 lambda 捕获 this 指针
    _mesh.onReceive([this](uint32_t from, String &msg) {
        this->receivedCallback(from, msg);
    });
    _mesh.onNewConnection([this](uint32_t nodeId) {
        this->newConnectionCallback(nodeId);
    });
    _mesh.onChangedConnections([this]() {
        this->changedConnectionCallback();
    });
    _mesh.onNodeTimeAdjusted([this](int32_t offset) {
        this->nodeTimeAdjustedCallback(offset);
    });
    
    // 将此节点（根节点）连接到外部路由器
    _mesh.stationManual(EXTERNAL_ROUTER_SSID, EXTERNAL_ROUTER_PASS);
    _mesh.setHostname("MeshGateway"); // 给根节点一个友好的主机名
}

void MeshManager::update() {
    _mesh.update();
}

void MeshManager::onReceive(DataReceivedCallback callback) {
    _dataCallback = callback;
}

// --- 内部回调函数的具体实现 ---

void MeshManager::receivedCallback(uint32_t from, String &msg) {
    Serial.printf("MeshManager: Received from %u: %s\n", from, msg.c_str());
    
    // 如果上层应用注册了回调函数，就调用它，把数据传递出去
    if (_dataCallback) {
        _dataCallback(from, msg);
    }
}

void MeshManager::newConnectionCallback(uint32_t nodeId) {
    Serial.printf("MeshManager: New Connection, nodeId = %u\n", nodeId);
}

void MeshManager::changedConnectionCallback() {
    Serial.printf("MeshManager: Changed connections %s\n", _mesh.subConnectionJson().c_str());
}

void MeshManager::nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("MeshManager: Adjusted time %u. Offset = %d\n", _mesh.getNodeTime(), offset);
}