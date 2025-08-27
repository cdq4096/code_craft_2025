#include "global_global.h"
#include "delete_action.h"
#include "write_action.h"
#include "read_action.h"
#include "tag_process.h"
static void timestamp_action(); // 时间戳同步
static void clean(); // 释放动态分配的内存

Request requests[MAX_REQUEST_NUM];// 所有读取请求的数组
Object objects[MAX_OBJECT_NUM];// 所有对象的数组
int T, M, N, V, G; // 时间片数T、标签数M、硬盘数N、存储单元数V、令牌数G
int V1_3; // 1/3V区，后续的不读！ 
int max_time = 0; // 最大时间片数，1 ～ T + 105
int max_time_group = 0; // 最大时间组数ceil(T / 1800.0), 组1：1 - 1800; 组2：1801 - 3600
int TIME_GROUP = 0; // 最大时间组数 + 1，用于初始化
vector<Disk> disks; // N个硬盘数目
vector<Tag> tags; // 标签数
vector<vector<double>> linkMatrix;
// vector<vector<int>> fre_del;
// vector<vector<int>> fre_write;
// vector<vector<int>> fre_read;
int current_time = 0; // 当前时间
int request_count = 0; // 请求总数
 
int main()
{
    // 全局预处理
    tag_init();
    read_init(); // pr策略初始化

    // 初始化磁盘磁头位置、磁盘号
    for (int i = 1; i <= N; i++) {
        disks[i].point = 1;
        disks[i].id = i;
    }


    // 处理每个时间片
    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        // 初始化每个磁盘剩余的令牌数
        for (int i = 1; i <= N; i++) {
            disks[i].g_remain = G;
            disks[i].rubbish_index = V;
        }

        timestamp_action();// 时间片同步
        delete_action();// 处理删除
        write_action();// 处理写入
        read_action();// 处理读取
        // read_action2();// 处理读取
    }
    clean();// 清理资源

    return 0;
}



void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);// 读取并忽略"TIMESTAMP"字符串，获取时间片编号
    printf("TIMESTAMP %d\n", timestamp);
    current_time = timestamp;
    fflush(stdout);// 刷新输出缓冲区确保及时响应
}

void clean()
{
    for (auto& obj : objects) {
        for (int i = 1; i <= REP_NUM; i++) {
            if (obj.rep[i].disk_index == nullptr)
                continue;
            free(obj.rep[i].disk_index);
            obj.rep[i].disk_index = nullptr;
        }
    }
}