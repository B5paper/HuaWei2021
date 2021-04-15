#include "dispatch.h"
#include "types.h"
#include "status.h"
#include "migrate.h"
#include "select.h"
#include <iostream>
#include <unordered_map>


int purchase_server(PurchaseScheme &purchase_scheme, ServSpecList &server_specs)
{
    int server_type = rand() % server_specs.size();
    // purchase_scheme.push_back(OnePurchaseScheme(serv_type, serv_))
    // purchase_scheme.add_server(server_type);
    return server_type;
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


void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    while (true)
    {
        OneAssignScheme oas;
        // oas = add_vm_select_server_bf(vm_id, vm_type, serv_stats, serv_specs, vm_specs);
        oas = add_vm_select_serv_bf_hash(vm_id, vm_type, serv_stats, vm_specs);
        
        // 没有一个现存的服务器可以放得下，购买一个新服务器
        // 目前只买能放得下 vm 的价格最低的服务器
        if (oas.server_id == -1)
        {
            // purchase_scheme.add_server(74);
            int min_price = INT32_MAX;
            int min_idx = -1;
            for (int i = 0; i < serv_specs.size(); ++i)
            {
                if (serv_specs[i].can_hold_vm(vm_type, vm_specs))
                {
                    if (serv_specs[i].cost < min_price)
                    {
                        min_price = serv_specs[i].cost;
                        min_idx = i;
                    }
                }
            }
            purchase_scheme.push_back(OnePurchaseScheme(min_idx, serv_stats.servs.size()));
            serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), min_idx, serv_specs)));
            
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;

        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node, vm_specs);
        break;
    }
}

void dispatch(ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps)
{
    for (int idx_day = 0; idx_day < reqs.size(); ++idx_day)
    {
        int num_single = 0;
        for (auto iter = ream.begin(); iter != ream.end(); ++iter)
        {
            num_single += iter->second.size();
        }
        int num_double = 0;
        for (auto iter = dream.begin(); iter != dream.end(); ++iter)
        {
            num_double += iter->second.size();
        }

        // cout << "day " << idx_day << endl;
        PurchaseScheme purchase_scheme;
        AssignScheme assign_scheme;
        int idx_req = 0;
        int op, vm_id, vm_type;

        // 处理迁移操作，使得尽量多的服务器关机
        MigScheme mig_scheme;
        // if (idx_day % 10 == 0)
            // migrate_2(mig_scheme, serv_stats, serv_specs, vm_specs);
            migrate_2(mig_scheme, serv_stats, serv_specs, vm_specs);
        // else
        // migrate_stop_early(mig_scheme, serv_stats, serv_specs, vm_specs);
        // if (mig_scheme.size() > 100)
        //     cout << "idx day: " << idx_day << ", num mig: " << mig_scheme.size() << endl;
        // cout << "day " << idx_day << ", migrate num: " << mig_scheme.size() << endl;


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

        

        // // 打印散列表和当天的服务器状态
        // cout << "------ servers ------" << endl;
        // for (int i = 0; i < serv_stats.servs.size(); ++i)
        // {
        //     ServStat &serv_stat = serv_stats.servs[i];
        //     cout << "serv " << serv_stat.id 
        //     << ": node 0 cores " << serv_stat.nodes[0].cores_ream
        //     << " mem " << serv_stat.nodes[0].mem_ream
        //     << ", node 1 cores " << serv_stat.nodes[1].cores_ream
        //     << " mem " << serv_stat.nodes[1].mem_ream
        //     << endl;
        // }
        // cout << endl;
        // cout << "------ hash list -------" << endl;
        // for (auto iter = ream.begin(); iter != ream.end(); ++iter)
        // {
        //     for (int i = 0; i < iter->second.size(); ++i)
        //     {
        //         cout << "serv " << iter->second[i].first
        //         << ": node " << iter->second[i].second << " res " << iter->first << endl;
        //     }
        // }
        // cout << endl;

        // 排序，映射服务器序号
        sort_and_fill_purchase_id(serv_stats.servs.size() - purchase_scheme.size(), purchase_scheme);
        map_serv_id(purchase_scheme, assign_scheme, mig_scheme, serv_stats);

        // // 将新服务器的节点状态写入到状态表中
        // int core_ream, mem_ream;
        // int serv_id;
        // for (int i = 0; i < purchase_scheme.size(); ++i)
        // {
        //     serv_id = purchase_scheme[i].purchase_id;
        //     for (int j = 0; j < 2; ++j)
        //     {
        //         ream[serv_stats[serv_id].nodes[j].cores_ream + serv_stats[serv_id].nodes[j].mem_ream].push_back(make_pair(serv_id, j));
        //     }
        //     dream[serv_stats[serv_id].nodes[0].cores_ream + serv_stats[serv_id].nodes[0].mem_ream + serv_stats[serv_id].nodes[1].cores_ream + serv_stats[serv_id].nodes[1].mem_ream].insert(serv_id);
        // }

        // 将购买方案写入到输出中
        vector<ServerTypeNum> server_type_num = purchase_scheme_to_serv_type_num(purchase_scheme);
        disps.new_day();
        for (int i = 0; i < server_type_num.size(); ++i)
        {
            disps.push_pur_server(idx_day, server_type_num[i].server_type, server_type_num[i].num);
        }

        // 将迁移方案写入到输出中
        for (int i = 0; i < mig_scheme.size(); ++i)
        {
            disps.push_mig_vm(idx_day, mig_scheme[i].vm_id, mig_scheme[i].serv_id, mig_scheme[i].serv_node);
        }

        // 将调度方案写入到输出中
        for (int i = 0; i < reqs[idx_day]->op.size(); ++i)
        {
            if (reqs[idx_day]->op[i] == 1)
            {
                int vm_id = reqs[idx_day]->id[i];
                disps.push_add_vm(idx_day, assign_scheme[vm_id].server_id, assign_scheme[vm_id].server_node);   
            }
        }
    }
}

