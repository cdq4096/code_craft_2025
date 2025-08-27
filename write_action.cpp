#include "write_action.h"
#include "global_global.h"
#include "debug.h"
// 连续的写入，返回能够连续写入的下标，否则返回0
int Disk::findSerial(int size, int _space_id){
    if(_space_id % 2 == 1){
        return findSerialReverse(size, _space_id);
    }
    int start = spaces[_space_id].start;
    int end = spaces[_space_id].end - size + 1;
    for(int index = start; index <= end; index++){
        if(cells[index].obj_id != 0) continue;
        int flag = 0;
        for(int j = 1; j < size; j++){
            int nexti = index + j;
            if(cells[nexti].obj_id != 0){
                flag = 1;
                break;
            }
        }
        if(flag == 1){
            // 不连续、继续查找
            continue;
        }
        // 连续，可以写入，返回下标
        return index;
    }
    return 0; // 没找到，返回0
}

// 连续的写入，返回能够连续写入的下标，否则返回0
int Disk::findSerialReverse(int size, int _space_id){
    int start = spaces[_space_id].start + size - 1;
    int end = spaces[_space_id].end;
    for(int index = end; index >= start; index--){
        if(cells[index].obj_id != 0) continue;
        int flag = 0;
        for(int j = 1; j < size; j++){
            int nexti = index - j;
            if(cells[nexti].obj_id != 0){
                flag = 1;
                break;
            }
        }
        if(flag == 1){
            // 不连续、继续查找
            continue;
        }
        // 连续，可以写入，返回下标
        return index - size + 1;
    }
    return 0; // 没找到，返回0
}

// 将对象object_id的一个副本写入磁盘disk_unit，记录写入位置的下标object_unit
// 副本分块存储策略：见缝插针、挨个存！
void Disk::writeObject(int* object_unit, int size, int object_id, int rep_id){
    int current_write_point = 0;
    // 副本2和3，从后往前，随便存
    if(rep_id != 1){
        // 最后1个往前，到着写
        for (int i = rubbish_index; i >= 1; i--) {
            if (cells[i].obj_id == 0) { // 查找空闲存储单元
                cells[i].obj_id = object_id; // 磁盘占用标记，对应位置值为对象的id
                spaces[cells[i].space_id].remain_size--;
                object_unit[++current_write_point] = i; // 记录副本分块的位置下标
                cells[i].block_id = current_write_point;
                // cells[i].tag_id = objects[object_id].tag;
                // cells[i].replica_id = rep_id;
                rubbish_index--;
                if (current_write_point == size) {
                    break;
                }
            }
        }
        // 确保有足够空间（题目保证）
        assert(current_write_point == size);
        return;
    }

    // 副本1，先尝试保证连续，无法连续，再尝试从start开始碎存。
    int tag_id = objects[object_id].tag;
    int curr_space = mapTagSpace[tag_id];
    // int curr_space = tag_id; // disks[i].space[标签号]直接就是对应的分区.start

    // 在当前tag对应的分区查找
    int start = findSerial(size, curr_space);
    if(start == 0){
        // 按相似度排序，挨个查找
        vector<pair<double,int>> similar_nums;
        similar_nums.push_back({0,0});
        for(int t = 1; t <= M; t++){
            if(t == tag_id) continue;
            similar_nums.push_back({linkMatrix[tag_id][t], t});
        }
        sort(similar_nums.begin() + 1, similar_nums.end());
        for(int i = 1; i < similar_nums.size(); i++){
            int space_id_ = mapTagSpace[similar_nums[i].second];
            start = findSerial(size, space_id_);
            if(start != 0){
                break;
            }
        }
    }

    // 保底：从头开始碎存！
    if(start == 0){
        // start = 1;
        start = spaces[curr_space].start;
    }

    // 无论连续还是不连续，都从start开始存即可，
    int Va = spaces[0].start - 1;
    for (int i = 1; i <= Va; i++) {
        int index = (start + i - 2) % Va + 1;
        if (cells[index].obj_id == 0) { // 查找空闲存储单元
            cells[index].obj_id = object_id; // 磁盘占用标记，对应位置值为对象的id
            spaces[cells[index].space_id].remain_size--;
            object_unit[++current_write_point] = index; // 记录副本分块的位置下标
            cells[index].block_id = current_write_point;
            // cells[index].tag_id = objects[object_id].tag;
            // cells[index].replica_id = rep_id;
            if (current_write_point == size) {
                break;
            }
        }
    }

    // 判断当前对象是否被连续存储
    int flag_serial = 1;
    for(int j = 2; j <= size; j++){
        if(object_unit[j] - object_unit[j-1] > 5){
            flag_serial = 0;
        }
    }
    objects[object_id].is_serial = flag_serial;
    
    // 确保有足够空间（题目保证）
    assert(current_write_point == size);
}

