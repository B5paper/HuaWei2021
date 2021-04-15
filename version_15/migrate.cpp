#include "migrate.h"
#include "dispatch.h"
#include "select.h"
#include "types.h"
#include <iostream>
#include <unordered_set>
#include <ctime>
#include <stack>
#include "args.h"

void find_non_full_load_serv(int core, int mem, int node, int min_out_serv, int &serv_id, int &serv_node)
{
    if (node == 2)
    {
        for (auto &serv_iter: non_full_load_servs)
        {
            ServStat &serv = serv_stats[serv_iter.second];
            if (serv.can_hold_vm(core, mem, 2) && serv.id != min_out_serv)
            {
                serv_id = serv.id;
                serv_node = 2;
                return;
            }
        }
    }

    if (node == 1)
    {
        for (auto &serv_iter: non_full_load_servs)
        {
            ServStat &serv = serv_stats[serv_iter.second];
            for (int i = 0; i < 2; ++i)
            {
                if (serv.can_hold_vm(core, mem, i) && min_out_serv != serv.id)
                {
                    serv_id = serv.id;
                    serv_node = i;
                    return;
                }
            }
        }
    }
}

bool make_node_full_load(int serv_id, int node, int &num_mig, int max_num_mig, MigScheme &mig_scheme)
{
    ServStat &serv = serv_stats[serv_id];

    // 首先尝试能不能找一个虚拟机直接让其满载
    OneAssignScheme oas = mig_serv_select_vm_full_load(serv_id, node, 1);
    if (oas.old_serv_id != -1)
    {
        mig_scheme.push_back(OneMigScheme(oas.vm_id, oas.server_id, oas.server_node));
        serv_stats.mig_vm(oas.vm_id, oas.old_serv_id, oas.server_id, oas.server_node);
        ++num_mig;
        return true;
    }

    // 若余量都小于 128 + thresh，那么试试迁出一个单节点虚拟机，看看还能不能使其满载
    if (serv.nodes[0].cores_ream <= 128 + core_node_full_load_thresh &&
        serv.nodes[0].mem_ream <= 128 + mem_node_full_load_thresh &&
        serv.nodes[1].cores_ream <= 128 + core_node_full_load_thresh &&
        serv.nodes[1].mem_ream <= 128 + mem_node_full_load_thresh)
    {
        int mig_out_vm_id, mig_out_new_serv_id, mig_out_new_serv_node, mig_out_old_serv_node;
        // int mig_in_vm_id, mig_in_old_serv_id, mig_in_new_serv_node;

        // 循环迁出一个虚拟机，看能否满足要求
        bool succeed_to_mig_in = false;
        auto vms_copy(serv.vms);
        for (auto &vm: vms_copy)
        {
            if (vm_specs[vm.second.type].nodes == 2)
                continue;

            int serv_id = -1, serv_node = -1;
            find_non_full_load_serv(vm_specs[vm.second.type].cores, vm_specs[vm.second.type].memcap, 1, serv.id, serv_id, serv_node);
            if (serv_id == -1)  // 迁不出去，找下一个
                continue;

            mig_out_vm_id = vm.first;
            mig_out_new_serv_id = serv_id;
            mig_out_new_serv_node = serv_node;
            mig_out_old_serv_node = vm.second.node;

            // mig_scheme.push_back(OneMigScheme(vm.first, serv_id, serv_node));
            serv_stats.mig_vm(vm.first, serv.id, serv_id, serv_node);
            ++num_mig;
            if (num_mig >= max_num_mig)
            {
                mig_scheme.push_back(OneMigScheme(mig_out_vm_id, mig_out_new_serv_id, mig_out_new_serv_node));
                return false;
            }

            // 成功迁出去后，再尝试找虚拟机
            oas = mig_serv_select_vm_full_load(serv_id, node, 1);
            if (oas.old_serv_id != -1)
            {
                mig_scheme.push_back(OneMigScheme(mig_out_vm_id, mig_out_new_serv_id, mig_out_new_serv_node));
                mig_scheme.push_back(OneMigScheme(oas.vm_id, oas.server_id, node));
                serv_stats.mig_vm(oas.vm_id, oas.old_serv_id, oas.server_id, oas.server_node);
                // succeed_to_mig_in = true;
                ++num_mig;
                return true;
            }
            // 没成功迁进来的话，把之前的再迁回来
            else
            {
                serv_stats.mig_vm(mig_out_vm_id, mig_out_new_serv_id, serv.id, mig_out_old_serv_node);
                --num_mig;
            }
        }
    }

    return false;
}

