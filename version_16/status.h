#ifndef STATUS
#define STATUS

#include "types.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include "args.h"
using namespace std;


typedef map<int, map<int, set<pair<int, int>>>> node_map;
typedef map<int, map<int, set<int>>> serv_map;
typedef map<int, map<int, map<int, set<int>>, greater<int>>, greater<int>> vm_map;

extern map<int, set<int>> num_vm_to_serv_ids;
extern map<int, int> vm_id_to_serv_id;
extern int num_vms_total;


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

#include <iostream>
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

struct comp_vms
{
    template<typename T>
    bool operator() (T &s1, T&s2)
    {
        if (serv_stats[s1].vms.size() < serv_stats[s2].vms.size())
            return true;
        return false;
    }
};

extern multiset<pair<int, int>> non_full_load_servs;  // (num_vm, serv_id)

struct VirtualServ
{
    NodeStat nodes[2];
    unordered_map<int, VMTypeNode> vms;
    int serv_type;

    VirtualServ(int serv_type): serv_type(serv_type)
    {
        int serv_core = serv_specs[serv_type].cores;
        int serv_mem = serv_specs[serv_type].memcap;
        for (int i = 0; i < 2; ++i)
        {
            nodes[i].cores_ream = serv_core / 2;
            nodes[i].mem_ream = serv_mem / 2;
        }
    }

    // 对于双节点虚拟机，直接添加，对于单节点虚拟机，使用 best fit
    OneAssignScheme add_vm(int vm_id, int vm_type, int node = -1)
    {
        int vm_core = vm_specs[vm_type].cores;
        int vm_mem = vm_specs[vm_type].memcap;
        int vm_node = vm_specs[vm_type].nodes;

        if (vm_node == 2)
        {
            if (nodes[0].cores_ream >= vm_core / 2 && nodes[0].mem_ream >= vm_mem / 2 &&
                nodes[1].cores_ream >= vm_core / 2 && nodes[1].mem_ream >= vm_mem / 2)
            {
                for (int i = 0; i < 2; ++i)
                    nodes[i]._change_node_stat(1, vm_core / 2, vm_mem / 2);
                vms.emplace(vm_id, VMTypeNode(vm_type, 2));
                return OneAssignScheme(vm_id, 0, 2);
            }
        }
        else
        {
            set<pair<int, int>> res;  // (res, node)
            for (int i = 0; i < 2; ++i)
            {
                if (nodes[i].cores_ream >= vm_core && nodes[i].mem_ream >= vm_mem)
                    res.emplace(nodes[i].cores_ream - vm_core + nodes[i].mem_ream - vm_mem, i);
            }

            if (!res.empty())
            {
                auto node_iter = res.begin();
                nodes[node_iter->second]._change_node_stat(1, vm_core, vm_mem);
                vms.emplace(vm_id, VMTypeNode(vm_type, node_iter->second));
                return OneAssignScheme(vm_id, 0, node_iter->second);
            }
                
        }

        return OneAssignScheme(-1, -1, -1, -1);
    }

    int get_res()
    {
        return (nodes[0].cores_ream + nodes[1].cores_ream) * mem_per_core + nodes[0].mem_ream  + nodes[1].mem_ream;
    }
};


#endif