// 必须同时选3个不一样的！返回每个副本对应的磁盘号
static vector<int> disk_select(int tag_id){
    vector<int> ans(4);
    int max_disk = rand() % N + 1;
    // 第1个副本，比较标签对应的分区。易错点：可能都满了，那就随机一个磁盘！
    for(int d = 2; d <= N; d++){
        int space_id1 = disks[d].mapTagSpace[tag_id];
        int space_id2 = disks[max_disk].mapTagSpace[tag_id];
        if(disks[d].spaces[space_id1].remain_size > disks[max_disk].spaces[space_id2].remain_size)
        {
            max_disk = d;
        }
    }
    ans[1] = max_disk;

    // 第2个副本，比较0号分区即可，每个磁盘的分区0按照剩余size排序，返回最后2个
    vector<pair<int,int>> diskSize;
    diskSize.push_back({0,0}); // 下标从1开始，占位0
    for(int d = 1; d <= N; d++){
        if(d == max_disk) continue; // 易错点：不能和副本1的重合
        diskSize.push_back({disks[d].spaces[0].remain_size, d});
    }
    sort(diskSize.begin() + 1, diskSize.end(), greater<pair<int,int>>()); // 剩余最大的，倒序排序！
    ans[2] = diskSize[1].second;
    ans[3] = diskSize[2].second;

    return ans;
}

static int mCount = 50;
// 6.2.3 对象写入事件交互
// 对象磁盘选择策略：根据id顺序对应！1号对象就在1、2、3号盘，2号在2、3、4号盘
void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size,tag;
        scanf("%d%d%d", &id, &size, &tag);
        objects[id].last_request_point = 0;// 初始化请求链表头
        // 选择对象3个副本存储的磁盘，写入对象副本
        vector<int> disk_sel = disk_select(tag);
        for (int j = 1; j <= REP_NUM; j++) {
            // int disk_id = (id + j) % N + 1;
            int disk_id = disk_sel[j];
            if(mCount-- >= 0){
                ostringstream ss;
                ss << "标签" << tag << ",副本" << j << ",磁盘" << disk_id << endl;
                writeInfo(ss.str());
            }
            objects[id].rep[j].disk_id = disk_id;// 副本j存储的磁盘号，3个副本连续3个磁盘；
            objects[id].rep[j].disk_index = static_cast<int*>(malloc(sizeof(int) * (size + 1)));
            objects[id].size = size; // 对象分块存储数
            objects[id].tag = tag;
            objects[id].is_delete = false;
            disks[disk_id].writeObject(objects[id].rep[j].disk_index, size, id, j);
            // do_object_write(object[id].disk_index[j], disk[object[id].disk_id[j]], size, id);
        }
        // 输出副本信息
        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", objects[id].rep[j].disk_id); // 副本存储的磁盘
            for (int k = 1; k <= size; k++) {
                printf(" %d", objects[id].rep[j].disk_index[k]); // 副本分块存储的下标
            }
            printf("\n");
        }
    }

    fflush(stdout);
}