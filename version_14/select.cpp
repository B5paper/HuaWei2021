#include "select.h"
#include <cmath>
#include <algorithm>
#include "migrate.h"
#include "args.h"
#include <ctime>
#include "types.h"

OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched, ServStatList &serv_stats, VMSpecList &vm_specs)
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
        auto lower_bound = serv_core_map_2.lower_bound(vm_core);
        auto upper_bound = serv_core_map_2.upper_bound(vm_core + core_node_full_load_thresh);
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 不能原地迁移
                if (serv.id == old_serv_id)
                    continue;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                    serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
                    continue;

                // 若这个节点的内存不满足满载需求，或者放不下虚拟机，那么跳过
                if (serv.nodes[serv_node].mem_ream < vm_mem ||
                    serv.nodes[serv_node].mem_ream - vm_mem >= mem_node_full_load_thresh)
                    continue;
                toc = clock();
                one_node_full_load_time += toc - tic;
                return OneAssignScheme(vm_id, serv.id, serv_node, old_serv_id);
            }
        }
    }
    toc = clock();
    one_node_full_load_time += toc - tic;
    
    tic = clock();
    // 双节点的虚拟机也是，先找马上满足满载要求的服务器
    if (vm_node == 2)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core / 2);
        auto upper_bound = serv_core_map_2.upper_bound(vm_core / 2 + core_node_full_load_thresh);
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 不能原地迁移
                if (serv.id == old_serv_id)
                    continue;

                // 放不下双节点的虚拟机时也跳过
                if (!serv.can_hold_vm(vm_core, vm_mem, 2))
                    continue;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                // if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                //     serv.nodes[1].mem_ream == serv.nodes[1].mem_ream)
                //     continue;

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
    toc = clock();
    two_node_full_load_time += toc - tic;
 
    tic = clock();
    // map<int, int> res_to_id;
    if (vm_node == 1)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 不能原地迁移
                if (serv.id == old_serv_id)
                    continue;

                // 若这个节点放不下虚拟机，那么跳过
                if (serv.nodes[serv_node].mem_ream < vm_mem)
                    continue;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                    serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
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
    toc = clock();
    one_node_hash_time += toc - tic;

    if (vm_node == 2)
    {
        set<int> serv_searched;
        auto lower_bound = serv_core_map_2.lower_bound(vm_core / 2);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                if (serv_searched.find(serv.id) != serv_searched.end())
                    continue;

                // 不能原地迁移
                if (serv.id == old_serv_id)
                    continue;

                // 放不下双节点的虚拟机时也跳过
                if (!serv.can_hold_vm(vm_core, vm_mem, 2))
                {
                    serv_searched.insert(serv.id);
                    continue;
                }

                // 如果核心数正好填满，那么两个节点的两种资源必须小于满载阈值
                if (serv.nodes[serv_node].cores_ream - vm_core / 2 == 0)
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

    two_node_hash_time += toc - tic;
    return OneAssignScheme(-1, -1, -1);
}

OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type, ServStatList &serv_stats, VMSpecList &vm_specs)
{
    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;
        
    // int max_num_search = 3000;

    if (vm_node == 1)
    {
        // const int core_thresh = 5, mem_thresh = 10;
        auto lower_bound = serv_core_map_2.lower_bound(vm_core);
        auto upper_bound = serv_core_map_2.upper_bound(vm_core + core_node_full_load_thresh);
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                    serv.nodes[0].mem_ream == serv.nodes[1].mem_ream)
                    continue;

                // 若这个节点的内存不满足满载需求，或者放不下虚拟机，那么跳过
                if (serv.nodes[serv_node].mem_ream < vm_mem ||
                    serv.nodes[serv_node].mem_ream - vm_mem >= mem_node_full_load_thresh)
                    continue;

                return OneAssignScheme(vm_id, serv.id, serv_node, -1);
            }
        }
    }

    // 双节点的虚拟机也是，先找马上满足满载要求的服务器
    if (vm_node == 2)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core / 2);
        auto upper_bound = serv_core_map_2.upper_bound(vm_core / 2 + core_node_full_load_thresh);
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 放不下双节点的虚拟机时也跳过
                if (!serv.can_hold_vm(vm_core, vm_mem, 2))
                    continue;

                // 若这个服务器不满足满载需求，那么跳过
                if (serv.nodes[0].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                    serv.nodes[0].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh ||
                    serv.nodes[1].cores_ream - vm_core / 2 >= core_node_full_load_thresh ||
                    serv.nodes[1].mem_ream - vm_mem / 2 >= mem_node_full_load_thresh)
                    continue;

                return OneAssignScheme(vm_id, serv.id, 2, -1);
            }
        }
    }

    // 对于无法使服务器满载的情况
    if (vm_node == 1)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 若这个节点放不下虚拟机，那么跳过
                if (serv.nodes[serv_node].mem_ream < vm_mem)
                    continue;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                    serv.nodes[0].mem_ream == serv.nodes[1].mem_ream && 
                    serv.nodes[0].cores_ream != serv_specs[serv.type].cores / 2)
                    continue;

                // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
                if (abs(serv.nodes[serv_node].mem_ream - vm_mem - (serv.nodes[serv_node].cores_ream - vm_core) * mem_per_core) <= tol)
                {
                    return OneAssignScheme(vm_id, serv.id, serv_node, -1);
                }
            }
        }
    }

    if (vm_node == 2)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core / 2);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 放不下双节点的虚拟机时跳过
                if (!serv.can_hold_vm(vm_core, vm_mem, 2))
                    continue;

                // 对于无法满载的服务器，剩余的核心数每增加一个，剩余的内存数增量不能超过 mem_per_core 个
                if (abs(serv.nodes[0].mem_ream - vm_mem / 2 - (serv.nodes[0].cores_ream - vm_core / 2) * mem_per_core) <= tol &&
                    abs(serv.nodes[1].mem_ream - vm_mem / 2 - (serv.nodes[1].cores_ream - vm_core / 2) * mem_per_core) <= tol)
                {
                    return OneAssignScheme(vm_id, serv.id, 2, -1);
                }
            }
        }
    }

    // 对于无法满足平衡要求的情况
    map<int, pair<int, int>> res_to_id;  // res -> (id, node)
    int res;
    bool stop = false;
    if (vm_node == 1)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 若这个节点放不下虚拟机，那么跳过
                if (serv.nodes[serv_node].mem_ream < vm_mem)
                    continue;

                // 如果这个服务器是平衡的，那么不要破坏，留着放双节点
                // if (serv.nodes[0].cores_ream == serv.nodes[1].cores_ream &&
                //     serv.nodes[0].mem_ream == serv.nodes[1].mem_ream && 
                //     serv.nodes[0].cores_ream != serv_specs[serv.type].cores / 2)
                //     continue;

                // 可以改成 serv_node
                res = (serv.nodes[0].cores_ream + serv.nodes[1].cores_ream - vm_core)
                        + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                // best fitting
                res_to_id.emplace(res, make_pair(serv.id, serv_node));
            }

            // 这里也可以改成 serv_node 试试
            if (!res_to_id.empty())
            {
                if (set_iter->first - vm_core > res_to_id.begin()->first)
                {
                    return OneAssignScheme(vm_id, res_to_id.begin()->second.first, res_to_id.begin()->second.second, -1);
                }
            }

        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second.first, res_to_id.begin()->second.second, -1);
    }

    if (vm_node == 2)
    {
        auto lower_bound = serv_core_map_2.lower_bound(vm_core / 2);
        auto upper_bound = serv_core_map_2.end();
        for (auto set_iter = lower_bound; set_iter != upper_bound; ++set_iter)
        {
            set<pair<int, pair<int, int>>> &node_set = set_iter->second;
            for (auto &node: node_set)
            {
                ServStat &serv = serv_stats[node.second.first];
                int serv_node = node.second.second;

                // 放不下双节点的虚拟机时跳过
                if (!serv.can_hold_vm(vm_core, vm_mem, 2))
                    continue;

                res = (serv.nodes[0].cores_ream + serv.nodes[0].cores_ream - vm_core)
                        + serv.nodes[0].mem_ream + serv.nodes[1].mem_ream - vm_mem;

                res_to_id.emplace(res, make_pair(serv.id, serv_node));
            }

            if (!res_to_id.empty())
            {
                if (set_iter->first - vm_core / 2 > res_to_id.begin()->first)
                {
                    return OneAssignScheme(vm_id, res_to_id.begin()->second.first, 2, -1);
                }
            }
        }

        if (!res_to_id.empty())
            return OneAssignScheme(vm_id, res_to_id.begin()->second.first, 2, -1);
    }


    
    // 对于单节点的虚拟机，直接用哈希表查
    // if (vm_node == 1)
    // {
    //     int shift = 0;
    //     int demand = vm_core + vm_mem;
    //     int max_shift = serv_core_mem_map.rbegin()->first + 1;
        
    //     while (shift < max_shift)
    //     {
    //         set<int> serv_ids = serv_stats.find_serv(demand + shift, 'b', 1);
    //         if (serv_ids.empty())
    //         {
    //             ++shift;
    //             continue;
    //         }
            
    //         for (auto serv_id: serv_ids)
    //         {
    //             ServStat &serv_stat = serv_stats.servs[serv_id];
    //             for (int i = 0; i < 2; ++i)
    //             {
    //                 if (serv_stat.nodes[i].cores_ream + serv_stat.nodes[i].mem_ream == demand + shift)
    //                 {
    //                     if (serv_stat.can_hold_vm(vm_core, vm_mem, i))
    //                         return OneAssignScheme(vm_id, serv_stat.id, i, -1);
    //                 }
    //             }
    //         }
    //         ++shift;
    //     }

    //     return OneAssignScheme(-1, -1, -1, -1);
    // }

    // // 双节点也用哈希表查
    // if (vm_node == 2)
    // {
    //     int shift = 0;
    //     int demand = vm_core + vm_mem;
    //     int max_shift = dserv_core_mem_map.rbegin()->first + 1;
        
    //     while (shift < max_shift)
    //     {
    //         set<int> serv_ids = serv_stats.find_serv(demand + shift, 'b', 2);
    //         if (serv_ids.empty())
    //         {
    //             ++shift;
    //             continue;
    //         }

    //         for (auto serv_id: serv_ids)
    //         {
    //             ServStat &serv_stat = serv_stats.servs[serv_id];
    //             if (serv_stat.can_hold_vm(vm_core, vm_mem, 2))
    //                 return OneAssignScheme(vm_id, serv_stat.id, 2, -1);
    //         }
    //         ++shift;
    //     }
    // }
    return OneAssignScheme(-1, -1, -1, -1);
}