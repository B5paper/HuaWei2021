#include "status.h"
#include <iostream>
#include "linopt.h"
#include <unordered_map>
#include <algorithm>
#include <set>

// NodeStat::NodeStat(const NodeStat &obj)
// {
//     cores_used = obj.cores_used;
//     mem_used = obj.mem_used;
// }

map<int, vector<pair<int, int>>> ream;
map<int, vector<int>> dream;
int ream_max = 0;
int dream_max = 0;
// unordered_map<int, pair<int, int>> core_to_node;
// unordered_map<int, pair<int, int>> mem_to_node;

map<int, vector<pair<int, int>>> serv_core_mem_map;
map<int, vector<pair<int, int>>> serv_core_map;
map<int, vector<pair<int, int>>> serv_mem_map;
map<int, vector<int>> dserv_core_mem_map;
map<int, vector<int>> dserv_core_map;
map<int, vector<int>> dserv_mem_map;
map<int, vector<int>> vm_core_mem_map;
map<int, vector<int>> vm_core_map;
map<int, vector<int>> vm_mem_map;
map<int, vector<int>> dvm_core_mem_map;
map<int, vector<int>> dvm_core_map;
map<int, vector<int>> dvm_mem_map;

map<int, int> vm_id_to_serv_id;


ServStat::ServStat(const ServStat &obj)

{
    id = obj.id;
    type = obj.type;
    group = obj.group;
    for (int i = 0; i < 2; ++i)
    {
        // nodes[i].cores_used = obj.nodes[i].cores_used;
        // nodes[i].mem_used = obj.nodes[i].mem_used;
        nodes[i].cores_ream = obj.nodes[i].cores_ream;
        nodes[i].mem_ream = obj.nodes[i].mem_ream;
    }
    vms = obj.vms;
}

