#include "select.h"
#include <cmath>
#include <algorithm>
#include "migrate.h"

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
                if (serv_iter->second.can_hold_vm(vm_core, vm_mem, i))
                {
                    return OneAssignScheme(vm_id, serv_iter->first, i);
                }
            }
            break;

            case 2:
            if (serv_iter->second.can_hold_vm(vm_core, vm_mem, 2))
            {
                return OneAssignScheme(vm_id, serv_iter->first, 2);
            }
            break;
        }
    }

    // 如果没有一个服务器能放得下，那么返回空值
    return OneAssignScheme(-1, -1, -1);
}


int calc_res(int vm_type, ServStat &serv_stat, int serv_node, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    if (!serv_stat.can_hold_vm(vm_core, vm_mem, serv_node))
        return -1;

    if (serv_node == 0 || serv_node == 1)
    {
        int cores_res = serv_stat.nodes[serv_node].cores_ream - vm_core;
        int mem_res = serv_stat.nodes[serv_node].mem_ream - vm_mem;

        return cores_res + mem_res;
    }

    if (serv_node == 2)
    {
        int cores_ream = 0, mem_ream = 0;
        for (int i = 0; i < 2; ++i)
        {
            cores_ream += serv_stat.nodes[serv_node].cores_ream;
            mem_ream += serv_stat.nodes[serv_node].mem_ream;
        }

        int cores_res = cores_ream - vm_core;
        int mem_res = mem_ream - vm_mem;

        return cores_res + mem_res;
    }

    return -1;
}

// bool comp_id_num(IdNum &i1, IdNum &i2)
// {
//     return i1.num_vm < i2.num_vm ? true : false;
// }

OneAssignScheme add_vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (serv_stats.servs.size() == 0)
        return OneAssignScheme(-1, -1, -1);

    int vm_node = vm_specs[vm_type].nodes;
    
    // 计算某个虚拟机与所有服务器的适应差值
    // vector<Res> ress;
    int res;
    int threshold = 10;
    int res_min = INT32_MAX;
    int id_min = -1;
    int node_min = -1;
    vector<IdNum> id_nums;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        id_nums.push_back(IdNum(serv_iter->first, serv_iter->second.vms.size()));
    }
    sort(id_nums.begin(), id_nums.end(), comp_id_num);

    if (vm_node == 1)
    {
        for (int i = 0; i < id_nums.size(); ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                res = calc_res(vm_type, serv_stats[id_nums[i].serv_id], j, vm_specs);
                if (res == -1)
                    continue;
                if (res < threshold)
                    return OneAssignScheme(vm_id, id_nums[i].serv_id, j);
                if (res < res_min)
                {
                    id_min = id_nums[i].serv_id;
                    res_min = res;
                    node_min = j;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < id_nums.size(); ++i)
        {
            res = calc_res(vm_type, serv_stats[id_nums[i].serv_id], 2, vm_specs);
            if (res == -1)
                continue;
            if (res < threshold)
                return OneAssignScheme(vm_id, id_nums[i].serv_id, 2);
            if (res < res_min)
            {
                id_min = id_nums[i].serv_id;
                res_min = res;
                node_min = 2;
            }
        }
        
    }
    
    // for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    // {
    //     // // 跳过 vm_id 当前所在的服务器
    //     // if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
    //     //     continue;

    //     // // 跳过本来就为空的服务器
    //     // if (serv_iter->second.vms.size() == 0)
    //     //     continue;

    //     if (vm_node == 1)
    //     {
    //         for (int i = 0; i < 2; ++i)
    //         {
    //             res = calc_res(vm_type, serv_iter->second, i, serv_specs, vm_specs);
    //             if (res == -1)
    //                 continue;
    //             if (res < threshold)
    //                 return OneAssignScheme(vm_id, serv_iter->first, i);
    //             if (res < res_min)
    //             {
    //                 id_min = serv_iter->first;
    //                 res_min = res;
    //                 node_min = i;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         res = calc_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
    //         if (res == -1)
    //             continue;
    //         if (res < threshold)
    //             return OneAssignScheme(vm_id, serv_iter->first, 2);
    //         if (res < res_min)
    //         {
    //             id_min = serv_iter->first;
    //             res_min = res;
    //             node_min = 2;
    //         }
            
    //     }
    // }

    if (id_min != -1)
        return OneAssignScheme(vm_id, id_min, node_min);

    return OneAssignScheme(-1, -1, -1);
}


