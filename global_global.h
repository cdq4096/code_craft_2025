#pragma once

#include <cstdio>
#include <cassert>
#include <cstdlib>
using namespace std;
#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "light_set.h"
#define MAX_DISK_NUM (10 + 1) // 最大磁盘数，题目中N≤10，+1避免越界
#define MAX_DISK_SIZE (16384 + 1) // 每个磁盘最大存储单元数，题目中V≤16384
#define MAX_REQUEST_NUM (30000000 + 1) // 最大请求数
#define MAX_OBJECT_NUM (100000 + 1) // 最大对象数，题目中写入次数≤100000
#define REP_NUM (3)  // 每个对象的副本数，固定为3
#define FRE_PER_SLICING (1800) // 每个时间片分段包含的时间片数（全局预处理用）
#define EXTRA_TIME (105) //// 额外的时间片数，总时间片为T+105

// 请求结构体：记录每个读取请求的状态
class Request{
public:
    int object_id;// 请求对应的对象ID
    int prev_id = 0;// 同一对象的上一个请求ID（链表结构）
    bool is_done = false;// 标记该请求是否已完成
    int current_phase = 0; // 当前请求，已经被读取的对象分块总数
    int arrive_time; // 当前请求到达的时间戳
    // int is_used = 0; // 当前请求已经被预订！不同磁盘读一个请求时会用到，现在没用。
    Int3Set remain_units; // 存储当前请求、对象size未被读取的分块。暂时没用。
    // unordered_set<int> remain_block;
};

// 副本类
class Replica{
public:
    int disk_id;
    int* disk_index; // 下标数组，大小为size
};
// 对象类
class Object{
public:
    Replica rep[REP_NUM + 1]; // 3个副本，使用1-3下标
    //int disk_id[REP_NUM + 1];// 每个副本存储的硬盘编号，题目给的，还是定义一个副本类好用！
    //int* disk_index[REP_NUM + 1];// 二维数组[4][size]，每个副本的分块存储的下标
    int size = 0;// 对象分块存储数
    int tag = 0; // 对象标签
    int last_request_point = 0;// 对象的最后一个请求id
    bool is_delete;// 标记对象是否已被删除
    int is_serial; // 标记对象的副本1是否被连续存储！
    unordered_set<int> req_ids; // 上面的链表记录请求，就是巨坑！还是集合方便！
};

// 磁盘存储单元
class Cell{
public:
    int obj_id; //磁盘存储单元状态，0表示空闲，非0为对象ID
    // int replica_id; // 副本号，能不写的就不写，不然太占内存了！
    int block_id; // 存储的对象分块号。暂时没用！
    int space_id; // 每个存储单元所处于的分区
    Cell(){
        this->obj_id = 0;
        this->space_id = 0;
        this->block_id = 0;
    }
};

// 磁盘分区类
class Space{
public:
    int tag_id;
    int start; 
    int end;
    int remain_size; // 当前分区剩余的大小
};

// 磁盘类
class Disk{
public:
    int id;
    int point;// 每个磁盘的当前磁头位置（初始为1）
    int g_remain; // 每个磁盘的剩余的令牌数
    int request_id; // 每个磁盘正在处理的请求index
    int object_id;// 每个磁盘正在处理的请求-对应的对象
    int replica_id;// 每个磁盘正在处理的请求-对应的对象-副本号
    char pre_action; // 每个磁盘的上一个动作
    int pre_act_g; // 每个磁盘的上一个动作消耗的令牌数
    int used_size;
    int block;
    int block_index;
    int rubbish_index;
    vector<int> tag_start; // 当前磁盘，每个标签开始的位置
    Cell cells[MAX_DISK_SIZE];
    vector<Space> spaces; // 当前磁盘，分区描述
    unordered_map<int, int> mapTagSpace; // 标签id 到 分区id 的映射

    Disk(int G, int M){
        this->id = 0;
        this->point = 1; // 初始化磁盘磁头位置
        this->g_remain = G;
        this->request_id = 0;
        this->object_id = 0;
        this->replica_id = 0;
        this->pre_action = 0;
        this->pre_act_g = 0;
        this->used_size = 0;
        this->block = 0;
        this->block_index = 0;
        this->tag_start.resize(M+1);
        this->rubbish_index = 0;
        this->spaces.resize(M + 1); // 多少个标签，分多少个区！
    }
    
    void writeObject(int* object_unit, int size, int object_id, int rep_id);
    int findSerial(int size, int _space_id);
    int findSerialReverse(int size, int _space_id); // 从end反向找！
    void deleteObject(const int* object_unit, int size);
};

// 标签类
class Tag{
public:
    int id;
    int disk_start; // 标签，分配的磁盘开始下标
    int disk_end; // 标签，分配的磁盘结束下标
    // int disk_point; // 
    vector<int> del_size; // 每个标签-每个时间段-删除的size大小
    int del_sumSize; // 总删除大小
    double del_sumRate; // tag总删除size占比
    vector<int> wr_size; // 每个标签-每个时间段-写入的size大小
    int wr_sumSize; // 总写入大小
    double wr_sumRate; // tag写入占比
    vector<int> rd_size; // 每个标签-每个时间段-写入的size大小
    vector<double> rd_size_1; // 每个标签-每个时间段-写入的size大小，归一化大小，不然太大了！
    int rd_sumSize; // 总读取大小
    double rd_sumRate; // tag读取请求占比
    int peak_time; // 每种标签，size峰值对应的时间组
    int peak_size; // 每种标签，size峰值
    int disk_order; // 每种标签分配的磁盘分区号，峰值时间小的磁盘分区号越小0 - M-1
    int disk_size; // 每种标签被分配的磁盘大小
    int read_similar; // 读取曲线相似度，默认标签1的为0，其余的为方差值！
    Tag(int TimeGroup);
};

// 时间片类
class TimeGroup{
public:
    // 当前时间片、每个标签读取、写入、删除的size
    vector<int> del_size;
    vector<int> wr_size;
    vector<int> rd_size;
};


// c++17，允许在.h文件中定义全局变量，防止重复定义，编译器只会链接一次！
extern Request requests[MAX_REQUEST_NUM];// 所有读取请求的数组
extern Object objects[MAX_OBJECT_NUM];// 所有对象的数组
extern int T, M, N, V, G; // 时间片数T、标签数M、硬盘数N、存储单元数V、令牌数G
extern int V1_3; // 1/3V区，后续的不读！
extern int max_time; // 最大时间片数，1 ～ T + 105
extern int max_time_group; // 最大时间组数ceil(T / 1800.0), 组1：1 - 1800; 组2：1801 - 3600
extern int TIME_GROUP; // 最大时间组数 + 1，用于初始化
extern vector<Disk> disks; // N个硬盘数目
extern vector<Tag> tags; // 标签数
extern vector<vector<double>> linkMatrix;
extern int current_time;
extern int request_count;