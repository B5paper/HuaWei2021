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

    NodeStat() {}
    NodeStat(int cores_used, int mem_used)
    : cores_used(cores_used), mem_used(mem_used) {}
};

struct ServerStat
{
    int type;
    int cores_used;
    int mem_used;
    NodeStat nodes[2];
    bool is_running;

    ServerStat() {}
    ServerStat(int type);
    ServerStat(const ServerStat &obj);
    void operator= (ServerStat &obj);
    bool can_hold_vm(int vm_core, int vm_mem, int server_node, ServerSpecList &server_specs);
};

struct ServerStatList
{
    unordered_map<int, ServerStat> servs;  // id -> server
    
    ServerStatList() {}
    // ServerStatList(const ServerStatList &obj);
    // void operator= (ServerStat &obj);
    // ~ServerStatList();

    void _change_server_stat(int op, int server_id, int core, int mem, int node);
    void _change_node_stat(int op, int server_id, int core, int mem, int node);

    void add_server(int server_id, int server_type);
    bool can_hold_vm(int vm_type, int server_id, int server_node, ServerSpecList &server_specs, VMSpecList &vm_specs);
    bool add_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs);
    void del_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs);
};



// struct ServerStatRecorder
// {
//     vector<int> types;
//     vector<int> ids;
//     int current_day;
//     vector<vector<int>*> run_days;

//     ServerStatRecorder();
//     void update_one_day(ServerStat &server_stat);
//     ~ServerStatRecorder();
// };

#endif