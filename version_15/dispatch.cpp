#include "dispatch.h"
#include "types.h"
#include "status.h"
#include "migrate.h"
#include "select.h"
#include "dump.h"
#include <iostream>
#include <unordered_map>
#include <set>
#include "args.h"
#include <ctime>
#include <cmath>
#include <cfloat>
#include <queue>

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
        }
    }

    for (auto &num_vm_serv_id: non_full_load_servs)
    {
        if (serv_stats[num_vm_serv_id.second].vms.size() != num_vm_serv_id.first)
            cout << "error in state map" << endl;
    }

    for (auto &serv: serv_stats.servs)
    {
        if (!serv.second.is_full_load() && !serv.second.is_empty() && non_full_load_servs.find(make_pair(serv.second.vms.size(), serv.second.id)) == non_full_load_servs.end())
            cout << "error in state map" << endl;

        for (auto &vm: serv.second.vms)
        {
            int vm_type = vm.second.type;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;
            int vm_node = vm_specs[vm_type].nodes;

            if (serv_stats.vm_stat[vm_core][vm_mem][vm_node].find(vm.first) == serv_stats.vm_stat[vm_core][vm_mem][vm_node].end())
                cout << "error in state map" << endl;
        }
    }

}

int process_add_ops(int idx_req, int idx_day, int num_days, int span,
    PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, DayReqList &reqs)
{
    map<int, AddReq> add_reqs;
    select_add_reqs(add_reqs, idx_day, idx_req, reqs, reqs[idx_day]->id.size());
    // ServStatList temp_servs;
    map<int, pair<int, int>> unprocessed_vms;  // idx_req -> (vm_id, vm_type)
    for (auto &add_req: add_reqs)
    {
        // 首先找现有服务器能不能放下，若能放下，则直接放
        int vm_id = reqs[idx_day]->id[idx_req];
        int vm_type = reqs[idx_day]->vm_type[idx_req];
        OneAssignScheme oas = add_vm_select_serv_bf_hash(vm_id, vm_type, serv_stats, vm_specs);
        if (oas.server_id != -1)
        {
            assign_scheme.emplace(vm_id, oas);
            serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node);
            ++idx_req;
            continue;
        }
        // 若放不下，则放到待处理的虚拟机列表里
        unprocessed_vms.emplace(idx_req, make_pair(vm_id, vm_type));
        ++idx_req;
    }

    while (!unprocessed_vms.empty())
    {
        // 首先找现有服务器能不能放下，若能放下，则直接放
        int vm_id = unprocessed_vms.begin()->second.first;
        int vm_type = unprocessed_vms.begin()->second.second;
        OneAssignScheme oas = add_vm_select_serv_bf_hash(vm_id, vm_type, serv_stats, vm_specs);
        if (oas.server_id != -1)
        {
            assign_scheme.emplace(vm_id, oas);
            serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node);
            unprocessed_vms.erase(unprocessed_vms.begin());
            continue;
        }

        multimap<float, pair<int, vector<tuple<int, int, int>>>> res_to_spec;  // res -> serv_spec, (vm_id, vm_type, serv_node)
        for (int i = 0; i < serv_specs.size(); ++i)
        {
            VirtualServ vs(i);
            vector<tuple<int, int, int>> vm_comb;  // vm_id, vm_type, node
            // 尽量添加虚拟机，直到无法添加下为止
            for (auto &vm: unprocessed_vms)
            {
                int vm_id = vm.second.first;
                int vm_type = vm.second.second;
                int vm_node = vm_specs[vm_type].nodes;
                int vm_core = vm_specs[vm_type].cores;
                int vm_mem = vm_specs[vm_type].memcap;

                OneAssignScheme oas = vs.add_vm(vm_id, vm_type);
                if (oas.server_node == -1)
                    break;
                vm_comb.emplace_back(oas.vm_id, vm_type, oas.server_node);
            }

            // 将余量写入到表里
            if (!vm_comb.empty())
            {
                // res_to_spec.emplace((float) vs.get_res() / (serv_specs[i].cores + serv_specs[i].memcap), make_pair(i, vm_comb));
                res_to_spec.emplace((float) vs.get_res() / vs.vms.size(), make_pair(i, vm_comb));
            }
        }

        // 挑选出余量最小的服务器，写入购买方案，添加方案，以及实际的服务器状态，清除掉这些虚拟机
        int rnd = min(rand() % 3, int(res_to_spec.size()) - 1);
        auto res_iter = res_to_spec.begin();
        advance(res_iter, rnd);
        int serv_should_buy = res_iter->second.first;
        int serv_id = serv_stats.servs.size();

        purchase_scheme.push_back(OnePurchaseScheme(serv_should_buy, serv_id));
        serv_stats.add_serv(serv_id, serv_should_buy);

        vector<tuple<int, int, int>> vm_comb = res_iter->second.second;
        ServStat &serv = serv_stats[serv_id];
        for (int i = 0; i < vm_comb.size(); ++i)
        {
            int vm_id = get<0>(vm_comb[i]);
            int vm_type = get<1>(vm_comb[i]);
            int serv_node = get<2>(vm_comb[i]);

            assign_scheme[vm_id].vm_id = vm_id;
            assign_scheme[vm_id].server_node = serv_node;
            assign_scheme[vm_id].server_id = serv_id;

            serv.add_vm(vm_type, vm_id, serv_node);
        }

        unprocessed_vms.erase(unprocessed_vms.begin(), next(unprocessed_vms.begin(), vm_comb.size()));
    }

    return add_reqs.size();
}