void make_two_nodes_full_load()
{

}

void make_serv_full_load(int &num_mig, int num_max_mig, MigScheme &mig_scheme)
{
    auto non_full_load_servs_copy(non_full_load_servs);  // 对副本迭代，防止迭代器失效
    
    // int lower_bound = 5;
    for (auto non_iter = non_full_load_servs_copy.rbegin(); non_iter != non_full_load_servs_copy.rend(); ++non_iter)
    {
        // if (non_iter->first < lower_bound)
        //     break;

        ServStat &serv = serv_stats[non_iter->second];
        if (serv.is_full_load() || serv.is_empty())
        {
            // except_servs.insert(serv.id);
            continue;
        }

        // 首先尝试能不能找个双节点的虚拟机让它满载
        OneAssignScheme oas = mig_serv_select_vm_full_load(serv.id, 2, 2);
        if (oas.old_serv_id != -1 && oas.old_serv_id != oas.server_id)
        {
            mig_scheme.push_back(OneMigScheme(oas.vm_id, serv.id, 2));
            serv_stats.mig_vm(oas.vm_id, oas.old_serv_id, serv.id, 2);
            ++num_mig;
            if (num_mig >= num_max_mig)
                return;
            // except_servs.insert(serv.id);
            continue;
        }

        // 如果找不到虚拟机让它满载，那么尝试迁出去一个双节点虚拟机，再看它是否能满载
        bool found_two_node_vm = false;
        bool succeed_to_mig = false;
        for (auto &vm: serv.vms)
        {
            if (vm.second.node == 2)
            {
                found_two_node_vm = true;

                // 尝试找个虚拟机数较少的服务器迁出
                int out_serv_id = -1, out_serv_node = -1;
                find_non_full_load_serv(vm_specs[vm.second.type].cores, vm_specs[vm.second.type].memcap, 2, serv.id, out_serv_id, out_serv_node);
                if (out_serv_id == -1)
                    continue;
                mig_scheme.push_back(OneMigScheme(vm.first, out_serv_id, 2));  // 先写入方案再迁移，因为 vm 是迭代器，迁移完后迭代器失效
                serv_stats.mig_vm(vm.first, serv.id, out_serv_id, 2);
                ++num_mig;
                if (num_mig >= num_max_mig)
                    return;
                succeed_to_mig = true;
                break;
            }
        }

        // 如果找到双节点虚拟机，并迁移成功，那么尝试找个双节点虚拟机迁入，看能否使其满足满载要求
        if (found_two_node_vm && succeed_to_mig)
        {
            OneAssignScheme oas = mig_serv_select_vm_full_load(serv.id, 2, 2);
            // 若找不到合适的双节点虚拟机，那么留着把这个服务器作为两个单节点处理
            if (oas.old_serv_id == -1)
            {
                // two_node_servs_fail_mig_in.insert(serv.id);
                continue;
            }

            mig_scheme.push_back(OneMigScheme(oas.vm_id, serv.id, 2));
            serv_stats.mig_vm(oas.vm_id, oas.old_serv_id, serv.id , 2);
            ++num_mig;
            if (num_mig >= num_max_mig)
                return;
        }

        // z处理完双节点后，把这个服务器看作两个单节点，分别判断是否满载，并处理
        for (int i = 0; i < 2; ++i)
        {
            if (serv.nodes[i].cores_ream > core_node_full_load_thresh ||
                serv.nodes[i].mem_ream > mem_node_full_load_thresh)
            {
                make_node_full_load(serv.id, i, num_mig, num_max_mig, mig_scheme);
                if (num_mig >= num_max_mig)
                    return;
            }
        }
    }
}

