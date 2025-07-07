#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <Arduino.h>
#include <painlessMesh.h>
#include <functional>

// 定义一个回调函数类型，用于将收到的数据传递给上层应用
// 使用 std::function 可以接受任何可调用对象（lambda, 函数指针等）
using DataReceivedCallback = std::function<void(uint32_t from, const String &msg)>;

class MeshManager {
public:
    // 构造函数
    MeshManager();

    // 初始化 Mesh 网络
    void init();

    // 必须在主循环中不断调用
    void update();

    // 注册一个回调函数，当收到数据时调用
    void onReceive(DataReceivedCallback callback);

private:
    // painlessMesh 实例
    painlessMesh _mesh;
    // 调度器
    Scheduler _userScheduler;
    // 上层应用提供的回调函数
    DataReceivedCallback _dataCallback;

    // --- 内部回调函数，用于处理 painlessMesh 的事件 ---
    void receivedCallback(uint32_t from, String &msg);
    void newConnectionCallback(uint32_t nodeId);
    void changedConnectionCallback();
    void nodeTimeAdjustedCallback(int32_t offset);
};

#endif // MESH_MANAGER_H