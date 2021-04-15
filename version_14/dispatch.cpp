#include "dispatch.h"
#include "types.h"
#include "status.h"
#include "migrate.h"
#include "select.h"
#include "dump.h"
#include <iostream>
#include <unordered_map>
#include <set>
#include "linopt.h"
#include "args.h"
#include <ctime>

void dispatch_span(int idx_day, int num_days, int span, ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps)
{
    time_t tic_day, toc_day, tic_mig, toc_mig, tic_add, toc_add, time_add;

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
    migrate(mig_scheme, serv_stats, serv_specs, vm_specs);
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
            serv_stat.del_vm(vm_id, vm_specs);
            ++idx_req;
            continue;
        }

        // 处理添加操作
        tic_add = clock();
        OneAssignScheme op_assign_scheme;
        if (op == 1)
        {
            process_add_op_span(idx_req, idx_day, num_days, span, vm_id, vm_type, purchase_scheme, op_assign_scheme, serv_stats, serv_specs, vm_specs, reqs);
            assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
            // cout << "vm id: " << vm_id << ", server id: " << op_assign_scheme.server_id << ", serv node: " << op_assign_scheme.server_node << endl;
            ++idx_req;
        }
        toc_add = clock();
        time_add += toc_add - tic_add;
    }

    sort_and_fill_purchase_id(serv_stats.servs.size() - purchase_scheme.size(), purchase_scheme);
    map_serv_id(purchase_scheme, assign_scheme, mig_scheme, serv_stats);

    // 做维护的 map 的状态检测
    if (DEBUG)
    {
        for (auto &serv: serv_stats.servs)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (serv_core_map_2[serv.second.nodes[i].cores_ream].find(make_pair(serv.second.nodes[i].mem_ream, make_pair(serv.second.id, i))) == 
                    serv_core_map_2[serv.second.nodes[i].cores_ream].end())
                {
                    cout << "error " << endl;
                }
            }
        }
    }

    if (DEBUG)
        dump_serv_stats(serv_stats, serv_specs, string("./after_add/day_") + to_string(idx_day) + string(".txt"));

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

    // 做维护的 map 的状态检测
    if (DEBUG)
    {
        for (auto &serv: serv_stats.servs)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (serv_core_map_2[serv.second.nodes[i].cores_ream].find(make_pair(serv.second.nodes[i].mem_ream, make_pair(serv.second.id, i))) == 
                    serv_core_map_2[serv.second.nodes[i].cores_ream].end())
                {
                    cout << "error " << endl;
                }
            }
        }
    }

    if (DEBUG)
    {
        cout << "day " << idx_day
            << ", day time: " << toc_day - tic_day
            << ", num add: " << disps.disp_list[idx_day]->req_server_ids.size()
            << ", time add: " << time_add
            << ", num mig: " << disps.disp_list[idx_day]->mig_vm_ids.size()
            << ", time mig: " << toc_mig - tic_mig 
            << ", serv buy: " << disps.disp_list[idx_day]->pur_server_name_ids.size()
            << ", serv total: " << serv_stats.servs.size() << endl;;
    }


        // disps.clear();
        // disps.new_day();
        // disps.push_pur_server(0, 0, 2);
        // disps.push_pur_server(0, 1, 3);
        // disps.push_add_vm(0, 0, 1);
        // disps.push_add_vm(0, 1, 0);
        // disps.push_mig_vm(0, 0, 1, 2);
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