void dispatch_span(int idx_day, int num_days, int span, DayReqList &reqs, DayDispList &disps)
{
    time_t tic_day, toc_day, tic_mig, toc_mig, tic_add, toc_add, time_add, tic_map, toc_map;
    static time_t copy_serv_stat_time = 0, copy_serv_stat_tic, copy_serv_stat_toc;

    // if (DEBUG)
    // {
    //     copy_serv_stat_tic = clock();
    //     auto serv_stat_copy(serv_stats);
    //     copy_serv_stat_toc = clock();
    //     cout << "copy serv stat time: " << copy_serv_stat_toc - copy_serv_stat_tic << endl;
    // }
    

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
            idx_req += process_add_ops(idx_req, idx_day, num_days, span, purchase_scheme, assign_scheme, reqs);
            // process_add_op_span(idx_req, idx_day, num_days, span, vm_id, vm_type, purchase_scheme, op_assign_scheme, serv_stats, serv_specs, vm_specs, reqs);
            // assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
            // cout << "vm id: " << vm_id << ", server id: " << op_assign_scheme.server_id << ", serv node: " << op_assign_scheme.server_node << endl;
            // ++idx_req;
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

 
        // vector<int> num_servs(serv_specs.size(), 0);
        // vector<int> full_load_nums(serv_specs.size(), 0);
        // for (auto &serv: serv_stats.servs)
        // {
        //     num_servs[serv.second.type] += 1;
        //     if (serv.second.is_full_load())
        //         full_load_nums[serv.second.type] += 1;
        // }

        // for (int i = 0; i < serv_specs.size(); ++i)
        // {
        //     if (num_servs[i] != 0)
        //     {
        //         cout << "serv " << i << " " << serv_specs[i].cores << " " << serv_specs[i].memcap << ": "
        //             << (float)full_load_nums[i] / num_servs[i] << ", ";
        //     }
        // }
        cout << endl;
    }

}

int select_add_reqs(map<int, AddReq> &add_reqs, int idx_day, int idx_req, DayReqList &reqs, int num_max_add_reqs)
{
    // map<int, AddReq> add_reqs;
    int vm_id;
    int num_selected = 0;
    for (int i = idx_req; i < reqs[idx_day]->op.size(); ++i)
    {
        vm_id = reqs[idx_day]->id[i];
        if (reqs[idx_day]->op[i] == 0)
            break;
        add_reqs.insert(make_pair(i, AddReq(reqs[idx_day]->vm_type[i], reqs[idx_day]->id[i])));
        ++num_selected;
        if (num_selected >= num_max_add_reqs)
            break;
    }
    return num_selected;    
}