void ServStat::operator= (ServStat &obj)
{
    id = obj.id;
    type = obj.type;
    group = obj.group;
    for (int i = 0; i < 2; ++i)
    {
        // nodes[i].cores_used = obj.nodes[i].cores_used;
        // nodes[i].mem_used = obj.nodes[i].mem_used;
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
        // nodes[i].cores_used = obj.nodes[i].cores_used;
        // nodes[i].mem_used = obj.nodes[i].mem_used;
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

// int ServStat::cores_ream(int serv_node, ServSpecList &serv_specs)
// {
//     return serv_specs[type].cores / 2 - nodes[serv_node].cores_used;
// }

// int ServStat::mem_ream(int serv_node, ServSpecList &serv_specs)
// {
//     return serv_specs[type].memcap / 2 - nodes[serv_node].mem_used;
// }


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

    // for (auto serv_iter = servs.begin(); serv_iter != servs.end(); ++serv_iter)
    // {
    //     auto vm_iter = serv_iter->second.vms.find(vm_id);
    //     if (vm_iter != serv_iter->second.vms.end())
    //         return serv_iter->second;
    // }
    // cout << "error in get_serv_by_vm_id" << endl;
}

void ServStat::del_vm(int vm_id, VMSpecList &vm_specs)
{

    auto vm_iter = vms.find(vm_id);
    if (vm_iter == vms.end())
        cout << "error in del_vm()" << endl;
    int vm_type = vm_iter->second.type;
    int vm_node = vm_iter->second.node;
   
    clear_hash_state(vm_node);
    clear_vm_hash_state(vm_id, vm_specs);

    // 改变当前节点的状态        
    _change_serv_stat(0, vm_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);
    vms.erase(vm_id);

    update_hash_state(vm_node);

    auto iter = vm_id_to_serv_id.find(vm_id);
    if (iter == vm_id_to_serv_id.end())
        cout << "error in del_vm" << endl;
    vm_id_to_serv_id.erase(iter);
}

void ServStat::add_vm(int vm_type, int vm_id, int serv_node, VMSpecList &vm_specs)
{
    // if (vms.find(vm_id) != vms.end())
    //     cout << "error in add_vm()" << endl;

    // 检查能否成功，若超时可以将其删去
    // if (!can_hold_vm(vm_specs[vm_type].cores, vm_specs[vm_type].memcap, serv_node))
    //     cout << "error in add_vm()" << endl;

    if (vms.size() == 0)
    {
        group = vm_specs[vm_type].nodes;
    }
        
    vms[vm_id] = VMTypeNode(vm_type, serv_node);
 
    // 先删除本节点之前的状态
    clear_hash_state(serv_node);

    // 改变本节点的状态
    _change_serv_stat(1, serv_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);

    update_hash_state(serv_node);
    update_vm_hash_state(vm_id, vm_type, vm_specs);

    vm_id_to_serv_id.insert(make_pair(vm_id, id));
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

void ServStatList::mig_vm(int vm_id, int serv_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    ServStat &serv_stat = get_serv_by_vm_id(vm_id);
    int vm_type = serv_stat.vms[vm_id].type;
    serv_stat.del_vm(vm_id, vm_specs);
    servs[serv_id].add_vm(vm_type, vm_id, serv_node, vm_specs);
}

// void ServStatList::add_serv(int id, int type, ServSpecList &serv_specs)
// {
//     servs.insert(make_pair(id, ServStat(type, serv_specs)));
// }

void ServStatList::mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node, VMSpecList &vm_specs)
{
    // if (servs.find(old_serv_id) == servs.end() || servs.find(new_serv_node) == servs.end())
    //     cout << "error in mig_vm" << endl;
    ServStat &serv_stat = servs[old_serv_id];
    int vm_type = serv_stat.vms[vm_id].type;
    serv_stat.del_vm(vm_id, vm_specs);
    servs[new_serv_id].add_vm(vm_type, vm_id, new_serv_node, vm_specs);
}

bool ServStat::is_full()
{
    if (nodes[0].cores_ream == 0 && nodes[0].mem_ream == 0 
        && nodes[1].cores_ream == 0 && nodes[1].mem_ream == 0)
    {
        return true;
    }
    
    // if (serv_specs[type].cores == nodes[0].cores_used + nodes[1].cores_used)
    //     return true;
    // if (serv_specs[type].memcap == nodes[0].mem_used + nodes[1].mem_used)
    //     return true;
    return false;
}

bool ServStat::is_empty()
{
    // if (nodes[0].cores_used == 0 && nodes[0].mem_used == 0 
    // && nodes[1].cores_used == 0 && nodes[1].mem_used == 0)
        return true;
    return false;
}

void ServStat::_erase_serv_map(map<int, vector<pair<int, int>>> &serv_map, char res_type, int node)
{
    // res_type: type of resources, 'c' for core, 'm' for memory, 'b' for both core and memory
    int res;
    switch (res_type)
    {
        case 'c':
        res = nodes[node].cores_ream;
        break;
        
        case 'm':
        res = nodes[node].mem_ream;
        break;

        case 'b':
        res = nodes[node].cores_ream + nodes[node].mem_ream;
        break;

        default:
        cout << "error in erase serv map" << endl;
    }

    // 按道理应该一定能找到才对
    auto iter = serv_map.find(res);
    if (iter == serv_map.end())
    {
        cout << "error in erase serv map, can't find res." << endl;
    }

    for (int i = 0; i < iter->second.size(); ++i)
    {
        if (iter->second[i].first == id && iter->second[i].second == node)
        {
            iter->second.erase(iter->second.begin() + i);
            break;
        }
    }

    if (iter->second.empty())
        serv_map.erase(iter);
}

void ServStat::_erase_dserv_map(map<int, vector<int>> &dserv_map, char res_type)
{
    // res_type: type of resources, 'c' for core, 'm' for memory, 'b' for both core and memory
    int res;
    switch (res_type)
    {
        case 'c':
        res = nodes[0].cores_ream + nodes[1].cores_ream;
        break;

        case 'm':
        res = nodes[0].mem_ream + nodes[1].mem_ream;
        break;

        case 'b':
        res = nodes[0].cores_ream + nodes[0].mem_ream + nodes[1].cores_ream + nodes[1].mem_ream;
        break;

        default:
        cout << "error in erase dserv map" << endl;
    }

    auto diter = dserv_map.find(res);
    if (diter == dserv_map.end())
    {
        cout << "error in erase dserv map, can't find res" << endl;
    }

    for (int i = 0; i < diter->second.size(); ++i)
    {
        if (diter->second[i] == id)
        {
            diter->second.erase(diter->second.begin() + i);
            break;
        }
    }

    if (diter->second.empty())
        dserv_map.erase(diter);
}

void ServStat::_erase_vm_map(map<int, vector<int>> &vm_map, int res)
{
    auto iter = vm_map.find(res);
    if (iter == vm_map.end())
    {
        cout << "error in erase vm map" << endl;
    }

    for (int i = 0; i < iter->second.size(); ++i)
    {
        if (iter->second[i] == id)  // 此虚拟机在当前服务器中
        {
            iter->second.erase(iter->second.begin() + i);
            break;
        }
    }

    if (iter->second.empty())
        vm_map.erase(iter);
}

void ServStat::clear_hash_state(int node)
{
    if (node < 2)
    {
        _erase_serv_map(serv_core_map, 'c', node);
        _erase_serv_map(serv_mem_map, 'm', node);
        _erase_serv_map(serv_core_mem_map, 'b', node);
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            _erase_serv_map(serv_core_map, 'c', i);
            _erase_serv_map(serv_mem_map, 'm', i);
            _erase_serv_map(serv_core_mem_map, 'b', i);
        }
    }
    _erase_dserv_map(dserv_core_map, 'c');
    _erase_dserv_map(dserv_mem_map, 'm');
    _erase_dserv_map(dserv_core_mem_map, 'b');

    if (node < 2)
    {
        // 删除单节点的状态
        auto iter = ream.find(nodes[node].cores_ream + nodes[node].mem_ream);
        if (iter != ream.end())
        {
            for (int i = 0; i < iter->second.size(); ++i)
            {
                if (iter->second[i].first == id && iter->second[i].second == node)
                {
                    iter->second.erase(iter->second.begin() + i);
                    break;
                }
            }
            if (iter->second.empty())
                ream.erase(iter);
        }       
    }
    else
    {
        for (int k = 0; k < 2; ++k)
        {
            auto iter = ream.find(nodes[k].cores_ream + nodes[k].mem_ream);
            if (iter != ream.end())
            {
                for (int i = 0; i < iter->second.size(); ++i)
                {
                    if (iter->second[i].first == id && iter->second[i].second == k)
                    {
                        iter->second.erase(iter->second.begin() + i);
                        break;
                    }
                }
                if (iter->second.empty())
                    ream.erase(iter);
            }
        }
    }

    // 删除双节点的状态
    auto diter = dream.find(nodes[0].cores_ream + nodes[0].mem_ream + nodes[1].cores_ream + nodes[1].mem_ream);
    if (diter != dream.end())
    {
        for (int i = 0; i < diter->second.size(); ++i)
        {
            if (diter->second[i] == id)
            {
                diter->second.erase(diter->second.begin() + i);
                break;
            }
        }
        if (diter->second.empty())
            dream.erase(diter);
    }
}

