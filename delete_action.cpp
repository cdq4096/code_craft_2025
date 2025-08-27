#include "delete_action.h"
#include "global_global.h"
void Disk::deleteObject(const int* object_unit, int size)
{
    for (int i = 1; i <= size; i++) {
        cells[object_unit[i]].obj_id = 0;
        cells[object_unit[i]].block_id = 0;
        // 删除后，增加容量
        spaces[cells[object_unit[i]].space_id].remain_size ++;
    }
}

// 6.2.2 对象删除事件交互
void delete_action()
{
    int n_delete; //输入：这一时间片，被删除的对象个数
    int abort_num = 0; // 输出：这一秒，被取消的读取请求的数量
    static int _id[MAX_OBJECT_NUM];// 需删除的对象ID

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }
    // 统计需取消的请求数
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = objects[id].last_request_point;
        while (current_id != 0) {
            if (requests[current_id].is_done == false) {
                abort_num++;
            }
            current_id = requests[current_id].prev_id;
        }
    }
    // 输出结果
    printf("%d\n", abort_num); 
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = objects[id].last_request_point;
        while (current_id != 0) {
            if (requests[current_id].is_done == false) {
                printf("%d\n", current_id);
                requests[current_id].is_done = true; // 标记为已取消？
                requests[current_id].remain_units.clear(); // 所有分块被读取！
                request_count--;
            }
            current_id = requests[current_id].prev_id;
        }
        // 删除对象
        for (int j = 1; j <= REP_NUM; j++) {
            // 取消磁盘里的占用标记
            disks[objects[id].rep[j].disk_id].deleteObject(objects[id].rep[j].disk_index, objects[id].size);
            // do_object_delete(objects[id].disk_index[j], disks[object[id].disk_id[j]], object[id].size);
        }
        objects[id].is_delete = true;
        // 删除对象的请求。可能有正在读取的请求！
        objects[id].req_ids.clear();
    }
    // 选手输出完毕后，需要刷新输出缓冲区
    fflush(stdout);
}