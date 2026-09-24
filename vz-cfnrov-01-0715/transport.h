// transport.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化传输层
/// @brief 执行“transport_init”对应的业务操作。
bool transport_init(const char* remote_ip, uint16_t remote_port, uint16_t local_port);

// 关闭传输层
/// @brief 执行“transport_close”对应的业务操作。
void transport_close(void);

// 发送数据
/// @brief 执行“transport_send”对应的业务操作。
int transport_send(const uint8_t* data, uint16_t len);

// 接收数据（非阻塞）
/// @brief 执行“transport_receive”对应的业务操作。
int transport_receive(uint8_t* buffer, uint16_t max_len);

// 检查是否已连接
/// @brief 执行“transport_is_connected”对应的业务操作。
bool transport_is_connected(void);

// 获取最后错误信息
/// @brief 执行错误对应的业务操作。
const char* transport_get_last_error(void);

// 兼容旧代码的宏
#define write_serial_port(data, len) transport_send((const uint8_t*)(data), (len))

#ifdef __cplusplus
}
#endif