void ServStat::update_hash_state(int node)
{
    if (node < 2)
    {
        _update_serv_map(serv_core_map, 'c', node);
        _update_serv_map(serv_mem_map, 'm', node);
        _update_serv_map(serv_core_mem_map, 'b', node);
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            _update_serv_map(serv_core_map, 'c', i);
            _update_serv_map(serv_mem_map, 'm', i);
            _update_serv_map(serv_core_mem_map, 'b', i);
        }
        
    }

    _update_dserv_map(dserv_core_map, 'c');
    _update_dserv_map(dserv_mem_map, 'm');
    _update_dserv_map(dserv_core_mem_map, 'b');
    
    // 对于单节点的操作，改变单节点和双节点的状态
    if (node < 2)
    {
        ream[nodes[node].cores_ream + nodes[node].mem_ream].push_back(make_pair(id, node));
    }

    // 对于双节点的操作，双节点的状态和两个单节点的状态都要改变
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            ream[nodes[i].cores_ream + nodes[i].mem_ream].push_back(make_pair(id, i));
        }
    }
    dream[nodes[0].cores_ream + nodes[0].mem_ream + nodes[1].cores_ream + nodes[1].mem_ream].push_back(id);
}

void ServStat::_update_serv_map(map<int, vector<pair<int, int>>> &serv_map, char res_type, int node)
{
    int res;
    switch (res_type)
    {
        case 'c':
        res = nodes[node].cores_ream;
        break;

        case 'm':
        res = nodes[node].mem_ream;
        break;

        case 'b':
        res = nodes[node].cores_ream + nodes[node].mem_ream;
        break;

        default:
        cout << "erro in update serv map" << endl;
    }

    serv_map[res].push_back(make_pair(id, node));
}

