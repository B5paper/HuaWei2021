#ifndef STATUS
#define STATUS

#include "types.h"
#include <unordered_map>
using namespace std;

// ream to <id, node>, just single node state
extern unordered_map<int, pair<int, int>> ream_to_serv_id;
extern unordered_map<int, int> dream_to_serv_id;  // 双节点

struct NodeStat
{
    // int cores_used;
    // int mem_used;
    int cores_ream;
    int mem_ream;

    // NodeStat(): cores_used(0), mem_used(0) {}
    NodeStat() {}
    NodeStat(const NodeStat &obj);
    // NodeStat(int core_used, int mem_used)
    // : cores_used(core_used), mem_used(mem_used) {}

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

#include <iostream>
struct ServStat
{
    int id;
    int type;
    NodeStat nodes[2];
    unordered_map<int, VMTypeNode> vms;  // vm_id -> vm_type
    
    ServStat(): id(-1) {}
    ServStat(int type, ServSpecList &serv_specs): type(type), id(-1) {
        for (int i = 0; i < 2; ++i)
        {
            nodes[i].cores_ream = serv_specs[type].cores / 2;
            nodes[i].mem_ream = serv_specs[type].memcap / 2;
        }
    }
    ServStat(const ServStat &obj);
    void operator= (ServStat &obj);
    void operator= (ServStat const &obj);
    // int cores_ream(int serv_node, ServSpecList &serv_specs);
    // int mem_ream(int serv_node, ServSpecList &serv_specs);
    bool can_hold_vm(int vm_core, int vm_mem, int server_node);
    void add_vm(int vm_type, int vm_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
    void del_vm(int vm_id, VMSpecList &vm_specs);
    // 核满，或内存满
    bool is_full(ServSpecList &serv_specs, VMSpecList &vm_specs);
    bool is_empty();
    
    void _change_serv_stat(int op, int serv_node, int core, int mem);
};


struct ServStatList
{
    unordered_map<int, ServStat> servs;  // id -> xxx

    ServStat& operator[] (int id);
    ServStat& get_serv_by_vm_id(int vm_id);
    void add_serv(int id, int type, ServSpecList &serv_specs);
    void mig_vm(int vm_id, int serv_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
    void mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
};


#endif