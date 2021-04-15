#include "select.h"
#include <cmath>
#include <algorithm>
#include "migrate.h"

OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched, ServStatList &serv_stats, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);

    
    // 对于单节点的虚拟机，直接用哈希表查
    if (vm_node == 1)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = -1;
        // int max_num_search = 1500;
        int num_search = 0;
        int max_shift = ream.rbegin()->first + 1;
        
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

                    if (serv_stats[new_serv_id].vms.size() < serv_stats[old_serv_id].vms.size())  // 不能往少的地方迁移
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

    // 双节点也用哈希表查
    if (vm_node == 2)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = 2;
        int res;
        int max_shift = dream.rbegin()->first + 1;
        
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

                    if (serv_searched.find(new_serv_id) != serv_searched.end())  // 已处理过的服务器不再处理
                        continue;

                    if (serv_stats[new_serv_id].vms.size() < serv_stats[old_serv_id].vms.size())  // 不能往少的地方迁移
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

    return OneAssignScheme(-1, -1, -1);
}

OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type,  ServStatList &serv_stats, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    int vm_node = vm_specs[vm_type].nodes;

    if (serv_stats.servs.empty())
        return OneAssignScheme(-1, -1, -1, -1);
        
    // int max_num_search = 3000;
    int res_thresh = 20;
    float bal_thresh = 1.5;
    
    // 对于单节点的虚拟机，直接用哈希表查
    if (vm_node == 1)
    {
        int shift = 0;
        int demand = vm_core + vm_mem;
        int new_serv_id = -1;
        int new_serv_node = -1;
        int num_search = 0;
        int max_shift = ream.rbegin()->first + 1;
        
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
                    {
                        return OneAssignScheme(vm_id, new_serv_id, new_serv_node, -1);
                    }
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
        int max_shift = dream.rbegin()->first + 1;
        
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