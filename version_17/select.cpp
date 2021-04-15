#include "select.h"
#include <cmath>
#include <algorithm>
#include "migrate.h"
#include "args.h"
#include <ctime>
#include "types.h"
#include "status.h"

OneAssignScheme mig_vm_select_serv_first_fit(int vm_id, int old_serv_id, unordered_set<int> &except_servs)
{
    int vm_type = serv_stats[vm_id_to_serv_id[vm_id]].vms[vm_id].type;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    // 方案 1 查表
    if (vm_node == 1)
    {
        auto lower_bound = serv_stats.nnode_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.nnode_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    ServStat &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    // 不能搜索已经处理过的，不然会循环迁移
                    if (except_servs.find(serv.id) != except_servs.end())
                        continue;

                    return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                }
            }
        }
    }
    
    if (vm_node == 2)
    {
        auto lower_bound = serv_stats.nserv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.nserv_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    ServStat &serv = serv_stats[serv_id];
                    // int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    if (except_servs.find(serv.id) != except_servs.end())
                        continue;

                    return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                }
            }
        }
    }

    return OneAssignScheme(-1, -1, -1, -1);
}

OneAssignScheme mig_vm_select_serv_balance_fit(int vm_id, int old_serv_id, unordered_set<int> &except_servs)
{
    int vm_type = serv_stats[vm_id_to_serv_id[vm_id]].vms[vm_id].type;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    // 方案 1 查表
    if (vm_node == 1)
    {
        auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
        // auto upper_bound = node_core_mem_map.end();
        
        int core_dist = distance(lower_bound, serv_stats.node_core_mem_map.end());
        int core_step = 0;
        int core_swing_step = 0;
        for (int i = 0; i < core_dist / 2 + 1; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                core_step = core_dist / 2 + core_swing_step;
                auto core_iter = next(lower_bound, core_step);
                if (core_iter == serv_stats.node_core_mem_map.end())
                    continue;

                auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
                auto mem_upper_bound = core_iter->second.end();
                // int mem_dist = distance(lower_bound, core_iter->second.end())
                // for (int k = 0; k < )
                for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
                {
                    for (auto &node: mem_iter->second)
                    {
                        ServStat &serv = serv_stats[node.first];
                        int serv_node = node.second;

                        // 不能原地迁移
                        if (serv.id == old_serv_id)
                            continue;

                        // 不能搜索已经处理过的，不然会循环迁移
                        if (except_servs.find(serv.id) != except_servs.end())
                            continue;

                        return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                    }
                }

                if (core_swing_step != 0)
                    core_swing_step = -core_swing_step;
            }
            ++core_swing_step;
        }
    }
    
    if (vm_node == 2)
    {
        auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
        int core_dist = distance(lower_bound, serv_stats.serv_core_mem_map.end());
        int core_step = 0;
        int core_swing_step = 0;
        for (int i = 0; i < core_dist / 2 + 1; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                core_step = core_dist / 2 + core_swing_step;
                auto core_iter = next(lower_bound, core_step);
                if (core_iter == serv_stats.serv_core_mem_map.end())
                    continue;

                auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
                auto mem_upper_bound = core_iter->second.end();
                for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
                {
                    for (auto &serv_id: mem_iter->second)
                    {
                        ServStat &serv = serv_stats[serv_id];
                        // int serv_node = node.second;

                        // 不能原地迁移
                        if (serv.id == old_serv_id)
                            continue;

                        if (except_servs.find(serv.id) != except_servs.end())
                            continue;

                        return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                    }
                }
            }

            if (core_swing_step != 0)
                core_swing_step = -core_swing_step;
        }
        ++core_swing_step;
    }

    return OneAssignScheme(-1, -1, -1, -1);
}


