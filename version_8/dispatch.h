#ifndef DISPATCH
#define DISPATCH

#include "types.h"
#include "status.h"
#include "io.h"
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

    void add_server(int server_type);

    vector<ServerTypeNum> get_ordered_server_types();
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

// ExpandScheme expand_servers(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs);
void process_del_op();
void dispatch_2(ServSpecList &serv_sepcs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps);
void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
// void print_vm_info(int vm_type, VMSpecList &vm_specs);
// void print_server_info(int server_id, ServStatList &server_stats, ServSpecList &server_specs);




#endif