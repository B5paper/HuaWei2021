#include "select.h"
#include <cmath>
#include <algorithm>

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
                if (res == -1)
                    continue;
                ress.push_back(Res(vm_id, serv_iter->first, i, res));
            }
        }
        else
        {
            res = calc_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            if (res == -1)
                    continue;
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
                if (res == -1)
                    continue;
                // ress.push_back(Res(vm_id, serv_iter->first, i, res));
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
            res = calc_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            if (res == -1)
                continue;
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


float calc_rel_res(int vm_type, ServStat &serv_stat, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    if (!serv_stat.can_hold_vm(vm_core, vm_mem, serv_node, serv_specs))
        return -1;

    float cores_cap = serv_specs[serv_stat.type].cores;
    float mem_cap = serv_specs[serv_stat.type].memcap;

    if (serv_node == 0 || serv_node == 1)
    {
        float cores_ream = serv_stat.cores_ream(serv_node, serv_specs);
        float mem_ream = serv_stat.mem_ream(serv_node, serv_specs);

        float cores_res = (cores_ream - vm_core) / cores_cap;
        float mem_res = (mem_ream - vm_mem) / mem_cap;

        return cores_res + mem_res;
    }

    if (serv_node == 2)
    {
        float cores_ream = 0, mem_ream = 0;
        for (int i = 0; i < 2; ++i)
        {
            cores_ream += serv_stat.cores_ream(i, serv_specs);
            mem_ream += serv_stat.mem_ream(i, serv_specs);
        }

        float cores_res = (cores_ream - vm_core) / cores_cap;
        float mem_res = (mem_ream - vm_mem) / mem_cap;

        return cores_res + mem_res;
    }

    return -1;
}

OneAssignScheme vm_select_server_bf_rel(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    
    // 计算某个虚拟机与所有服务器的适应差值
    vector<RelRes> rel_ress;
    float rel_res;
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
                rel_res = calc_rel_res(vm_type, serv_iter->second, i, serv_specs, vm_specs);
                rel_ress.push_back(RelRes(vm_id, serv_iter->first, i, rel_res));
            }
        }
        else
        {
            rel_res = calc_rel_res(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            rel_ress.push_back(RelRes(vm_id, serv_iter->first, 2, rel_res));
        }
    }

    // 这里可以优化，其实只要上一步记录一个最小的 res 就可以了
    float res_min = __FLT_MAX__;
    int idx_min = -1;
    for (int i = 0; i < rel_ress.size(); ++i)
    {
        rel_res = rel_ress[i].rel_res;
        if (rel_res < res_min && rel_res > -1)
        {
            idx_min = i;
            res_min = rel_res;
        }
    }

    if (idx_min != -1)
        return OneAssignScheme(vm_id, rel_ress[idx_min].serv_id, rel_ress[idx_min].serv_node);

    return OneAssignScheme(-1, -1, -1);
}



OneAssignScheme vm_select_server_bfd(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    // 对于双节点的，先选双节点


    
    return OneAssignScheme(-1, -1, -1);
}


OneAssignScheme add_vm_select_server_bf_plus(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    return OneAssignScheme(-1, -1, -1);
}

// 计算虚拟机与某个服务器的平衡匹配度，越小越好
float calc_bal_ratio(int vm_type, ServStat &serv_stat, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    // int vm_node = vm_specs[vm_type].nodes;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    if (!serv_stat.can_hold_vm(vm_core, vm_mem, serv_node, serv_specs))
        return __FLT_MAX__;

    float vm_core_mem_ratio = (float) vm_core / vm_mem;

    // 单节点的情况
    if (serv_node == 0 || serv_node == 1)
    {
        int serv_core_ream = serv_stat.cores_ream(serv_node, serv_specs);
        int serv_mem_ream = serv_stat.mem_ream(serv_node, serv_specs);
        float serv_ream_core_mem_ratio = (float) serv_core_ream / serv_mem_ream;

        float bal_ratio = abs(serv_ream_core_mem_ratio - vm_core_mem_ratio);
        return bal_ratio;
    }

    // 双节点的情况
    if (serv_node == 2)
    {
        int serv_core_ream, serv_mem_ream;
        float serv_ream_core_mem_ratio;
        float bal_ratios[2];
        for (int i = 0; i < 2; ++i)
        {
            serv_core_ream = serv_stat.cores_ream(i, serv_specs);
            serv_mem_ream = serv_stat.mem_ream(i, serv_specs);
            serv_ream_core_mem_ratio = (float) serv_core_ream / serv_mem_ream;
            bal_ratios[i] = abs(vm_core_mem_ratio - serv_ream_core_mem_ratio);
        }

        float bal_ratio = max(bal_ratios[0], bal_ratios[1]);

        return bal_ratio;
    }

    return __FLT_MAX__;
}

bool comp_bal(Bal &b1, Bal &b2)
{
    if (b1.bal < b2.bal)
        return true;
    else
        return false;
}