bool comp_serv_type(OnePurchaseScheme &p1, OnePurchaseScheme &p2)
{
    if (p1.type < p2.type)
        return true;
    else
        return false;
}

void sort_and_fill_purchase_id(int idx_start, PurchaseScheme &purchase_scheme)
{
    sort(purchase_scheme.begin(), purchase_scheme.end(), comp_serv_type);
    for (int i = 0; i < purchase_scheme.size(); ++i)
    {
        purchase_scheme[i].purchase_id = idx_start + i;
    }
}

void map_serv_id(PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, MigScheme &mig_scheme, ServStatList &serv_stats)
{
    unordered_map<int, ServStat> temp_serv_stats;
    int serv_stat_id, purchase_id;
    for (int i = 0; i < purchase_scheme.size(); ++i)
    {
        serv_stat_id = purchase_scheme[i].serv_stat_id;
        temp_serv_stats.insert(make_pair(serv_stat_id, serv_stats[serv_stat_id]));
    }

    unordered_map<int, int> id_old_to_new;
    for (int i = 0; i < purchase_scheme.size(); ++i)
    {
        purchase_id = purchase_scheme[i].purchase_id;
        serv_stat_id = purchase_scheme[i].serv_stat_id;
        serv_stats[purchase_id] = temp_serv_stats[serv_stat_id];
        id_old_to_new.insert(make_pair(serv_stat_id, purchase_id));
        serv_stats[purchase_id].id = purchase_id;
    }

    for (auto assign_iter = assign_scheme.begin(); assign_iter != assign_scheme.end(); ++assign_iter)
    {
        auto iter = id_old_to_new.find(assign_iter->second.server_id);
        if (iter != id_old_to_new.end())
            assign_iter->second.server_id = id_old_to_new[assign_iter->second.server_id];
    }

    for (int i = 0; i < mig_scheme.size(); ++i)
    {
        auto iter = id_old_to_new.find(mig_scheme[i].serv_id);
        if (iter != id_old_to_new.end())
            mig_scheme[i].serv_id = id_old_to_new[mig_scheme[i].serv_id];
    }

    // 对散列表进行遍历，删除没有值的键，映射存储的服务器编号
    for (auto uiter = ream.begin(); uiter != ream.end(); ++uiter)
    {
        for (int i = 0; i < uiter->second.size(); ++i)
        {
            if (id_old_to_new.find(uiter->second[i].first) != id_old_to_new.end())
            {
                uiter->second[i].first = id_old_to_new[uiter->second[i].first];
            }
        }
    }

    for (auto diter = dream.begin(); diter != dream.end(); ++diter)
    {
        for (int i = 0; i < diter->second.size(); ++i)
        {
            if (id_old_to_new.find(diter->second[i]) != id_old_to_new.end())
            {
                diter->second[i] = id_old_to_new[diter->second[i]];
            }
        }
    }
}

vector<ServerTypeNum> purchase_scheme_to_serv_type_num(PurchaseScheme &purchase_scheme)
{
    vector<ServerTypeNum> serv_type_nums;
    if (purchase_scheme.size() == 0)
        return serv_type_nums;

    serv_type_nums.push_back(ServerTypeNum(purchase_scheme[0].type, 1));
    for (int i = 1; i < purchase_scheme.size(); ++i)
    {
        if (purchase_scheme[i].type == serv_type_nums.back().server_type)
            serv_type_nums.back().num++;
        else
            serv_type_nums.push_back(ServerTypeNum(purchase_scheme[i].type, 1));
    }

    return serv_type_nums;
}

