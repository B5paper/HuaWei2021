#ifndef STATUS
#define STATUS

#include <unordered_map>
#include "types.h"
using namespace std;

struct NodeStat
{
    int cores_used;
    int mem_used;

    NodeStat(): cores_used(0), mem_used(0) {}
    NodeStat(const NodeStat &obj);
    NodeStat(int core_used, int mem_used)
    : cores_used(core_used), mem_used(mem_used) {}

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
    int type;
    NodeStat nodes[2];
    unordered_map<int, VMTypeNode> vms;  // vm_id -> vm_type
    
    ServStat() {}
    ServStat(int type): type(type) {}
    ServStat(const ServStat &obj);
    void operator= (ServStat &obj);
    void operator= (ServStat const &obj);
    int cores_ream(int serv_node, ServSpecList &serv_specs);
    int mem_ream(int serv_node, ServSpecList &serv_specs);
    bool can_hold_vm(int vm_core, int vm_mem, int server_node, ServSpecList &serv_specs);
    void add_vm(int vm_type, int vm_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
    void del_vm(int vm_id, VMSpecList &vm_specs);
    
    void _change_serv_stat(int op, int serv_node, int core, int mem);
};


struct ServStatList
{
    unordered_map<int, ServStat> servs;  // id -> xxx

    ServStat& operator[] (int id);
    ServStat& get_serv_by_vm_id(int vm_id);
    void add_serv(int id, int type) {}
};


#endif