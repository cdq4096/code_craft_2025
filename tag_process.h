#pragma once
#include "global_global.h"

void tag_init();

// 全局预处理：标签 1-M 在时间组 T/1800 的读取对象size之和
void tag_read_process();

// 全局预处理：写入对象size
void tag_write_process();

// 全局预处理：在删除、写入、读取处理后，整体处理！
void tag_all_process();