void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    while (true)
    {
        OneAssignScheme oas;
        // oas = add_vm_select_server_bf(vm_id, vm_type, serv_stats, serv_specs, vm_specs);
        oas = add_vm_select_serv_bf_hash(vm_id, vm_type, serv_stats, vm_specs);
        
        // 没有一个现存的服务器可以放得下，购买一个新服务器
        // 先选装完虚拟机后，剩余的节点都是平衡的状态的型号，然后从这些型号中挑出适中的，或适中偏大/偏小的（调参）
        if (oas.server_id == -1)
        {
            map<int, int> ratio_to_spec, res_to_spec;
            int vm_node = vm_specs[vm_type].nodes;
            int vm_core = vm_specs[vm_type].cores;
            int vm_mem = vm_specs[vm_type].memcap;

            
            // if (vm_node == 1)
            // {
            //     for (int i = 0; i < serv_specs.size(); ++i)
            //     {
            //         if (serv_specs[i].memcap / 2 <= vm_mem || serv_specs[i].cores / 2 <= vm_core)
            //             continue;

            //         ratio_to_spec.emplace(abs((serv_specs[i].memcap / 2 - vm_mem) / float(serv_specs[i].cores / 2 - vm_core) - mem_per_core), i);
            //     }
            // }
            // else
            // {
            //     for (int i = 0; i < serv_specs.size(); ++i)
            //     {

            //         if (serv_specs[i].memcap <= vm_mem || serv_specs[i].cores <= vm_core)
            //             continue;
                
            //         ratio_to_spec.emplace(abs((serv_specs[i].memcap - vm_mem) / (serv_specs[i].cores - vm_core) - mem_per_core), i);   
            //     }
            // }

            // auto upper_bound = next(ratio_to_spec.upper_bound(ratio_to_spec.rbegin()->first / 2));
            // for (auto iter = ratio_to_spec.begin(); iter != upper_bound; ++iter)
            // {
            //     res_to_spec.emplace(serv_specs[iter->second].cores + serv_specs[iter->second].memcap, iter->second);
            // }

            // if (res_to_spec.empty())
            //     cout << "error in purchase server" << endl;

            int min_price = INT32_MAX;
            int min_idx = -1;
            if (serv_specs[38].can_hold_vm(vm_type, vm_specs))
                min_idx = 38;
            else
            {
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
            }

            
            purchase_scheme.push_back(OnePurchaseScheme(min_idx, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), min_idx, serv_specs)));
            serv_stats.add_serv(serv_stats.servs.size(), min_idx);

            // int serv_spec_to_buy = next(res_to_spec.begin(), res_to_spec.size() / 2)->second;
            // purchase_scheme.push_back(OnePurchaseScheme(serv_spec_to_buy, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), serv_spec_to_buy, serv_specs)));
            
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;

        if (DEBUG)
        {
            for (auto &serv: serv_stats.servs)
            {
                for (int i = 0; i < 2; ++i)
                {
                    if (serv_core_map_2[serv.second.nodes[i].cores_ream].find(make_pair(serv.second.nodes[i].cores_ream, make_pair(serv.second.id, i))) == 
                        serv_core_map_2[serv.second.nodes[i].cores_ream].end())
                    {
                        cout << "error " << endl;
                    }
                }
            }
        }

        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node, vm_specs);
        if (DEBUG)
        {
            for (auto &serv: serv_stats.servs)
            {
                for (int i = 0; i < 2; ++i)
                {
                    if (serv_core_map_2[serv.second.nodes[i].cores_ream].find(make_pair(serv.second.nodes[i].cores_ream, make_pair(serv.second.id, i))) == 
                        serv_core_map_2[serv.second.nodes[i].cores_ream].end())
                    {
                        cout << "error " << endl;
                    }
                }
            }
        }
        break;
    }
}

set<int> find_vm_comb(int core, int mem, map<int, int> &vm_id_type, VMSpecList &vm_specs, int &max_val)
{
    vector<vector<float>> A;
    vector<float> b;
    vector<float> c;
    A.push_back(vector<float>());
    A.push_back(vector<float>());
    b.push_back(core);
    b.push_back(mem);

    int i = 0;
    for (auto iter = vm_id_type.begin(); iter != vm_id_type.end(); ++iter)
    {
        A[0].push_back(vm_specs[iter->second].cores);
        A[1].push_back(vm_specs[iter->second].memcap);
        A.push_back(vector<float>(vm_id_type.size(), 0));
        A.back()[i] = 1;
        b.push_back(1);
        c.push_back(vm_specs[iter->second].cores + vm_specs[iter->second].memcap);
        ++i;
    }

    float temp_max_val;
    vector<int> x = branch_and_cut_max(A, b, c, temp_max_val, core + mem);
    max_val = int(floor(temp_max_val + 0.5));

    set<int> vm_comb;
    i = 0;
    for (auto iter = vm_id_type.begin(); iter != vm_id_type.end(); ++iter)
    {
        if (x[i] == 1)
            vm_comb.insert(iter->first);
        ++i;
    }
    return vm_comb;
}