OneAssignScheme mig_vm_select_server_bf_bal(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    vector<Bal> bals;
    float bal;

    // 对于单节点虚拟机，考虑平衡比，类似于俄罗斯方块
    // 这样的话，这台服务器的余量能放下的虚拟机才能尽可能多
    // 再考虑余量的绝对值从小到大
    // 计算某个虚拟机与所有服务器的平衡比
    if (vm_node == 1)
    {
        for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
        {
            // 跳过 vm_id 当前所在的服务器
            if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
                continue;

            // 跳过本来就为空的服务器
            if (serv_iter->second.vms.size() == 0)
                continue;

            for (int i = 0; i < 2; ++i)
            {
                bal = calc_bal_ratio(vm_type, serv_iter->second, i , serv_specs, vm_specs);
                if (bal == __FLT_MAX__)
                    continue;
                else
                    bals.push_back(Bal(vm_id, serv_iter->first, i, bal));
            }
        }
    }

    // 对于双节点虚拟机，考虑两个节点的平衡比的最大值
    if (vm_node == 2)
    {
        for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
        {
            // 跳过 vm_id 当前所在的服务器
            if (serv_iter->second.vms.find(vm_id) != serv_iter->second.vms.end())
                continue;

            // 跳过本来就为空的服务器
            if (serv_iter->second.vms.size() == 0)
                continue;

            int serv_id = serv_iter->first;

            bal = calc_bal_ratio(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            if (bal == __FLT_MAX__)
                continue;
            else
                bals.push_back(Bal(vm_id, serv_iter->first, 2, bal));
        }
    }

    if (bals.size() == 0)
        return OneAssignScheme(-1, -1, -1);


    // 计算 bals 中每组数据的余量
    int serv_id, serv_node;
    for (int i = 0; i < bals.size(); ++i)
    {
        serv_id = bals[i].serv_id;
        serv_node = bals[i].serv_node;
        ServStat &serv_stat = serv_stats[serv_id];
        bals[i].res = calc_res(vm_type, serv_stat, serv_node, serv_specs, vm_specs);
    }

    // 对 balance 进行排序
    sort(bals.begin(), bals.end(), comp_bal);

    // 如果对于同一台虚拟机，两个服务器的 balance 仅相差阈值 threshold，那么选择两者中余量较小的
    float threshold = 0.5;
    if (bals.size() > 1)
    {
        if (bals[1].bal - bals[0].bal < threshold)
            return bals[0].res < bals[1].res ? OneAssignScheme(vm_id, bals[0].serv_id, bals[0].serv_node) : OneAssignScheme(vm_id, bals[1].serv_id, bals[1].serv_node);
    }
    return OneAssignScheme(vm_id, bals[0].serv_id, bals[0].serv_node);
}

OneAssignScheme add_vm_select_server_bf_bal(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int vm_node = vm_specs[vm_type].nodes;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    vector<Bal> bals;
    float bal;

    if (vm_node == 1)
    {
        for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
        {
            // // 跳过本来就为空的服务器
            // if (serv_iter->second.vms.size() == 0)
            //     continue;

            for (int i = 0; i < 2; ++i)
            {
                bal = calc_bal_ratio(vm_type, serv_iter->second, i , serv_specs, vm_specs);
                if (bal == __FLT_MAX__)
                    continue;
                else
                    bals.push_back(Bal(vm_id, serv_iter->first, i, bal));
            }
        }
    }

    // 对于双节点虚拟机，考虑两个节点的平衡比的最大值
    if (vm_node == 2)
    {
        for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
        {
            // // 跳过本来就为空的服务器
            // if (serv_iter->second.vms.size() == 0)
            //     continue;

            bal = calc_bal_ratio(vm_type, serv_iter->second, 2, serv_specs, vm_specs);
            if (bal == __FLT_MAX__)
                continue;
            else
                bals.push_back(Bal(vm_id, serv_iter->first, 2, bal));
        }
    }

    if (bals.size() == 0)
        return OneAssignScheme(-1, -1, -1);


    // 计算 bals 中每组数据的余量
    int serv_id, serv_node;
    for (int i = 0; i < bals.size(); ++i)
    {
        serv_id = bals[i].serv_id;
        serv_node = bals[i].serv_node;
        ServStat &serv_stat = serv_stats[serv_id];
        bals[i].res = calc_res(vm_type, serv_stat, serv_node, serv_specs, vm_specs);
    }

    // 对 balance 进行排序
    sort(bals.begin(), bals.end(), comp_bal);

    // 如果对于同一台虚拟机，两个服务器的 balance 仅相差阈值 threshold，那么选择两者中余量较小的
    float threshold = 0.1;
    if (bals.size() > 1)
    {
        if (bals[1].bal - bals[0].bal < threshold)
            return bals[0].res < bals[1].res ? OneAssignScheme(vm_id, bals[0].serv_id, bals[0].serv_node) : OneAssignScheme(vm_id, bals[1].serv_id, bals[1].serv_node);
    }
    return OneAssignScheme(vm_id, bals[0].serv_id, bals[0].serv_node);
}

OneAssignScheme mig_serv_select_vm_bf(int serv_id, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    ServStat &serv_stat = serv_stats[serv_id];

    // 从当前服务器往后找，因为前面的都已经尽量填满了，从所有的虚拟机中挑出适应度最高的
    int min_res = INT32_MAX;
    int min_vm_id = -1;
    int min_serv_id = -1;
    int min_serv_node = -1;
    int res = -1;
    for (auto serv_iter = ++(serv_stats.servs.find(serv_id)); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        for (int i = 0; i < 2; ++i)
        {
            for (auto vm_iter = serv_iter->second.vms.begin(); vm_iter != serv_iter->second.vms.end(); ++vm_iter)
            {
                int vm_id = vm_iter->first;

                // 目前先只考虑单节点的虚拟机
                if (vm_iter->second.node == 2)
                    continue;

                res = calc_res(vm_iter->second.type, serv_stats[serv_id], i, serv_specs, vm_specs);
                if (res == -1)
                    continue;

                if (res < min_res)
                {
                    min_serv_node = i;
                    min_vm_id = vm_iter->first;
                    min_res = res;
                    min_serv_id = serv_iter->first;
                }
            }
        }
    }

    if (min_vm_id != -1)
        return OneAssignScheme(min_vm_id, serv_id, min_serv_node, min_serv_id);
    return OneAssignScheme(-1, -1, -1, -1);
}