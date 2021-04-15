#ifndef DISPATCH
#define DISPATCH

#include "types.h"
#include "io.h"
#include "status.h"
#include <unordered_map>
#include <algorithm>
using namespace std;

struct ServerTypeNum
{
    int server_type;
    int num;

    ServerTypeNum() {}
    ServerTypeNum(int server_type, int num)
    : server_type(server_type), num(num) {}
};

struct ServIdType
{
    int id;
    int type;

    ServIdType() {}
    ServIdType(int id, int type): id(id), type(type) {}
};

struct PurchaseScheme
{
    // vector<OnePurchaseScheme> scheme;
    vector<int> server_types;

    void add_server(int server_type)
    {
        server_types.push_back(server_type);
    }

    vector<ServerTypeNum> get_ordered_server_types()
    {
        vector<ServerTypeNum> ordered_server_type_num;
        if (server_types.empty())
            return ordered_server_type_num;

        sort(server_types.begin(), server_types.end());
        ordered_server_type_num.push_back(ServerTypeNum(server_types[0], 1));
        for (int i = 1; i < server_types.size(); ++i)
        {
            if (server_types[i] == ordered_server_type_num.back().server_type)
                ordered_server_type_num.back().num++;
            else
                ordered_server_type_num.push_back(ServerTypeNum(server_types[i], 1));
        }

        return ordered_server_type_num;
    }
};

// typedef vector<OnePurchaseScheme> PurchaseScheme;

struct MigrateScheme
{
    vector<int> vm_ids;
    vector<int> server_ids;
};

struct OneAssignScheme
{
    int vm_id;
    int server_id;
    int server_node;  // 0 for node A, 1 for node B, 2 for both

    OneAssignScheme() {}
    OneAssignScheme(int vm_id, int server_id, int server_node)
    : vm_id(vm_id), server_id(server_id), server_node(server_node) {}
};

struct AddReq
{
    int vm_type;
    int vm_id;

    AddReq() {}
    AddReq(int vm_type, int vm_id)
    : vm_type(vm_type), vm_id(vm_id) {}
};

typedef unordered_map<int, OneAssignScheme> AssignScheme;  // vm_idxs -> 

void assign_vms();
void process_del_op();
void dispatch(ServSpecList &serv_sepcs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps);
void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);


#endif