#include "dispatch.h"
#include <iostream>
#include "bfd.h"
using namespace std;

// ExpandScheme expand_servers(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs)
// {

// }

// MigrateScheme migrate_vms()
// {
//     return 
// }

bool comp_by_type(ServerTypeNum &s1, ServerTypeNum &s2)
{
    if (s1.server_type < s2.server_type) return true;
    else return false;
}

int purchase_server(PurchaseScheme &purchase_scheme, ServerSpecList &server_specs)
{
    int server_type = rand() % server_specs.cores.size();
    purchase_scheme.add_server(server_type);
    return server_type;
}

void process_add_reqs(vector<AddReq> &add_reqs, PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, ServerStatList &server_stat, VMStat &vm_stat, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    vector<int> vm_types;
    for (int i = 0; i < add_reqs.size(); ++i)
        vm_types.push_back(add_reqs[i].vm_type);
    vector<int> vm_ids;
    for (int i = 0; i < add_reqs.size(); ++i)
        vm_ids.push_back(add_reqs[i].vm_id);
    
    // 因为 server_stat 的状态在本函数外修改，所以本函数只修改 server_stat 的副本
    ServerStatList virtual_server_stat;
    PurchaseScheme virtual_purchase_scheme;
    AssignScheme bfd_assign_scheme;

    // virtual_purchase_scheme = purchase_scheme;  // 接到本天上次 add 请求
    while (true)
    {
        virtual_server_stat = server_stat;

        // 将购买的服务器信息加入到虚拟服务器状态中
        for (int i = 0; i < purchase_scheme.server_types.size(); ++i)
        {
            virtual_server_stat.add_server(virtual_server_stat.servs.size(), purchase_scheme.server_types[i]);
        }

        // for (int i = 0; i < vm_ids.size(); ++i)
        // {
        //     if (vm_ids[i] == 341548707)
        //         cout << "nihao" << endl;
        // }

        // 尝试用当前已存在的服务器进行处理，若无法处理完，买个新服务器再处理
        bfd_assign_scheme.clear();
        if (!bfd_2(bfd_assign_scheme, vm_ids, vm_types, virtual_server_stat, server_specs, vm_specs))
        {
            purchase_server(purchase_scheme, server_specs);
            continue;
        }
        else
        {
            // assign_scheme = bfd_assign_scheme;
            for (int i = 0; i < add_reqs.size(); ++i)
            {
                int vm_id = add_reqs[i].vm_id;
                int vm_type = add_reqs[i].vm_type;
                unordered_map<int, OneAssignScheme>::iterator iter = bfd_assign_scheme.find(vm_id);
                if (iter == bfd_assign_scheme.end())
                    cout << "error" << endl;
                assign_scheme.insert(make_pair(vm_id, iter->second));
            }
            // for (auto iter = bfd_assign_scheme.begin(); iter != bfd_assign_scheme.end(); ++iter)
            // {
            //     int iter_first = iter->first;
            //     int vm_id = add_reqs[iter->first].vm_id;
            //     if (bfd_assign_scheme.find(vm_id) == bfd_assign_scheme.end())
            //         cout << "error" << endl;
            //     assign_scheme.insert(make_pair(add_reqs[iter->first].vm_id, iter->second));
            // }
            for (int i = 0; i < add_reqs.size(); ++i)
            {
                assign_scheme[add_reqs[i].vm_id].vm_id = add_reqs[i].vm_id;
            }
            break;
        }
    }
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


void swap_server(ServerStatList &server_stats, int idx_1, int idx_2)
{
    ServerStat temp = server_stats.servs[idx_1];
    server_stats.servs[idx_1] = server_stats.servs[idx_2];
    server_stats.servs[idx_2] = temp;
}

void map_server_ids(unordered_map<int, int> id_old_to_new, int id_start, PurchaseScheme &purchase_scheme, AssignScheme &assign_scheme, ServerStatList &server_stats)
{
    unordered_map<int, int>::iterator iter;
    for (auto i = assign_scheme.begin(); i != assign_scheme.end(); ++i)
    {
        iter = id_old_to_new.find(assign_scheme[i->first].server_id);
        if (iter != id_old_to_new.end())
            assign_scheme[i->first].server_id = iter->second;
        // else
        // {
        //     int old_server_id = assign_scheme[i->first].server_id;
        //     cout << "error" << endl;
        // }
    }

    ServerStatList server_stats_copy(server_stats);
    server_stats.servs.clear();
    for (int i = id_start; i < id_start + purchase_scheme.server_types.size(); ++i)
    {
        // server_stats.servs[id_old_to_new[i]] = server_stats_copy.servs[i];
        server_stats.servs.insert(make_pair(id_old_to_new[i], server_stats_copy.servs[i]));
    }
    for (int i = 0; i < id_start; ++i)
    {
        if (server_stats.servs.find(i) == server_stats.servs.end())
            server_stats.servs.insert(make_pair(i, server_stats_copy.servs[i]));
    }
    if (server_stats.servs.size() != server_stats_copy.servs.size())
        cout << "error" << endl;
    if (server_stats.servs.find(0) == server_stats.servs.end())
        cout << "error" << endl;
}


unordered_map<int, int> get_id_old_to_new(PurchaseScheme &purchase_scheme, ServerStatList &server_stat)
{
    vector<ServIdType> id_type;
    int id_start = server_stat.servs.size() - purchase_scheme.server_types.size();
    for (int i = 0; i < purchase_scheme.server_types.size(); ++i)
    {
        id_type.push_back(ServIdType(id_start + i, purchase_scheme.server_types[i]));
    }
    sort(id_type.begin(), id_type.end(), comp_id_type);
    unordered_map<int, int> id_old_to_new;
    for (int i = 0; i < id_type.size(); ++i)
    {
        id_old_to_new.insert(make_pair(id_type[i].id, id_start + i));
    }
    return id_old_to_new;
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
        add_reqs.push_back(AddReq(reqs[idx_day]->name_id[i], reqs[idx_day]->id[i]));
    }
    return add_reqs;
}

