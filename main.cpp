#include "global_global.h"
#include "delete_action.h"
#include "write_action.h"
#include "read_action.h"
#include "tag_process.h"
static void timestamp_action(); // ʱ���ͬ��
static void clean(); // �ͷŶ�̬������ڴ�

Request requests[MAX_REQUEST_NUM];// ���ж�ȡ���������
Object objects[MAX_OBJECT_NUM];// ���ж��������
int T, M, N, V, G; // ʱ��Ƭ��T����ǩ��M��Ӳ����N���洢��Ԫ��V��������G
int V1_3; // 1/3V���������Ĳ����� 
int max_time = 0; // ���ʱ��Ƭ����1 �� T + 105
int max_time_group = 0; // ���ʱ������ceil(T / 1800.0), ��1��1 - 1800; ��2��1801 - 3600
int TIME_GROUP = 0; // ���ʱ������ + 1�����ڳ�ʼ��
vector<Disk> disks; // N��Ӳ����Ŀ
vector<Tag> tags; // ��ǩ��
vector<vector<double>> linkMatrix;
// vector<vector<int>> fre_del;
// vector<vector<int>> fre_write;
// vector<vector<int>> fre_read;
int current_time = 0; // ��ǰʱ��
int request_count = 0; // ��������
 
int main()
{
    // ȫ��Ԥ����
    tag_init();
    read_init(); // pr���Գ�ʼ��

    // ��ʼ�����̴�ͷλ�á����̺�
    for (int i = 1; i <= N; i++) {
        disks[i].point = 1;
        disks[i].id = i;
    }


    // ����ÿ��ʱ��Ƭ
    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        // ��ʼ��ÿ������ʣ���������
        for (int i = 1; i <= N; i++) {
            disks[i].g_remain = G;
            disks[i].rubbish_index = V;
        }

        timestamp_action();// ʱ��Ƭͬ��
        delete_action();// ����ɾ��
        write_action();// ����д��
        read_action();// �����ȡ
        // read_action2();// �����ȡ
    }
    clean();// ������Դ

    return 0;
}



void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);// ��ȡ������"TIMESTAMP"�ַ�������ȡʱ��Ƭ���
    printf("TIMESTAMP %d\n", timestamp);
    current_time = timestamp;
    fflush(stdout);// ˢ�����������ȷ����ʱ��Ӧ
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