void purchase_server(map<int, AddReq> &add_reqs, PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, 
    ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    // 处理购买服务器的需求
    // 先遍历需求中 m 个双节点的虚拟机，将这个双节点虚拟机对所有服务器型号进行遍历
    // 服务器装下这个双节点的虚拟机后，使用单节点虚拟机对这个服务器的两个节点分别优化，记录余量和分配方案
    // 若余量为 0，就可以中止了
    // 然后遍历需求中 m + 1 或 m - 1 个双节点的虚拟机，将这些双节点虚拟机对服务器型号进行遍历
    // 然后同上，使用单节点虚拟机，优化服务器两个节点，记录余量和分配方方案
    // 理想情况下，几个双节点虚拟机搭配几个单节点虚拟机可以把某种型号的服务器填满
    // 参数：起始双节点虚拟机的数量，双节点虚拟机数量的最大搜索深度，搜索方式是向上，向下，还是摇摆

    if (add_reqs.empty())
        return;

    map<int, int> dvms, dvms_copy;  // double nodes vm, (vm_id, type)
    map<int, int> svms, svms_copy;
    for (auto &add_req: add_reqs)
    {
        if (vm_specs[add_req.second.vm_type].nodes == 1)
            svms.insert(make_pair(add_req.second.vm_id, add_req.second.vm_type));
        else
            dvms.insert(make_pair(add_req.second.vm_id, add_req.second.vm_type));    
    }

    // if (dvms.empty() && svms.empty())
    //     return;
    
    multimap<int, int> res_to_serv_type;
    vector<map<int, OneAssignScheme>> assign_schemes;  // ((vm_id, one assign scheme))
    int max_val;
    dvms_copy = dvms;
    svms_copy = svms;
    int new_serv_id = serv_stats.servs.size();
    for (int i = 0; i < serv_specs.size(); ++i)
    {
        dvms = dvms_copy;
        svms = svms_copy;

        ServSpec &serv_spec = serv_specs[i];
        int serv_core = serv_spec.cores;
        int serv_mem = serv_spec.memcap;

        int serv_core_res = serv_core, serv_mem_res = serv_mem, serv_res = serv_core + serv_mem;
        int vm_type;
        assign_schemes.emplace_back(map<int, OneAssignScheme>());
        // 先使用双节点服务器进行填充
        if (!dvms.empty())
        {
            set<int> vm_comb = find_vm_comb(serv_core, serv_mem, dvms, vm_specs, max_val);
            for (auto &vm_id: vm_comb)
            {
                vm_type = dvms[vm_id];
                serv_core_res -= vm_specs[vm_type].cores;
                serv_mem_res -= vm_specs[vm_type].memcap;
                assign_schemes.back().emplace(vm_id, OneAssignScheme(vm_id, new_serv_id, 2));
                dvms.erase(vm_id);
            }
            serv_res -= max_val;
        }

        // 如果遇到将服务器填满的情况，那么提前中止
        if (serv_res == 0)
        {
            res_to_serv_type.emplace(serv_core_res + serv_mem_res, i);
            // cout << "serv type " << i << ": " << serv_res << ", core: " << serv_core_res << ", mem: " << serv_mem_res << endl;
            break;
        }

        // 对两个节点分别用单节点填充
        // map<int, int> svms_copy(svms);
        set<int> vm_comb_0, vm_comb_1;
        if (!svms.empty())
        {
            vm_comb_0 = find_vm_comb(int(serv_core_res / 2), int(serv_mem_res / 2), svms, vm_specs, max_val);
            for (auto &vm_id: vm_comb_0)
                svms.erase(vm_id);
            serv_res -= max_val;
        }

        if (!svms.empty())
        {
            vm_comb_1 = find_vm_comb(int(serv_core_res / 2), int(serv_mem_res / 2), svms, vm_specs, max_val);
            for (auto &vm_id: vm_comb_1)
                svms.erase(vm_id);
            serv_res -= max_val;
        }

        for (auto &vm_id: vm_comb_0)
        {
            vm_type = svms_copy[vm_id];
            serv_core_res -= vm_specs[vm_type].cores;
            serv_mem_res -= vm_specs[vm_type].memcap;
            assign_schemes.back().emplace(vm_id, OneAssignScheme(vm_id, new_serv_id, 0));
        }
        for (auto &vm_id: vm_comb_1)
        {
            vm_type = svms_copy[vm_id];
            serv_core_res -= vm_specs[vm_type].cores;
            serv_mem_res -= vm_specs[vm_type].memcap;
            assign_schemes.back().emplace(vm_id, OneAssignScheme(vm_id, new_serv_id, 1));
        }

        if (assign_schemes.back().empty())
        {
            assign_schemes.back().emplace(-1, OneAssignScheme(-1, -1, -1));
            res_to_serv_type.emplace(INT32_MAX, i);
        }
        else
        {
            res_to_serv_type.emplace(serv_res, i);
        }

        // cout << "serv type " << i << ": " << serv_res << ", core: " << serv_core_res << ", mem: " << serv_mem_res << endl;
        
        if (serv_res == 0)
            break;
    }

    // 将选型和放置方案写入到输出中
    int best_serv_type = res_to_serv_type.begin()->second;
    purchase_scheme.push_back(OnePurchaseScheme(best_serv_type, serv_stats.servs.size()));
    auto &best_assign_scheme = assign_schemes[best_serv_type];
    for (auto &one_assign_scheme: best_assign_scheme)
        assign_scheme.insert(make_pair(one_assign_scheme.first, one_assign_scheme.second));

    // 改变服务器的状态，删除已经处理过的操作
    // serv_stats.servs.emplace(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), best_serv_type, serv_specs));
    map<int, AddReq> new_add_reqs;
    map<int, OneAssignScheme>::iterator iter;
    for (auto req_iter = add_reqs.begin(); req_iter != add_reqs.end(); ++req_iter)
    {
        iter = best_assign_scheme.find(req_iter->second.vm_id);
        if (iter == best_assign_scheme.end())
        {
            new_add_reqs.emplace(req_iter->first, AddReq(req_iter->second.vm_type, req_iter->second.vm_id));
            continue;
        }
        serv_stats[iter->second.server_id].add_vm(req_iter->second.vm_type, req_iter->second.vm_id, 
            iter->second.server_node, vm_specs);
    }
    add_reqs = new_add_reqs;
}

