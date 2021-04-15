#include "select.h"

OneAssignScheme vm_select_server_ff(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    vector<int> server_types_purchsed;

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    // 遍历当前所有的服务器，找到第一个能放下的服务器
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        switch (vm_node)
        {
            case 1:
            for (int i = 0; i < 2; ++i)
            {
                if (serv_iter->second.can_hold_vm(vm_core, vm_mem, i, serv_specs))
                {
                    return OneAssignScheme(vm_id, serv_iter->first, i);
                }
            }
            break;

            case 2:
            if (serv_iter->second.can_hold_vm(vm_core, vm_mem, 2, serv_specs))
            {
                return OneAssignScheme(vm_id, serv_iter->first, 2);
            }
            break;
        }
    }

    // 如果没有一个服务器能放得下，那么返回空值
    return OneAssignScheme(-1, -1, -1);
}


int calc_res(int vm_type, ServStat &serv_stat, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    if (!serv_stat.can_hold_vm(vm_core, vm_mem, serv_node, serv_specs))
        return -1;

    if (serv_node == 0 || serv_node == 1)
    {
        int cores_ream = serv_stat.cores_ream(serv_node, serv_specs);
        int mem_ream = serv_stat.mem_ream(serv_node, serv_specs);

        int cores_res = cores_ream - vm_core;
        int mem_res = mem_ream - vm_mem;

        return cores_res + mem_res;
    }

    if (serv_node == 2)
    {
        int cores_ream = 0, mem_ream = 0;
        for (int i = 0; i < 2; ++i)
        {
            cores_ream += serv_stat.cores_ream(i, serv_specs);
            mem_ream += serv_stat.mem_ream(i, serv_specs);
        }

        int cores_res = cores_ream - vm_core;
        int mem_res = mem_ream - vm_mem;

        return cores_res + mem_res;
    }

    return -1;
}

OneAssignScheme add_vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    
    // 计算某个虚拟机与所有服务器的适应差值
    vector<Res> ress;
    int res;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        // // 跳过 vm_id 当前所在的服务器
        // if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
        //     continue;

        // // 跳过本来就为空的服务器
        // if (serv_iter->second.vms.size() == 0)
        //     continue;

        if (vm_node == 1)
        {
            for (int i = 0; i < 2; ++i)
            {
                res = calc_res(vm_type, serv_iter->second, i, serv_specs, vm_specs);
                ress.push_back(Res(vm_id, serv_iter->first, i, res));
            }
        }
        else
        {
            res = calc_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            ress.push_back(Res(vm_id, serv_iter->first, 2, res));
        }
    }

    // 这里可以优化，其实只要上一步记录一个最小的 res 就可以了
    int res_min = INT32_MAX;
    int idx_min = -1;
    for (int i = 0; i < ress.size(); ++i)
    {
        res = ress[i].res;
        if (res < res_min && res > -1)
        {
            idx_min = i;
            res_min = res;
        }
    }

    if (idx_min != -1)
        return OneAssignScheme(vm_id, ress[idx_min].serv_id, ress[idx_min].serv_node);

    return OneAssignScheme(-1, -1, -1);
}


OneAssignScheme vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    
    // 计算某个虚拟机与所有服务器的适应差值
    vector<Res> ress;
    int res;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        // 跳过 vm_id 当前所在的服务器
        if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
            continue;

        // 跳过本来就为空的服务器
        if (serv_iter->second.vms.size() == 0)
            continue;

        if (vm_node == 1)
        {
            for (int i = 0; i < 2; ++i)
            {
                res = calc_res(vm_type, serv_iter->second, i, serv_specs, vm_specs);
                ress.push_back(Res(vm_id, serv_iter->first, i, res));
            }
        }
        else
        {
            res = calc_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            ress.push_back(Res(vm_id, serv_iter->first, 2, res));
        }
    }

    // 这里可以优化，其实只要上一步记录一个最小的 res 就可以了
    int res_min = INT32_MAX;
    int idx_min = -1;
    for (int i = 0; i < ress.size(); ++i)
    {
        res = ress[i].res;
        if (res < res_min && res > -1)
        {
            idx_min = i;
            res_min = res;
        }
    }

    if (idx_min != -1)
        return OneAssignScheme(vm_id, ress[idx_min].serv_id, ress[idx_min].serv_node);

    return OneAssignScheme(-1, -1, -1);
}