void ServStat::_update_dserv_map(map<int, vector<int>> &dserv_map, char res_type)
{
    int res;
    switch (res_type)
    {
        case 'c':
        res = nodes[0].cores_ream + nodes[1].cores_ream;
        break;

        case 'm':
        res = nodes[0].mem_ream + nodes[1].mem_ream;
        break;

        case 'b':
        res = nodes[0].cores_ream + nodes[0].mem_ream + nodes[1].cores_ream + nodes[1].mem_ream;
        break;

        default:
        cout << "error in update dserv map" << endl; 
    }

    dserv_map[res].push_back(id);
}


void ServStat::_update_vm_map(map<int, vector<int>> &vm_map, int vm_id, int res)
{
    vm_map[res].push_back(vm_id);
}

set<int> ServStatList::find_vm(int res, char type, int node)
{
    map<int, vector<int>>::iterator iter;
    bool found = false;
    switch (type)
    {
        case 'c':
        iter = vm_core_map.find(res);
        if (iter != vm_core_map.end())
            found = true;
        break;

        case 'm':
        iter = vm_mem_map.find(res);
        if (iter != vm_mem_map.end())
            found = true;
        break;

        case 'b':
        iter = vm_core_mem_map.find(res);
        if (iter != vm_core_mem_map.end())
            found = true;
        break;

        default:
        cout << "error in find_vm()" << endl;
    }

    set<int> vm_ids;
    if (!found)
        return vm_ids;

    int vm_id;
    switch (node)
    {
        case 1:
        for (int i = 0; i < iter->second.size(); ++i)
        {
            vm_id = iter->second[i];
            ServStat &serv_stat = get_serv_by_vm_id(vm_id);
            if (get_serv_by_vm_id(vm_id).vms[vm_id].node != 2)
                vm_ids.insert(vm_id);
        }
        break;

        case 2:
        for (int i = 0; i < iter->second.size(); ++i)
        {
            vm_id = iter->second[i];
            if (get_serv_by_vm_id(vm_id).vms[vm_id].node == 2)
                vm_ids.insert(vm_id);
        }
        break;

        case 3:
        for (int i = 0; i < iter->second.size(); ++i)
        {
            vm_ids.insert(iter->second[i]);
        }
        break;

        default:
        cout << "error in find_vm()" << endl;
    }
    
    return vm_ids;
}

