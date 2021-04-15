#ifndef SELECT
#define SELECT

#include "status.h"
#include "migrate.h"

// unordered_map<int, vector<pair<int, int>>> ream;
// unordered_map<int, unordered_set<int>> dream;

struct Res
{
    int vm_id;
    int serv_id;
    int serv_node;
    int res;

    Res() {}
    Res(int vm_id, int serv_id, int serv_node, int res)
    : vm_id(vm_id), serv_id(serv_id), serv_node(serv_node), res(res) {}
};

struct RelRes
{
    int vm_id;
    int serv_id;
    int serv_node;
    float rel_res;

    RelRes() {}
    RelRes(int vm_id, int serv_id, int serv_node, int rel_res)
    : vm_id(vm_id), serv_id(serv_id), serv_node(serv_node), rel_res(rel_res) {}
};

// calcute residual between virtual machine and server
int calc_res(int vm_type, ServStat &serv_stat, int serv_node, VMSpecList &vm_specs);

// first fitting
OneAssignScheme vm_select_server_ff(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

// best fitting
OneAssignScheme mig_vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
OneAssignScheme add_vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
// 考虑使用 计算核心数的残差为 0.1，内存的残差为 0.15，两者加起来为 0.25。如果这种策略效果好，那么保留下来，应用到下面的 bfd 函数里
float calc_rel_res(int vm_type, ServStat &serv_stat, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);
OneAssignScheme vm_select_server_bf_rel(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
OneAssignScheme add_vm_select_server_bf_rel(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);


// best fitting decreasing
int calc_vm_score(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
// 优先考虑双节点虚拟机，优先考虑占资源大的（需要修改函数参数，写成 list 模式，因为要对多个虚拟机进行评价）
OneAssignScheme add_vm_select_server_bfd(ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
OneAssignScheme vm_select_server_bfd(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
// 考虑到服务器两个节点的平衡度，对于双节点的服务器，优先选择两个节点平衡度高的服务器
OneAssignScheme add_vm_select_server_bfd_plus(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);


OneAssignScheme mig_serv_select_vm_bf(int serv_id, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

// double point
OneAssignScheme mig_vm_select_serv_dp(int vm_id, int vm_type, int idx_serv, int idx_right, vector<IdNum> &id_nums, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);


// 散列表查询实现 best fit
OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched, ServStatList &serv_stats, VMSpecList &vm_specs);
OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type, ServStatList &serv_stats, VMSpecList &vm_specs);
#endif