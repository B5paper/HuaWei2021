#ifndef DISPATCH
#define DISPATCH

#include "types.h"
#include "status.h"
#include "io.h"
#include "migrate.h"
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

struct OnePurchaseScheme
{
    int type;
    int serv_stat_id;
    int purchase_id;

    OnePurchaseScheme() {}
    OnePurchaseScheme(int type, int serv_stat_id)
    : type(type), serv_stat_id(serv_stat_id) {}
};


typedef vector<OnePurchaseScheme> PurchaseScheme;

bool comp_serv_type(OnePurchaseScheme &p1, OnePurchaseScheme &p2);
void sort_and_fill_purchase_id(int idx_start, PurchaseScheme &purchase_scheme);
void map_serv_id(PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, MigScheme &mig_scheme, ServStatList &serv_stats);
vector<ServerTypeNum> purchase_scheme_to_serv_type_num(PurchaseScheme &purchase_scheme);

// struct PurchaseScheme
// {
//     // vector<OnePurchaseScheme> scheme;
//     vector<int> server_types;

//     void add_server(int server_type);

//     vector<ServerTypeNum> get_ordered_server_types();
// };


struct AddReq
{
    int vm_type;
    int vm_id;

    AddReq() {}
    AddReq(int vm_type, int vm_id)
    : vm_type(vm_type), vm_id(vm_id) {}
};



// ExpandScheme expand_servers(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs);
void dispatch(ServSpecList &serv_sepcs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps);
void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
void process_add_op_span(int idx_req, int idx_day, int num_days, int span, int vm_id, int vm_type, 
    PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, 
    ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs);
void dispatch_span(int idx_day, int num_days, int span, ServSpecList &serv_sepcs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps);

#endif