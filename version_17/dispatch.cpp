#include "dispatch.h"
#include "types.h"
#include "status.h"
#include "migrate.h"
#include "select.h"
#include "dump.h"
#include <iostream>
#include "args.h"
#include <ctime>
#include <cmath>
#include <cfloat>


void process_add_op_span(int idx_req, int idx_day, int num_days, int span, int vm_id, int vm_type, 
    PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, DayReqList &reqs)
{

    static int serv_should_buy;
    static long long num_cores = 0, num_mems = 0;
    static double core_mem_ratio;
    static int current_day = -1;
    static double res_core_mem_ratio;
    static long long serv_core_res = 0, serv_mem_res = 0;
    static double weighted_result;

    while (true)
    {
        OneAssignScheme oas = add_vm_select_serv_bf_hash(vm_id, vm_type);
        
        // 没有一个现存的服务器可以放得下，购买一个新服务器
        if (oas.server_id == -1)
        {
            multimap<float, int> ratio_to_spec, res_to_spec;
            
            int vm_node = vm_specs[vm_type].nodes;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;

            if (idx_day != current_day)
            {
                num_cores = 0;
                num_mems = 0;
                core_mem_ratio = 0;

                for (int i = 0; i < reqs[idx_day]->op.size(); ++i)
                {
                    if (reqs[idx_day]->op[i] == 1)
                    {
                        num_cores += vm_specs[reqs[idx_day]->vm_type[i]].cores;
                        num_mems += vm_specs[reqs[idx_day]->vm_type[i]].memcap;
                    }
                }

                core_mem_ratio = (double) (num_cores - num_mems) / (num_cores + num_mems);
                core_mem_ratio *= 12;

                if (core_mem_ratio < 0)
                    core_mem_ratio = max(-1.3, core_mem_ratio);
                else
                    core_mem_ratio = min(2.0, core_mem_ratio);

                serv_core_res = 0, serv_mem_res = 0;
                for (auto &serv: serv_stats.servs)
                {
                    if (!serv.second.is_full_load() && !serv.second.is_empty())
                    {
                        serv_core_res += serv.second.nodes[0].cores_ream + serv.second.nodes[1].cores_ream;
                        serv_mem_res += serv.second.nodes[0].mem_ream + serv.second.nodes[1].mem_ream;
                    }
                }
                if (serv_core_res + serv_mem_res != 0)
                    res_core_mem_ratio = (double) (serv_core_res - serv_mem_res) / (serv_core_res + serv_mem_res);
                else
                    res_core_mem_ratio = 0.0;

                weighted_result = (core_mem_ratio * (double) (num_cores + num_mems) / (serv_core_res + serv_mem_res + num_cores + num_mems)
                     - res_core_mem_ratio * (double) (serv_core_res + serv_mem_res) / (serv_core_res + serv_mem_res + num_cores + num_mems)); 

                if (DEBUG)
                {
                    cout << "idx_day: " << idx_day << ", add core mem ratio: " << core_mem_ratio
                        << ", add res total: " << (num_cores + num_mems)
                        << ", res core mem ratio: " << res_core_mem_ratio
                        << ", mig res total: " << (serv_core_res + serv_mem_res)
                        << ", weighted result: " << weighted_result << endl;
                }
                current_day = idx_day;
            }

            // if (num_serv_mem_res_global != 0)
            //     res_core_mem_ratio = (double) num_serv_core_res_global / num_serv_mem_res_global;
            // else
            //     res_core_mem_ratio = 0;

            double min_price = DBL_MAX;
            int min_idx = -1;
            double price = 0;
            for (int i = 0; i < serv_specs.size(); ++i)
            {
                if (serv_specs[i].can_hold_vm(vm_type, vm_specs))
                {
                    // price = ((2.34 + weighted_result) / serv_specs[i].cores + 1.0 / serv_specs[i].memcap) * (serv_specs[i].cost + serv_specs[i].consume * (num_days - idx_day));

                    price = ((2.3 + core_mem_ratio - res_core_mem_ratio) / serv_specs[i].cores + 1.0 / serv_specs[i].memcap) * (serv_specs[i].cost + serv_specs[i].consume * (num_days - idx_day));
                    
                    // price = ((2.3 + (core_mem_ratio * (double) (num_cores + num_mems) / (serv_core_res + serv_mem_res + num_cores + num_mems)
                    //  - res_core_mem_ratio * (double) (serv_core_res + serv_mem_res) / (serv_core_res + serv_mem_res + num_cores + num_mems)))
                    //  / serv_specs[i].cores + 1.0 / serv_specs[i].memcap) * (serv_specs[i].cost + serv_specs[i].consume * (num_days - idx_day));
                    if (price < min_price)
                    {
                        min_price = price;
                        min_idx = i;
                    }
                }
            }

            serv_should_buy = min_idx;
          
            // serv_should_buy = 4;
            purchase_scheme.push_back(OnePurchaseScheme(serv_should_buy, serv_stats.servs.size()));
            serv_stats.add_serv(serv_stats.servs.size(), serv_should_buy);
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;
        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node);
        break;
    }
}


