#ifndef STATUS
#define STATUS

#include "types.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
using namespace std;

// bool comp_node_mem(pair<int, int> &n1, pair<int, int> &n2);
struct comp_node_mem
{
    template <typename T>
    bool operator()(const T &n1, const T &n2) const;
};

// ream to <id, node>, just single node state
extern map<int, vector<pair<int, int>>> ream;
extern int ream_max;  // 记录单节点资源余量的最大值
extern map<int, vector<int>> dream;  // 双节点
extern int dream_max;  // 记录双节点资源余量的最大值
// extern unordered_map<int, pair<int, int>> core_to_node;
// extern unordered_map<int, pair<int, int>> mem_to_node;
extern map<int, vector<pair<int, int>>> serv_core_mem_map;
extern map<int, vector<pair<int, int>>> serv_core_map;  // core -> serv_id, node
extern map<int, set<pair<int, pair<int, int>>>> serv_core_map_2;  // core -> (mem, (serv_id, node))
extern map<int, vector<pair<int, int>>> serv_mem_map;
extern map<int, vector<int>> dserv_core_mem_map;
extern map<int, vector<int>> dserv_core_map;
extern map<int, vector<int>> dserv_mem_map;
extern map<int, vector<int>> vm_core_mem_map;  // cores + mems -> vm_id
extern multimap<int, pair<int, int>> vm_core_mem_map_2;  // cores + mems -> (vm_id, vm_type)，最大也就 800 个，因为虚拟机才 800 种
extern map<int, vector<int>> vm_core_map;
extern map<int, vector<int>> vm_mem_map;
extern map<int, vector<int>> dvm_core_mem_map;
extern map<int, vector<int>> dvm_core_map;
extern map<int, set<pair<int, int>>> dvm_core_map_2;
extern map<int, vector<int>> dvm_mem_map;
extern map<int, set<int>> num_vm_to_serv_ids;

extern map<int, int> vm_id_to_serv_id;

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
    int group;  // 1 for single, 2 for double, 3 for empty
    NodeStat nodes[2];
    unordered_map<int, VMTypeNode> vms;  // vm_id -> vm_type
    
    ServStat(): id(-1) {}
    ServStat(int id, int type): id(id), type(type) {
        for (int i = 0; i < 2; ++i)
        {
            nodes[i].cores_ream = serv_specs[type].cores / 2;
            nodes[i].mem_ream = serv_specs[type].memcap / 2;
            // update_hash_state(i);
        }
        // clear_hash_state();
        update_hash_state(2);
    }
    ServStat(const ServStat &obj);
    void operator= (ServStat &obj);
    void operator= (ServStat const &obj);
    // int cores_ream(int serv_node, ServSpecList &serv_specs);
    // int mem_ream(int serv_node, ServSpecList &serv_specs);
    bool can_hold_vm(int vm_core, int vm_mem, int server_node);
    void add_vm(int vm_type, int vm_id, int serv_node, VMSpecList &vm_specs);
    void del_vm(int vm_id, VMSpecList &vm_specs);

    // 核满，或内存满
    bool is_full();
    bool is_empty();
    void clear_hash_state(int node = 2);
    void update_hash_state(int node = 2);
    void clear_vm_hash_state(int vm_id, VMSpecList &vm_specs);
    void update_vm_hash_state(int vm_id, int vm_type, VMSpecList &vm_specs);
    void _erase_serv_map(map<int, vector<pair<int, int>>> &serv_map, char res_type, int node);
    void _erase_dserv_map(map<int, vector<int>> &serv_map, char res_type);
    void _erase_vm_map(map<int, vector<int>> &vm_map, int res);

    void _update_serv_map(map<int, vector<pair<int, int>>> &ser_map, char res_type, int node);
    void _update_dserv_map(map<int, vector<int>> &dserv_map, char res_type);
    void _update_vm_map(map<int, vector<int>> &vm_map, int vm_id, int res);

    void _change_serv_stat(int op, int serv_node, int core, int mem);
};


struct ServStatList
{
    map<int, ServStat> servs;  // id -> xxx

    ServStat& operator[] (int id);
    ServStat& get_serv_by_vm_id(int vm_id);
    void add_serv(int id, int type);
    void mig_vm(int vm_id, int serv_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
    void mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node, VMSpecList &vm_specs);
    set<int> find_vm(int res, char type, int node);  // (vm_id), type:
    // 'c': for core, 'm': for mem, 'b': both core and memory
    set<int> find_vm(int core, int mem, int node);  // (vm_id), node: 1 for single, 2 for two noddes, 3 for either
    set<int> find_vm_coarsely(int core, int max_core_diff, int mem, int max_mem_diff, int node);  // find the vm that
    // satisfy the difference requirement
    vector<set<int>> find_vms(int core, int mem, int node);  // (comb(vm_id)), node: 1: just single node, 2: 
    // just two nodes, 3: mixed single node and two nodes
    set<int> find_serv(int res, char type, int node, bool except_full = false, bool except_empty = false);  // (serv_id)
    set<int> find_serv_coaresly(int core, int max_core_diff, int mem, int max_mem_diff, int node);
    void map_serv_id(unordered_map<int, int> &id_old_to_new, int idx_start, int idx_end);
};

extern ServStatList serv_stats;


#endif