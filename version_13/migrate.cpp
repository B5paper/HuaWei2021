#include "migrate.h"
#include "dispatch.h"
#include "select.h"
#include "types.h"
#include <iostream>
#include <unordered_set>
#include <ctime>
#include <stack>

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

// vector<int> find_vm_comb(ServStatList &serv_stats)
// {
//     vector<int> vm_comb;  // vm_ids
    
// }

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

        // 使用整数线性规划，从已存在的所有虚拟机中，找到一组，使得这个服务器剩余的资源数最少

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

void migrate(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs, 
            int &num_miged, bool &stop_mig)
{
    if (serv_stats.servs.size() == 0)
    {
        stop_mig = true;
        return;
    }

    unordered_set<int> serv_searched;  // 已处理过的服务器不再处理

    // 统计当前虚拟机数量，与虚拟机资源数
    vector<IdNum> id_nums;
    int num_vms = 0;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        num_vms += serv_iter->second.vms.size();
        int num_vm_res = 0;
        for (auto vm_iter = serv_iter->second.vms.begin(); vm_iter != serv_iter->second.vms.end(); ++vm_iter)
        {
            num_vm_res += vm_specs[vm_iter->second.type].cores + vm_specs[vm_iter->second.type].memcap;
        }
        id_nums.push_back(IdNum(serv_iter->first, serv_iter->second.vms.size(), num_vm_res));
    }

    int max_num_mig = min(int(float(num_vms) * 5 / 1000), 30);

    if (max_num_mig == 0)
    {
        stop_mig = true;
        return;
    }

    sort(id_nums.begin(), id_nums.end(), comp_id_num);
    int num_mig = num_miged;

    int thresh_1 = 3;
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
                        mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, 
                            one_assign_scheme.server_node));
                        serv_stats.mig_vm(vm_iter->first, serv_stat.id,
                            one_assign_scheme.server_id, one_assign_scheme.server_node, vm_specs);
                        ++num_mig;
                        if (num_mig >= max_num_mig)
                        {
                            num_miged = num_mig;
                            stop_mig = true;
                            return;
                        }
                        break;
                    }
                }
            }
        }
    }

    if (num_mig >= max_num_mig)
    {
        num_miged = num_mig;
        stop_mig = true;
        return;
    }

    // hash best fit, stop early
    const int span_size = 200;
    int num_fail_to_clear = 0, num_succeed_to_cleaar = 0;
    int num_serv_processed = 0;
    int num_fail_left = 0, num_fail_right = 0;
    bool succeed_to_clear = false;
    for (int i = 0; i < id_nums.size(); ++i)
    {
        ServStat &serv_stat = serv_stats[id_nums[i].serv_id];
        serv_searched.insert(id_nums[i].serv_id);
        if (serv_stat.vms.empty())
            continue;
        
        for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end();)
        {
            OneAssignScheme one_assign_scheme = mig_vm_select_serv_bf_hash(vm_iter->first, vm_iter->second.type, id_nums[i].serv_id, serv_searched, serv_stats, vm_specs);
            if (one_assign_scheme.server_id != -1)
            {
                mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, one_assign_scheme.server_node));
                serv_stats.mig_vm(vm_iter++->first, id_nums[i].serv_id, one_assign_scheme.server_id, one_assign_scheme.server_node,
                    vm_specs);
                ++num_mig;
                succeed_to_clear = true;
                if (num_mig >= max_num_mig)
                {
                    num_miged = num_mig;
                    stop_mig = true;
                    return;
                }
                continue;
            }
            else
            {
                num_miged = num_mig;
                // break;
            }
            ++vm_iter;
        }
        ++num_serv_processed;

        // 统计成功处理和无法处理的服务器的数量（成功处理不一定要清空）
        if (succeed_to_clear)
        {
            ++num_succeed_to_cleaar;
            succeed_to_clear = false;
        }
        else
            ++num_fail_to_clear;

        // 判断一下是否再迁移下去已经没有意义
        if (num_serv_processed % span_size == 0)
        {
            // 第一个 span，即 0 ~ 200 个非空服务器都无法迁移
            if (num_succeed_to_cleaar == 0)
            {
                num_miged = num_mig;
                stop_mig = true;
                return;
            }

            // if (num_succeed_to_cleaar == 0)
            // {
            //     if (num_serv_processed == span_size)
            //     {
            //         stop_mig = true;
            //         num_miged = num_mig;
            //         break;
            //     }
            //     else
            //     {
            //         num_miged = num_mig;
            //         break;
            //     }
            // }

            num_fail_right = num_fail_to_clear;
            if ((num_fail_right - num_fail_left) / span_size == 1)
            {
                num_miged = num_mig;
                break;
            }
            num_fail_left = num_fail_right;
        }
    }
    
    num_miged = num_mig;
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