void dispatch_span(int idx_day, int num_days, int span, DayReqList &reqs, DayDispList &disps)
{
    time_t tic_day, toc_day, tic_mig, toc_mig, tic_add, toc_add, time_add, tic_map, toc_map;
    static time_t copy_serv_stat_time = 0, copy_serv_stat_tic, copy_serv_stat_toc;

    tic_day = clock();
    time_add = 0;

    PurchaseScheme purchase_scheme;
    AssignScheme assign_scheme;
    MigScheme mig_scheme;

    // process reqs
    int idx_req = 0;
    int op, vm_id, vm_type;

    int num_servs = serv_stats.servs.size();

    tic_mig = clock();
    migrate(mig_scheme, reqs, idx_day);
    toc_mig = clock();


    if (DEBUG)
        dump_serv_stats(serv_stats, serv_specs, string("./after_mig/day_") + to_string(idx_day) + string(".txt"));

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
            serv_stat.del_vm(vm_id);
            ++idx_req;
            continue;
        }

        // 处理添加操作
        tic_add = clock();
        OneAssignScheme op_assign_scheme;
        if (op == 1)
        {
            process_add_op_span(idx_req, idx_day, num_days, span, vm_id, vm_type, purchase_scheme, op_assign_scheme, reqs);
            assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
            ++idx_req;
        }
        toc_add = clock();
        time_add += toc_add - tic_add;

    }

    sort_and_fill_purchase_id(serv_stats.servs.size() - purchase_scheme.size(), purchase_scheme);
    tic_map = clock();
    map_serv_id(purchase_scheme, assign_scheme, mig_scheme, serv_stats);
    toc_map = clock();

    if (DEBUG)
        dump_serv_stats(serv_stats, serv_specs, string("./after_add/day_") + to_string(idx_day) + string(".txt"));

    // 做维护的 map 的状态检测
    if (DEBUG)
        check_map_correct();

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
        int vm_id = mig_scheme[i].vm_id;
        disps.push_mig_vm(idx_day, mig_scheme[i].vm_id, mig_scheme[i].serv_id, mig_scheme[i].serv_node);
    }

    // 将调度方案写入到输出中
    DayReq &day_req = *(reqs[idx_day]);
    for (int i = 0; i < day_req.op.size(); ++i)
    {
        if (reqs[idx_day]->op[i] == 1)
        {
            int vm_id = reqs[idx_day]->id[i];
            disps.push_add_vm(idx_day, assign_scheme[vm_id].server_id, assign_scheme[vm_id].server_node);   
        }
    }

    toc_day = clock();

    if (DEBUG)
    {
        cout << "day " << idx_day
            << ", day time: " << toc_day - tic_day
            << ", num add: " << disps.disp_list[idx_day]->req_server_ids.size()
            << ", time add: " << time_add
            << ", num mig: " << disps.disp_list[idx_day]->mig_vm_ids.size()
            << ", time mig: " << toc_mig - tic_mig 
            << ", time map: " << toc_map - tic_map
            << ", serv buy: " << disps.disp_list[idx_day]->pur_server_name_ids.size()
            << ", serv total: " << serv_stats.servs.size() << endl;
    }

}


