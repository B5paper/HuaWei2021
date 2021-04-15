#include "status.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <set>
#include "types.h"
#include "args.h"


ServStatList serv_stats;
multiset<pair<int, int>> non_full_load_servs;
map<int, int> vm_id_to_serv_id;
int num_vms_total = 0;

long long num_serv_mem_res_global = 0;
long long num_serv_core_res_global = 0;

ServStat::ServStat(int id, int type): id(id), type(type) 
{
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_ream = serv_specs[type].cores / 2;
        nodes[i].mem_ream = serv_specs[type].memcap / 2;
    }

    // num_serv_core_res_global += nodes[0].cores_ream + nodes[1].cores_ream;
    // num_serv_mem_res_global += nodes[0].mem_ream + nodes[1].mem_ream;
}

ServStat::ServStat(const ServStat &obj)

{
    id = obj.id;
    type = obj.type;
    group = obj.group;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_ream = obj.nodes[i].cores_ream;
        nodes[i].mem_ream = obj.nodes[i].mem_ream;
    }
    vms = obj.vms;
}

void ServStat::operator= (ServStat const &obj)
{
    id = obj.id;
    type = obj.type;
    group = obj.group;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_ream = obj.nodes[i].cores_ream;
        nodes[i].mem_ream = obj.nodes[i].mem_ream;
    }
    vms = obj.vms;
}

bool ServStat::can_hold_vm(int vm_core, int vm_mem, int serv_node)
{
    switch (serv_node)
    {
        case 0:
        case 1:
        if (nodes[serv_node].cores_ream < vm_core)
            return false;
        if (nodes[serv_node].mem_ream < vm_mem)
            return false;
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            if (nodes[i].cores_ream < vm_core / 2)
                return false;
            if (nodes[i].mem_ream < vm_mem / 2)
                return false;
        }
        break;

        default:
        cout << "error server node" << endl;       
    }
    return true;
}

ServStat& ServStatList::operator[] (int id)
{
    return servs[id];
}

ServStat& ServStatList::get_serv_by_vm_id(int vm_id)
{
    auto iter = vm_id_to_serv_id.find(vm_id);
    if (iter == vm_id_to_serv_id.end())
        cout << "error in get_serv_by_vm_id()" << endl;
    return servs[vm_id_to_serv_id[vm_id]];
}