void migrate(MigScheme &mig_scheme, DayReqList &reqs, int idx_day)
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

    int max_num_mig = int(float(num_vms_total) * 3 / 100);
    // max_num_mig = 0;
    if (max_num_mig == 0)
        return;

    int num_mig = 0;



    // 将非满载的节点迁移为满载节点，从虚拟机最多的服务器开始迁移
    // 如果有时间，制定一个下界。不需要对所有服务器进行遍历
    // if (rand() % 2 == 0)
    // {
    //     make_serv_full_load(num_mig, max_num_mig, mig_scheme);
    //     return;
    // }
    

    unordered_set<int> empty_set;

    // 先简单迁移一下剩余资源多的服务器，为下面的消灭不平衡腾出空间
    // 统计剩余资源数
    vector<pair<int, int>> res_to_id;

    serv_searched.clear();
    res_to_id.clear();
    for (auto &serv_id: serv_stats.servs)
    {
        ServStat &serv = serv_stats.servs[serv_id.first];
        res_to_id.emplace_back(serv.nodes[0].cores_ream + serv.nodes[0].mem_ream 
            + serv.nodes[1].cores_ream + serv.nodes[1].mem_ream, serv.id);
        // res_to_id.emplace_back(serv.vms.size(), serv.id);
    }
    // sort(res_to_id.begin(), res_to_id.end());
    sort(res_to_id.begin(), res_to_id.end(), greater<pair<int ,int>>());  // 似乎清除剩余资源少的服务器更好？


    for (int i = 0; i < res_to_id.size(); ++i)
    {

        ServStat &serv = serv_stats.servs[res_to_id[i].second];
        serv_searched.insert(serv.id);

        auto vms_copy(serv.vms);
        for (auto &vm: vms_copy)
        {
            OneAssignScheme oas;
            if (rand() % 2 == 0)
                oas = mig_vm_select_serv_bf_hash(vm.first, vm.second.type, serv.id, empty_set);
            else
                oas = mig_vm_select_serv_first_fit(vm.first, serv.id, serv_searched);
            // OneAssignScheme oas = mig_vm_select_serv_worst_fit(vm.first, serv.id, serv_searched);
            
            if (oas.server_id == -1)
                continue;
            mig_scheme.push_back(OneMigScheme(oas.vm_id, oas.server_id, oas.server_node));
            serv_stats.mig_vm(vm.first, serv.id, oas.server_id, oas.server_node);
            ++num_mig;
            if (rand() % 2 == 0)
            {
                if (num_mig >= max_num_mig)
                    return;
            }
            else
            {
                if (num_mig * 1.2 >= max_num_mig)
                    return;
            }
        }
        // servs_to_search.erase(serv.id);
    }
    


    

    // OneAssignScheme oas_2 = mig_serv_select_vm_best_fit(148, 318, 2, serv_id, empty_set);
    


    // 先迁移一遍，腾出一些空间，用于下面的清除不平衡
    // unordered_set<int> serv_processed;  // 防止循环迁移
    // auto non_full_load_servs_copy(non_full_load_servs);
    // for (auto &serv_iter: non_full_load_servs_copy)
    // {
    //     int serv_id = serv_iter.second;
    //     ServStat &serv = serv_stats[serv_id];
    //     auto vms_copy(serv.vms);
    //     for (auto &vm: vms_copy)
    //     {
    //         OneAssignScheme oas = mig_vm_select_serv_bf_hash(vm.first, vm.second.type, serv_id, serv_processed);
    //         if (oas.server_id == -1)
    //             continue;
    //         mig_scheme.push_back(OneMigScheme(oas.vm_id, oas.server_id, oas.server_node));
    //         serv_stats.mig_vm(vm.first, serv.id, oas.server_id, oas.server_node);
    //         ++num_mig;
    //         if (num_mig >= max_num_mig)
    //             return;
    //     }
    //     serv_processed.insert(serv_id);
    // }


    if (num_mig >= max_num_mig)
        return;

    // int thresh_1 = 10;
    // int thresh_2 = 50;

    // // 使不平衡的服务器尽量满载，如果不能满载，就消减不平衡度
    // // 专治不平衡
    // for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    // {
    //     ServStat &serv_stat = serv_stats[serv_iter->first];
    //     // 处理一下单节点中不平衡的单节点虚拟机
    //     for (int i = 0; i < 2; ++i)
    //     {
    //         int core_ream = serv_stat.nodes[i].cores_ream;
    //         int mem_ream = serv_stat.nodes[i].mem_ream;
    //         if (!(((core_ream < thresh_1) || (mem_ream < thresh_1)) && abs(core_ream - mem_ream) > thresh_2))
    //             continue;

    //         for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end(); ++vm_iter)
    //         {
    //             if (vm_iter->second.node == i)
    //             {
    //                 OneAssignScheme one_assign_scheme = add_vm_select_serv_bf_hash(
    //                 vm_iter->first, vm_iter->second.type, serv_stats, vm_specs);
    //                 if (one_assign_scheme.server_id == serv_stat.id) continue;
    //                 if (one_assign_scheme.server_id != -1)
    //                 {
    //                     int vm_id = vm_iter->first;
    //                     mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, 
    //                         one_assign_scheme.server_node));
    //                     serv_stats.mig_vm(vm_iter->first, serv_stat.id,
    //                         one_assign_scheme.server_id, one_assign_scheme.server_node);
    //                     ++num_mig;
    //                     if (num_mig >= max_num_mig)
    //                         return;
    //                     else
    //                         break;
    //                 }
    //             }
    //         }
    //     }
    // }

    

    // // 循环迁移，直到再也迁不动
    // time_t tic, toc;
    // const int span_size = 200;  // 其实这个影响不大，因为目前每天都能迁够 30 次
    
    // // while (true)
    // // {
    // tic = clock();
    // int num_fail_to_clear = 0, num_succeed_to_clear = 0;
    // int num_serv_processed = 0;
    // int num_fail_left = 0, num_fail_right = 0;
    // bool stop_current_mig = false;
    
    // // 迁的时候从最少虚拟机的服务器开始迁，找的时候从最接近满载的服务器开始找

    // non_full_load_servs_copy = non_full_load_servs;
    // for (auto iter = non_full_load_servs_copy.begin(); iter != non_full_load_servs_copy.end(); ++iter)
    // {
    //     tic = clock();
    //     pair<int, int> num_vm_serv_id = *iter;
    //     int serv_id = num_vm_serv_id.second;

    //     if (serv_processed.find(serv_id) != serv_processed.end())
    //         continue;

    //     serv_processed.emplace(serv_id);
    //     bool succeed_to_clear = false;

    //     ServStat &serv_stat = serv_stats[serv_id];
    //     serv_searched.insert(serv_stat.id);
    //     int num_vm = serv_stat.vms.size();

    //     for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end();)
    //     {
    //         OneAssignScheme one_assign_scheme = mig_vm_select_serv_bf_hash(vm_iter->first, vm_iter->second.type, serv_stat.id, serv_searched, serv_stats, vm_specs);
    //         if (one_assign_scheme.server_id != -1)
    //         {
    //             int vm_id = vm_iter->first;
    //             mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, one_assign_scheme.server_node));
    //             serv_stats.mig_vm(vm_iter++->first, serv_stat.id, one_assign_scheme.server_id, one_assign_scheme.server_node);
    //             succeed_to_clear = true;
    //             ++num_mig;
    //             if (num_mig >= max_num_mig)
    //                 return;
    //             continue;
    //         }
    //         ++vm_iter;
    //     }

    //     ++num_serv_processed;

    //     // 统计成功处理和无法处理的服务器的数量（成功处理不一定要清空）
    //     if (succeed_to_clear)
    //         ++num_succeed_to_clear;
    //     else
    //         ++num_fail_to_clear;

    //     // 判断一下是否再迁移下去已经没有意义
    //     if (num_serv_processed % span_size == 0)
    //     {
    //         // 第一个 span，即 0 ~ 200 个非空服务器都无法迁移
    //         // migrate 只有两个终止条件，要么是达到最大迁移数量，要么是再也迁不动
    //         if (num_succeed_to_clear == 0)
    //             return;

    //         num_fail_right = num_fail_to_clear;
    //         if ((num_fail_right - num_fail_left) / span_size == 1)
    //         {
    //             stop_current_mig = true;
    //             return;
    //         }
    //         num_fail_left = num_fail_right;
            
    //     }

    //     toc = clock();
    // }
        // vm_num_to_serv_id = vm_num_to_serv_id_new;
        // vm_num_to_serv_id_new.clear();
    // }
    return;
}