OneAssignScheme mig_serv_select_vm_best_fit(int core_ream, int mem_ream, int node, int serv_id, unordered_set<int> &except_servs)
{
    ServStat &serv = serv_stats[serv_id];
    int vm_type, vm_core, vm_mem;
    bool stop_core_iter = false, stop_mem_iter = false, stop_node_iter = false;;

    // 使用双节点虚拟机填充服务器的两个节点
    if (node == 2)
    {
        map<int, int> res_to_vm_id;
        int min_core = min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream);
        int max_core = max(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream);
        auto vm_core_lower_bound = serv_stats.vm_stat.lower_bound(min_core * 2);
        auto vm_core_upper_bound = serv_stats.vm_stat.upper_bound(0);

        // if (vm_core_lower_bound->first > min_core * 2)
        //     return OneAssignScheme(-1, -1, -1, -1);

        for (auto vm_core_iter = vm_core_lower_bound; vm_core_iter != vm_core_upper_bound; ++vm_core_iter)
        {
            int min_mem = min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream);
            int max_mem = max(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream);
            auto vm_mem_lower_bound = vm_core_iter->second.lower_bound(min_mem * 2);
            auto vm_mem_upper_bound = vm_core_iter->second.upper_bound(0);
            stop_mem_iter = false;

            // if (!res_to_vm_id.empty())
            // {
            //     if (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core_iter->first > res_to_vm_id.begin()->first)
            //     {
            //         break;
            //     }
            // }
            
            for (auto vm_mem_iter = vm_mem_lower_bound; vm_mem_iter != vm_mem_upper_bound; ++vm_mem_iter)
            {
                auto vm_node_iter = vm_mem_iter->second.lower_bound(2);
                if (vm_node_iter != vm_mem_iter->second.end())
                {
                    for (auto &vm_id: vm_node_iter->second)
                    {
                        ServStat &search_serv = serv_stats.get_serv_by_vm_id(vm_id);
                        if (non_full_load_servs.find(make_pair(search_serv.vms.size(), search_serv.id)) == non_full_load_servs.end())
                            continue;
                        // if (except_servs.find(serv.id) != except_servs.end())
                        //     continue;
                        if (search_serv.id == serv_id)
                            continue;

                        vm_type = search_serv.vms[vm_id].type;
                        vm_core = vm_specs[vm_type].cores;
                        vm_mem = vm_specs[vm_type].memcap;

                        res_to_vm_id.emplace(
                            (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core) +
                            (serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem), vm_id);

                        stop_mem_iter = true;
                        break;
                    }
                    if (stop_mem_iter)
                    {
                        break;
                    }
                }
            }
        }

        if (!res_to_vm_id.empty())
        {
            int min_res_vm_id = res_to_vm_id.begin()->second;
            return OneAssignScheme(min_res_vm_id, serv_id, 2, vm_id_to_serv_id[min_res_vm_id]);
        }
    }


    // 使用单节点虚拟机填充服务器的 node 节点
    if (node < 2)
    {
        map<int, int> res_to_vm_id;
        int core_ream = serv.nodes[node].cores_ream;
        auto vm_core_lower_bound = serv_stats.vm_stat.lower_bound(core_ream);
        auto vm_core_upper_bound = serv_stats.vm_stat.upper_bound(0);

        // c++ 似乎无法保证 lower_bound 一定小于等于 upper_bound
        // if (vm_core_upper_bound->first > core_ream)
        //     return OneAssignScheme(-1, -1, -1, -1);
        for (auto vm_core_iter = vm_core_lower_bound; vm_core_iter != vm_core_upper_bound; ++vm_core_iter)
        {
            int mem_ream = serv.nodes[node].mem_ream;
            auto vm_mem_lower_bound = vm_core_iter->second.lower_bound(mem_ream);
            auto vm_mem_upper_bound = vm_core_iter->second.end();


            if (!res_to_vm_id.empty())
            {
                if (serv.nodes[node].cores_ream - vm_core_iter->first > res_to_vm_id.begin()->first)
                {
                    break;
                }
            }

            stop_mem_iter = false;
            for (auto vm_mem_iter = vm_mem_lower_bound; vm_mem_iter != vm_mem_upper_bound; ++vm_mem_iter)
            {
                // auto vm_node_lower_bound = vm_mem_iter->second.lower_bound(1);
                // auto vm_node_upper_bound = vm_mem_iter->second.upper_bound(2);
                auto vm_node_iter = vm_mem_iter->second.find(1);
                if (vm_node_iter == vm_mem_iter->second.end())
                    continue;
        
                for (auto &vm_id: vm_node_iter->second)
                {
                    ServStat &search_serv = serv_stats.get_serv_by_vm_id(vm_id);
                    if (non_full_load_servs.find(make_pair(search_serv.vms.size(), search_serv.id)) == non_full_load_servs.end())
                        continue;
                    // if (except_servs.find(serv.id) != except_servs.end())
                    //     continue;
                    if (search_serv.id == serv_id)
                        continue;

                    return OneAssignScheme(vm_id, serv.id, node);

                    vm_type = search_serv.vms[vm_id].type;
                    vm_core = vm_specs[vm_type].cores;
                    vm_mem = vm_specs[vm_type].memcap;

                    res_to_vm_id.emplace(
                        (serv.nodes[node].cores_ream - vm_core) * mem_per_core + 
                        (serv.nodes[node].mem_ream - vm_mem), vm_id);

                    stop_mem_iter = true;
                    break;
                }
                if (stop_mem_iter)
                    break;
            }
        }

        if (!res_to_vm_id.empty())
        {
            int min_res_vm_id = res_to_vm_id.begin()->second;
            return OneAssignScheme(min_res_vm_id, serv_id, node, vm_id_to_serv_id[min_res_vm_id]);
        }
    }

    return OneAssignScheme(-1, -1, -1, -1);
}

