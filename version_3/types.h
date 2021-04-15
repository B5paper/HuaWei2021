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
    int name_id;
    int cores;
    int memcap;
    int cost;
    int consume;

    ServerSpec() {}
    ServerSpec(int name_id, int cores, int memcap, int cost, int consume)
    : name_id(name_id), cores(cores), memcap(memcap), cost(cost), consume(consume) {}
};

struct VMSpec
{
    int name_id;
    int cores;
    int memcap;
    int nodes;  // 0 for node A, 1 for node B, 2 for both

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

struct ServerSpecList
{
    vector<int> name_id;
    vector<int> cores;
    vector<int> memcap;
    vector<int> cost;
    vector<int> consume;
};

struct VMSpecList
{
    vector<int> name_id;
    vector<int> cores;
    vector<int> memcap;
    vector<int> nodes;  // 1 for single node, 2 for two nodes
};

typedef vector<DayReq*> DayReqList;


#endif