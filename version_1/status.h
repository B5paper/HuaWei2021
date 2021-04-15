#ifndef STATUS
#define STATUS

#include <vector>
#include "types.h"
using namespace std;


struct VMSingle
{
    int type;
    int vm_id;
    int server_id;
    int node;  // 0 for node A, 1 for node B, 2 for both

    VMSingle(int type, int vm_id, int server_id, int node)
    : type(type), vm_id(vm_id), server_id(server_id), node(node) {}
};

struct VMStat
{
    vector<int> types;
    vector<int> vm_ids;
    vector<int> server_ids;
    vector<int> nodes;  // 0 for node A, 1 for node B, 2 for both
};

struct NodeStat
{
    int cores_used;
    int cores_ream;
    int mem_used;
    int mem_ream;

    NodeStat(int cores_used, int cores_ream, int mem_used, int mem_ream)
    : cores_used(cores_used), cores_ream(cores_ream), mem_used(mem_used), mem_ream(mem_ream) {}
};


struct ServerStat  // 或许可以用 unordered_map: ids -> others 的方式存储，这样查询插入都更快；或者存储一个 ids -> idx 的 map
{
    vector<int> types;
    vector<int> ids;
    vector<int> cores_used;
    vector<int> cores_ream;
    vector<int> mem_used;
    vector<int> mem_ream;
    vector<NodeStat*> node_a;
    vector<NodeStat*> node_b;

    void clear();  // clear all servers
    void add(ServerSpec &server_speServerStatc);
    void add(int server_type, ServerSpecList &server_specs);
    void reset_stat(ServerSpecList &server_specs);  // recover the status as the unused servers
    void update_stat(VMStat &vm_stat, VMSpecList &vm_specs);  // revise the resources information using vm_stat
    void update_stat(const VMSingle &vm_single, VMSpecList &vm_specs);
    int _get_idx_by_id(int id);
    int get_cores_total();
    int get_cores_ream();
    int get_mem_total();
    int get_mem_ream();
    void pirnt_summary();
    ~ServerStat();
};

#endif