#include "tag_process.h"
#include "debug.h"
Tag::Tag(int TimeGroup){
        this->id = 0;
        this->disk_start = 1;
        this->disk_end = 0;
        this->del_size.resize(TimeGroup);
        this->wr_size.resize(TimeGroup);
        this->rd_size.resize(TimeGroup);
        this->rd_size_1.resize(TimeGroup);
        this->del_sumSize = 0;
        this->wr_sumSize = 0;
        this->rd_sumSize = 0;
        this->del_sumRate = 0;
        this->rd_sumRate = 0;
        this->wr_sumRate = 0;
        this->peak_time = 1; // 每种标签，size峰值对应的时间组
        this->peak_size = 0; // 每种标签，size峰值
        this->disk_order = 0; // 每种标签分配的磁盘分区号1 - M
        this->disk_size = 0;
        this->read_similar = 0;
}

void tag_init(){
    /* 输入样例： 5 2 3 10 100 
        有5 + 105个时间片，2种对象标签，3个硬盘，每个硬盘有10个存储单元，每个磁
        头在每个时间片最多消耗100个令牌
    */
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    V1_3 = V / 3 * 20 / 19;
    // 跳过预处理数据（示例未使用，实际需根据策略解析）
    max_time = T + 105;
    TIME_GROUP = ceil(T / 1800.0) + 1;
    max_time_group = TIME_GROUP - 1; // 时间片1800个一组：1 - 1800; 1801 - 3600...的删除操作数

    tags.assign(M+1, Tag(TIME_GROUP)); // 记录标签信息
    // 初始化标签的id号
    for(int i = 1; i <= M; i++){
        tags[i].id = i;
    }

    disks.assign(N+1, Disk(G,M));
    // fre_del.assign(M + 1, vector<int>(TIME_GROUP));
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            // scanf("%d", &fre_del[i][j]);
            scanf("%d", &tags[i].del_size[j]);
            tags[i].del_sumSize += tags[i].del_size[j];
        }
    }
    // fre_write.assign(M + 1, vector<int>(TIME_GROUP));
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tags[i].wr_size[j]);
            tags[i].wr_sumSize += tags[i].wr_size[j];
        }
    }
    // fre_read.assign(M + 1, vector<int>(TIME_GROUP));
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%d", &tags[i].rd_size[j]);
            tags[i].rd_sumSize += tags[i].rd_size[j];
        }
    }

    // 测试：
    ostringstream ss;
    ss << "总时间：" << T << ", 时间分组: 1 ~ " << max_time_group << ", 等于 " << (T - 1) / FRE_PER_SLICING + 1 << endl;
    writeInfo(ss.str());
    ss.clear();
    

    tag_write_process();
    tag_read_process();
    tag_all_process();

    printf("OK\n"); //对于全局预处理阶段，选手预处理完成后，需要输出： 1. OK 
    fflush(stdout); // 请注意： 选手输出完毕后，需要刷新输出缓冲区
}

static double cosineSimilarity(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0f;

    double dot_product = 0.0, norm_x = 0.0, norm_y = 0.0;
    for (size_t i = 1; i <= max_time_group; ++i) {
        dot_product += x[i] * y[i];
        norm_x += x[i] * x[i];
        norm_y += y[i] * y[i];
    }
    norm_x = sqrt(norm_x);
    norm_y = sqrt(norm_y);

    if (norm_x == 0 || norm_y == 0) return 0.0f;
    return dot_product / (norm_x * norm_y);
}


vector<double> time_group_size; // 每个时间组的size（所有标签size之和）
vector<double> time_group_size_rate; // 每个时间组的size 占总size的比例，从大到小映射为1 - max_time_group
// 每种标签、每个时间组的size，占每个时间组size的比例
vector<vector<double>> tag_time_size_rate; 
static double tag_all_size = 0; 
// vector<int> tag_peak_time; // 每种标签，size峰值对应的时间组
// vector<int> tag_peak_size; // 每种标签，size峰值
vector<int> time_peak_tag; // 每个时间组，对应的size最大标签号
vector<int> time_peak_tag_size; // 每个时间组，最大的标签size
// vector<int> tag_disk_order; // 每种标签分配的磁盘分区号，峰值时间小的磁盘分区号越小0 - M-1