set<int> ServStatList::find_vm(int core, int mem, int node)
{
    map<int, vector<int>>::iterator iter;
    bool found = false;

    set<int> core_vm_ids, mem_vm_ids, vm_ids;
    
    iter = vm_core_map.find(core);
    if (iter != vm_core_map.end())
    {
        for (int i = 0; i < iter->second.size(); ++i)
        {
            core_vm_ids.insert(iter->second[i]);
        }
    }

    iter = vm_mem_map.find(mem);
    if (iter != vm_mem_map.end())
    {
        for (int i = 0; i < iter->second.size(); ++i)
        {
            mem_vm_ids.insert(iter->second[i]);
        }
    }

    set_intersection(core_vm_ids.begin(), core_vm_ids.end(), mem_vm_ids.begin(), mem_vm_ids.end(), 
        inserter(vm_ids, vm_ids.begin()));
    
    int vm_node;
    switch (node)
    {
        case 1:
        for (auto iter = vm_ids.begin(); iter != vm_ids.end();)
        {
            ServStat &serv_stat = get_serv_by_vm_id(*iter);
            vm_node = get_serv_by_vm_id(*iter).vms[*iter].node;
            if (vm_node == 2)
            {
                vm_ids.erase(iter++);
                continue;
            }
            ++iter;
        }
        break;

        case 2:            
        for (auto iter = vm_ids.begin(); iter != vm_ids.end();)
        {
            vm_node = get_serv_by_vm_id(*iter).vms[*iter].node;
            if (vm_node != 2)
            {
                vm_ids.erase(iter++);
                continue;
            }
            ++iter;
        }
        break;

        case 3:
        // do nothing
        break;

        default:
        cout << "error in find_vm()" << endl;
    }

    return vm_ids;
}

void ServStat::clear_vm_hash_state(int vm_id, VMSpecList &vm_specs)
{
    int vm_type = vms[vm_id].type;
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;

    auto iter = vm_core_map.find(vm_core);
    if (iter != vm_core_map.end())
    {
        for (int i = 0; i < iter->second.size(); ++i)
        {
            if (iter->second[i] == vm_id)
            {
                iter->second.erase(iter->second.begin() + i);
                break;
            }
        }
    }

    iter = vm_mem_map.find(vm_mem);
    if (iter != vm_mem_map.end())
    {
        for (int i = 0; i < iter->second.size(); ++i)
        {
            if (iter->second[i] == vm_id)
            {
                iter->second.erase(iter->second.begin() + i);
                break;
            }
        }
    }

    iter = vm_core_mem_map.find(vm_core + vm_mem);
    if (iter != vm_core_mem_map.end())
    {
        for (int i = 0; i < iter->second.size(); ++i)
        {
            if (iter->second[i] == vm_id)
            {
                iter->second.erase(iter->second.begin() + i);
                break;
            }
        }
    }
}

void ServStat::update_vm_hash_state(int vm_id, int vm_type, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores;
    int vm_mem = vm_specs[vm_type].memcap;
    
    vm_core_map[vm_core].push_back(vm_id);
    vm_mem_map[vm_mem].push_back(vm_id);
    vm_core_mem_map[vm_core + vm_mem].push_back(vm_id);
}

set<int> ServStatList::find_serv(int res, char type, int node, bool except_full, bool except_empty)  // (serv_id)
{
    set<int> serv_ids;
    map<int, vector<pair<int, int>>>::iterator siter;  // iter for single node resources
    map<int, vector<int>>::iterator diter;  // iter for two node resources
    switch (node)
    {
        case 1:
        switch (type)
        {
            case 'c':
            siter = serv_core_map.find(res);
            break;

            case 'm':
            siter = serv_mem_map.find(res);
            break;

            case 'b':
            siter = serv_core_mem_map.find(res);
            break;

            default:
            cout << "error in find serv" << endl;
        }
        break;

        case 2:
        switch (type)
        {
            case 'c':
            diter = dserv_core_map.find(res);
            break;

            case 'm':
            diter = dserv_mem_map.find(res);
            break;

            case 'b':
            diter = dserv_core_mem_map.find(res);
            break;

            default:
            cout << "error in find serv" << endl;
        }
        break;

        default:
        cout << "error in find_serv" << endl;
    }

    switch (node)
    {
        case 1:
        for (int i = 0; i < siter->second.size(); ++i)
        {
            ServStat &serv_stat = servs[siter->second[i].first];
            // if (except_full)
            //     if (serv_stat.nodes[0].cores_ream == 0 && 
            serv_ids.insert(serv_stat.id);
        }
        break;

        case 2:
        for (int i = 0; i < diter->second.size(); ++i)
        {
            ServStat &serv_stat = servs[diter->second[i]];
            serv_ids.insert(serv_stat.id);
        }
        break;

        default:
        cout << "error in find_serv" << endl;
    }

    return serv_ids;
}






