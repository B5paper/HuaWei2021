
#include <random>
#include <ctime>
#include <algorithm>
#include <iostream>
#include "dispatch.h"
#include "bfd.h"

void simplex(vector<int> &server_ty)
{

}


float approx_residual(int vm_type, int server_type, ServerSpecList &server_specs, VMSpecList &vm_specs, float ratio_cores)
{
    float cores_weight;
    float mem_weight;
    if (vm_specs.nodes[vm_type] == 1)
    {
        cores_weight = server_specs.cores[server_type] / 2 - vm_specs.cores[vm_type];
        mem_weight = server_specs.memcap[server_type] / 2 - vm_specs.memcap[vm_type];
    }
    else
    {
        cores_weight = server_specs.cores[server_type] - vm_specs.cores[vm_type];
        mem_weight = server_specs.memcap[server_type] - vm_specs.memcap[vm_type];
    }
    return cores_weight * ratio_cores + mem_weight * (1 - ratio_cores);
}

float approx_residual(int vm_type, int server_id, int server_node, ServerStatList &server_stat, ServerSpecList &server_specs, VMSpecList &vm_specs, float ratio_cores)
{
    float cores_weight;
    float mem_weight;
    int server_type = server_stat.servs[server_id].type;

    int server_cores_cap;
    int server_cores_used;
    int vm_cores_cap;

    if (!server_stat.can_hold_vm(vm_type, server_id, server_node, server_specs, vm_specs))
        return __FLT_MAX__;

    switch (server_node)
    {
        case 0:
        case 1:
        
        cores_weight = server_specs.cores[server_type] / 2 - server_stat.servs[server_id].nodes[server_node].cores_used - vm_specs.cores[vm_type];
        mem_weight = server_specs.memcap[server_type] / 2 - server_stat.servs[server_id].nodes[server_node].mem_used - vm_specs.memcap[vm_type];
        // if (cores_weight < 0 || mem_weight < 0)
        //     return __FLT_MAX__;
        break;

        case 2:
        cores_weight = server_specs.cores[server_type] - server_stat.servs[server_id].cores_used - vm_specs.cores[vm_type];
        mem_weight = server_specs.memcap[server_type] - server_stat.servs[server_id].mem_used - vm_specs.memcap[vm_type];
        // if (cores_weight < 0 || mem_weight < 0)
        //     return __FLT_MAX__;
        break;
    }
    return cores_weight * ratio_cores + mem_weight * (1 - ratio_cores);
}

bool comp_res(Residual &res_1, Residual &res_2)
{
    if (res_1.res < res_2.res)
        return true;
    if (res_1.res == res_2.res)
    {
        if (res_1.server_id < res_2.server_id)
            return true;
        if (res_1.server_id == res_2.server_id)
        {
            if (res_1.server_node < res_2.server_node)
                return true;
        }
    }
    return false;
}

float calc_residual(int vm_cores, int vm_mem, int server_node, ServerStat &server_stat, ServerSpecList &server_specs)
{
    float ratio_cores = 0.7;

    float cores_weight;
    float mem_weight;
    int server_type = server_stat.type;

    int server_cores_cap;
    int server_cores_used;
    int vm_cores_cap;

    if (!server_stat.can_hold_vm(vm_cores, vm_mem, server_node, server_specs))
        return __FLT_MAX__;
    
    switch (server_node)
    {
        case 0:
        case 1:
        cores_weight = server_specs.cores[server_type] / 2 - server_stat.nodes[server_node].cores_used - vm_cores;
        mem_weight = server_specs.memcap[server_type] / 2 - server_stat.nodes[server_node].mem_used - vm_mem;
        // if (cores_weight < 0 || mem_weight < 0)
        //     return __FLT_MAX__;
        break;

        case 2:
        cores_weight = server_specs.cores[server_type] - server_stat.nodes[server_node].cores_used - vm_cores;
        mem_weight = server_specs.memcap[server_type] - server_stat.nodes[server_node].mem_used - vm_mem;
        // if (cores_weight < 0 || mem_weight < 0)
        //     return __FLT_MAX__;
        break;
    }
    return cores_weight * ratio_cores + mem_weight * (1 - ratio_cores);
}

vector<Residual> calc_residuals(int vm_core, int vm_mem, int vm_node, ServerStatList &server_stats, ServerSpecList &server_specs)
{
    vector<Residual> residuals;
    float res = 0;
    for (int j = 0; j < server_stats.servs.size(); ++j)
    {
        if (vm_node == 1)
        {
            float min_res = __FLT_MAX__;
            int min_node = 0;
            for (int k = 0; k < 2; ++k)
            {
                res = calc_residual(vm_core, vm_mem, k, server_stats.servs[j], server_specs);
                // res = approx_residual(vm_type, j, k, server_stat, server_specs, vm_specs, 0.7);
                if (res < min_res)
                {
                    min_res = res;
                    min_node = k;
                }
            }
            residuals.push_back(Residual(-1, j, min_node, min_res));
        }
        if (vm_node == 2)
        {
            res = calc_residual(vm_core, vm_mem, 2, server_stats.servs[j], server_specs);
            residuals.push_back(Residual(-1, j, 2, res));
        }
    }
    return residuals;
}


