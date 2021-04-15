#include "status.h"
#include <iostream>


void VMStat::add_vm(int type, int vm_id, int server_id, int node)
{
    vms.insert(make_pair(vm_id, new VMSingle(type, server_id, node)));
}

void VMStat::del_vm(int vm_id)
{
    auto iter = vms.find(vm_id);
    delete iter->second;
    vms.erase(iter);
}

VMStat::~VMStat()
{
    for (auto iter = vms.begin(); iter != vms.end(); ++iter)
    {
        delete iter->second;
    }
}

ServerStat::~ServerStat()
{
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < nodes[i].size(); ++j)
            delete nodes[i][j];
}

void ServerStat::clear()
{
    types.clear();
    ids.clear();
    cores_used.clear();
    mem_used.clear();

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < nodes[i].size(); ++j)
            delete nodes[i][j];
        nodes[i].clear();
    }

    is_running.clear();
}

void ServerStat::reset_stat(ServerSpecList &server_specs)
{
    for (int i = 0; i < cores_used.size(); ++i)
    {
        cores_used[i] = 0;
        mem_used[i] = 0;
        for (int j = 0; j < 2; ++j)
        {
            nodes[j][i]->cores_used = 0;
            nodes[j][i]->mem_used = 0;
        }
        is_running[i] = false;
    }
}   

int ServerStat::_get_idx_by_id(int id)
{
    for (int i = 0; i < ids.size(); ++i)
        if (ids[i] == id)
            return i;
    return -1;
}

void ServerStat::add_server(int server_type, ServerSpecList &server_specs)
{
    types.push_back(server_type);
    ids.push_back(ids.size());
    cores_used.push_back(0);
    mem_used.push_back(0);
    for (int i = 0; i < 2; ++i)
        nodes[i].push_back(new NodeStat(0, 0));
    is_running.push_back(false);
}

void ServerStat::_change_server_stat(int op, int server_id, int core, int mem, int node)
{
    if (op == 0)  // reduce resources
    {
        cores_used[server_id] -= core;
        mem_used[server_id] -= mem;
        if (cores_used[server_id] == 0)
            is_running[server_id] = false;
    }

    if (op == 1)  // occupy resources
    {
        server_id = server_id;
        cores_used[server_id] += core;
        mem_used[server_id] += mem;
        is_running[server_id] = true;
    }

    _change_node_stat(op, server_id, core, mem, node);
}

void ServerStat::_change_node_stat(int op, int server_id, int core, int mem, int node)
{
    int server_idx = server_id;

    if (op == 0)  // reduce resources
    {
        switch (node)
        {
            case 0:
            case 1:
            nodes[node][server_idx]->cores_used -= core;
            nodes[node][server_idx]->mem_used -= mem;
            break;

            case 2:
            for (int i = 0; i < 2; ++i)
            {
                nodes[i][server_idx]->cores_used -= core / 2;
                nodes[i][server_idx]->mem_used -= mem / 2; 
            }
        }
    }

    if (op == 1)  // occupy resources
    {
        switch (node)
        {
            case 0:
            case 1:
            nodes[node][server_idx]->cores_used += core;
            nodes[node][server_idx]->mem_used += mem;
            break;

            case 2:
            for (int i = 0; i < 2; ++i)
            {
                nodes[i][server_idx]->cores_used += core / 2;
                nodes[i][server_idx]->mem_used += mem / 2; 
            }
            break;
        }
    }
}


bool ServerStat::can_hold_vm(int vm_type, int server_id, int server_node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    switch (server_node)
    {
        case 0:
        case 1:
        if (server_specs.cores[types[server_id]] / 2 - nodes[server_node][server_id]->cores_used < vm_specs.cores[vm_type])
            return false;
        if (server_specs.memcap[types[server_id]] / 2 - nodes[server_node][server_id]->mem_used < vm_specs.memcap[vm_type])
            return false;
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            if (server_specs.cores[types[server_id]] / 2 - nodes[i][server_id]->cores_used < vm_specs.cores[vm_type] / 2)
                return false;
            if (server_specs.memcap[types[server_id]] / 2 - nodes[i][server_id]->mem_used < vm_specs.memcap[vm_type] / 2)
                return false;
        }
        break;
    }
    return true;
}


bool ServerStat::add_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (!can_hold_vm(vm_type, server_id, node, server_specs, vm_specs))
        return false;
    _change_server_stat(1, server_id, vm_specs.cores[vm_type], vm_specs.memcap[vm_type], node);
    return true; 
}

bool ServerStat::del_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (cores_used[server_id] < vm_specs.cores[vm_type])
        return false;
    if (mem_used[server_id] < vm_specs.memcap[vm_type])
        return false;
    _change_server_stat(0, server_id, vm_specs.cores[vm_type], vm_specs.memcap[vm_type], node);
    return true;   
}

int ServerStat::get_cores_total(ServerSpecList &server_specs)
{
    int num_cores = 0;
    for (int i = 0; i < types.size(); ++i)
        num_cores += server_specs.cores[types[i]];
    return num_cores;
}

int ServerStat::get_cores_ream(ServerSpecList &server_specs)
{
    int num_cores;
    for (int i = 0; i < types.size(); ++i)
        num_cores += server_specs.cores[types[i]] - cores_used[i];
    return num_cores;
}

int ServerStat::get_mem_total(ServerSpecList &server_specs)
{
    int num_mem;
    for (int i = 0; i < types.size(); ++i)
        num_mem += server_specs.memcap[types[i]];
    return num_mem;
}

int ServerStat::get_mem_ream(ServerSpecList &server_specs)
{
    int num_mem;
    for (int i = 0; i < types.size(); ++i)
        num_mem += server_specs.memcap[types[i]] - mem_used[i];
    return num_mem;
}


ServerStatRecorder::ServerStatRecorder()
: current_day(0)
{

}


void ServerStatRecorder::update_one_day(ServerStat &server_stat)
{
    for (int i = run_days.size(); i < server_stat.ids.size(); ++i)
    {
        types.push_back(server_stat.types[i]);
        ids.push_back(server_stat.ids[i]);
        run_days.push_back(new vector<int>);
    }

    for (int i = 0; i < server_stat.ids.size(); ++i)
    {
        if (server_stat.cores_used[i] != 0 || server_stat.mem_used[i] != 0)   // if (server_stat.is_running[i])
        {
            run_days[i]->push_back(current_day);
        }
    }

    ++current_day;
}


ServerStatRecorder::~ServerStatRecorder()
{
    for (int i = 0; i < run_days.size(); ++i)
    {
        delete run_days[i];
    }
}