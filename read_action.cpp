#include "read_action.h"
#include "debug.h"

// 5 × 8 的矩阵，[size - 1][pre_index - 1];
static vector<int> r_cost_g = { 0, 64, 52, 42, 34, 28, 23, 19, 16 };
static unordered_map<int, int> cost_index;
static vector<vector<int>> readMaxLen(16, vector<int>(9));
static int read_bestLen(int pre_act_g, int obj_size) {
    // token映射表
    // vector<int> r_cost_g = { 0, 64, 52, 42, 34, 28, 23, 19, 16 }; // 0-7, size为8
    for (int i = 1; i < r_cost_g.size(); i++) {
        cost_index[r_cost_g[i]] = i;
    }
    // 暴力搜索！对比pprrr 和 rrrrr
    int ans = -1; // -1，表示无论距离多少，pr都更优秀！
    int MAX_LEN = 200;
    vector<int> prLenCost(MAX_LEN + 1); // 距离为1-200的消耗，肯定递增！
    vector<int> rrLenCost(MAX_LEN + 1); // 距离为1-200的消耗，肯定递增！
    for (int len = 1; len <= MAX_LEN; len++) {
        // 1. 计算该距离下，pr的消耗
        prLenCost[len] = len;
        for (int s = 1; s <= obj_size; s++) {
            int index = s > 8 ? 8 : s;
            prLenCost[len] += r_cost_g[index];
        }

        // 2. 计算该距离下，rr的消耗
        //int rrCost = 0;
        int rrIndex = cost_index[pre_act_g];
        for (int s = 1; s <= len + obj_size; s++) {
            int index = rrIndex + s;
            index = index > 8 ? 8 : index;
            rrLenCost[len] += r_cost_g[index];
        }
        
    }
    // 3. 比较消耗，返回第1个rr更大的下标
    for (int len = 0; len <= MAX_LEN; len++) {
        if (rrLenCost[len] <= prLenCost[len]) continue;
        ans = len;
        break;
    }
    return ans - 1; // 不可能返回0，可能返回 > 0
}
void read_init(){
    ostringstream ss;
    for (int i = 1; i <= 15; i++) {
        for (int j = 1; j <= 8; j++) {
            readMaxLen[i][j] = read_bestLen(r_cost_g[j], i);
            ss << readMaxLen[i][j] << " ";
        }
        ss << endl;
    }
    writeInfo(ss.str());
}

// 6.2.4 对象读取事件交互
void read_action()
{
    // 读取并记录下当前请求
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request_count++;
        requests[request_id].object_id = object_id;
        // 更新对象的请求链表
        requests[request_id].prev_id = objects[object_id].last_request_point;
        objects[object_id].last_request_point = request_id;
        // 加入该对象的请求集合！
        objects[object_id].req_ids.insert(request_id);

        requests[request_id].is_done = false;
        requests[request_id].current_phase = 0; // 初始化完成块数为0
        // 加入待读取的所有分块，用于判断请求是否读取完毕！
        for(int block = 1; block < objects[object_id].size; block++){
            requests[request_id].remain_units.insert(block);
        }
        requests[request_id].arrive_time = current_time;
        // requests[request_id].is_used = 0;
    }

    // 无请求，而且也没有新请求(n_read = 0)
    if (request_count == 0) {
        for (int i = 1; i <= N; i++) {
            printf("#\n");// 无动作标记
        }
        printf("0\n");// 无完成请求
        fflush(stdout);
        return;
    } 

    int n_rsp = 0; // 当前时间片，完成的请求数
    vector<int> request_finish_id;

    // 输出N行：每个磁盘的动作 优化，核心得分代码
    for (int i = 1; i <= N; i++) {
        // 1.当前没有请求、当前磁盘的请求被删除； 重新查找请求
        if(disks[i].request_id == 0 || requests[disks[i].request_id].is_done == true){
            find_request(i);
        }

        // 没找到，退出，轮到下一个磁盘
        if(disks[i].request_id == 0){
            printf("#\n");
            // free_predict(i);
            continue;
        }

        // 读取策略1：大于G就j，小于就p，到点就r
        read_and_pass(i, n_rsp, request_finish_id);

    }

    // 输出请求完成信息
    cout << n_rsp << '\n';
    for(int inr = 0; inr < n_rsp; inr++){
        cout << request_finish_id[inr] << '\n';
    }

    fflush(stdout);
}