static vector<Tag> tempTags;
static vector<int> tag_sequence = {0}; // 第1个位置不用，占位
void tag_read_process()
{
    // 1. 求任意两个标签之间的相似度（距离）、构成矩阵
    linkMatrix.resize(M + 1, vector<double>(M+1));
    ostringstream ss;
    // 测试：打印读取的大小
    // for(int i = 1; i <= max_time_group; i++){
    //     ss << tags[i].rd_size[i]
    // }

    // 归一化大小！
    for(int i = 1; i <= max_time_group; i++){
        for(int t = 1; t <= M; t++){
            tags[t].rd_size_1[i] = tags[t].rd_size[i] / (double)tags[t].rd_sumSize;
        }
    }
     
    for(int i = 1; i <= M; i++){
        for(int j = 1; j <= M; j++){
            if(i == j) continue;
            // 1.1 曼哈顿距离，不太行啊！
            // int dist = 0;
            // for(int k = 1; k <= max_time_group; k++){
            //     dist += abs(tags[i].rd_size[k] - tags[j].rd_size[k]);
            // }
            // linkMatrix[i][j] = dist;
            // 1.2 余弦相似度: size都为正数，所以相似度在0-1之间！越大越相似。反过来，改成越小越相似
            linkMatrix[i][j] = 1 - cosineSimilarity(tags[i].rd_size_1, tags[j].rd_size_1);
        }
        linkMatrix[i][i] = INT_MAX;
    }

    // 测试，相似度矩阵结果
    for(int i = 1; i <= M; i++){
        ss << i << ": ";
        for(int j = 1; j <= M; j++){
            ss << linkMatrix[i][j] << ",   ";
        }
        ss << endl;
    }
    // writeInfo(ss.str());

    // 2. 旅行商问题：求一个顺序，走完所有标签，使距离最短；贪心
    vector<int> isVisited(M + 1);
    int start = 2; // 从标签1开始走
    tag_sequence.push_back(start);
    isVisited[start] = 1;
    for(int cnt = 2; cnt <= M; cnt++){ // 还需要加入M - 1个标签
        // 遍历，找到距离start最近的
        vector<pair<double,int>> vecNums;
        vecNums.push_back({0,0});
        for(int j = 1; j <= M; j++){
            if(isVisited[j] == 1) continue;
            vecNums.push_back({linkMatrix[start][j], j});
        }
        auto it = min_element(vecNums.begin() + 1, vecNums.end());
        tag_sequence.push_back((*it).second);
        isVisited[(*it).second] = 1;
        start = (*it).second;
    }
    // 调试，标签写入顺序
    // ostringstream ss;
    ss.clear();
    for(int i = 1; i <= M; i++){
        ss << "分区：" << i  << "标签号：" << tag_sequence[i] << "\n";
    }
    writeInfo(ss.str());
    
}

// vector<int> tag_write_size_sum; // 横向求和：每种tag的size
// vector<double> tag_write_size_rate; // 每种tag的size占比
static double tag_write_all_size = 0;
static double tag_read_all_size = 0;
static double tag_delete_all_size = 0;
static double tag_write_delete_size = 0; // 写入 - 删除的总大小
void tag_write_process()
{
    // 横向遍历
    for(int tag = 1; tag <= M; tag++){
        tag_write_all_size += tags[tag].wr_sumSize;
        tag_read_all_size += tags[tag].rd_sumSize;
        tag_delete_all_size += tags[tag].del_sumSize;
        tag_write_delete_size += tags[tag].wr_sumSize - tags[tag].del_sumSize;
    }
    // 占比
    for(int tag = 1; tag <= M; tag++){
        tags[tag].wr_sumRate = tags[tag].wr_sumSize / tag_write_all_size;
        tags[tag].rd_sumRate = tags[tag].rd_sumSize / tag_read_all_size;
        tags[tag].del_sumRate = tags[tag].del_sumSize / tag_delete_all_size;
        tags[tag].disk_size = V1_3 * tags[tag].wr_sumRate;  // 策略1：按写入总大小
        // 策略2：按写入 - 删除后的总大小
        // tags[tag].disk_size = V1_3 * (tags[tag].wr_sumSize - tags[tag].del_sumSize) / tag_write_delete_size;

    }
}