void process_add_ops(map<int, AddReq> &add_reqs, PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, 
    ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    // 先尽可能分配请求操作，然后从 add_reqs 中删除已经处理的请求，直到确定剩下的请求都需要买服务器
    // 目前先使用 best fit 分配
    map<int, AddReq> add_reqs_need_purchase;
    int vm_id;
    for (auto req_iter = add_reqs.begin(); req_iter != add_reqs.end(); ++req_iter)
    {
        vm_id = req_iter->second.vm_id;
        OneAssignScheme one_assign_scheme = add_vm_select_serv_bf_hash(req_iter->second.vm_id, req_iter->second.vm_type, serv_stats, vm_specs);
        if (one_assign_scheme.server_id == -1)
        {
            add_reqs_need_purchase.emplace(req_iter->first, AddReq(req_iter->second.vm_type, req_iter->second.vm_id));
            continue;
        }
        assign_scheme.emplace(one_assign_scheme.vm_id, OneAssignScheme(one_assign_scheme.vm_id, 
            one_assign_scheme.server_id, one_assign_scheme.server_node));
        serv_stats[one_assign_scheme.server_id].add_vm(req_iter->second.vm_type, one_assign_scheme.vm_id, one_assign_scheme.server_node, vm_specs);
    }
    
    // 目前测试，用单节点虚拟机把服务器的一个单节点填满
    // 目前测试，混合装填
    // 目前测试，多服务器混合装填
    // 目前测试，写出购买和放置方案
    // 服务器选型，分配
    add_reqs = add_reqs_need_purchase;
    purchase_server(add_reqs, purchase_scheme, assign_scheme, serv_stats, serv_specs, vm_specs);
}