int rCostTest(int len, char pre_action, int pre_act_g){
    int cost_g = 0; // 预期消耗
    int all_cost = 0;
    for(int ci = 1; ci <= len + 1; ci++)
    {
        if(pre_action != 'r'){
            cost_g = 64;
        }
        else{
            cost_g = max(16, (int)ceil(pre_act_g * 0.8));
        }
        pre_action = 'r'; // 记录上一次的动作
        pre_act_g = cost_g; // 上一次动作
        all_cost += cost_g;
    }
    return all_cost;
}

int pCostTest(int len){
    return len + 64; // 到位置后，还得读！
}



// 读取策略2：大于G就j，小于就p，到点就r
void read_and_pass(int i, int &n_rsp, vector<int> &request_finish_id)
{
    // 2. 找到了请求，开始读取
    string outputStr = "";
    while(disks[i].g_remain > 0){
        // 求：到目标块，需要移动的次数（距离）
        int passLen_to_target = (disks[i].block_index - disks[i].point + V) % V;
        // 到达目的，等待读取
        if(passLen_to_target == 0){
            // 剩余令牌数不够读取，退出
            int need_g;
            if(disks[i].pre_action != 'r'){
                need_g = 64;
            }
            else{
                need_g = max(16, (int)ceil(disks[i].pre_act_g * 0.8));
            }
            if(disks[i].g_remain < need_g){
                break;
            }
            // 令牌数够，开始读取
            outputStr.push_back('r');
            disks[i].point = disks[i].point % V + 1; // 磁头移动
            disks[i].g_remain -= need_g; 
            disks[i].pre_act_g = need_g;
            disks[i].pre_action = 'r'; // 记录上一次的动作

            // 当前请求已经读取的块数
            requests[disks[i].request_id].current_phase++; // 总的读取块数
            // 判断：未读取完毕，更新到下一个块
            if(requests[disks[i].request_id].current_phase != objects[disks[i].object_id].size)
            {
                disks[i].block++; // 当前磁盘正在读取的请求块id
                disks[i].block_index = objects[disks[i].object_id].rep[disks[i].replica_id].disk_index[disks[i].block];
                continue; // 继续下一个循环
            }

            // 请求读取完毕了！所有请求都能一起完成？
            int temp_req_id = disks[i].request_id;
            while(temp_req_id != 0){
                if(requests[temp_req_id].is_done == false){
                    requests[temp_req_id].is_done = true; // 标记请求读取完毕
                    n_rsp++; // 当前时间片，完成的请求数
                    request_finish_id.push_back(temp_req_id);
                    request_count--; // 请求总数
                }
                temp_req_id = requests[temp_req_id].prev_id;
            }
            find_request(i); // 查找下一个请求
            
            // 没找到，退出，轮到下一个磁盘
            if(disks[i].request_id == 0){
                // free_next_predict(i, outputStr);
                break; // 退出整个while语句
            }
        }
        else if(passLen_to_target > 0)
        {
            // 尽可能的向目标移动，大于G就直接跳跃，但是得下个时隙读了！
            if(passLen_to_target >= G && disks[i].g_remain >= G){
                // 跳转到目的地，并直接退出了！ 
                outputStr += "j ";
                disks[i].pre_action = 'j';
                disks[i].pre_act_g = G;
                outputStr += to_string(disks[i].block_index);
                disks[i].point = disks[i].block_index; // 磁头移动
                disks[i].g_remain -= G;
                break;
            }
            // 距离小于L时，读过去、p过去；模拟两种操作的令牌消耗，选择一个消耗较少的
            // int r_cost = rCostTest(passLen_to_target, disks[i].pre_action, disks[i].pre_act_g);
            // int p_cost = pCostTest(passLen_to_target);
            // if(r_cost <= p_cost){
            // 计算最佳读取距离（限制上一个为r的情况）。多个连续的情况，size直接取最大的情况！
            // int bestLen = readMaxLen[10][disks[i].pre_act_g]; 不太行啊，反而下降了！
            if(passLen_to_target <= 8 && disks[i].pre_action == 'r'){
                int cost_g = 0; // 预期消耗
                int break_flag = 0;
                for(int ci = 1; ci <= passLen_to_target; ci++)
                {
                    if(disks[i].pre_action != 'r'){
                        cost_g = 64;
                    }
                    else{
                        cost_g = max(16, (int)ceil(disks[i].pre_act_g * 0.8));
                    }
                    if(disks[i].g_remain < cost_g){
                        break_flag = 1;
                        break; // 退出当前循环
                    }
                    // 判断当前读取的地方是否有对象、请求！
                    int temp_obj_id = disks[i].cells[disks[i].point].obj_id;

                    outputStr.push_back('r'); // 读取代替移动！
                    disks[i].pre_action = 'r'; // 记录上一次的动作
                    disks[i].g_remain -= cost_g; // 令牌消耗
                    disks[i].pre_act_g = cost_g; // 上一次动作
                    disks[i].point = disks[i].point % V + 1; // 磁头移动

                    

                }
                if(break_flag == 1){
                    break; // 退出while循环
                }
            }
            else 
            {
                // 距离比较大时，还是p过去！
                if(disks[i].g_remain >= passLen_to_target){
                    // 令牌够，移动到对应位置
                    outputStr += string(passLen_to_target, 'p');
                    disks[i].pre_action = 'p';
                    disks[i].pre_act_g = 1;
                    disks[i].point = disks[i].block_index; // 磁头移动
                    disks[i].g_remain -= passLen_to_target; // 剩余令牌
                }
                else{
                    // 令牌不够了，移动令牌个就行
                    outputStr += string(disks[i].g_remain, 'p');
                    disks[i].pre_action = 'p';
                    disks[i].pre_act_g = 1;
                    disks[i].point = (disks[i].point + disks[i].g_remain - 1) % V + 1; // 磁头移动
                    disks[i].g_remain = 0; // 剩余令牌
                    // 退出
                    break;
                }
            }
        }
    }
    if(outputStr[0] != 'j') outputStr += "#";
    cout << outputStr << '\n';
}