bool vm_make_node_balance(int vm_id, int vm_type, ServStat &serv_stat, int serv_node, VMSpecList &vm_specs, int thresh_1 = 5, int thresh_2 = 30)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    
    if ((serv_stat.nodes[serv_node].cores_ream - vm_core < thresh_1 && 
        abs(serv_stat.nodes[serv_node].cores_ream - vm_core - 
            serv_stat.nodes[serv_node].mem_ream + vm_mem) < thresh_2) ||
        (serv_stat.nodes[serv_node].mem_ream - vm_mem < thresh_1 &&
        abs(serv_stat.nodes[serv_node].cores_ream - vm_core - 
            serv_stat.nodes[serv_node].mem_ream + vm_mem) < thresh_2))
    {
        return true;
    }
    return false;
}

float calc_balance(int vm_type, ServStat &serv_stat, int serv_node, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    return abs(float(serv_stat.nodes[serv_node].cores_ream - vm_core) / (serv_stat.nodes[serv_node].mem_ream - vm_mem) - 1);
}

vector<VMInfo> search_vm_comb(ServStat &serv_stat, int serv_node, vector<pair<int, int>> &single_old, unordered_map<int, int> &balanced_nodes, 
                            ServStatList &serv_stats, VMSpecList &vm_specs,
                            int thresh_1 = 5, int thresh_2 = 30)
{
    vector<VMInfo> vm_infos;

    // 最多搜索 100 个虚拟机
    int num_max_search_vm = 1000;

    // 平衡度阈值
    float balance_thresh = 0.15;

    // 使用贪心算法，每次都找符合平衡度阈值的
    unordered_set<int> single_old_servs;
    for (int i = 0; i < single_old.size(); ++i)
        single_old_servs.insert(single_old[i].first);
    stack<MigOp> mig_ops;
    MigOp mig_op;
    int num_searched = 0;
    for (auto search_serv_iter = single_old_servs.begin(); search_serv_iter != single_old_servs.end(); ++search_serv_iter)
    {
        // 跳过本服务器
        if (*search_serv_iter == serv_stat.id)
            continue;
        
        int search_serv_id = *search_serv_iter;
        for (auto vm_iter = serv_stats[search_serv_id].vms.begin(); vm_iter != serv_stats[search_serv_id].vms.end();)
        {
            // 若达到最大搜索次数，或节点已经平衡
            if (num_searched >= num_max_search_vm || (serv_stat.nodes[serv_node].cores_ream < thresh_1 && abs(serv_stat.nodes[serv_node].cores_ream - serv_stat.nodes[serv_node].mem_ream) < thresh_2)
                || (serv_stat.nodes[serv_node].mem_ream < thresh_1 && abs(serv_stat.nodes[serv_node].cores_ream - serv_stat.nodes[serv_node].mem_ream) < thresh_2))
            {
                while (!mig_ops.empty())
                {
                    mig_op = mig_ops.top();
                    mig_ops.pop();
                    serv_stats.mig_vm(mig_op.vm_id, mig_op.new_serv_id, mig_op.old_serv_id, mig_op.old_serv_node, vm_specs);
                }
                return vm_infos;
            }

            // 若虚拟机存放在已经平衡的节点，则跳过
            auto iter = balanced_nodes.find(search_serv_id);
            if (iter != balanced_nodes.end())
            {
                if (iter->second == vm_iter->second.node)
                {
                    vm_iter++;
                    continue;
                }
            }

            int vm_id = vm_iter->first;
            int vm_type = vm_iter->second.type;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;

            if (!serv_stat.can_hold_vm(vm_core, vm_mem, serv_node))
            {
                ++num_searched;
                vm_iter++;
                continue;
            }

            if (vm_make_node_balance(vm_id, vm_type, serv_stat, serv_node, vm_specs))
            {
                vm_infos.clear();
                vm_infos.push_back(VMInfo(vm_id, vm_type, search_serv_id, serv_stats[search_serv_id].vms[vm_id].node));
                while (!mig_ops.empty())
                {
                    mig_op = mig_ops.top();
                    mig_ops.pop();
                    serv_stats.mig_vm(mig_op.vm_id, mig_op.new_serv_id, mig_op.old_serv_id, mig_op.old_serv_node, vm_specs);
                }
                return vm_infos;
            }
            else
            {
                if (calc_balance(vm_type, serv_stat, serv_node, vm_specs) < balance_thresh)
                {
                    mig_ops.push(MigOp(vm_id, vm_type, *search_serv_iter, vm_iter->second.node, serv_stat.id, serv_node));
                    serv_stats.mig_vm(vm_iter++->first, search_serv_id, serv_stat.id, serv_node, vm_specs);
                    vm_infos.push_back(VMInfo(vm_id, vm_type, search_serv_id, serv_stats[search_serv_id].vms[vm_id].node));
                }
                else
                {
                    ++num_searched;
                    ++vm_iter;
                }
            }
        }
    }

    while (!mig_ops.empty())
    {
        mig_op = mig_ops.top();
        mig_ops.pop();
        serv_stats.mig_vm(mig_op.vm_id, mig_op.new_serv_id, mig_op.old_serv_id, mig_op.old_serv_node, vm_specs);
    }

    return vm_infos;
}