void tag_all_process()
{
    // 直接按照tag的size分区即可，不同磁盘位置都对齐的！extra 700万
    // int current_start = 1;
    // for(int i = 1; i <= M; i++){
    //     tags[i].disk_start = current_start;
    //     current_start += V * tags[i].wr_sumRate;
    // }
    
    
    // 为每个磁盘分区! 不同磁盘的标签位置错开!
    // for(int d = 1; d <= N; d++){
    //     int current_start = 1;
    //     for(int i = 1; i <= M; i++){
    //         int t = (d + i) % M + 1;
    //         disks[d].tag_start[t] = current_start;
    //         current_start += tags[t].disk_size;
    //     }
    // }

    // 磁盘分区：M个分区，确定每个分区的开始和结束下标。暂时每个磁盘的分区都一样！
    for(int d = 1; d <= N; d++){
        int current_start = 1;
        for(int j = 1; j <= M; j++){
            // disks[d].tag_start[t] = current_start;
            // int t = tempTags[j].id; // 当前分区的标签号，排序后，430万，反倒少了20万
            // int t = tags[j].id; // 不排序，直接顺序写 512万
            int t = tag_sequence[j]; // 按余弦相似度，最短路径和518万
            disks[d].spaces[j].tag_id = t;
            disks[d].spaces[j].start = current_start;
            current_start += tags[t].disk_size;
            disks[d].spaces[j].end = current_start - 1;
            disks[d].spaces[j].remain_size = disks[d].spaces[j].end - disks[d].spaces[j].start + 1;
            disks[d].mapTagSpace[t] = j; // 暂时每个磁盘的分区都一样！
        }
        // 垃圾分区
        disks[d].spaces[0].start = current_start;
        disks[d].spaces[0].end = V;
        disks[d].spaces[0].remain_size = disks[d].spaces[0].end - disks[d].spaces[0].start + 1;
    }

    // 调试，查看分区结果，每个磁盘都一样
    ostringstream ss;
    for(int i = 1; i <= M; i++){
        ss << "分区号：" << i << ", 标签号 " << disks[1].spaces[i].tag_id << " [";
        ss << disks[1].spaces[i].start << ',' << disks[1].spaces[i].end << ',' << disks[1].spaces[i].remain_size << "]" << endl;
    }
    ss << disks[1].spaces[0].start << '/' << disks[1].spaces[0].end << '/' << disks[1].spaces[0].remain_size << endl;
    ss << "实际大小：" << disks[1].spaces[0].start - 1 << ", 理论大小V1_3: " << V1_3 << endl;
    writeInfo(ss.str());


    ss.clear();
    // 初始化每个磁盘存储单元cell的分区
    for(int d = 1; d <= N; d++){
        for(int t = 1; t <= M; t++){
            for(int j = disks[d].spaces[t].start; j <= disks[d].spaces[t].end; j++){
                disks[d].cells[j].space_id = t;
            }
        }
    }
    // 垃圾分区，单独处理！
    for(int d = 1; d <= N; d++){
        for(int t = 1; t <= M; t++){
            for(int j = disks[d].spaces[0].start; j <= disks[d].spaces[0].end; j++){
                disks[d].cells[j].space_id = 0;
            }
        }
    }
    // 测试
    // for(int i = 1; i <= V; i++){
    //     ss << disks[1].cells[i].space_id << " ";
    // }
    // ss << '\n';
    // writeInfo(ss.str());

    return;
}
