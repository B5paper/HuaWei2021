#ifndef TYPES_2
#define TYPES_2

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

struct ServSpec
{
    int cores;
    int memcap;
    int cost;
    int consume;

    ServSpec() {}
    ServSpec(int cores, int memcap, int cost, int consume)
    : cores(cores), memcap(memcap), cost(cost), consume(consume) {}
};

typedef vector<ServSpec> ServSpecList;

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

struct DayReq
{
    vector<int> op;  // 0 for delete, 1 for add
    vector<int> vm_type;  // -1 for none
    vector<int> id;
};

typedef vector<DayReq*> DayReqList;

#endif