void count_balance(ServStatList &serv_stats)
{
    int num_balanced;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (abs(serv_iter->second.nodes[i].cores_ream - serv_iter->second.nodes[i].mem_ream) < 30)
                num_balanced++;
        }   
    }
    cout << "num balanced: " << num_balanced << endl;
}


// void migrate_3(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
// {
//     if (serv_stats.servs.size() == 0)
//         return;

//     // 统计当前虚拟机的数量
//     // vector<IdNum> id_nums;
//     int num_vms = 0;
//     for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
//     {
//         // id_nums.push_back(IdNum(serv_iter->first, serv_iter->second.vms.size()));
//         num_vms += serv_iter->second.vms.size();
//     }

//     int max_num_mig = min(int(float(num_vms) * 5 / 1000), 30);
//     if (max_num_mig == 0)
//         return;

//     // sort(id_nums.begin(), id_nums.end(), comp_id_num);

//     int num_mig = 0;

//     int max_num_adjust = max_num_mig / 3;
//     int max_num_formal = max_num_mig - max_num_adjust;
    
//     int num_adjust = 0;
//     int thresh_1 = 5, thresh_2 = 30;
//     vector<pair<int, int>> single_old, single_new, double_old, double_new, empty;  // <serv_id, node>
//     for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
//     {
//         if (serv_iter->second.group == 1)
//         {
//             for (int i = 0; i < 2; ++i)
//             {
//                 if (serv_iter->second.nodes[i].cores_ream < thresh_1 ||
//                     serv_iter->second.nodes[i].mem_ream < thresh_1)
//                 {
//                     single_old.push_back(make_pair(serv_iter->first, i));
//                 }
//                 else
//                 {
//                     single_new.push_back(make_pair(serv_iter->first, i));
//                 }
//             }
//         }

//         if (serv_iter->second.group == 2)
//         {
//             if (serv_iter->second.nodes[0].cores_ream < thresh_1 ||
//                 serv_iter->second.nodes[0].mem_ream < thresh_1)
//             {
//                 double_old.push_back(make_pair(serv_iter->first, 2));
//             }
//             else
//             {
//                 double_new.push_back(make_pair(serv_iter->first, 2));
//             }
//         }

//         if (serv_iter->second.group == 3)
//             empty.push_back(make_pair(serv_iter->first, -1));
//     }