// 查找请求，输入磁盘i
// 策略3：遍历磁盘单元查找！
void find_request(int i){
    // 参数归位
    // disk_request[i] = 0;
    disks[i].request_id = 0;
    disks[i].object_id = 0;
    disks[i].replica_id = 0;
    disks[i].block = 0;
    disks[i].block_index = 0;

    int V_actual = disks[i].spaces[0].start - 1; // 实际存储副本的空间大小

    // 从当前磁头位置开始往后查找！
    for(int iv = 1; iv <= V_actual; iv++){
        int ihead = (disks[i].point - 2 + iv) % V_actual + 1; // 从当前磁头位置，往后遍历V个单元
        int object_id2 = disks[i].cells[ihead].obj_id; // 获取当前磁头位置的对象
        // 判断：当前位置 - 没有对象，下一个
        if(object_id2 == 0){
            continue;
        }

        // 有对象，获取对象的信息：对象标签tag、对象object_id2，对象副本号obj_rep_id
        int tag = objects[object_id2].tag; // 获取请求-对象.标签
        int obj_rep_id = 1; // 获取该请求存在本磁盘的副本id

        // 获取对象的请求链表，最新请求id
        int current_id = objects[object_id2].last_request_point;
        // 判断：当前位置-对象-没有请求，下一个。
        if(current_id == 0){
            continue;
        }
        // 链表：超时的、完成的还得挨个判断；集合：直接判空即可！非空肯定有不超时的请求
        if(objects[object_id2].req_ids.empty()){
            continue;
        }

        // 判断：有请求，但第1个请求都超时了，那么后面的肯定超时，下一个
        if(current_time - requests[current_id].arrive_time > 100){
            continue;
        }

        // 有不超时的请求，找到最后1个。先得到所有可能的请求
        vector<int> all_requests;
        int temp_request = current_id;
        while(temp_request != 0){
            // cout << temp_request << "/";
            // 判断：请求不超时、没被读取完成、没被正在读取、没被预订！
            if( (current_time - requests[temp_request].arrive_time < 90)
                && requests[temp_request].is_done == false
                && requests[temp_request].current_phase == 0)
                // && requests[temp_request].is_used == 0)
            {
                all_requests.push_back(temp_request);
                if((int)all_requests.size() >= 1){ // 只能是读取最新的请求，不然分更低！
                    break;
                }
            }
            temp_request = requests[temp_request].prev_id;
        }
        if(all_requests.empty()){
            continue;
        }
        current_id = all_requests.back();
        // cout << current_id << endl;

        // 判断：该对象的全部请求都已经完成，下一个
        if(current_id == 0){
            continue;
        }

        // 判断:这个对象没有连续存储(差值不超过2就行), 下一个
        if(objects[object_id2].is_serial == 0){
            continue;
        }


        // requests[current_id].is_used = 1; // 标记即将使用！

        disks[i].request_id = current_id;
        disks[i].object_id = object_id2;
        disks[i].replica_id = obj_rep_id;
        disks[i].block = 1;
        disks[i].block_index = objects[object_id2].rep[obj_rep_id].disk_index[1];
        return;
    }
}