void print_vm_info(int vm_type, VMSpecList &vm_specs)
{
    cout << "vm type: " << vm_type << ", cores: " << vm_specs.cores[vm_type] << ", mem: " << vm_specs.memcap[vm_type] 
        << ", node: " << vm_specs.nodes[vm_type] << endl;
}

void print_server_info(int server_id, ServerStatList &server_stats, ServerSpecList &server_specs)
{
    cout << "server id:" << server_id << ", server type: " << server_stats.servs[server_id].type
        << ", cores used: " << server_stats.servs[server_id].cores_used << ", mem used: " << server_stats.servs[server_id].mem_used << endl;
    for (int n = 0; n < 2; ++n)
    {
        cout << "node " << n 
            << ": cores used: " << server_stats.servs[server_id].nodes[n].cores_used
            << ", cores ream: " << server_specs.cores[server_stats.servs[server_id].type] / 2 - server_stats.servs[server_id].nodes[n].cores_used
            << ", mem used: " << server_stats.servs[server_id].nodes[n].mem_used
            << ", mem ream: " << server_specs.memcap[server_stats.servs[server_id].type] / 2 - server_stats.servs[server_id].nodes[n].mem_used;
        cout << endl;
    }
}

void process_del_op(int vm_id, ServerStatList &server_stats, VMStat &vm_stat, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    int vm_type = vm_stat.vms.at(vm_id)->type;
    int server_id = vm_stat.vms.at(vm_id)->server_id;
    int node = vm_stat.vms.at(vm_id)->node;
    server_stats.del_vm(vm_type, server_id, node, server_specs, vm_specs);
    vm_stat.del_vm(vm_id);
}