//     // cout << "------ single old ------" << endl;
//     // for (int i = 0; i < single_old.size(); ++i)
//     // {
//     //     cout << "id: " << single_old[i].first << ", " 
//     //     << "node " << single_old[i].second << " core ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].cores_ream
//     //     << ", mem ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ single new ------" << endl;
//     // for (int i = 0; i < single_new.size(); ++i)
//     // {
//     //     cout << "id: " << single_new[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[single_new[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[single_new[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[single_new[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[single_new[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ double old ------" << endl;
//     // for (int i = 0; i < double_old.size(); ++i)
//     // {
//     //     cout << "id: " << double_old[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[double_old[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[double_old[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[double_old[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[double_old[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ double new ------" << endl;
//     // for (int i = 0; i < double_new.size(); ++i)
//     // {
//     //     cout << "id: " << double_new[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[double_new[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[double_new[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[double_new[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[double_new[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }
//     // count_balance(serv_stats);
//     // 处理老的单服务器，使之尽量平衡
//     unordered_map<int, int> balanced_nodes;  // serv_id -> node
//     for (int i = 0; i < single_old.size(); ++i)
//     {
//         int serv_id = single_old[i].first;
//         int serv_node = single_old[i].second;
//         ServStat &serv_stat = serv_stats[serv_id];

//         int cores_ream = serv_stat.nodes[serv_node].cores_ream;
//         int mem_ream = serv_stat.nodes[serv_node].mem_ream;
//         if (abs(cores_ream - mem_ream) < thresh_2)
//         {
//             balanced_nodes.insert(make_pair(serv_id, serv_node));
//             continue;
//         }

//         // cout << "------ before process ------" << endl;
//         // for (int i = 0; i < single_old.size(); ++i)
//         // {
//         //     cout << "id: " << single_old[i].first << ", " 
//         //     << "node " << single_old[i].second << " core ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].cores_ream
//         //     << ", mem ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].mem_ream
//         //     << endl;
//         // }
//         // cout << endl;
        
//         // 从这个不平衡的节点中，删除一个虚拟机
//         bool vm_removed = false;
//         for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end(); )
//         {
//             int vm_id = vm_iter->first;
//             if (vm_iter->second.node != serv_node)
//             {
//                 ++vm_iter;
//                 continue;
//             }

//             int vm_type = vm_iter->second.type;
//             if (cores_ream < thresh_1)
//             {
//                 if (vm_specs[vm_type].cores > vm_specs[vm_type].memcap)
//                 {
//                     ServIdNode id_node = vm_select_new_single(vm_iter->first, vm_iter->second.type, single_new, serv_stats, serv_specs, vm_specs);
//                     if (id_node.id == -1)
//                     {
//                         ++vm_iter;
//                         continue;
//                     }
//                     serv_stats.mig_vm(vm_iter++->first, serv_id, id_node.id, id_node.node, vm_specs);
//                     mig_scheme.push_back(OneMigScheme(vm_id, id_node.id, id_node.node));
//                     vm_removed = true;
//                     ++num_adjust;
//                     break;
//                 }
//             }

//             if (mem_ream < thresh_1)
//             {
//                 if (vm_specs[vm_type].memcap > vm_specs[vm_type].cores)
//                 {
//                     ServIdNode id_node = vm_select_new_single(vm_iter->first, vm_iter->second.type, single_new, serv_stats, serv_specs, vm_specs);
//                     if (id_node.id == -1)
//                     {
//                         ++vm_iter;
//                         continue;
//                     }
//                     serv_stats.mig_vm(vm_iter++->first, serv_id, id_node.id, id_node.node, vm_specs);
//                     mig_scheme.push_back(OneMigScheme(vm_id, id_node.id, id_node.node));
//                     vm_removed = true;
//                     ++num_adjust;
//                     break;
//                 }
//             }
//             ++vm_iter;
//         }

//         // 如果真的找不到合适的，就随便删一个
//         int vm_id = -1;
//         if (!vm_removed)
//         {
//             for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end(); ++vm_iter)
//             {
//                 if (vm_iter->second.node == serv_node)
//                 {
//                     vm_id = vm_iter->first;
//                     break;
//                 }
//             }
            
//             if (vm_id != -1)
//             {
//                 ServIdNode id_node = vm_select_new_single(vm_id, serv_stat.vms[vm_id].type, single_new, serv_stats, serv_specs, vm_specs);
//                 // 连迁都没地方迁，说明这个节点无可救药了
//                 if (id_node.id == -1)
//                     continue;
//                 serv_stats.mig_vm(vm_id, serv_id, id_node.id, id_node.node, vm_specs);
//                 mig_scheme.push_back(OneMigScheme(vm_id, id_node.id, id_node.node));
//                 ++num_adjust;
//             }
//         }

//         if (num_adjust >= max_num_adjust)
//             break;
        