ServIdNode vm_select_server(int vm_id, int vm_type, ServerStatList &server_stats, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    int vm_core = vm_specs.cores[vm_type];
    int vm_mem = vm_specs.memcap[vm_type];
    int vm_node = vm_specs.nodes[vm_type];

    for (int i = 0; i < server_stats.servs.size(); ++i)
    {
        if (vm_node == 1)
        {
            for (int j = 0; j < 2; ++j)
            {
                if (server_stats.servs.at(i).can_hold_vm(vm_core, vm_mem, vm_node, server_specs))
                {
                    return ServIdNode(i, vm_node);
                }
            }
        }

        if (vm_node == 2)
        {
            if (server_stats.servs.at(i).can_hold_vm(vm_core, vm_mem, vm_node, server_specs))
            {
                return ServIdNode(i, 2);
            }
        }
    }

    return ServIdNode(-1, -1);

    // 计算适应度 residual
    vector<Residual> residuals = calc_residuals(vm_core, vm_mem, vm_node, server_stats, server_specs);

    // 按最小的 residual 开始尝试放置第 i 个虚拟机
    sort(residuals.begin(), residuals.end(), comp_res);
    for (int j = 0; j < residuals.size(); ++j)
    {
        int server_id = residuals[j].server_id;
        int server_node = residuals[j].server_node;
        if (server_stats.servs[server_id].can_hold_vm(vm_core, vm_mem, server_node, server_specs))
            return ServIdNode(server_id, server_node);
    }
    return ServIdNode(-1, -1);
}

bool bfd(OneAssignScheme &assign_scheme, int vm_id, int vm_type, ServerStatList &server_stats, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (server_stats.servs.empty() == 0)
        return false;

    for (int i = 0; i < server_stats.servs.size(); ++i)
    {
        
    }
}

bool bfd_2(AssignScheme &assign_scheme, vector<int> &vm_ids, vector<int> &vm_types, ServerStatList &server_stat, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (server_stat.servs.size() == 0)
        return false;

    vector<float> vm_scores;

    // normalize vms
    int max_vm_core = 0;
    int max_vm_mem = 0;
    for (int i = 0; i < vm_types.size(); ++i)
    {
        if (vm_specs.cores[vm_types[i]] > max_vm_core)
            max_vm_core = vm_specs.cores[vm_types[i]];
        if (vm_specs.memcap[vm_types[i]] > max_vm_mem)
            max_vm_mem = vm_specs.memcap[vm_types[i]];
    }

    // assign each vm a score
    float ratio_core;
    for (int i = 0; i < vm_types.size(); ++i)
    {
        vm_scores.push_back(ratio_core * vm_specs.cores[i] / (float)max_vm_core + (1 - ratio_core) * vm_specs.memcap[i] / (float)max_vm_mem);
    }

    // sort vms by scores
    vector<int> sorted_args = argsort(vm_scores);
    reverse(sorted_args.begin(), sorted_args.end());

    // best fit decreasing
    // if succeed, return true, false else
    for (int i = 0; i < sorted_args.size(); ++i)  // 对虚拟机请求进行遍历
    {
        int vm_id = vm_ids[sorted_args[i]];
        int vm_type = vm_types[sorted_args[i]];
        int vm_node = vm_specs.nodes[vm_type];
        ServIdNode serv_id_node = vm_select_server(vm_id, vm_type, server_stat, server_specs, vm_specs);
        if (serv_id_node.serv_id == -1)
            return false;

        assign_scheme.insert(make_pair(vm_id, OneAssignScheme(vm_id, serv_id_node.serv_id, serv_id_node.serv_node)));
        server_stat.add_vm(vm_type, serv_id_node.serv_id, serv_id_node.serv_node, server_specs, vm_specs);
    }
    return true;
}

// int purchase_server(AssignScheme &assign_scheme, vector<int> &vm_types, ServerStat &server_stat, ServerSpecList &server_specs, VMSpecList &vm_specs)
// {
//     AssignScheme assign_scheme;
//     int server_type;
//     while (true)
//     {
//         if (bfd(assign_scheme, vm_types, server_stat, server_specs, vm_specs))
//         {
//             break;
//         }
//         else
//         {
//             server_stat.(server_types.end() - 1);
            
//             server_types.push_back(server_type);
//         }
//     }
//     return server_type;
// }
