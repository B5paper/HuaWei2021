#include "migrate.h"
#include "dispatch.h"
#include "select.h"
#include "types.h"
#include <iostream>
#include <unordered_set>
#include <ctime>
#include <stack>
#include "args.h"

bool comp_id_num(IdNum &i1, IdNum &i2)
{
    // 按资源数排序效果不好
    if (i1.num_vm < i2.num_vm)
        return true;
    // if (i1.num_vm == i2.num_vm)
    // {
    //     if (i1.num_vm_res < i2.num_vm_res)
    //         return true;
    // }
    return false;
}

void migrate_full_first(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs,
    int &num_miged, bool &stop_mig)
{
    if (serv_stats.servs.empty())
    {
        stop_mig = true;
        return;
    }

    unordered_set<int> serv_searched;  // 已处理过的服务器不再处理

    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        int serv_id = serv_iter->first;
        ServStat &serv_stat = serv_iter->second;

        if (serv_stat.is_full())
        {
            serv_searched.insert(serv_id);
            continue;
        }

        // 目前先使用贪心把两个节点都看作单节点塞满
        for (int i = 0; i < 2; ++i)
        {
            if (serv_stat.nodes[i].cores_ream + serv_stat.nodes[i].mem_ream < 5)
                continue;
            int res = serv_stat.nodes[i].cores_ream + serv_stat.nodes[i].mem_ream;
            for (int res_to_search = res; res_to_search < vm_core_mem_map.rbegin()->first; ++res_to_search)
            {
                auto iter = vm_core_mem_map.find(res_to_search);
                if (iter == vm_core_mem_map.end())
                    continue;
                for (int j = 0; j < iter->second.size(); ++j)
                {
                    int serv_id = vm_id_to_serv_id[iter->second[j]];
                    if (serv_searched.find(serv_id) != serv_searched.end())
                        continue;
                    ServStat &searched_serv_stat = serv_stats[serv_id];
                    int vm_type = searched_serv_stat.vms.find(iter->second[j])->second.type;
                    if (!serv_stat.can_hold_vm(vm_specs[vm_type].cores, vm_specs[vm_type].memcap, i))
                        continue;
                    serv_stats.mig_vm(iter->second[j], searched_serv_stat.id, serv_stat.id, i, vm_specs);
                }
            }
        }
        serv_searched.insert(serv_stat.id);
    }
}

