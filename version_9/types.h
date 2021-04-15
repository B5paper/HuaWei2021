#ifndef TYPES
#define TYPES

#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

extern unordered_map<string, int> server_names_map;
extern int server_names_id_top;
extern vector<string> server_names_vec;

extern unordered_map<string, int> vm_names_map;
extern int vm_names_id_top;
extern vector<string> vm_names_vec;

extern int min_core_vm_type;
extern int min_mem_vm_type;

struct VMSpec
{
    string name;
    int cores;
    int memcap;
    int nodes;  // 1 for single node, 2 for two nodes

    VMSpec() {}
    VMSpec(string name, int cores, int memcap, int nodes)
    : name(name), cores(cores), memcap(memcap), nodes(nodes) {}
};

typedef vector<VMSpec> VMSpecList;

struct ServSpec
{
    int cores;
    int memcap;
    int cost;
    int consume;

    ServSpec() {}
    ServSpec(int cores, int memcap, int cost, int consume)
    : cores(cores), memcap(memcap), cost(cost), consume(consume) {}

    bool can_hold_vm(int vm_type, VMSpecList &vm_specs);
};

typedef vector<ServSpec> ServSpecList;

struct DayReq
{
    vector<int> op;  // 0 for delete, 1 for add
    vector<int> vm_type;  // -1 for none
    vector<int> id;
};

typedef vector<DayReq*> DayReqList;

struct OneAssignScheme
{
    int vm_id;
    int server_id;
    int server_node;  // 0 for node A, 1 for node B, 2 for both
    int old_serv_id;

    OneAssignScheme() {}
    OneAssignScheme(int vm_id, int server_id, int server_node)
    : vm_id(vm_id), server_id(server_id), server_node(server_node) {}
    OneAssignScheme(int vm_id, int server_id, int server_node, int old_serv_id)
    : vm_id(vm_id), server_id(server_id), server_node(server_node), old_serv_id(old_serv_id) {}
};

typedef unordered_map<int, OneAssignScheme> AssignScheme;  // vm_idxs -> 

#endif