void process_add_op(int vm_id, int vm_type, PurchaseScheme &purchase_scheme, OneAssignScheme &assign_scheme, ServerStatList &server_stats, VMStat &vm_stat, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    AssignScheme bfd_assign_scheme;
    vector<int> server_types_purchsed;

    while (true)
    {
        ServIdNode serv_id_node = vm_select_server(vm_id, vm_type, server_stats, server_specs, vm_specs);
        if (serv_id_node.serv_id == -1)
        {
            purchase_server(purchase_scheme, server_specs);
            server_stats.add_server(server_stats.servs.size(), purchase_scheme.server_types.back());
        }
        else
        {
            if (vm_id == 193726959)
            {
                print_server_info(24, server_stats, server_specs);
                for (auto iter = vm_stat.vms.begin(); iter != vm_stat.vms.end(); ++iter)
                {
                    if (iter->second->server_id == 24)
                    {
                        cout << "vm id: " << iter->first << endl;
                        print_vm_info(iter->second->type, vm_specs);
                    }
                }
                cout << endl;
            }
            if (!server_stats.add_vm(vm_type, serv_id_node.serv_id, serv_id_node.serv_node, server_specs, vm_specs))
            {
                cout << "error" << endl;
            }
            vm_stat.add_vm(vm_type, vm_id, serv_id_node.serv_id, serv_id_node.serv_node);
            assign_scheme.vm_id = vm_id;
            assign_scheme.server_id = serv_id_node.serv_id;
            assign_scheme.server_node = serv_id_node.serv_node;
            break;
        }
    }
}