float calc_vm_core_mem_ratio_median(DayReqList &reqs, int &day_start, int &k, int &num_days, VMSpecList &vm_specs)
{
    int day_end = min(day_start + k, num_days);
    multiset<float> vm_core_mem_ratios;
    int vm_type;
    for (int idx_day = day_start; idx_day != day_end; ++idx_day)
    {
        for (int idx_req = 0; idx_req < reqs[idx_day]->op.size(); ++idx_req)
        {
            if (reqs[idx_day]->op[idx_req] == 0)
                continue;
            vm_type = reqs[idx_day]->vm_type[idx_req];
            vm_core_mem_ratios.emplace((float)vm_specs[vm_type].cores / (float)vm_specs[vm_type].memcap);
        }
    }
    int size = vm_core_mem_ratios.size();
    return *next(vm_core_mem_ratios.begin(), vm_core_mem_ratios.size() / 2);
}




// void process_add_ops(map<int, AddReq> &add_reqs, PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, 
//     ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
// {
//     // 先尽可能分配请求操作，然后从 add_reqs 中删除已经处理的请求，直到确定剩下的请求都需要买服务器
//     // 目前先使用 best fit 分配
//     map<int, AddReq> add_reqs_need_purchase;
//     int vm_id;
//     for (auto req_iter = add_reqs.begin(); req_iter != add_reqs.end(); ++req_iter)
//     {
//         vm_id = req_iter->second.vm_id;
//         OneAssignScheme one_assign_scheme = add_vm_select_serv_bf_hash(req_iter->second.vm_id, req_iter->second.vm_type, serv_stats, vm_specs);
//         if (one_assign_scheme.server_id == -1)
//         {
//             add_reqs_need_purchase.emplace(req_iter->first, AddReq(req_iter->second.vm_type, req_iter->second.vm_id));
//             continue;
//         }
//         assign_scheme.emplace(one_assign_scheme.vm_id, OneAssignScheme(one_assign_scheme.vm_id, 
//             one_assign_scheme.server_id, one_assign_scheme.server_node));
//         serv_stats[one_assign_scheme.server_id].add_vm(req_iter->second.vm_type, one_assign_scheme.vm_id, one_assign_scheme.server_node);
//     }
    