//         // 或许这个可以不要
//         // 添加一些虚拟机到本节点中，使之恢复平衡
//         vector<VMInfo> vm_infos = search_vm_comb(serv_stat, serv_node, single_old, balanced_nodes, serv_stats, vm_specs);
//         if (vm_infos.size() != 0)
//         {
//             for (int j = 0; j < vm_infos.size(); ++j)
//             {
//                 serv_stats.mig_vm(vm_infos[j].vm_id, vm_infos[j].serv_id, serv_id, serv_node, vm_specs);
//                 mig_scheme.push_back(OneMigScheme(vm_infos[j].vm_id, vm_infos[j].serv_id, vm_infos[j].serv_node));
//                 ++num_adjust;
//                 if (num_adjust >= max_num_adjust)
//                     break;
//             }

//             // 可能效果不理想，但也算是处理过了，不能再动了
//             balanced_nodes.insert(make_pair(serv_id, serv_node));
//         }

//         // cout << "------ after process ------" << endl;
//         // for (int i = 0; i < single_old.size(); ++i)
//         // {
//         //     cout << "id: " << single_old[i].first << ", " 
//         //     << "node " << single_old[i].second << " core ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].cores_ream
//         //     << ", mem ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].mem_ream
//         //     << endl;
//         // }
//         // cout << endl;
//     }
//     // count_balance(serv_stats);

//     // cout << "------ single old ------" << endl;
//     // for (int i = 0; i < single_old.size(); ++i)
//     // {
//     //     int cores_ream = serv_stats[single_old[i].first].nodes[single_old[i].second].cores_ream;
//     //     cout << "id: " << single_old[i].first << ", " 
//     //     << "node " << single_old[i].second << " core ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].cores_ream
//     //     << ", mem ream: " << serv_stats[single_old[i].first].nodes[single_old[i].second].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ single new ------" << endl;
//     // for (int i = 0; i < single_new.size(); ++i)
//     // {
//     //     cout << "id: " << single_new[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[single_new[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[single_new[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[single_new[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[single_new[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ double old ------" << endl;
//     // for (int i = 0; i < double_old.size(); ++i)
//     // {
//     //     cout << "id: " << double_old[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[double_old[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[double_old[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[double_old[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[double_old[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }
//     // cout << "------ double new ------" << endl;
//     // for (int i = 0; i < double_new.size(); ++i)
//     // {
//     //     cout << "id: " << double_new[i].first << ", " 
//     //     << "node 0 core ream: " << serv_stats[double_new[i].first].nodes[0].cores_ream
//     //     << ", mem ream: " << serv_stats[double_new[i].first].nodes[0].mem_ream
//     //     << ", node 1 core ream: " << serv_stats[double_new[i].first].nodes[1].cores_ream
//     //     << ", mem ream: " << serv_stats[double_new[i].first].nodes[1].mem_ream
//     //     << endl;
//     // }

//     // 对新服务器进行迁移
//     max_num_formal = max_num_mig - num_adjust;
//     num_mig = 0;
//     vector<IdNum> id_nums;
//     for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
//     {
//         id_nums.push_back(IdNum(serv_iter->first, serv_iter->second.vms.size()));
//     }
//     sort(id_nums.begin(), id_nums.end(), comp_id_num);

//     num_mig = 0;
//     for (int i = 0; i < id_nums.size(); ++i)
//     {
//         ServStat &serv_stat = serv_stats[id_nums[i].serv_id];
        
//         for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end();)
//         {
//             // OneAssignScheme one_assign_scheme = select_best_fit_serv(vm_iter->second.type, vm_iter->first, i+1, id_nums, serv_stats, serv_specs, vm_specs);            
//             OneAssignScheme one_assign_scheme = mig_vm_select_serv_bf_hash(vm_iter->first, vm_iter->second.type, id_nums[i].serv_id, serv_stats, vm_specs);
//             if (one_assign_scheme.server_id != -1)
//             {
//                 mig_scheme.push_back(OneMigScheme(vm_iter->first, one_assign_scheme.server_id, one_assign_scheme.server_node));
//                 serv_stats.mig_vm(vm_iter++->first, id_nums[i].serv_id, one_assign_scheme.server_id, one_assign_scheme.server_node,
//                     vm_specs);
//                 ++num_mig;
//                 if (num_mig >= max_num_formal)
//                     return;
//                 continue;
//             }
//             ++vm_iter;
//         }
//     }
// }