void ServStat::del_vm(int vm_id)
{

    auto vm_iter = vms.find(vm_id);
    if (vm_iter == vms.end())
        cout << "error in del_vm()" << endl;

    int vm_type = vm_iter->second.type;
    int vm_node = vm_iter->second.node;
   
    if (vm_node < 2)
    {
        node_map_remove(id, vm_node);
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            node_map_remove(id, i);
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        nnode_map_remove(id, i);
    }

    serv_map_remove(id);
    nserv_map_remove(id);

    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    auto vm_core_iter = serv_stats.vm_stat.find(vm_core);
    if (vm_core_iter != serv_stats.vm_stat.end())
    {
        auto vm_mem_iter = vm_core_iter->second.find(vm_mem);
        if (vm_mem_iter != vm_core_iter->second.end())
        {
            auto vm_node_iter = vm_mem_iter->second.find(vm_node < 2 ? 1 : 2);
            if (vm_node_iter != vm_mem_iter->second.end())
            {
                auto vm_iter = vm_node_iter->second.find(vm_id);
                if (vm_iter != vm_node_iter->second.end())
                {
                    vm_node_iter->second.erase(vm_id);

                    if (vm_node_iter->second.empty())
                    {
                        vm_mem_iter->second.erase(vm_node_iter);
                        if (vm_mem_iter->second.empty())
                        {
                            vm_core_iter->second.erase(vm_mem_iter);
                            if (vm_core_iter->second.empty())
                            {
                                serv_stats.vm_stat.erase(vm_core_iter);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!is_empty() && !is_full_load())
    {
        for (int i = 0; i < 2; ++i)
        {
            num_serv_core_res_global -= nodes[i].cores_ream;
            num_serv_mem_res_global -= nodes[i].mem_ream;
        }
    }

    // 改变当前节点的状态        
    _change_serv_stat(0, vm_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);
    vms.erase(vm_id);

    if(!vms.empty())
    {
        for (int i = 0; i < 2; ++i)
        {
            nnode_map_add(id, i);
        }

        nserv_map_add(id);
    }

    if (vm_node < 2)
        node_map_add(id, vm_node);
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            node_map_add(id, i);
            // node_core_mem_map[nodes[i].cores_ream].emplace(nodes[i].mem_ream, make_pair(id, i));
        }
    }

    serv_map_add(id);

    auto iter = vm_id_to_serv_id.find(vm_id);
    if (iter == vm_id_to_serv_id.end())
        cout << "error in del_vm" << endl;
    vm_id_to_serv_id.erase(iter);


    // num_serv_mem_res_global -=
    auto non_iter = non_full_load_servs.find(make_pair(vms.size()+1, id));
    if (non_iter != non_full_load_servs.end())
        non_full_load_servs.erase(non_iter);
    if (!is_full_load() && !is_empty())
    {
        for (int i = 0; i < 2; ++i)
        {
            num_serv_core_res_global += nodes[i].cores_ream;
            num_serv_mem_res_global += nodes[i].mem_ream;
        }
        non_full_load_servs.emplace(vms.size(), id);
    }
    

    --num_vms_total;
}

void ServStat::add_vm(int vm_type, int vm_id, int serv_node)
{
    if (!can_hold_vm(vm_specs[vm_type].cores, vm_specs[vm_type].memcap, serv_node))
        cout << "error in add vm" << endl;
    vms[vm_id] = VMTypeNode(vm_type, serv_node);
    
 
    // 先删除本节点之前的状态
    
    if (serv_node < 2)
    {
        node_map_remove(id, serv_node);
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            node_map_remove(id, i);
        }
    }
    serv_map_remove(id);

    if (vms.size() > 1)
    {
        for (int i = 0; i < 2; ++i)
        {
            nnode_map_remove(id, i);
        }
        nserv_map_remove(id);
    }

    if (!is_empty() && !is_full_load())
    {
        for (int i = 0; i < 2; ++i)
        {
            num_serv_core_res_global -= nodes[i].cores_ream;
            num_serv_mem_res_global -= nodes[i].mem_ream;
        }
    }
    
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    auto vm_core_iter = serv_stats.vm_stat.find(vm_core);
    if (vm_core_iter != serv_stats.vm_stat.end())
    {
        auto vm_mem_iter = vm_core_iter->second.find(vm_mem);
        if (vm_mem_iter != vm_core_iter->second.end())
        {
            auto vm_node_iter = vm_mem_iter->second.find(serv_node < 2 ? 1 : 2);
            if (vm_node_iter != vm_mem_iter->second.end())
            {
                auto vm_iter = vm_node_iter->second.find(vm_id);
                if (vm_iter != vm_node_iter->second.end())
                {
                    vm_node_iter->second.erase(vm_id);

                    if (vm_node_iter->second.empty())
                    {
                        vm_mem_iter->second.erase(vm_node_iter);
                        if (vm_mem_iter->second.empty())
                        {
                            vm_core_iter->second.erase(vm_mem_iter);
                            if (vm_core_iter->second.empty())
                            {
                                serv_stats.vm_stat.erase(vm_core_iter);
                            }
                        }
                    }
                }
            }
        }
    }

    // 改变本节点的状态
    _change_serv_stat(1, serv_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);

    for (int i = 0; i < 2; ++i)
    {
        num_serv_core_res_global += nodes[i].cores_ream;
        num_serv_mem_res_global += nodes[i].mem_ream;
    }
    
    for (int i = 0; i < 2; ++i)
    {
        nnode_map_add(id, i);
    }
    nserv_map_add(id);


    if (serv_node < 2)
    {
        node_map_add(id, serv_node);
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            node_map_add(id, i);
        }
    }

    serv_map_add(id);

    vm_id_to_serv_id.insert(make_pair(vm_id, id));


    auto non_iter = non_full_load_servs.find(make_pair(vms.size()-1, id));
    if (non_iter != non_full_load_servs.end())
        non_full_load_servs.erase(non_iter);
    
    if (!is_full_load())
    {
        non_full_load_servs.emplace(vms.size(), id);
        serv_stats.vm_stat[vm_core][vm_mem][vm_specs[vm_type].nodes].emplace(vm_id);

        if (!is_empty())
        {
            for (int i = 0; i < 2; ++i)
            {
                num_serv_core_res_global += nodes[i].cores_ream;
                num_serv_mem_res_global += nodes[i].mem_ream;
            }
        }
    }

    ++num_vms_total;
}

void NodeStat::_change_node_stat(int op, int core, int mem)
{
    if (op == 0)  // release resources
    {
        cores_ream += core;
        mem_ream += mem;
    }

    if (op == 1)  // occupy resources
    {
        cores_ream -= core;
        mem_ream -= mem;

    }
}


void ServStat::_change_serv_stat(int op, int serv_node, int core, int mem)
{
    switch (serv_node)
    {
        case 0:
        case 1:
        nodes[serv_node]._change_node_stat(op, core, mem);
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            nodes[i]._change_node_stat(op, core / 2, mem / 2);
        }
        break;

        default:
        cout << "error in _change_serv_stat()" << endl;
    }
}

void ServStatList::mig_vm(int vm_id, int serv_id, int serv_node)
{
    ServStat &serv_stat = get_serv_by_vm_id(vm_id);
    int vm_type = serv_stat.vms[vm_id].type;
    serv_stat.del_vm(vm_id);
    servs[serv_id].add_vm(vm_type, vm_id, serv_node);
}

void ServStatList::mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node)
{
    // if (servs.find(old_serv_id) == servs.end() || servs.find(new_serv_node) == servs.end())
    //     cout << "error in mig_vm" << endl;
    ServStat &serv_stat = servs[old_serv_id];
    int vm_type = serv_stat.vms[vm_id].type;
    serv_stat.del_vm(vm_id);
    servs[new_serv_id].add_vm(vm_type, vm_id, new_serv_node);
}

