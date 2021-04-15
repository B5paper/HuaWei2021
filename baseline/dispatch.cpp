#include "dispatch.h"
#include "types.h"
#include "status.h"
#include <iostream>


int purchase_server(PurchaseScheme &purchase_scheme, ServSpecList &server_specs)
{
    int server_type = rand() % server_specs.size();
    purchase_scheme.add_server(server_type);
    return server_type;
}

bool comp_id_type(ServIdType &s1, ServIdType &s2)
{
    if (s1.type < s2.type) return true;
    if (s1.type == s2.type)
    {
        if (s1.id < s2.id)
            return true;
    }
    return false;
}


vector<AddReq> select_add_reqs(int idx_day, int idx_req, DayReqList &reqs)
{
    vector<AddReq> add_reqs;
    for (int i = idx_req; i < reqs[idx_day]->op.size(); ++i)
    {
        if (reqs[idx_day]->op[i] == 0)
        {
            return add_reqs;
        }
        add_reqs.push_back(AddReq(reqs[idx_day]->id[i], reqs[idx_day]->id[i]));
    }
    return add_reqs;
}

OneAssignScheme vm_select_server_2(int vm_id, int vm_type, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
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

void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    while (true)
    {
        OneAssignScheme oas;
        oas = vm_select_server_2(vm_id, vm_type, serv_stats, serv_specs, vm_specs);
        
        // 没有一个现存的服务器可以放得下，购买一个新服务器
        // 目前只买 type 0 服务器
        if (oas.server_id == -1)
        {
            purchase_scheme.add_server(74);
            serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(74)));
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;

        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node, serv_specs, vm_specs);
        break;
    }
}

void dispatch(ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps)
{
    bool exist = false;
    for (int idx_day = 0; idx_day < reqs.size(); ++idx_day)
    {
        // cout << "day " << idx_day << endl;
        PurchaseScheme purchase_scheme;
        AssignScheme assign_scheme;
        int idx_req = 0;
        int op, vm_id, vm_type;
        
        while (idx_req < reqs[idx_day]->id.size())
        {
            // cout << "req " << idx_req << endl;

            // 解析下一天的操作
            op = reqs[idx_day]->op[idx_req];
            vm_id = reqs[idx_day]->id[idx_req];
            vm_type = reqs[idx_day]->vm_type[idx_req];

            // 处理删除操作
            if (op == 0)
            {
                ServStat &serv_stat = serv_stats.get_serv_by_vm_id(vm_id);
                serv_stat.del_vm(vm_id, vm_specs);
                ++idx_req;
                continue;
            }

            // 处理添加操作
            OneAssignScheme op_assign_scheme;
            if (op == 1)
            {
                process_add_op(vm_id, vm_type, purchase_scheme, op_assign_scheme, serv_stats, serv_specs, vm_specs);
                assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
                // cout << "vm id: " << vm_id << ", server id: " << op_assign_scheme.server_id << ", serv node: " << op_assign_scheme.server_node << endl;
                ++idx_req;
            }
        }

        // 将购买方案写入到输出中
        vector<ServerTypeNum> server_type_num = purchase_scheme.get_ordered_server_types();
        disps.new_day();
        for (int i = 0; i < server_type_num.size(); ++i)
        {
            disps.push_pur_server(idx_day, server_type_num[i].server_type, server_type_num[i].num);
        }

        // 将调度方案写入到输出中
        for (int i = 0; i < reqs[idx_day]->op.size(); ++i)
        {
            if (reqs[idx_day]->op[i] == 1)
            {
                int vm_id = reqs[idx_day]->id[i];
                int server_id = assign_scheme.at(vm_id).server_id;
                int server_node = assign_scheme.at(vm_id).server_node;
                // cout << "vm id: " << vm_id << ", serv id: " << server_id << ", serv node: " << server_node << endl;
                disps.push_add_vm(idx_day, server_id, server_node);   
            }
        }
    }
}