void dispatch(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs, 
            ServerStatList &server_stats, VMStat &vm_stat, DayDispList &disps)
{
    for (int idx_day = 0; idx_day < reqs.size(); ++idx_day)
    {
        // cout << "day " << idx_day << endl;
        int idx_req = 0;
        int op, vm_id, vm_type;
        PurchaseScheme purchase_scheme;
        AssignScheme assign_scheme;
        
        while (idx_req < reqs[idx_day]->op.size())
        {
            // 解析下一天的操作
            op = reqs[idx_day]->op[idx_req];
            vm_id = reqs[idx_day]->id[idx_req];
            vm_type = reqs[idx_day]->name_id[idx_req];

            // 处理删除操作
            if (op == 0)
            {
                process_del_op(vm_id, server_stats, vm_stat, server_specs, vm_specs);
                ++idx_req;
                continue;
            }

            // 处理添加操作
            if (op == 1)
            {
                OneAssignScheme op_assign_scheme;
                process_add_op(vm_id, vm_type, purchase_scheme, op_assign_scheme, server_stats, vm_stat, server_specs, vm_specs);
                assign_scheme.insert(make_pair(vm_id, op_assign_scheme));
                ++idx_req;
            }
        }

        // 重新对服务器序号排序
        unordered_map<int, int> id_old_to_new;

        id_old_to_new = get_id_old_to_new(purchase_scheme, server_stats);
        int id_start = server_stats.servs.size() - purchase_scheme.server_types.size();
        map_server_ids(id_old_to_new, id_start, purchase_scheme, assign_scheme, server_stats);

        // 将购买方案和调度方案写入到输出中
        vector<ServerTypeNum> server_type_num = purchase_scheme.get_ordered_server_types();
        disps.new_day();
        for (int i = 0; i < server_type_num.size(); ++i)
        {
            disps.push_pur_server(idx_day, server_type_num[i].server_type, server_type_num[i].num);
        }

        for (auto iter = assign_scheme.begin(); iter != assign_scheme.end(); ++iter)
        {
            int vm_id = iter->first;
            int server_id = iter->second.server_id;
            int server_node = iter->second.server_node;
            if (server_id >= server_stats.servs.size())
                cout << "error" << endl;
        }

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


void dispatch_old(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs, 
            ServerStatList &server_stat, VMStat &vm_stat, DayDispList &disps)
{
    for (int idx_day = 0; idx_day < reqs.size(); ++idx_day)
    {
        // cout << "day " << idx_day << endl;
        int idx_req = 0;
        int op, vm_id, vm_type;
        PurchaseScheme purchase_scheme;
        AssignScheme assign_scheme;
        vector<PurchaseScheme> add_reqs_purchase_schemes;
        vector<AddReq> day_add_reqs;

        int add_count = 0;
        while (idx_req < reqs[idx_day]->op.size())
        {
            // 解析下一天的操作
            op = reqs[idx_day]->op[idx_req];
            vm_id = reqs[idx_day]->id[idx_req];
            vm_type = reqs[idx_day]->name_id[idx_req];

            // 如果下一个操作是 del，那么直接处理
            if (op == 0)
            {
                vm_type = vm_stat.vms[vm_id]->type;
                int server_id = vm_stat.vms[vm_id]->server_id;
                int node = vm_stat.vms[vm_id]->node;
                server_stat.del_vm(vm_type, server_id, node, server_specs, vm_specs);
                vm_stat.del_vm(vm_id);
                ++idx_req;
                continue;
            }

            // 如果下一个操作是 add，那么尝试拿出 add 序列，直到遇到下一个 del 或当天结束
            vector<AddReq> add_reqs = select_add_reqs(idx_day, idx_req, reqs);
            day_add_reqs.insert(day_add_reqs.end(), add_reqs.begin(), add_reqs.end());
            idx_req += add_reqs.size();
            add_count += add_reqs.size();

            // 处理 add 序列，对当日的购买方案和分配方案进行修改
            // purchase_scheme 和 assign_scheme 在当日都是递增的
            PurchaseScheme add_reqs_purchase_scheme;
            process_add_reqs(add_reqs, add_reqs_purchase_scheme, assign_scheme, server_stat, vm_stat, server_specs, vm_specs);
            add_reqs_purchase_schemes.push_back(add_reqs_purchase_scheme);

            // 按方案处理扩容和 add 请求，将结果保存到状态中
            for (int i = 0; i < add_reqs_purchase_scheme.server_types.size(); ++i)
            {
                server_stat.add_server(server_stat.servs.size(), add_reqs_purchase_scheme.server_types[i]);
            }

            for (auto i = 0; i < add_reqs.size(); ++i)
            {
                // if (i == 16)
                // {
                //     print_vm_info(add_reqs[i].vm_type, vm_specs);
                //     print_server_info(assign_scheme[add_reqs[i].vm_id].server_id, server_stat, server_specs);
                //     cout << endl;
                // }
                if (server_stat.add_vm(add_reqs[i].vm_type, assign_scheme[add_reqs[i].vm_id].server_id, assign_scheme[add_reqs[i].vm_id].server_node, server_specs, vm_specs))
                {
                    vm_stat.add_vm(add_reqs[i].vm_type, add_reqs[i].vm_id, assign_scheme[add_reqs[i].vm_id].server_id, assign_scheme[add_reqs[i].vm_id].server_node);
                }
                else
                {
                    cout << "error" << endl;
                    print_vm_info(add_reqs[i].vm_type, vm_specs);
                    print_server_info(assign_scheme[add_reqs[i].vm_id].server_id, server_stat, server_specs);
                    cout << endl;
                }
            }
        }

        // 检查 assign scheme 的正确性
        int assign_scheme_size = assign_scheme.size();
        if (assign_scheme.size() != add_count)
            cout << "error" << endl;

        // 合并多次的 purchase_scheme
        for (int i = 0; i < add_reqs_purchase_schemes.size(); ++i)
        {
            for (int j = 0; j < add_reqs_purchase_schemes[i].server_types.size(); ++j)
            {
                purchase_scheme.add_server(add_reqs_purchase_schemes[i].server_types[j]);
            }
        }

        // 一天处理完成后，对购买方案，调度方案以及服务器状态中的服务器编号进行重排序，以适应输出
        unordered_map<int, int> id_old_to_new = get_id_old_to_new(purchase_scheme, server_stat);
        int id_start = server_stat.servs.size() - purchase_scheme.server_types.size();
        map_server_ids(id_old_to_new, id_start, purchase_scheme, assign_scheme, server_stat);

        // 将购买方案和调度方案写入到输出中
        vector<ServerTypeNum> server_type_num = purchase_scheme.get_ordered_server_types();
        disps.new_day();
        for (int i = 0; i < server_type_num.size(); ++i)
        {
            disps.push_pur_server(idx_day, server_type_num[i].server_type, server_type_num[i].num);
        }

        for (auto iter = assign_scheme.begin(); iter != assign_scheme.end(); ++iter)
        {
            int vm_id = iter->first;
            int server_id = iter->second.server_id;
            int server_node = iter->second.server_node;
            if (server_id >= server_stat.servs.size())
                cout << "error" << endl;
        }

        if (assign_scheme.size() == 0)
            cout << "error" << endl;
        for (int i = 0; i < day_add_reqs.size(); ++i)
        {
            disps.push_add_vm(idx_day, assign_scheme[day_add_reqs[i].vm_id].server_id, assign_scheme[day_add_reqs[i].vm_id].server_node);
        }
        // for (auto iter = assign_scheme.begin(); iter != assign_scheme.end(); ++iter)
        // {
        //     disps.push_add_vm(idx_day, assign_scheme[iter->first].server_id, assign_scheme[iter->first].server_node);
        // }
        if (disps.disp_list[idx_day]->req_server_ids.size() == 0)
            cout << "error" << endl;

        // output dispatch info
        cout << "day " << idx_day << ": " << endl;
        cout << "buy servers:" << endl;
        for (int i = 0; i < disps.disp_list.back()->pur_server_name_ids.size(); ++i)
        {
            cout << server_names_vec[disps.disp_list.back()->pur_server_name_ids[i]] << ": "
                 << disps.disp_list.back()->pur_server_nums[i] << endl;
        }

        cout << "add vms:" << endl;
        for (int i = 0; i < disps.disp_list.back()->req_server_ids.size(); ++i)
        {
            cout << "req " << i << ":  ";
            cout << "vm id: " << day_add_reqs[i].vm_id << ", vm type: " << vm_names_vec[day_add_reqs[i].vm_type] << " -> "
                << "serv id: " << disps.disp_list.back()->req_server_ids[i] << ", serv type: "
                << server_names_vec[server_stat.servs[disps.disp_list.back()->req_server_ids[i]].type]
                << ", serv node: " << disps.disp_list.back()->req_server_nodes[i] << endl;
        }
        cout << endl;
    }
}



// void dispatch(ServerSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs, 
//             ServerStat &server_stat, VMStat &vm_stat, ServerStatRecorder &rec,
//             DayDispList &disps)
// {
//     static int idx_day = 0;
//     static int idx_req = 0;
//     int idx = 0;

//     vector<int> add_ops = count_add_ops(reqs);

//     PurchaseScheme purchase_scheme;
//     while (true)
//     {
//         // cout << idx_day << " " << idx_req << endl;
//         if (idx_day == reqs.size())  // 最后一天处理完成
//             break;

//         if (idx_req == reqs[idx_day]->id.size())  // 一天的结尾
//         {
//             ++idx_day;
//             idx_req = 0;
//             continue;
//         }        

//         // 先处理删除操作
//         int op = reqs[idx_day]->op[idx_req];
//         int vm_id = reqs[idx_day]->id[idx_req];
//         int vm_type = reqs[idx_day]->name_id[idx_req];
//         if (op == 0)
//         {
//             vm_type = vm_stat.vms[vm_id]->type;
//             int server_id = vm_stat.vms[vm_id]->server_id;
//             int node = vm_stat.vms[vm_id]->node;
//             server_stat.del_vm(vm_type, server_id, node, server_specs, vm_specs);
//             vm_stat.del_vm(vm_id);
//             ++idx_req;
//             continue;
//         }

//         // 按 del 操作以及天切分出 add 操作列表
//         DayReqList add_reqs;
//         int current_day = idx_day;
//         add_reqs = select_add_reqs(idx_day, idx_req, reqs);
//         vector<int> vm_ids;
//         for (int i = 0; i < add_reqs.size(); ++i)
//         {
//             for (int j = 0; j < add_reqs[i]->id.size(); ++j)
//             {
//                 vm_ids.push_back(add_reqs[i]->id[j]);
//             }
//         }

//         int add_count = 0;
//         for (int i = 0; i < add_reqs.size(); ++i)
//             add_count += add_reqs[i]->id.size();

//         if (add_count != add_ops[idx])
//             cout << "error" << endl;
//         idx++;

//         // 处理多天、多次的 add 请求
//         vector<int> vm_types;
//         for (int i = 0; i < add_reqs.size(); ++i)
//         {
//             for (int j = 0; j < add_reqs[i]->id.size(); ++j)
//             {
//                 vm_types.push_back(add_reqs[i]->name_id[j]);
//             }
//         }

//         vector<int> sorted_server_types;
//         vector<int> server_types;
//         AssignScheme assign_scheme;

//         for (int i = 0; i < server_stat.types.size(); ++i)
//         {
//             server_types.push_back(server_stat.types[i]);
//         }

        
//         vector<ServerTypeNum> purchase_servers_type_to_num;
//         while (true)
//         {
//             // 处理购买服务器的信息，标号，制作一个 server_stat 的副本
//             ServerStat virtual_server_stat = server_stat;
//             purchase_servers_type_to_num = purchase_scheme.get_ordered_server_types();
//             for (int i = 0; i < purchase_servers_type_to_num.size(); ++i)
//             {
//                 for (int j = 0; j < purchase_servers_type_to_num[i].num; ++j)
//                 {
//                     virtual_server_stat.add_server(purchase_servers_type_to_num[i].server_type, server_specs);
//                 }
//             }

//             // 尝试对 server_stat 的副本进行分配资源，若成功，则返回新购买的服务器的信息和调度信息；否则购买新的服务器
//             assign_scheme.clear();
//             if (bfd(assign_scheme, vm_types, virtual_server_stat, server_specs, vm_specs))  // 可以用当前服务器满足请求
//             {
//                 // 将新增的服务器信息写入服务器状态
//                 for (int i = 0; i != purchase_servers_type_to_num.size(); ++i)
//                 {
//                     for (int j = 0; j < purchase_servers_type_to_num[i].num; ++j)
//                     {
//                         server_stat.add_server(purchase_servers_type_to_num[i].server_type, server_specs);
//                     }
//                 }

//                 // 写入调度信息
//                 for (int i = 0; i < assign_scheme.size(); ++i)
//                 {
//                     // output debug info
//                     // if (vm_ids[i] == 956267648)
//                     // {
//                     //     cout << "Assignment " << i << " :" << endl;
//                     //     int server_id = assign_scheme[i].server_id;
//                     //     int server_type = server_stat.types[server_id];
//                     //     cout << "server id: " << server_id << ", server type: " << server_type << " " << server_names_vec[server_type] << endl;
//                     //     for (int j = 0; j < 2; ++j)
//                     //     {
//                     //         cout << "resources: node " << j 
//                     //         << " cores remain " << server_specs.cores[server_type] / 2  - server_stat.nodes[j][server_id]->cores_used << ", "
//                     //         << "mem remain " << server_specs.memcap[server_type] / 2 - server_stat.nodes[j][server_id]->mem_used << endl;
//                     //     }
//                     //     cout << "vm type: " << vm_types[i] << " " << vm_names_vec[vm_types[i]] << ", id: " << vm_ids[i]
//                     //     << ", cores: " << vm_specs.cores[vm_types[i]] << ", mem: " << vm_specs.memcap[vm_types[i]] 
//                     //     << ", num node: " << vm_specs.nodes[vm_types[i]] << ", assigned noode: " << assign_scheme[i].server_node << endl;
//                     //     cout << endl;
//                     // }


//                     if (server_stat.add_vm(vm_types[i], assign_scheme[i].server_id, assign_scheme[i].server_node, server_specs, vm_specs))
//                     {
//                         vm_stat.add_vm(vm_types[i], vm_ids[i], assign_scheme[i].server_id, assign_scheme[i].server_node);
//                         disps.push_add_vm(assign_scheme[i].server_id, assign_scheme[i].server_node);
//                     }
//                     else
//                         cout << "error" << endl;
//                 }

//                 // 写入购买信息
//                 purchase_servers_type_to_num = purchase_scheme.get_ordered_server_types();
//                 for (int i = 0; i < purchase_servers_type_to_num.size(); ++i)
//                 {
//                     disps.push_pur_server(current_day, purchase_servers_type_to_num[i].server_type, purchase_servers_type_to_num[i].num);
//                 }
//                 purchase_scheme.server_types.clear();
//                 break;
//             }
//             else  // 无法用当前已有的服务器满足要求，随机买个新的服务器
//             {
//                 purchase_server(purchase_scheme, server_specs);
//                 continue;
//             }
//         }
//     }
// }