bool ServStat::is_full()
{
    if (nodes[0].cores_ream == 0 && nodes[0].mem_ream == 0 
        && nodes[1].cores_ream == 0 && nodes[1].mem_ream == 0)
        return true;
    
    return false;
}

bool ServStat::is_full_load()
{
    if (nodes[0].cores_ream <= core_node_full_load_thresh && nodes[0].mem_ream <= mem_node_full_load_thresh &&
        nodes[1].cores_ream <= core_node_full_load_thresh && nodes[1].mem_ream <= mem_node_full_load_thresh)
        return true;
    return false;
}

bool ServStat::is_empty()
{
    if (serv_specs[type].cores - nodes[0].cores_ream - nodes[1].cores_ream == 0 &&
        serv_specs[type].memcap - nodes[0].mem_ream - nodes[1].mem_ream == 0)
        return true;
    return false;
}


void ServStatList::add_serv(int id, int type)
{
    servs.emplace(id, ServStat(id, type));
    ServStat &serv = servs[id];
    for (int i = 0; i < 2; ++i)
        serv.node_map_add(id, i);
    serv.serv_map_add(id);
}


void ServStat::node_map_remove(int serv_id, int node)
{
    if (node > 1)
        cout << "error in node map remove" << endl;

    int node_core = serv_stats[serv_id].nodes[node].cores_ream;
    int node_mem = serv_stats[serv_id].nodes[node].mem_ream;

    auto core_iter = serv_stats.node_core_mem_map.find(node_core);
    if (core_iter == serv_stats.node_core_mem_map.end())
        cout << "error in node map remove" << endl;
    
    auto mem_iter = core_iter->second.find(node_mem);
    if (mem_iter == core_iter->second.end())
        cout << "error in node map remove" << endl;

    auto node_iter = mem_iter->second.find(make_pair(serv_id, node));
    if (node_iter == mem_iter->second.end())
        cout << "error in node map remove" << endl;

    mem_iter->second.erase(node_iter);
    if (mem_iter->second.empty())
        core_iter->second.erase(mem_iter);
    if (core_iter->second.empty())
        serv_stats.node_core_mem_map.erase(core_iter);
}

void ServStat::node_map_add(int serv_id, int node)
{
    if (node > 1)
        cout << "error in node map add" << endl;

    int node_core = serv_stats[serv_id].nodes[node].cores_ream;
    int node_mem = serv_stats[serv_id].nodes[node].mem_ream;

    serv_stats.node_core_mem_map[node_core][node_mem].emplace(serv_id, node);
}

