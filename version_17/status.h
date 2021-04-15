#ifndef STATUS
#define STATUS

#include "types.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include "args.h"
#include <iostream>
using namespace std;


typedef map<int, map<int, set<pair<int, int>>>> node_map;
typedef map<int, map<int, set<int>>> serv_map;
typedef map<int, map<int, map<int, set<int>>, greater<int>>, greater<int>> vm_map;

extern map<int, int> vm_id_to_serv_id;
extern int num_vms_total;

extern long long num_serv_mem_res_global;
extern long long num_serv_core_res_global;


struct NodeStat
{
    int cores_ream;
    int mem_ream;

    NodeStat() {}
    NodeStat(const NodeStat &obj);
    void _change_node_stat(int op, int core, int mem);
};

struct VMTypeNode
{
    int type;
    int node;

    VMTypeNode() {}
    VMTypeNode(int type, int node): type(type), node(node) {}
    VMTypeNode(const VMTypeNode &obj) {type = obj.type; node = obj.node;}
    void operator= (VMTypeNode const &obj) {type = obj.type; node = obj.node;}  // 似乎必须要加上 const 才可以
};

struct ServStat
{
    int id;
    int type;
    int group;  // 1 for single, 2 for double, 3 for empty
    NodeStat nodes[2];
    unordered_map<int, VMTypeNode> vms;  // vm_id -> vm_type
    
    ServStat() {}
    ServStat(int id, int type);
    ServStat(const ServStat &obj);
    void operator= (ServStat const &obj);
    bool can_hold_vm(int vm_core, int vm_mem, int serv_node);
    void add_vm(int vm_type, int vm_id, int serv_node);
    void del_vm(int vm_id);

    // 核满，或内存满
    bool is_full();
    bool is_full_load();
    bool is_empty();

    // mantain the state table
    void node_map_remove(int serv_id, int node);
    void node_map_add(int serv_id, int node);
    void serv_map_remove(int serv_id);
    void serv_map_add(int serv_id);

    // status of non-empty servs
    void nnode_map_remove(int serv_id, int node);
    void nnode_map_add(int serv_id, int node);
    void nserv_map_remove(int serv_id);
    void nserv_map_add(int serv_id);

    void _change_serv_stat(int op, int serv_node, int core, int mem);
};


struct ServStatList
{
    map<int, ServStat> servs;  // id -> xxx
    node_map node_core_mem_map;  // node core -> (node mem, (serv_id, node))
    node_map nnode_core_mem_map; // 非空服务器节点的状态
    serv_map serv_core_mem_map;  // min core -> (min mem, (serv_id))
    serv_map nserv_core_mem_map; // 非空服务器节点最小资源的状态
    vm_map vm_stat;  // vm core -> (vm mem, (vm_node -> (vm_id))

    ServStat& operator[] (int id);
    ServStat& get_serv_by_vm_id(int vm_id);
    void add_serv(int id, int type);
    void mig_vm(int vm_id, int serv_id, int serv_node);
    void mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node);
};


extern ServStatList serv_stats;
extern multiset<pair<int, int>> non_full_load_servs;  // (num_vm, serv_id)


#endif