//     // 目前测试，用单节点虚拟机把服务器的一个单节点填满
//     // 目前测试，混合装填
//     // 目前测试，多服务器混合装填
//     // 目前测试，写出购买和放置方案
//     // 服务器选型，分配
//     add_reqs = add_reqs_need_purchase;
//     purchase_server(add_reqs, purchase_scheme, assign_scheme, serv_stats);
// }


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


    for (auto &oid_nid: id_old_to_new)
    {
        auto iter = non_full_load_servs.find(make_pair(serv_stats[oid_nid.second].vms.size(), oid_nid.first));
        if (iter != non_full_load_servs.end())
            non_full_load_servs.erase(iter);

        for (int i = 0; i < 2; ++i)
        {
            int core = serv_stats[oid_nid.second].nodes[i].cores_ream;
            int mem = serv_stats[oid_nid.second].nodes[i].mem_ream;
            serv_stats.node_core_mem_map[core][mem].erase(make_pair(oid_nid.first, i));
        }

        int min_core = min(serv_stats[oid_nid.second].nodes[0].cores_ream, serv_stats[oid_nid.second].nodes[1].cores_ream);
        int min_mem = min(serv_stats[oid_nid.second].nodes[0].mem_ream, serv_stats[oid_nid.second].nodes[1].mem_ream);
        serv_stats.serv_core_mem_map[min_core][min_mem].erase(oid_nid.first);
    }

    for (auto &oid_nid: id_old_to_new)
    {
        non_full_load_servs.emplace(serv_stats[oid_nid.second].vms.size(), oid_nid.second);

        for (int i = 0; i < 2; ++i)
        {
            int core = serv_stats[oid_nid.second].nodes[i].cores_ream;
            int mem = serv_stats[oid_nid.second].nodes[i].mem_ream;
            serv_stats.node_core_mem_map[core][mem].emplace(oid_nid.second, i);
        }

        int min_core = min(serv_stats[oid_nid.second].nodes[0].cores_ream, serv_stats[oid_nid.second].nodes[1].cores_ream);
        int min_mem = min(serv_stats[oid_nid.second].nodes[0].mem_ream, serv_stats[oid_nid.second].nodes[1].mem_ream);
        serv_stats.serv_core_mem_map[min_core][min_mem].emplace(oid_nid.second);
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

void init_purchase_scheme(vector<float> &size_scrs, vector<float> &serv_ratios)
{
    int N = serv_specs.size();

    vector<float> serv_cores, serv_mems, serv_core_mems;
    float median_serv_cores, median_serv_mems, median_serv_core_mems;
    float sum_serv_cores = 0, sum_serv_mems = 0, sum_serv_core_mems = 0;
    // float mid_serv_cores, mid_serv_mems, mid_serv_core_mems;
    for (auto &serv_spec: serv_specs)
    {
        serv_cores.emplace_back(serv_spec.cores);
        serv_mems.emplace_back(serv_spec.memcap);
        serv_core_mems.emplace_back(serv_spec.cores + serv_spec.memcap);

        sum_serv_cores += serv_spec.cores;
        sum_serv_mems += serv_spec.memcap;
        sum_serv_core_mems += serv_spec.cores + serv_spec.memcap;
    }
    sort(serv_cores.begin(), serv_cores.end());
    sort(serv_mems.begin(), serv_mems.end());
    sort(serv_core_mems.begin(), serv_core_mems.end());
    median_serv_cores = N % 2 == 0 ? (serv_cores[N / 2] + serv_cores[N / 2 - 1]) / 2 : serv_cores[N / 2];
    median_serv_mems = N % 2 == 0 ? (serv_mems[N / 2] + serv_mems[N / 2 - 1]) / 2 : serv_cores[N / 2];
    median_serv_core_mems = N % 2 == 0? (serv_core_mems[N / 2] + serv_core_mems[N / 2 - 1]) / 2 : serv_core_mems[N / 2];

    size_scrs.resize(N);
    for (int i = 0; i < N; ++i)
    {
        size_scrs[i] = sqrt(pow(serv_specs[i].cores - median_serv_cores, 2) + 
                       pow(serv_specs[i].memcap - median_serv_mems, 2));
    }

    float min_scr = FLT_MAX;
    float max_scr = -FLT_MAX;
    for (int i = 0; i < N; ++i)
    {
        if (size_scrs[i] < min_scr)
            min_scr = size_scrs[i];
        if (size_scrs[i] > max_scr)
            max_scr = size_scrs[i];
    }

    float span = max_scr - min_scr;
    for (int i = 0; i < N; ++i)
    {
        size_scrs[i] = (size_scrs[i] - min_scr) / span;
    }

    serv_ratios.resize(N);
    for (int i = 0; i < N; ++i)
    {
        serv_ratios[i] = float(serv_specs[i].cores - serv_specs[i].memcap) / (serv_specs[i].cores + serv_specs[i].memcap);
    }
}


void process_add_op_span(int idx_req, int idx_day, int num_days, int span, int vm_id, int vm_type, 
    PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, 
    ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs)
{
    static vector<float> size_scrs, serv_ratios;
    static vector<int> size_order;
    static bool inited_purch = false;
    if (!inited_purch)
    {
        init_purchase_scheme(size_scrs, serv_ratios);
        inited_purch = true;
    }

    static int serv_should_buy;
    
    // static int idx_day_last = 0;
    // 对一个大于 span 的数取余
    // if (idx_day % span == 0 && idx_req == 0)
    // {
    //     time_t tic = clock();
    //     core_mem_ratio_mid = calc_vm_core_mem_ratio_median(reqs, idx_day, span, num_days, vm_specs);
    //     vector<pair<float, int>> res;
    //     vector<int> res_order;
    //     for (int i = 0; i < serv_specs.size(); ++i)
    //     {
    //         res.emplace_back(abs((float)serv_specs[i].cores / serv_specs[i].memcap - core_mem_ratio_mid), i);
    //     }
    //     sort(res.begin(), res.end());
        
    //     res_order.resize(serv_specs.size());
    //     for (int i = 0; i < serv_specs.size(); ++i)
    //     {
    //         res_order[i] = res[i].second;  // serv_spec -> order
    //     }

    //     vector<int> scores;
    //     scores.resize(serv_specs.size());
    //     for (int i = 0; i < res.size(); ++i)
    //     {
    //         scores[i] = res_order[i] + size_order[i];
    //     }

    //     // sort(scores.begin(), scores.end());

    //     serv_should_buy = scores[0];
    //     time_t toc = clock();
    //     if (DEBUG)
    //         cout << "select serv type time: " << toc - tic << endl;
    // }

    while (true)
    {
        OneAssignScheme oas;
        // oas = add_vm_select_server_bf(vm_id, vm_type, serv_stats, serv_specs, vm_specs);
        oas = add_vm_select_serv_bf_hash(vm_id, vm_type, serv_stats, vm_specs);
        
        // 没有一个现存的服务器可以放得下，购买一个新服务器
        // 先选装完虚拟机后，剩余的节点都是平衡的状态的型号，然后从这些型号中挑出适中的，或适中偏大/偏小的（调参）
        if (oas.server_id == -1)
        {
            multimap<float, int> ratio_to_spec, res_to_spec;
            
            int vm_node = vm_specs[vm_type].nodes;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;

            vector<pair<float, int>> serv_scrs(serv_specs.size());
            float vm_ratio = float(vm_core - vm_mem) / (vm_core + vm_mem);
            float ratio_scr;
            for (int i = 0; i < serv_specs.size(); ++i)
            {
                ratio_scr = abs(serv_ratios[i] - vm_ratio);
                serv_scrs[i] = make_pair(ratio_scr + size_scrs[i], i);
            }

            sort(serv_scrs.begin(), serv_scrs.end());

            for (int i = 0; i < serv_specs.size(); ++i)
            {
                if (serv_specs[serv_scrs[i].second].can_hold_vm(vm_type, vm_specs))
                {
                    serv_should_buy = serv_scrs[i].second;
                    break;
                }
            }

            // int min_price = INT32_MAX;
            // int min_idx = -1;
            // // serv_should_buy = 41;
            // if (serv_specs[serv_should_buy].can_hold_vm(vm_type, vm_specs))
            // {
            //     min_idx = serv_should_buy;
            // }
            // else
            // {
            //     for (int i = 0; i < serv_specs.size(); ++i)
            //     {
            //         if (serv_specs[i].can_hold_vm(vm_type, vm_specs))
            //         {
            //             if (serv_specs[i].cost < min_price)
            //             {
            //                 min_price = serv_specs[i].cost;
            //                 min_idx = i;
            //             }
            //         }
            //     }
            // }

            // 根据接下来一些请求，选择最合适的型号
            int n = reqs[idx_day]->id.size();
            int m = min(n - idx_req, 16);
            vector<int> req_vm_types;
            // int m = 0;
            for (int i = idx_req; i != n; ++i)
            {
                if (reqs[idx_day]->op[idx_req] == 1)
                {
                    req_vm_types.push_back(reqs[idx_day]->vm_type[i]);
                    if (req_vm_types.size() >= m)
                        break;
                }
            }

            float cores = 0, mems = 0, cm_ratio;
            for (int i = 0; i < req_vm_types.size(); ++i)
            {
                cores += vm_specs[req_vm_types[i]].cores;
                mems += vm_specs[req_vm_types[i]].memcap;
            }

            cm_ratio = (cores - mems) / (cores + mems);

            vector<pair<float, int>> ratio_diff_idx;

            for (int i = 0; i < req_vm_types.size(); ++i)
            {
                ratio_diff_idx.emplace_back(abs(cm_ratio - serv_ratios[i]), i);
            }

            sort(ratio_diff_idx.begin(), ratio_diff_idx.end());

            for (int i = 0; i < req_vm_types.size(); ++i)
            {
                serv_should_buy = ratio_diff_idx[i].second;
                if (serv_specs[serv_should_buy].can_hold_vm(vm_type, vm_specs))
                    break;
            }
          
            serv_should_buy = 4;
            purchase_scheme.push_back(OnePurchaseScheme(serv_should_buy, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), min_idx, serv_specs)));
            serv_stats.add_serv(serv_stats.servs.size(), serv_should_buy);

            // int serv_spec_to_buy = next(res_to_spec.begin(), res_to_spec.size() / 2)->second;
            // purchase_scheme.push_back(OnePurchaseScheme(serv_spec_to_buy, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), serv_spec_to_buy, serv_specs)));
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;
        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node);
        break;
    }
}