void ServStat::serv_map_remove(int serv_id)
{
    int min_node_core = min(serv_stats[serv_id].nodes[0].cores_ream, serv_stats[serv_id].nodes[1].cores_ream);
    int min_node_mem = min(serv_stats[serv_id].nodes[0].mem_ream, serv_stats[serv_id].nodes[1].mem_ream);

    auto core_iter = serv_stats.serv_core_mem_map.find(min_node_core);
    if (core_iter == serv_stats.serv_core_mem_map.end())
        cout << "error in serv map remove" << endl;
    
    auto mem_iter = core_iter->second.find(min_node_mem);
    if (mem_iter == core_iter->second.end())
        cout << "error in serv map remove" << endl;

    auto serv_iter = mem_iter->second.find(serv_id);
    if (serv_iter == mem_iter->second.end())
        cout << "error in serv map remove" << endl;

    mem_iter->second.erase(serv_iter);
    if (mem_iter->second.empty())
        core_iter->second.erase(mem_iter);
    if (core_iter->second.empty())
        serv_stats.serv_core_mem_map.erase(core_iter);
}

void ServStat::serv_map_add(int serv_id)
{
    int min_node_core = min(serv_stats[serv_id].nodes[0].cores_ream, serv_stats[serv_id].nodes[1].cores_ream);
    int min_node_mem = min(serv_stats[serv_id].nodes[0].mem_ream, serv_stats[serv_id].nodes[1].mem_ream);
    serv_stats.serv_core_mem_map[min_node_core][min_node_mem].emplace(serv_id);
}

void ServStat::nnode_map_remove(int serv_id, int node)
{
    if (node > 1)
        cout << "error in node map remove" << endl;

    int node_core = serv_stats[serv_id].nodes[node].cores_ream;
    int node_mem = serv_stats[serv_id].nodes[node].mem_ream;

    auto core_iter = serv_stats.nnode_core_mem_map.find(node_core);
    if (core_iter == serv_stats.nnode_core_mem_map.end())
        cout << "error in node map remove" << endl;
    
    auto mem_iter = core_iter->second.find(node_mem);
    if (mem_iter == core_iter->second.end())
        cout << "error in node map remove" << endl;

    auto node_iter = mem_iter->second.find(make_pair(serv_id, node));
    if (node_iter == mem_iter->second.end())
        cout << "error in node map remove" << endl;

    mem_iter->second.erase(node_iter);
    if (mem_iter->second.empty())
        core_iter->second.erase(mem_iter);
    if (core_iter->second.empty())
        serv_stats.nnode_core_mem_map.erase(core_iter);
}

void ServStat::nnode_map_add(int serv_id, int node)
{
    if (node > 1)
        cout << "error in node map add" << endl;

    int node_core = serv_stats[serv_id].nodes[node].cores_ream;
    int node_mem = serv_stats[serv_id].nodes[node].mem_ream;

    serv_stats.nnode_core_mem_map[node_core][node_mem].emplace(serv_id, node);
}

void ServStat::nserv_map_remove(int serv_id)
{
    int min_node_core = min(serv_stats[serv_id].nodes[0].cores_ream, serv_stats[serv_id].nodes[1].cores_ream);
    int min_node_mem = min(serv_stats[serv_id].nodes[0].mem_ream, serv_stats[serv_id].nodes[1].mem_ream);

    auto core_iter = serv_stats.nserv_core_mem_map.find(min_node_core);
    if (core_iter == serv_stats.nserv_core_mem_map.end())
        cout << "error in serv map remove" << endl;
    
    auto mem_iter = core_iter->second.find(min_node_mem);
    if (mem_iter == core_iter->second.end())
        cout << "error in serv map remove" << endl;

    auto serv_iter = mem_iter->second.find(serv_id);
    if (serv_iter == mem_iter->second.end())
        cout << "error in serv map remove" << endl;

    mem_iter->second.erase(serv_iter);
    if (mem_iter->second.empty())
        core_iter->second.erase(mem_iter);
    if (core_iter->second.empty())
        serv_stats.nserv_core_mem_map.erase(core_iter);
}

void ServStat::nserv_map_add(int serv_id)
{
    int min_node_core = min(serv_stats[serv_id].nodes[0].cores_ream, serv_stats[serv_id].nodes[1].cores_ream);
    int min_node_mem = min(serv_stats[serv_id].nodes[0].mem_ream, serv_stats[serv_id].nodes[1].mem_ream);
    serv_stats.nserv_core_mem_map[min_node_core][min_node_mem].emplace(serv_id);
}