OneAssignScheme mig_vm_select_server_bf(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    
    // 计算某个虚拟机与所有服务器的适应差值
    // vector<Res> ress;
    int res;

    int res_min = INT32_MAX;
    // int idx_min = -1;
    int min_serv_id = -1;
    int min_serv_node = -1;

    int threshold = 20;

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
                res = calc_res(vm_type, serv_iter->second, i, vm_specs);
                if (res == -1)
                    continue;
                // ress.push_back(Res(vm_id, serv_iter->first, i, res));
                if (res < threshold)
                {
                    return OneAssignScheme(vm_id, serv_iter->first, i);
                }
                if (res < res_min)
                {
                    min_serv_id = serv_iter->first;
                    min_serv_node = i;
                    res_min = res;
                }
            }
        }
        else
        {
            res = calc_res(vm_type, serv_iter->second, 2, vm_specs);
            if (res == -1)
                continue;
            if (res < threshold)
            {
                return OneAssignScheme(vm_id, serv_iter->first, 2);
            }
            // ress.push_back(Res(vm_id, serv_iter->first, 2, res));
            if (res < res_min)
            {
                min_serv_node = 2;
                min_serv_id = serv_iter->first;
                res_min = res;
            }
        }
    }

    // // 这里可以优化，其实只要上一步记录一个最小的 res 就可以了
    // int res_min = INT32_MAX;
    // int idx_min = -1;
    // for (int i = 0; i < ress.size(); ++i)
    // {
    //     res = ress[i].res;
    //     if (res < res_min && res > -1)
    //     {
    //         idx_min = i;
    //         res_min = res;
    //     }
    // }

    if (min_serv_id != -1)
        return OneAssignScheme(vm_id, min_serv_id, min_serv_node);

    return OneAssignScheme(-1, -1, -1);
}


OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched, ServStatList &serv_stats, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    
    // 对于单节点的虚拟机，直接用哈希表查
    if (vm_node == 1)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = -1;
        int res;
        int max_shift = 2500;
        // int max_num_search = 1500;
        int num_search = 0;
        
        while (shift < max_shift)
        {
            auto iter = ream.find(demand + shift);
            if (iter != ream.end())
            {
                for (int i = 0; i < iter->second.size(); ++i)
                {
                    new_serv_id = iter->second[i].first;
                    new_serv_node = iter->second[i].second;

                    if (new_serv_id == old_serv_id)  // 不能给自己迁移
                        continue;

                    if (serv_searched.find(new_serv_id) != serv_searched.end())  // 已处理过的服务器不再处理
                        continue;

                    // if (serv_stats[new_serv_id].vms.size() <= serv_stats[old_serv_id].vms.size())  // 不能往少的地方迁移
                    //     continue;

                    if (serv_stats[new_serv_id].can_hold_vm(vm_core, vm_mem, new_serv_node))
                        return OneAssignScheme(vm_id, new_serv_id, new_serv_node, old_serv_id);

                    // if (num_search++ > max_num_search)
                    //     return OneAssignScheme(-1, -1, -1, -1);
                }            
            }
            ++shift;
        }

        return OneAssignScheme(-1, -1, -1, -1);
    }

    // 双节点也用哈希表查
    if (vm_node == 2)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = 2;
        int res;
        int max_shift = 2500;
        // int max_num_search = 1000;
        int num_search = 0;
        
        while (shift < max_shift)
        {
            auto iter = dream.find(demand + shift);
            if (iter != dream.end())
            {
                for (int i = 0; i < iter->second.size(); ++i)
                {
                    new_serv_id = iter->second[i];

                    if (new_serv_id == old_serv_id)  // 不能给自己迁移
                        continue;

                    if (serv_stats[new_serv_id].vms.size() <= serv_stats[old_serv_id].vms.size())  // 不能往少的地方迁移
                        continue;

                    if (serv_stats[new_serv_id].can_hold_vm(vm_core, vm_mem, new_serv_node))
                        return OneAssignScheme(vm_id, new_serv_id, new_serv_node, old_serv_id);

                    // if (num_search++ > max_num_search)
                    //     return OneAssignScheme(-1, -1, -1, -1);
                }            
            }
            ++shift;
        }

        return OneAssignScheme(-1, -1, -1, -1);
    }

    // // 双节点的虚拟机仍用传统方法遍历
    // int res, res_min = INT32_MAX;
    // int min_serv_id = -1, min_serv_node = -1;
    // int threshold = 1;
    // if (vm_node == 2)
    // {
    //     for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    //     {
    //         // 跳过 vm_id 当前所在的服务器
    //         if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
    //             continue;

    //         // 跳过本来就为空的服务器
    //         if (serv_iter->second.vms.size() == 0)
    //             continue;

    //         res = calc_res(vm_type, serv_iter->second, 2, vm_specs);
    //         if (res == -1)
    //             continue;
    //         if (res < threshold)
    //         {
    //             return OneAssignScheme(vm_id, serv_iter->first, 2);
    //         }
    //         // ress.push_back(Res(vm_id, serv_iter->first, 2, res));
    //         if (res < res_min)
    //         {
    //             min_serv_node = 2;
    //             min_serv_id = serv_iter->first;
    //             res_min = res;
    //         }
    //     }
    // }

    // if (min_serv_id != -1)
    //     return OneAssignScheme(vm_id, min_serv_id, min_serv_node);

    return OneAssignScheme(-1, -1, -1);
}

OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type,  ServStatList &serv_stats, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    int max_shift = 2500;
    // int max_num_search = 3000;

    
    // 对于单节点的虚拟机，直接用哈希表查
    if (vm_node == 1)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = -1;
        int num_search = 0;
        
        while (shift < max_shift)
        {
            auto iter = ream.find(demand + shift);
            if (iter != ream.end())
            {
                for (int i = 0; i < iter->second.size(); ++i)
                {
                    new_serv_id = iter->second[i].first;
                    new_serv_node = iter->second[i].second;

                    if (serv_stats[new_serv_id].can_hold_vm(vm_core, vm_mem, new_serv_node))
                        return OneAssignScheme(vm_id, new_serv_id, new_serv_node, -1);

                    // if (num_search++ > max_num_search)
                    //     return OneAssignScheme(-1, -1, -1, -1);
                }            
            }
            ++shift;
        }

        return OneAssignScheme(-1, -1, -1, -1);
    }

    // 双节点也用哈希表查
    if (vm_node == 2)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = 2;
        int num_search = 0;
        
        while (shift < max_shift)
        {
            auto iter = dream.find(demand + shift);
            if (iter != dream.end())
            {
                for (int i = 0; i < iter->second.size(); ++i)
                {
                    new_serv_id = iter->second[i];

                    if (serv_stats[new_serv_id].can_hold_vm(vm_core, vm_mem, new_serv_node))
                        return OneAssignScheme(vm_id, new_serv_id, new_serv_node, -1);

                    // if (num_search++ > max_num_search)
                    //     return OneAssignScheme(-1, -1, -1, -1);
                }            
            }
            ++shift;
        }

        return OneAssignScheme(-1, -1, -1, -1);
    }

    return OneAssignScheme(-1, -1, -1, -1);
}