OneAssignScheme mig_serv_select_vm_full_load(int serv_id, int node, int vm_node)
{
    // node: 要填充的是服务器的哪个节点，可取值 0, 1, 2
    // vm_node：要选择的虚拟机的型号，可取值 1, 2
    ServStat &serv = serv_stats[serv_id];

    multimap<int, int> res_to_id;  // res -> vn_id

    int vm_type = -1;

    // 使用双节点虚拟机填充服务器的两个节点
    if (node == 2)
    {
        int min_core = min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream);
        int max_core = max(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream);
        auto vm_core_lower_bound = serv_stats.vm_stat.lower_bound(min_core * 2);
        auto vm_core_upper_bound = serv_stats.vm_stat.upper_bound((min_core - core_node_full_load_thresh) * 2);

        // if (vm_core_lower_bound->first > min_core * 2)
        //     return OneAssignScheme(-1, -1, -1, -1);

        for (auto vm_core_iter = vm_core_lower_bound; vm_core_iter != vm_core_upper_bound; ++vm_core_iter)
        {
            int min_mem = min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream);
            int max_mem = max(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream);
            auto vm_mem_lower_bound = vm_core_iter->second.lower_bound(min_mem * 2);
            auto vm_mem_upper_bound = vm_core_iter->second.upper_bound((min_mem - mem_node_full_load_thresh) * 2);
            
            // if (vm_mem_lower_bound->first > min_mem * 2)
            //     continue;

            for (auto vm_mem_iter = vm_mem_lower_bound; vm_mem_iter != vm_mem_upper_bound; ++vm_mem_iter)
            {
                auto vm_node_iter = vm_mem_iter->second.lower_bound(2);
                if (vm_node_iter != vm_mem_iter->second.end())
                {
                    for (auto &vm_id: vm_node_iter->second)
                    {
                        ServStat &search_serv = serv_stats.get_serv_by_vm_id(vm_id);
                        if (non_full_load_servs.find(make_pair(search_serv.vms.size(), search_serv.id)) == non_full_load_servs.end())
                            continue;
                        // if (except_servs.find(serv.id) != except_servs.end())
                        //     continue;
                        if (search_serv.id == serv_id)
                            continue;

                        res_to_id.emplace(vm_core_iter->first + vm_mem_iter->first, vm_id);
                    }
                }
            }
        }

        if (!res_to_id.empty())
        {
            int rtn_vm_id = res_to_id.rbegin()->second;
            return OneAssignScheme(rtn_vm_id, serv_id, 2, vm_id_to_serv_id[rtn_vm_id]);
        }
        
    }


    // 使用单节点虚拟机填充服务器的 node 节点
    if (node < 2)
    {
        int core_ream = serv.nodes[node].cores_ream;
        auto vm_core_lower_bound = serv_stats.vm_stat.lower_bound(core_ream);
        auto vm_core_upper_bound = serv_stats.vm_stat.upper_bound(core_ream - core_node_full_load_thresh);

        // c++ 似乎无法保证 lower_bound 一定小于等于 upper_bound
        // if (vm_core_upper_bound->first > core_ream)
        //     return OneAssignScheme(-1, -1, -1, -1);
        for (auto vm_core_iter = vm_core_lower_bound; vm_core_iter != vm_core_upper_bound; ++vm_core_iter)
        {
            int mem_ream = serv.nodes[node].mem_ream;
            auto vm_mem_lower_bound = vm_core_iter->second.lower_bound(mem_ream);
            auto vm_mem_upper_bound = vm_core_iter->second.lower_bound(mem_ream - mem_node_full_load_thresh);

            if (vm_mem_lower_bound->first > mem_ream)
                continue;

            for (auto vm_mem_iter = vm_mem_lower_bound; vm_mem_iter != vm_mem_upper_bound; ++vm_mem_iter)
            {
                // auto vm_node_lower_bound = vm_mem_iter->second.lower_bound(1);
                // auto vm_node_upper_bound = vm_mem_iter->second.upper_bound(2);
                auto vm_node_iter = vm_mem_iter->second.find(1);
                if (vm_node_iter == vm_mem_iter->second.end())
                    continue;
        
                for (auto &vm_id: vm_node_iter->second)
                {
                    ServStat &search_serv = serv_stats.get_serv_by_vm_id(vm_id);
                    if (non_full_load_servs.find(make_pair(search_serv.vms.size(), search_serv.id)) == non_full_load_servs.end())
                        continue;
                    // if (except_servs.find(serv.id) != except_servs.end())
                    //     continue;
                    if (search_serv.id == serv_id)
                        continue;

                    return OneAssignScheme(vm_id, serv_id, node, search_serv.id);
                }
            }
        }
    }

    return OneAssignScheme(-1, -1, -1, -1);
}

OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched)
{
    static int invok_count = 0;
    ++invok_count;
    
    static time_t one_node_full_load_time = 0, two_node_full_load_time = 0, one_node_hash_time = 0, two_node_hash_time = 0;

    if (DEBUG)
    {
        if (invok_count % 10000 == 0)
            cout << "one node full load: " << one_node_full_load_time << ", two node full load: " << two_node_full_load_time
                << ", one node hash: " << one_node_hash_time << ", two node hash: " << two_node_hash_time << endl;
    }

    time_t tic, toc;
    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    tic = clock();
    // 首先找马上满载的节点，若找不到，再执行原来的方案
    if (vm_node == 1)
    {
        auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.node_core_mem_map.upper_bound(vm_core + core_node_full_load_thresh);
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.upper_bound(vm_mem + mem_node_full_load_thresh);
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    ServStat &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    // 不能搜索已经处理过的，不然会循环迁移
                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                    if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                        serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
                        continue;

                    toc = clock();
                    one_node_full_load_time += toc - tic;
                    return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                }
            }
        }
    }
    toc = clock();
    one_node_full_load_time += toc - tic;
    
    tic = clock();
    // 双节点的虚拟机也是，先找马上满足满载要求的服务器
    if (vm_node == 2)
    {
        auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.serv_core_mem_map.upper_bound(vm_core / 2 + core_node_full_load_thresh);
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.upper_bound(vm_mem / 2 + mem_node_full_load_thresh);
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    ServStat &serv = serv_stats[serv_id];
                    // int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 若两个节点有一个不满足满载需求，那么跳过
                    if (serv.nodes[0].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh ||
                        serv.nodes[0].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                        serv.nodes[1].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                        serv.nodes[1].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh)
                        continue;

                    toc = clock();
                    two_node_full_load_time += toc - tic;
                    return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                }
            }
        }
    }
    toc = clock();
    two_node_full_load_time += toc - tic;
 
    tic = clock();
    // map<int, int> res_to_id;
    if (vm_node == 1)
    {
        auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.node_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    auto &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 如果核心数正好填满，那么内存必须小于满载阈值
                    if (serv.nodes[serv_node].cores_ream - vm_core == 0)
                    {
                        if (serv.nodes[serv_node].mem_ream - vm_mem <= mem_node_full_load_thresh)
                        {
                            toc = clock();
                            one_node_hash_time += toc - tic;
                            return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                        }
                    }
                    else
                    // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
                    {
                        if (abs((serv.nodes[serv_node].mem_ream - vm_mem) / (serv.nodes[serv_node].cores_ream - vm_core) - mem_per_core) <= tol)
                        {
                            toc = clock();
                            one_node_hash_time += toc - tic;
                            return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                        }
                    }
                }
            }
        }
    }
    toc = clock();
    one_node_hash_time += toc - tic;

    if (vm_node == 2)
    {
        set<int> serv_searched;
        auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.serv_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    auto &serv = serv_stats[serv_id];
                    // int serv_node = node.second;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 如果核心数正好填满，那么两个节点的两种资源必须小于满载阈值
                    if (serv.nodes[0].cores_ream - vm_core / 2 == 0 || serv.nodes[1].cores_ream - vm_core / 2 == 0)
                    {
                        if (serv.nodes[0].cores_ream - vm_core / 2 <= core_node_full_load_thresh &&
                            serv.nodes[0].mem_ream - vm_mem / 2 <= mem_node_full_load_thresh &&
                            serv.nodes[1].cores_ream - vm_core / 2 <= core_node_full_load_thresh &&
                            serv.nodes[1].mem_ream - vm_mem / 2 <= mem_node_full_load_thresh)
                        {
                            toc = clock();
                            two_node_hash_time += toc - tic;
                            return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                        }

                    }
                    // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
                    else
                    {
                        if (abs((serv.nodes[0].mem_ream - vm_mem / 2) / (serv.nodes[0].cores_ream - vm_core / 2) - mem_per_core) <= tol &&
                            abs((serv.nodes[1].mem_ream - vm_mem / 2) / (serv.nodes[1].cores_ream - vm_core / 2) - mem_per_core) <= tol)
                        {
                            toc = clock();
                            two_node_hash_time += toc - tic;
                            return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                        }
                    }
                    serv_searched.insert(serv.id);
                }
            }
        }
    }

    two_node_hash_time += toc - tic;
    return OneAssignScheme(-1, -1, -1);
}