void migrate(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (serv_stats.servs.size() == 0)
        return;

    // 将服务器划分为两组，一组是每个节点剩余资源都满足 core_thresh = 5, mem_thresh = 10 的，视为满载的服务器
    // 这些服务器在后续的平衡处理和迁移处理中不再考虑
    // 另一组是非满载服务器，非满载服务器需要迁移出去一台虚拟机，然后从虚拟机数量最少的服务器中挑出一些虚拟机组合，使其满载，
    // 然后将新平衡的服务器加入满载列表中，后续不再考虑

    // 对于一个服务器中，一个节点满载，另一个节点不满载的情况，若非满载节点只有双节点虚拟机，
    // 那么从非平衡的其它服务器中直接找单节点虚拟机，使这个节点满载
    // 若服务器中，两个节点都不满载，若剩余资源都不超过 128，那么尝试从单节点虚拟机中找，使之满载
    // 若找不到，或者若某个节点剩余核心数或内存数超过 128 （虚拟机最大占用资源数），那么就找虚拟机的组合使两个节点分别满载
    // 找虚拟机时，优先找双节点虚拟机，因为单节点虚拟机灵活，很宝贵

    // 最坏的情况，两个节点中，一种资源满载，另一种资源不满载，此时需要迁出去一台或几台虚拟机，然后找虚拟机使其满载

    

    unordered_set<int> serv_searched;  // 已处理过的服务器不再处理

    // 统计当前虚拟机数量，与虚拟机资源数


    map<int, set<int>> vm_num_to_serv_id;
    int num_total_vms = 0;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        if (serv_iter->second.vms.size() != 0)
        {
            vm_num_to_serv_id[serv_iter->second.vms.size()].emplace(serv_iter->first);
            num_total_vms += serv_iter->second.vms.size();
        }
    }


    int max_num_mig = min(int(float(num_total_vms) * 3 / 100), 100);

    if (max_num_mig == 0)
        return;

    int num_mig = 0;

    // new migration scheme
    // int core_thresh = 5, mem_thresh = 10;
    multimap<int, int> nonfull_load_servs;  // (res_ream, serv_id)，不包含满载的服务器和空的服务器
    int core_cap, mem_cap;
    for (auto &serv: serv_stats.servs)
    {
        if (serv.second.nodes[0].cores_ream == 0 && serv.second.nodes[0].mem_ream == 0 &&
            serv.second.nodes[1].cores_ream == 0 && serv.second.nodes[1].mem_ream == 0)
        {
            continue;
        }

        core_cap = serv_specs[serv.second.type].cores / 2;
        mem_cap = serv_specs[serv.second.type].memcap / 2;

        if ((core_cap - serv.second.nodes[0].cores_ream) < core_node_full_load_thresh &&
            (mem_cap - serv.second.nodes[0].mem_ream) < mem_node_full_load_thresh &&
            (core_cap - serv.second.nodes[1].cores_ream) < core_node_full_load_thresh &&
            (mem_cap - serv.second.nodes[1].mem_ream) < mem_node_full_load_thresh)
        {
            continue;
        }

        nonfull_load_servs.emplace(serv.second.nodes[0].cores_ream + serv.second.nodes[1].mem_ream, serv.first);
    }


    // for (auto serv_iter = nonfull_load_servs.rbegin(); serv_iter != nonfull_load_servs.rend();)
    // {
    //     // 若这两个节点是平衡节点，即两个节点剩余的核心数和内存数完全相等
    //     // 那么先找双节点虚拟机有没有能满足满载要求的
    //     ServStat &serv = serv_stats[serv_iter->second];
    //     if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
    //         serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
    //     {
    //         // 若两种资源中有一种是 0，那么就往外迁虚拟机，再迁入使其满载

    //         // 若两种资源都不为 0，那么尝试搜索虚拟机，使其满载
    //         int core_ream = serv.nodes[0].cores_ream * 2;
    //         int mem_ream = serv.nodes[0].mem_ream * 2;

    //         // 首先尝试一个双节点虚拟机，直接用表查
    //         int core_lower_bound = max(core_ream - core_thresh * 2, 0);
    //         int mem_lower_bound = max(mem_ream - mem_thresh * 2, 0);
    //         for (int i = core_ream; i > core_lower_bound; --i)
    //         {
    //             if (dvm_core_map_2[i].empty())
    //                 continue;
    //             for (auto &vm: dvm_core_map_2[i])
    //             {
    //                 // 若虚拟机所在的服务器不是非满载服务器，那么跳过
    //                 int serv_id = vm_id_to_serv_id[vm.first];
    //                 if (nonfull_load_servs.find(serv_id) == nonfull_load_servs.end())
    //                     continue;

    //                 // 若虚拟机在本服务器中，那么跳过
    //                 if (serv_id == serv.id)
    //                     continue;

    //                 if (vm_specs[vm.second].memcap <= mem_ream && vm_specs[vm.second].memcap > mem_lower_bound)
    //                 {
    //                     int vm_id = vm.first;
    //                     mig_scheme.push_back(OneMigScheme(vm.first, serv.id, 2));  // 先写入方案再迁移，因为 vm 是迭代器，迁移完后迭代器失效
    //                     serv_stats.mig_vm(vm.first, serv_id, serv.id, 2, vm_specs);
    //                     nonfull_load_servs.erase(next(serv_iter).base());
    //                     ++num_mig;
    //                     if (num_mig >= max_num_mig)
    //                     {
    //                         num_miged = num_mig;
    //                         stop_mig = true;
    //                         // for (auto &mig_sch: mig_scheme)
    //                         // {
    //                         //     if (mig_sch.vm_id < 0)
    //                         //         cout << "error" << endl;
    //                         // }
    //                         return;
    //                     }
    //                     goto FOUND_VM;
    //                 }
    //             }
    //         }

           
