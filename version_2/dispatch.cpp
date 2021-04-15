#include "dispatch.h"
#include <iostream>

// ExpandScheme expand_servers(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs)
// {

// }

// MigrateScheme migrate_vms()
// {
//     return 
// }

void assign_vms()
{

}


bool process_reqs(int idx_day, int &idx_req, DayReqList &reqs,
                ServerSpecList &server_specs, VMSpecList &vm_specs,
                ServerStat &server_stat, VMStat &vm_stat, ServerStatRecorder &rec,
                DayDispList &disps, bool &error)
{
    while (true)
    {
        if (idx_req == reqs[idx_day]->id.size())
        {
            return true;
        }

        if (reqs[idx_day]->op[idx_req] == 0)  // delete a vm
        {
            int vm_id = reqs[idx_day]->id[idx_req];
            int vm_type = vm_stat.types[vm_stat._get_idx_by_id(vm_id)];
            int server_id = vm_stat.server_ids[vm_stat._get_idx_by_id(vm_id)];
            int node = vm_stat.nodes[vm_stat._get_idx_by_id(vm_id)];
            if (server_stat.del_vm(vm_type, server_id, node, server_specs, vm_specs))  // succeed to delete the virtual machine
            {
                vm_stat.del_vm(vm_id);
                ++idx_req;
            }
            else
            {
                error = true;  // fail to delete the virtual machine
                return false;
            }
        }

        if (reqs[idx_day]->op[idx_req] == 1)  // assign a vm
        {
            int vm_id = reqs[idx_day]->id[idx_req];
            int vm_type = reqs[idx_day]->name_id[idx_req];
            int vm_node = vm_specs.nodes[vm_type];  // 1 for single node, 2 for two nodes
            int vm_core = vm_specs.cores[vm_type];
            int vm_mem = vm_specs.memcap[vm_type];
            
            int server_id = -1;
            int server_node = -1;

            for (int j = 0; j < server_stat.ids.size(); ++j)
            {
                if (vm_node == 2)
                {
                    if (server_stat.add_vm(vm_type, j, 2, server_specs, vm_specs))  // succeed to add
                    {
                        server_id = j;
                        server_node = 2;
                        ++idx_req;
                        vm_stat.add_vm(vm_type, vm_id, server_id, server_node, server_specs, vm_specs);
                        disps.disp_list.back()->req_server_ids.push_back(server_id);
                        disps.disp_list.back()->req_server_nodes.push_back(server_node);
                        break;
                    }
                }

                if (vm_node == 1)
                {
                    for (int k = 0; k < 2; ++k)
                    {
                        if (server_stat.add_vm(vm_type, j, k, server_specs, vm_specs))
                        {
                            server_id = j;
                            server_node = k;
                            break;
                        }
                    }

                    if (server_id != -1)
                    {
                        vm_stat.add_vm(vm_type, vm_id, server_id, server_node, server_specs, vm_specs);
                        disps.disp_list.back()->req_server_ids.push_back(server_id);
                        disps.disp_list.back()->req_server_nodes.push_back(server_node);
                        ++idx_req;
                        break;
                    }                    
                }
            }

            if (server_id == -1)  // buy a server
            {
                for (int i = 0; i < server_specs.name_id.size(); ++i)
                {
                    if (server_specs.cores[i] > vm_core && server_specs.memcap[i] > vm_mem)
                    {
                        server_stat.add_server(i, server_specs);
                        disp_pur_server(i, disps);
                        return false;
                    }
                }
            }
        }
    }
}


// dispatch requirements of one day
int dispatch(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs, 
            ServerStat &server_stat, VMStat &vm_stat, ServerStatRecorder &rec,
            DayDispList &disps)
{
    disps.disp_list.push_back(new DayDisp);
    int days_processed;

    // write the algorithm here.
    static int idx_day = 0;
    int idx_req = 0;
    bool error = false;
    int fail_count = 0;
    while (true)
    {
        if (fail_count == 1000)
        {
            cout << "reach the max fail count" << endl;
            return -1;
        }

        if (!process_reqs(idx_day, idx_req, reqs, server_specs, vm_specs, server_stat, vm_stat, rec, disps, error))
        {
            if (error)
                return -1;
            else
            {
                fail_count++;
                continue;
            }
        }
        else
        {
            break;
        }
    }

    ++idx_day;

    // decide to process how many days
    days_processed = 1;

    // expand servers
    // for (int i = 0; i < 10; ++i)
    // {
    //     server_stat.add(i, server_specs);
    // }

    // migrate virtual machines
    // 直接操作 vm_stat 里的数据即可
    // server_stat.update_stat(VMSingle(1, 0, 0, 1), vm_specs);

    // assign virtual machines
    // 直接操作 vm_stat 里的数据即可，然后用 server_stat.update_stat(vm_stat, vm_specs); 更新 server_stat

    // update server status
    rec.update_one_day(server_stat);

    return days_processed;
}