void check_map_correct()
{
    // for (auto coreit = node_core_mem_map.begin(); coreit != node_core_mem_map.end(); ++coreit)
    // {
    //     for (auto memit = coreit->second.begin(); memit != coreit->second.end(); ++memit)
    //     {
    //         for (auto &node: memit->second)
    //         {
    //             if (serv_stats[node.first].nodes[node.second].cores_ream != coreit->first ||
    //                 serv_stats[node.first].nodes[node.second].mem_ream != memit->first)
    //                 cout << "error in state map" << endl;
    //         }
    //     }
    // }

    for (auto &servit: serv_stats.servs)
    {
        ServStat &serv = servit.second;
        for (int i = 0; i < 2; ++i)
        {
            if (serv_stats.node_core_mem_map[serv.nodes[i].cores_ream][serv.nodes[i].mem_ream].find(make_pair(serv.id, i)) ==
                serv_stats.node_core_mem_map[serv.nodes[i].cores_ream][serv.nodes[i].mem_ream].end())
                cout << "error in state map" << endl;

            if (serv_stats.serv_core_mem_map[min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream)][min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream)].find(serv.id) ==
                serv_stats.serv_core_mem_map[min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream)][min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream)].end())
                cout << "error in serv core mem map" << endl;

            if (!serv.vms.empty())
            {
                if (serv_stats.nnode_core_mem_map[serv.nodes[i].cores_ream][serv.nodes[i].mem_ream].find(make_pair(serv.id, i)) == 
                serv_stats.nnode_core_mem_map[serv.nodes[i].cores_ream][serv.nodes[i].mem_ream].end())
                    cout << "error in nstate map" << endl;
            }


            if (serv_stats.nserv_core_mem_map[min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream)][min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream)].find(serv.id) ==
                serv_stats.nserv_core_mem_map[min(serv.nodes[0].cores_ream, serv.nodes[1].cores_ream)][min(serv.nodes[0].mem_ream, serv.nodes[1].mem_ream)].end())
            {
                if (!serv.vms.empty())
                    cout << "error in serv core mem map" << endl;
            }
                
        }
    }

    for (auto &num_vm_serv_id: non_full_load_servs)
    {
        if (serv_stats[num_vm_serv_id.second].vms.size() != num_vm_serv_id.first)
            cout << "error in state map" << endl;
    }

    long long serv_cores = 0, serv_mems = 0;
    for (auto &serv: serv_stats.servs)
    {
        if (!serv.second.is_full_load() && !serv.second.is_empty() && non_full_load_servs.find(make_pair(serv.second.vms.size(), serv.second.id)) == non_full_load_servs.end())
        {
            cout << "error in state map" << endl;
            serv_cores += serv.second.nodes[0].cores_ream + serv.second.nodes[1].cores_ream;
            serv_mems += serv.second.nodes[0].mem_ream + serv.second.nodes[1].mem_ream;
        }    

        for (auto &vm: serv.second.vms)
        {
            int vm_type = vm.second.type;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;
            int vm_node = vm_specs[vm_type].nodes;


            if (!serv.second.is_full_load() && serv_stats.vm_stat[vm_core][vm_mem][vm_node].find(vm.first) == serv_stats.vm_stat[vm_core][vm_mem][vm_node].end())
            {
                auto vm_core_iter = serv_stats.vm_stat.find(vm_core);
                if (vm_core_iter != serv_stats.vm_stat.end())
                {
                    auto vm_mem_iter = vm_core_iter->second.find(vm_mem);
                    if (vm_mem_iter != vm_core_iter->second.end())
                    {
                        auto vm_node_iter = vm_mem_iter->second.find(vm_node);
                        if (vm_node_iter != vm_mem_iter->second.end())
                        {
                            if (vm_node_iter->second.find(vm.first) != vm_node_iter->second.end())
                            {
                                cout << "error in state map" << endl;
                            }
                        }
                    }
                }
            }    
        }

        
    }

    // if (serv_cores != num_serv_core_res_global)
    // {
    //     cout << "error" << endl;
    // }

    // if (serv_mems != num_serv_mem_res_global)
    // {
    //     cout << "error" << endl;
    // }
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


    for (auto iter = id_old_to_new.begin(); iter != id_old_to_new.end(); ++iter)
    {
        for (auto &vm: serv_stats[iter->second].vms)
        {
            vm_id_to_serv_id[vm.first] = iter->second;
        }
    }

    unordered_map<int, bool> id_to_found;
    for (auto &oid_nid: id_old_to_new)
    {
        int new_id = oid_nid.second;
        int old_id = oid_nid.first;
        ServStat &serv = serv_stats[oid_nid.second];
        auto iter = non_full_load_servs.find(make_pair(serv_stats[oid_nid.second].vms.size(), oid_nid.first));
        if (iter != non_full_load_servs.end())
        {
            non_full_load_servs.erase(iter);
            id_to_found.emplace(oid_nid.second, true);
        }
        else
        {
            id_to_found.emplace(oid_nid.second, false);
        }

        for (int i = 0; i < 2; ++i)
        {
            int core = serv_stats[oid_nid.second].nodes[i].cores_ream;
            int mem = serv_stats[oid_nid.second].nodes[i].mem_ream;
            serv_stats.node_core_mem_map[core][mem].erase(make_pair(oid_nid.first, i));
            if (!serv_stats[oid_nid.second].vms.empty())
            {
                serv_stats.nnode_core_mem_map[core][mem].erase(make_pair(oid_nid.first, i));
            }
        }

        int min_core = min(serv_stats[oid_nid.second].nodes[0].cores_ream, serv_stats[oid_nid.second].nodes[1].cores_ream);
        int min_mem = min(serv_stats[oid_nid.second].nodes[0].mem_ream, serv_stats[oid_nid.second].nodes[1].mem_ream);
        serv_stats.serv_core_mem_map[min_core][min_mem].erase(oid_nid.first);
        if (!serv_stats[oid_nid.second].vms.empty())
        {
            serv_stats.nserv_core_mem_map[min_core][min_mem].erase(oid_nid.first);
        }
    }

    for (auto &oid_nid: id_old_to_new)
    {
        if (id_to_found[oid_nid.second])
        {
            non_full_load_servs.emplace(serv_stats[oid_nid.second].vms.size(), oid_nid.second);
        }

        for (int i = 0; i < 2; ++i)
        {
            int core = serv_stats[oid_nid.second].nodes[i].cores_ream;
            int mem = serv_stats[oid_nid.second].nodes[i].mem_ream;
            serv_stats.node_core_mem_map[core][mem].emplace(oid_nid.second, i);
            if (!serv_stats[oid_nid.second].vms.empty())
            {
                serv_stats.nnode_core_mem_map[core][mem].emplace(oid_nid.second, i);
            }
        }

        int min_core = min(serv_stats[oid_nid.second].nodes[0].cores_ream, serv_stats[oid_nid.second].nodes[1].cores_ream);
        int min_mem = min(serv_stats[oid_nid.second].nodes[0].mem_ream, serv_stats[oid_nid.second].nodes[1].mem_ream);
        serv_stats.serv_core_mem_map[min_core][min_mem].emplace(oid_nid.second);
        if (!serv_stats[oid_nid.second].vms.empty())
        {
            serv_stats.nserv_core_mem_map[min_core][min_mem].emplace(oid_nid.second);
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


