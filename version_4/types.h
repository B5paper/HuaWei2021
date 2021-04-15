#ifndef TYPES
#define TYPES

#include <vector>
#include <unordered_map>
#include <string>
using namespace std;

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;

// map string to integer
extern unordered_map<string, int> server_names_map;
extern int server_names_id_top;
extern vector<string> server_names_vec;

extern unordered_map<string, int> vm_names_map;
extern int vm_names_id_top;
extern vector<string> vm_names_vec;

string server_id_to_name(int id);
string vm_id_to_name(int id);


// machine specifications and requirement infomation
struct ServerSpec
{
    int cores;
    int memcap;
    int cost;
    int consume;

    ServerSpec() {}
    ServerSpec( int cores, int memcap, int cost, int consume)
    : cores(cores), memcap(memcap), cost(cost), consume(consume) {}
};

struct VMSpec
{
    int name_id;
    int cores;
    int memcap;
    int nodes;  // 1 for single node, 2 for two nodes

    VMSpec() {}
    VMSpec(int name_id, int cores, int memcap, int nodes):
    name_id(name_id), cores(cores), memcap(memcap), nodes(nodes) {}
};

struct DayReq
{
    vector<int> op;  // 0 for delete, 1 for add
    vector<int> name_id;  // -1 for none
    vector<int> id;
};

struct VMSpecList
{
    vector<int> cores;
    vector<int> memcap;
    vector<int> nodes;  // 1 for single node, 2 for two nodes
};

struct ServerSpecList
{
    vector<int> cores;
    vector<int> memcap;
    vector<int> cost;
    vector<int> consume;
    bool can_hold_vm(int server_type, int vm_type, ServerSpecList &server_specs, VMSpecList &vm_specs);
};

typedef vector<DayReq*> DayReqList;


#endif