//         }
// FOUND_VM:        ++serv_iter;
//     }




    int thresh_1 = 4;
    int thresh_2 = 50;

    // 在迁移之前，清除一下极端不平衡的服务器（这个是有用的）
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        ServStat &serv_stat = serv_stats[serv_iter->first];
        // 处理一下单节点中不平衡的单节点虚拟机
        for (int i = 0; i < 2; ++i)
        {
            int core_ream = serv_stat.nodes[i].cores_ream;
            int mem_ream = serv_stat.nodes[i].mem_ream;
            if (!(((core_ream < thresh_1) || (mem_ream < thresh_1)) && abs(core_ream - mem_ream) > thresh_2))
                continue;

            for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end(); ++vm_iter)
            {
                if (vm_iter->second.node == i)
                {
                    OneAssignScheme one_assign_scheme = add_vm_select_serv_bf_hash(
                    vm_iter->first, vm_iter->second.type, serv_stats, vm_specs);
                    if (one_assign_scheme.server_id == serv_stat.id) continue;
                    if (one_assign_scheme.server_id != -1)
                    {
                        int vm_id = vm_iter->first;
                        mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, 
                            one_assign_scheme.server_node));
                        serv_stats.mig_vm(vm_iter->first, serv_stat.id,
                            one_assign_scheme.server_id, one_assign_scheme.server_node, vm_specs);
                        ++num_mig;
                        if (num_mig >= max_num_mig)
                            return;
                        else
                            break;
                    }
                }
            }
        }
    }

    // 循环迁移，直到再也迁不动
    time_t tic, toc;
    const int span_size = 50;  // 其实这个影响不大，因为目前每天都能迁够 30 次
    map<int, set<int>> vm_num_to_serv_id_new;
    set<int> serv_processed;  // 防止循环迁移
    // while (true)
    // {
        tic = clock();
        int num_fail_to_clear = 0, num_succeed_to_clear = 0;
        int num_serv_processed = 0;
        int num_fail_left = 0, num_fail_right = 0;
        bool stop_current_mig = false;
        
        // 迁的时候从最少虚拟机的服务器开始迁，找的时候从最接近满载的服务器开始找
        for (auto &num_id: vm_num_to_serv_id)
        // for (auto serv_iter = nonfull_load_servs.begin(); serv_iter != nonfull_load_servs.end(); ++serv_iter)
        {
            tic = clock();
            for (auto &serv_id: num_id.second)
            {
                if (serv_processed.find(serv_id) != serv_processed.end())
                    continue;
                serv_processed.emplace(serv_id);
                bool succeed_to_clear = false;

                ServStat &serv_stat = serv_stats[serv_id];
                serv_searched.insert(serv_stat.id);
                int num_vm = serv_stat.vms.size();

                for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end();)
                {
                    OneAssignScheme one_assign_scheme = mig_vm_select_serv_bf_hash(vm_iter->first, vm_iter->second.type, serv_stat.id, serv_searched, serv_stats, vm_specs);
                    if (one_assign_scheme.server_id != -1)
                    {
                        int vm_id = vm_iter->first;
                        mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, one_assign_scheme.server_node));
                        serv_stats.mig_vm(vm_iter++->first, serv_stat.id, one_assign_scheme.server_id, one_assign_scheme.server_node,
                            vm_specs);
                        succeed_to_clear = true;
                        ++num_mig;
                        if (num_mig >= max_num_mig)
                            return;
                        continue;
                    }
                    ++vm_iter;
                }

                if (!serv_stat.vms.empty())
                    vm_num_to_serv_id_new[serv_stat.vms.size()].emplace(serv_stat.id);

                ++num_serv_processed;

                // 统计成功处理和无法处理的服务器的数量（成功处理不一定要清空）
                if (succeed_to_clear)
                    ++num_succeed_to_clear;
                else
                    ++num_fail_to_clear;

                // 判断一下是否再迁移下去已经没有意义
                if (num_serv_processed % span_size == 0)
                {
                    // 第一个 span，即 0 ~ 200 个非空服务器都无法迁移
                    // migrate 只有两个终止条件，要么是达到最大迁移数量，要么是再也迁不动
                    if (num_succeed_to_clear == 0)
                        return;

                    num_fail_right = num_fail_to_clear;
                    if ((num_fail_right - num_fail_left) / span_size == 1)
                    {
                        stop_current_mig = true;
                        // break;
                    }
                    num_fail_left = num_fail_right;
                }
            }

            toc = clock();
            if (DEBUG)
                cout << "clear one res need time: " << toc - tic << endl;

            // if (stop_current_mig)
                // break;
        }
        vm_num_to_serv_id = vm_num_to_serv_id_new;
        vm_num_to_serv_id_new.clear();
    // }
    return;
}

ServIdNode vm_select_new_single(int vm_id, int vm_type, vector<pair<int, int>> &single_new, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    for (int i = 0; i < single_new.size(); ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            if (serv_stats[single_new[i].first].can_hold_vm(vm_specs[vm_type].cores, vm_specs[vm_type].memcap, j))
            {
                return ServIdNode(single_new[i].first, j);
            }
        }
    }
    return ServIdNode(-1, -1);
}