OneAssignScheme mig_vm_select_serv_bf(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched)
{
    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    if (vm_node == 1)
    {
        auto lower_bound = serv_stats.nnode_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.nnode_core_mem_map.upper_bound(vm_core + core_node_full_load_thresh);
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.upper_bound(vm_mem + mem_node_full_load_thresh);
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    ServStat &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    // 不能搜索已经处理过的，不然会循环迁移
                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                    if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                        serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
                        continue;

                    return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
                }
            }
        }
    }

    if (vm_node == 2)
    {
        auto lower_bound = serv_stats.nserv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.nserv_core_mem_map.upper_bound(vm_core / 2 + core_node_full_load_thresh);
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.upper_bound(vm_mem / 2 + mem_node_full_load_thresh);
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    ServStat &serv = serv_stats[serv_id];
                    // int serv_node = node.second;

                    // 不能原地迁移
                    if (serv.id == old_serv_id)
                        continue;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    if (serv.vms.empty())
                        continue;

                    // 若两个节点有一个不满足满载需求，那么跳过
                    if (serv.nodes[0].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh ||
                        serv.nodes[0].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                        serv.nodes[1].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                        serv.nodes[1].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh)
                        continue;

                    return OneAssignScheme(vm_id, serv.id, 2, old_serv_id);
                }
            }
        }
    }



    int res;
    bool stop = false;
    if (vm_node == 1)
    {
        map<int, pair<int, int>> res_to_id;  // res -> (id, node)

        auto lower_bound = serv_stats.nnode_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.nnode_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            if (!res_to_id.empty())
            {
                if (core_iter->first - vm_core > res_to_id.begin()->first)
                    break;
            }
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    auto &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    if (serv.vms.empty())
                        continue;

                    if (serv.id == old_serv_id)
                        continue;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    // 可以改成 serv_node
                    res = (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core) * mem_per_core
                            + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                    // best fitting
                    res_to_id.emplace(res, make_pair(serv.id, serv_node));

                    // break;
                }

                break;
            }
        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second.first, res_to_id.begin()->second.second, old_serv_id);
    }

    if (vm_node == 2)
    {
        map<int, int> res_to_id;  // res -> (id, node)

        auto lower_bound = serv_stats.nserv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.nserv_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            if (!res_to_id.empty())
            {
                if (core_iter->first - vm_core / 2 > res_to_id.begin()->first)
                    break;
            }

            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    auto &serv = serv_stats[serv_id];

                    if (serv.vms.empty())
                        continue;

                    if (serv.id == old_serv_id)
                        continue;

                    if (serv_searched.find(serv.id) != serv_searched.end())
                        continue;

                    res = (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core) * mem_per_core
                            + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                    res_to_id.emplace(res, serv.id);
                    // break;
                }
                break;
            }
        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second, 2, old_serv_id);
    }

    return OneAssignScheme(-1, -1, -1, -1);
}

OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type)
{
    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    // if (vm_node == 1)
    // {
    //     auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
    //     auto upper_bound = serv_stats.node_core_mem_map.upper_bound(vm_core + core_node_full_load_thresh);
    //     for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
    //     {
    //         auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
    //         auto mem_upper_bound = core_iter->second.upper_bound(vm_mem + mem_node_full_load_thresh);
    //         for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
    //         {
    //             for (auto &node: mem_iter->second)
    //             {
    //                 auto &serv = serv_stats[node.first];
    //                 int serv_node = node.second;

    //                 // // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
    //                 // if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
    //                 //     serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
    //                 //     continue;

    //                 // 若这个节点的内存不满足满载需求，或者放不下虚拟机，那么跳过
    //                 if (serv.nodes[serv_node].mem_ream < vm_mem ||
    //                     serv.nodes[serv_node].mem_ream - vm_mem >= mem_node_full_load_thresh)
    //                     continue;

    //                 return OneAssignScheme(vm_id, serv.id, serv_node, -1);
    //             }
    //         }
    //     }
    // }

    // // 双节点的虚拟机也是，先找马上满足满载要求的服务器
    // if (vm_node == 2)
    // {
    //     auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
    //     auto upper_bound = serv_stats.serv_core_mem_map.upper_bound(vm_core / 2 + core_node_full_load_thresh);
    //     for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
    //     {
    //         auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
    //         auto mem_upper_bound = core_iter->second.upper_bound(vm_mem / 2 + mem_node_full_load_thresh);
    //         for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
    //         {
    //             for (auto &serv_id: mem_iter->second)
    //             {
    //                 auto &serv = serv_stats[serv_id];

    //                 // 若这个服务器不满足满载需求，那么跳过
    //                 if (serv.nodes[0].cores_ream - vm_core / 2 > core_node_full_load_thresh ||
    //                     serv.nodes[0].mem_ream - vm_mem / 2 > mem_node_full_load_thresh ||
    //                     serv.nodes[1].cores_ream - vm_core / 2 > core_node_full_load_thresh ||
    //                     serv.nodes[1].mem_ream - vm_mem / 2 > mem_node_full_load_thresh)
    //                     continue;

    //                 return OneAssignScheme(vm_id, serv.id, 2, -1);
    //             }
    //         }
    //     }
    // }

    // // 对于无法使服务器满载的情况
    // if (vm_node == 1)
    // {
    //     auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
    //     auto upper_bound = serv_stats.node_core_mem_map.end();
    //     for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
    //     {
    //         auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
    //         auto mem_upper_bound = core_iter->second.end();
    //         for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
    //         {
    //             for (auto &node: mem_iter->second)
    //             {
    //                 auto &serv = serv_stats[node.first];
    //                 int serv_node = node.second;

    //                 // 若这个节点放不下虚拟机，那么跳过
    //                 if (serv.nodes[serv_node].mem_ream < vm_mem)
    //                     continue;

    //                 // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
    //                 if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
    //                     serv.nodes[0].mem_ream == serv.nodes[1].mem_ream && 
    //                     serv.nodes[0].cores_ream != serv_specs[serv.type].cores / 2)
    //                     continue;

    //                 // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
    //                 if (abs(serv.nodes[serv_node].mem_ream - vm_mem - (serv.nodes[serv_node].cores_ream - vm_core) * mem_per_core) <= tol)
    //                 {
    //                     return OneAssignScheme(vm_id, serv.id, serv_node, -1);
    //                 }
    //             }
    //         }
    //     }
    // }

    // if (vm_node == 2)
    // {
    //     auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
    //     auto upper_bound = serv_stats.serv_core_mem_map.end();
    //     for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
    //     {
    //         auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
    //         auto mem_upper_bound = core_iter->second.end();
    //         for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
    //         {
    //             for (auto &serv_id: mem_iter->second)
    //             {
    //                 auto &serv = serv_stats[serv_id];
    //                 // int serv_node = node.second;

    //                 // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
    //                 if (abs(serv.nodes[0].mem_ream - vm_mem / 2 - (serv.nodes[0].cores_ream - vm_core / 2) * mem_per_core) <= tol &&
    //                     abs(serv.nodes[1].mem_ream - vm_mem / 2 - (serv.nodes[1].cores_ream - vm_core / 2) * mem_per_core) <= tol)
    //                 {
    //                     return OneAssignScheme(vm_id, serv.id, 2, -1);
    //                 }
    //             }
    //         }
    //     }
    // }

    // 对于无法满足平衡要求的情况
    
    int res;
    bool stop = false;
    if (vm_node == 1)
    {
        map<int, pair<int, int>> res_to_id;  // res -> (id, node)

        auto lower_bound = serv_stats.node_core_mem_map.lower_bound(vm_core);
        auto upper_bound = serv_stats.node_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            if (!res_to_id.empty())
            {
                if (core_iter->first - vm_core > res_to_id.begin()->first)
                    break;
            }
            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &node: mem_iter->second)
                {
                    auto &serv = serv_stats[node.first];
                    int serv_node = node.second;

                    // 可以改成 serv_node
                    res = (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core) * (1 + mem_per_core)
                            + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                    // best fitting
                    res_to_id.emplace(res, make_pair(serv.id, serv_node));

                    // break;
                }

                break;
            }
        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second.first, res_to_id.begin()->second.second, -1);
    }

    if (vm_node == 2)
    {
        map<int, int> res_to_id;  // res -> (id, node)

        auto lower_bound = serv_stats.serv_core_mem_map.lower_bound(vm_core / 2);
        auto upper_bound = serv_stats.serv_core_mem_map.end();
        for (auto core_iter = lower_bound; core_iter != upper_bound; ++core_iter)
        {
            if (!res_to_id.empty())
            {
                if (core_iter->first - vm_core / 2 > res_to_id.begin()->first)
                    break;
            }

            auto mem_lower_bound = core_iter->second.lower_bound(vm_mem / 2);
            auto mem_upper_bound = core_iter->second.end();
            for (auto mem_iter = mem_lower_bound; mem_iter != mem_upper_bound; ++mem_iter)
            {
                for (auto &serv_id: mem_iter->second)
                {
                    auto &serv = serv_stats[serv_id];

                    res = (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core) * mem_per_core
                            + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                    res_to_id.emplace(res, serv.id);
                    // break;
                }
                break;
            }
        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second, 2, -1);
    }

    return OneAssignScheme(-1, -1, -1, -1);
}