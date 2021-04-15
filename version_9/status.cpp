#include "status.h"
#include <iostream>

NodeStat::NodeStat(const NodeStat &obj)
{
    cores_used = obj.cores_used;
    mem_used = obj.mem_used;
}


ServStat::ServStat(const ServStat &obj)
{
    type = obj.type;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = obj.nodes[i].cores_used;
        nodes[i].mem_used = obj.nodes[i].mem_used;
    }
    vms = obj.vms;
}

void ServStat::operator= (ServStat &obj)
{
    type = obj.type;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = obj.nodes[i].cores_used;
        nodes[i].mem_used = obj.nodes[i].mem_used;
    }
    vms = obj.vms;
}

void ServStat::operator= (ServStat const &obj)
{
    type = obj.type;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = obj.nodes[i].cores_used;
        nodes[i].mem_used = obj.nodes[i].mem_used;
    }
    vms = obj.vms;
}

bool ServStat::can_hold_vm(int vm_core, int vm_mem, int serv_node, ServSpecList &serv_specs)
{
    switch (serv_node)
    {
        case 0:
        case 1:
        if (cores_ream(serv_node, serv_specs) < vm_core)
            return false;
        if (mem_ream(serv_node, serv_specs) < vm_mem)
            return false;
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            if (cores_ream(i, serv_specs) < vm_core / 2)
                return false;
            if (mem_ream(i, serv_specs) < vm_mem / 2)
                return false;
        }
        break;

        default:
        cout << "error server node" << endl;       
    }
    return true;
}

int ServStat::cores_ream(int serv_node, ServSpecList &serv_specs)
{
    return serv_specs[type].cores / 2 - nodes[serv_node].cores_used;
}

int ServStat::mem_ream(int serv_node, ServSpecList &serv_specs)
{
    return serv_specs[type].memcap / 2 - nodes[serv_node].mem_used;
}


ServStat& ServStatList::operator[] (int id)
{
    return servs[id];
}

ServStat& ServStatList::get_serv_by_vm_id(int vm_id)
{
    for (auto serv_iter = servs.begin(); serv_iter != servs.end(); ++serv_iter)
    {
        auto vm_iter = serv_iter->second.vms.find(vm_id);
        if (vm_iter != serv_iter->second.vms.end())
            return serv_iter->second;
    }
    cout << "error in get_serv_by_vm_id" << endl;
}

void ServStat::del_vm(int vm_id, VMSpecList &vm_specs)
{
    auto vm_iter = vms.find(vm_id);
    if (vm_iter == vms.end())
        cout << "error in del_vm()" << endl;
    int vm_type = vm_iter->second.type;
    int vm_node = vm_iter->second.node;
    _change_serv_stat(0, vm_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);
    vms.erase(vm_id);
}

void ServStat::add_vm(int vm_type, int vm_id, int serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (vms.find(vm_id) != vms.end())
        cout << "error in add_vm()" << endl;

    // 检查能否成功，若超时可以将其删去
    if (!can_hold_vm(vm_specs[vm_type].cores, vm_specs[vm_type].memcap, serv_node, serv_specs))
        cout << "error in add_vm()" << endl;
        
    vms[vm_id] = VMTypeNode(vm_type, serv_node);
    _change_serv_stat(1, serv_node, vm_specs[vm_type].cores, vm_specs[vm_type].memcap);
}



void NodeStat::_change_node_stat(int op, int core, int mem)
{
    if (op == 0)  // release resources
    {
        cores_used -= core;
        mem_used -= mem;
    }

    if (op == 1)  // occupy resources
    {
        cores_used += core;
        mem_used += mem;
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
    servs[serv_id].add_vm(vm_type, vm_id, serv_node, serv_specs, vm_specs);
}

void ServStatList::add_serv(int id, int type)
{
    servs.insert(make_pair(id, ServStat(type)));
}

void ServStatList::mig_vm(int vm_id, int old_serv_id, int new_serv_id, int new_serv_node, ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (servs.find(old_serv_id) == servs.end() || servs.find(new_serv_node) == servs.end())
        cout << "error in mig_vm" << endl;
    ServStat &serv_stat = servs[old_serv_id];
    int vm_type = serv_stat.vms[vm_id].type;
    serv_stat.del_vm(vm_id, vm_specs);
    servs[new_serv_id].add_vm(vm_type, vm_id, new_serv_node, serv_specs, vm_specs);
}

bool ServStat::is_full(ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    if (serv_specs[type].cores == nodes[0].cores_used + nodes[1].cores_used)
        return true;
    if (serv_specs[type].memcap == nodes[0].mem_used + nodes[1].mem_used)
        return true;
    return false;
}

bool ServStat::is_empty()
{
    if (nodes[0].cores_used == 0 && nodes[0].mem_used == 0 
    && nodes[1].cores_used == 0 && nodes[1].mem_used == 0)
        return true;
    return false;
}
