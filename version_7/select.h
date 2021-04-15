#ifndef SELECT
#define SELECT

#include "types_2.h"
#include "status_2.h"

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

// calcute residual between virtual machine and server
int calc_res(int vm_type, ServStat &serv_stat, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs);

// first fitting
OneAssignScheme vm_select_server_ff();

// best fitting
OneAssignScheme vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

#endif