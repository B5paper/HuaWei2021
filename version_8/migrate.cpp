#include "migrate.h"
#include "dispatch.h"
#include "select.h"
#include <iostream>
#include <unordered_set>


void migrate(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (serv_stats.servs.size() == 0)
        return;

    bool fail_to_mig = false;

    unordered_set<int> servs_unmiged;
    unordered_set<int> servs_miged;


    // 统计当前虚拟机数量
    int num_vms = 0;
    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        num_vms += serv_iter->second.vms.size();
    }

    for (auto serv_iter = serv_stats.servs.begin(); serv_iter != serv_stats.servs.end(); ++serv_iter)
    {
        servs_unmiged.insert(serv_iter->first);
    }

    int max_num_mig = int(float(num_vms) * 5 / 1000);
    int num_mig = 0;

    if (max_num_mig == 0)
        return;

    while (true)
    {
        // cout << "servs_unmiged size: " << servs_unmiged.size() << endl;
        if (servs_unmiged.empty())
            break;

        // 找到服务器里虚拟机最少的，但是不能一个也没有
        // 如果这个服务器已经处理过了，那么跳过
        int vm_nums;
        int min_vm_nums = INT32_MAX;
        int min_vm_nums_id;

        for (auto serv_id = servs_unmiged.begin(); serv_id != servs_unmiged.end(); ++serv_id)
        {
            ServStat &serv_stat = serv_stats[*serv_id];
            vm_nums = serv_stat.vms.size();
            if (vm_nums < min_vm_nums && vm_nums > 0)
            {
                min_vm_nums = vm_nums;
                min_vm_nums_id = *serv_id;
            }
        }

        // 剩下的服务器全都是零台虚拟机
        if (min_vm_nums == INT32_MAX)
            break;

        servs_unmiged.erase(min_vm_nums_id);

        // 对这个服务器里的虚拟机进行遍历，使用 best fitting 算法，找到最合适的服务器
        ServStat &serv_stat = serv_stats[min_vm_nums_id];
        vector<int> vm_ids;
        vector<VMTypeNode> vm_type_nodes;
        for (auto vm_iter = serv_stat.vms.begin(); vm_iter != serv_stat.vms.end(); ++vm_iter)
        {
            vm_ids.push_back(vm_iter->first);
            vm_type_nodes.push_back(vm_iter->second);
        }

        for (int i = 0; i < vm_ids.size(); ++i)
        {
            int vm_id = vm_ids[i];
            int vm_type = vm_type_nodes[i].type;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;

            OneAssignScheme one_assign_scheme = vm_select_server_bf(vm_id, vm_type, serv_stats, serv_specs, vm_specs);

            if (one_assign_scheme.server_id == -1)
            {
                // fail_to_mig = true;
                continue;
                // break;
            }

            int serv_id = one_assign_scheme.server_id;
            int serv_node = one_assign_scheme.server_node;
            
            // cout << "mig vm " << vm_id << " to " << serv_id << endl;
            mig_scheme.push_back(OneMigScheme(vm_id, serv_id, serv_node));
            if (!serv_stats[serv_id].can_hold_vm(vm_core, vm_mem, serv_node, serv_specs))
                cout << "error in migrate" << endl;
            serv_stats.mig_vm(vm_id, serv_id, serv_node, serv_specs, vm_specs);  // 可优化，直接指定 server_id 也可以
            ++num_mig;

            if (num_mig >= max_num_mig)
            {
                fail_to_mig = true;
                break;
            }
        }

        if (fail_to_mig)
            break;
    }
}