#ifndef STATUS
#define STATUS

#include <vector>
#include <unordered_map>
#include "types.h"
using namespace std;


struct VMSingle
{
    int type;
    int server_id;
    int node;  // 0 for node A, 1 for node B, 2 for both

    VMSingle(int type, int server_id, int node)
    : type(type), server_id(server_id), node(node) {}
};

struct VMStat
{
    unordered_map<int, VMSingle*> vms;

    void add_vm(int type, int vm_id, int server_id, int node);
    void del_vm(int vm_id);
    ~VMStat();
};

// struct VMStat
// {
//     vector<int> types;
//     vector<int> vm_ids;
//     vector<int> server_ids;
//     vector<int> nodes;  // 0 for node A, 1 for node B, 2 for both
//     // unordered_map<int, int> id_to_idx;

//     int _get_idx_by_id(int vm_id);
//     void add_vm(int type, int vm_id, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs);
//     void del_vm(int vm_id);
// };


struct NodeStat
{
    int cores_used;
    int mem_used;

    NodeStat(int cores_used, int mem_used)
    : cores_used(cores_used), mem_used(mem_used) {}
};


struct ServerStat  // 或许可以用 unordered_map: ids -> others 的方式存储，这样查询插入都更快；或者存储一个 ids -> idx 的 map
{
    vector<int> types;
    vector<int> ids;
    vector<int> cores_used;
    vector<int> mem_used;
    vector<NodeStat*> nodes[2];
    vector<bool> is_running;

    void clear();  // clear all 
    
    void reset_stat(ServerSpecList &server_specs);  // recover the status as the unused servers
    // void update_stat(VMStat &vm_stat, VMSpecList &vm_specs);  // revise the resources information using vm_stat

    int _get_idx_by_id(int id);  // 如果所有的 server 都是按顺序存放在 vector 中，那么这个函数也许可以不需要
    void _change_node_stat(int op, int server_id, int core, int mem, int node);  // 0 for node A, 1 for node B, 2 for both
    void _change_server_stat(int op, int server_id, int core, int mem, int node);

    void add_server(int server_type, ServerSpecList &server_specs);
    bool can_hold_vm(int vm_type, int server_id, int server_node, ServerSpecList &server_specs, VMSpecList &vm_specs);
    bool add_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs);
    bool del_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs);

    int get_cores_total(ServerSpecList &server_specs);
    int get_cores_ream(ServerSpecList &server_specs);
    int get_mem_total(ServerSpecList &server_specs);
    int get_mem_ream(ServerSpecList &server_specs);
    void pirnt_summary();

    ~ServerStat();
};

struct ServerStatRecorder
{
    vector<int> types;
    vector<int> ids;
    int current_day;
    vector<vector<int>*> run_days;

    ServerStatRecorder();
    void update_one_day(ServerStat &server_stat);
    ~ServerStatRecorder();
};

#endif