void dispatch(ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs, ServStatList &serv_stats, DayDispList &disps)
{
    time_t tic_day, toc_day, tic_mig, toc_mig, tic_add, toc_add, time_add;
    for (int idx_day = 0; idx_day < reqs.size(); ++idx_day)
    {
        PurchaseScheme purchase_scheme;
        AssignScheme assign_scheme;
        int idx_req = 0;
        int op, vm_id, vm_type;

        tic_day = clock();
        time_add = 0;

        // // 映射 vm_id -> idx_req
        // unordered_multimap<int, int> vm_id_to_idx_req;
        // DayReq &day_req = *(reqs[idx_day]);
        // for (int i = 0; i < day_req.op.size(); ++i)
        //     vm_id_to_idx_req.emplace(day_req.id[i], i);

        // 处理迁移操作，使得尽量多的服务器关机
        tic_mig = clock();
        MigScheme mig_scheme;
        bool stop_mig = false;
        int num_miged = 0;

        migrate(mig_scheme, serv_stats, serv_specs, vm_specs);
        toc_mig = clock();

        if (DEBUG)
            dump_serv_stats(serv_stats, serv_specs, string("./after_mig/day_") + to_string(idx_day) + ".txt");

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

            // 处理添加操作，总是处理最大搜索深度
            // int num_max_reqs = 5;
            // int num_reqs_left = 0;
            // int num_add_reqs = 0;
            // int idx_last_add_op = idx_req;
            // map<int, AddReq> add_reqs;
            // while (true)
            // {
            //     idx_last_add_op += select_add_reqs(add_reqs, idx_day, idx_last_add_op, reqs, num_max_reqs - num_reqs_left);  // (idx_req, )
            //     num_add_reqs = add_reqs.size();
            //     if (add_reqs.size() == 0 && num_reqs_left == 0)
            //         break;
            //     process_add_ops(add_reqs, purchase_scheme, assign_scheme, serv_stats, serv_specs, vm_specs);
            //     num_reqs_left = add_reqs.size();
            //     // cout << "idx_req: " << idx_last_add_op << endl;
            // }
            // idx_req = idx_last_add_op;

            // cout << "end" << endl;
            tic_add = clock();
            OneAssignScheme op_assign_scheme;
            if (op == 1)
            {
                process_add_op(vm_id, vm_type, purchase_scheme, op_assign_scheme, serv_stats, serv_specs, vm_specs);
                assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
                // cout << "vm id: " << vm_id << ", server id: " << op_assign_scheme.server_id << ", serv node: " << op_assign_scheme.server_node << endl;
                ++idx_req;
            }
            toc_add = clock();
            time_add += toc_add - tic_add;
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
        
        if (DEBUG)
            dump_serv_stats(serv_stats, serv_specs, string("./after_add/day_") + to_string(idx_day) + string(".txt"));

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
                << ", time mig: " << toc_mig - tic_mig << endl;
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

    // 对散列表进行遍历，映射存储的服务器编号
    for (auto uiter = serv_core_mem_map.begin(); uiter != serv_core_mem_map.end(); ++uiter)
    {
        for (int i = 0; i < uiter->second.size(); ++i)
        {
            if (id_old_to_new.find(uiter->second[i].first) != id_old_to_new.end())
            {
                uiter->second[i].first = id_old_to_new[uiter->second[i].first];
            }
        }
    }

    for (auto diter = dserv_core_mem_map.begin(); diter != dserv_core_mem_map.end(); ++diter)
    {
        for (int i = 0; i < diter->second.size(); ++i)
        {
            if (id_old_to_new.find(diter->second[i]) != id_old_to_new.end())
            {
                diter->second[i] = id_old_to_new[diter->second[i]];
            }
        }
    }

    for (auto iter = id_old_to_new.begin(); iter != id_old_to_new.end(); ++iter)
    {
        for (auto &vm: serv_stats[iter->second].vms)
        {
            vm_id_to_serv_id[vm.first] = iter->second;
        }
    }

    unordered_map<int, int>::iterator iter;
    set<pair<int, pair<int, int>>> new_serv_set;
    int res = 0;
    for (auto const &cont: serv_core_map_2)
    {
        res = cont.first;
        new_serv_set.clear();
        for (auto &node: cont.second)
        {
            iter = id_old_to_new.find(node.second.first);
            if (iter != id_old_to_new.end())
                new_serv_set.emplace(node.first, make_pair(iter->second, node.second.second));
            else
                new_serv_set.emplace(node.first, node.second);
        }
        serv_core_map_2[cont.first] = new_serv_set;
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


void process_add_op_span(int idx_req, int idx_day, int num_days, int span, int vm_id, int vm_type, 
    PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, 
    ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs, DayReqList &reqs)
{
    static float core_mem_ratio_mid;
    static int serv_should_buy;
    // static int idx_day_last = 0;
    // 对一个大于 span 的数取余
    if (idx_day % span == 0 && idx_req == 0)
    {
        time_t tic = clock();
        core_mem_ratio_mid = calc_vm_core_mem_ratio_median(reqs, idx_day, span, num_days, vm_specs);
        vector<pair<float, int>> res;
        for (int i = 0; i < serv_specs.size(); ++i)
        {
            res.emplace_back(abs((float)serv_specs[i].cores / serv_specs[i].memcap - core_mem_ratio_mid), i);
        }
        partial_sort(res.begin(), res.begin()+1, res.end());
        serv_should_buy = res[0].second;
        time_t toc = clock();
        if (DEBUG)
            cout << "select serv type time: " << toc - tic << endl;
    }

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

            // float serv_core, serv_mem;
            // for (int i = 0; i < serv_specs.size(); ++i)
            // {
            //     // int serv_core = serv_specs[i].cores;
            //     // int serv_mem = serv_specs[i].memcap;
            //     // float ratio = float(serv_core) / serv_mem;
            //     // float diff = abs(ratio - core_mem_ratio_mid);
            //     ratio_to_spec.emplace(abs((float)serv_specs[i].cores / serv_specs[i].memcap - core_mem_ratio_mid), i);
            // }

            int min_price = INT32_MAX;
            int min_idx = -1;
            serv_should_buy = 4;
            if (serv_specs[serv_should_buy].can_hold_vm(vm_type, vm_specs))
            {
                min_idx = serv_should_buy;
            }
            else
            {
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
            }

            
            purchase_scheme.push_back(OnePurchaseScheme(min_idx, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), min_idx, serv_specs)));
            serv_stats.add_serv(serv_stats.servs.size(), min_idx);

            // int serv_spec_to_buy = next(res_to_spec.begin(), res_to_spec.size() / 2)->second;
            // purchase_scheme.push_back(OnePurchaseScheme(serv_spec_to_buy, serv_stats.servs.size()));
            // serv_stats.servs.insert(make_pair(serv_stats.servs.size(), ServStat(serv_stats.servs.size(), serv_spec_to_buy, serv_specs)));
            
            continue;
        }

        assign_scheme.server_id = oas.server_id;
        assign_scheme.server_node = oas.server_node;
        assign_scheme.vm_id = vm_id;
        serv_stats[oas.server_id].add_vm(vm_type, vm_id, oas.server_node, vm_specs);
        break;
    }
}
