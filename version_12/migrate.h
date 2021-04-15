#ifndef MIGRATE
#define MIGRATE

#include "types.h"
#include "status.h"

struct OneMigScheme
{
    int vm_id;
    int serv_id;
    int serv_node;

    OneMigScheme(int vm_id, int serv_id, int serv_node)
    : vm_id(vm_id), serv_id(serv_id), serv_node(serv_node) {}
};

typedef vector<OneMigScheme> MigScheme;
void migrate(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs, 
            int &num_miged, bool &stop_mig);


// 如果发现某个服务器有虚拟机迁移不掉，那么就放弃这台服务器！反正也腾不空！
// 比较服务器中虚拟机的数量和虚拟机所占的资源数
struct IdNum
{
    int serv_id;
    int num_vm;
    int num_vm_res;

    IdNum(int serv_id, int num_vm, int num_vm_res)
    : serv_id(serv_id), num_vm(num_vm), num_vm_res(num_vm_res) {}

    // IdNum(int serv_id, int num_vm)
    // : serv_id(serv_id), num_vm(num_vm) {}
};
bool comp_id_num(IdNum &i1, IdNum &i2);

struct ServIdNode
{
    int id;
    int node;
    ServIdNode() {}
    ServIdNode(int id, int node): id(id), node(node) {}
};

struct VMInfo
{
    int vm_id;
    int vm_type;
    int serv_id;
    int serv_node;

    VMInfo() {}
    VMInfo(int vm_id, int vm_type, int serv_id, int serv_node)
    : vm_id(vm_id), vm_type(vm_type), serv_id(serv_id), serv_node(serv_node) {}
};

struct MigOp
{
    int vm_id;
    int vm_type;
    int old_serv_id;
    int old_serv_node;
    int new_serv_id;
    int new_serv_node;

    MigOp() {}
    MigOp(int vm_id, int vm_type, int old_serv_id, int old_serv_node, int new_serv_id, int new_serv_node)
    : vm_id(vm_id), vm_type(vm_type), old_serv_id(old_serv_id), old_serv_node(old_serv_node), new_serv_id(new_serv_id), new_serv_node(new_serv_node) {}
};

ServIdNode vm_select_new_single(int vm_id, int vm_type, vector<pair<int, int>> &single_new, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

#endif