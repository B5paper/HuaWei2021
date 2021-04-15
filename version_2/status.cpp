#include "status.h"
#include <iostream>

int VMStat::_get_idx_by_id(int vm_id)
{
    for (int i = 0; i < types.size(); ++i)
    {
        if (vm_id == vm_ids[i])
            return i;
    }
    cout << "not found vm_id" << endl;
    return -1;
}

void VMStat::add_vm(int type, int vm_id, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    types.push_back(type);
    vm_ids.push_back(vm_id);
    server_ids.push_back(server_id);
    nodes.push_back(node);
}

void VMStat::del_vm(int vm_id)
{
    int vm_idx = _get_idx_by_id(vm_id);
    types.erase(types.begin() + vm_idx);
    vm_ids.erase(vm_ids.begin() + vm_idx);
    server_ids.erase(server_ids.begin() + vm_idx);
    nodes.erase(nodes.begin() + vm_idx);
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

void ServerStat::update_stat(VMStat &vm_stat, VMSpecList &vm_specs)
{
    int server_idx = 0;
    int vm_type = 0;
    for (int i = 0; i < vm_stat.types.size(); ++i)
    {
        server_idx = _get_idx_by_id(vm_stat.server_ids[i]);
        vm_type = vm_stat.types[i];

        cores_used[server_idx] += vm_specs.cores[vm_type];
        mem_used[server_idx] += vm_specs.memcap[vm_type];
        is_running[server_idx] = true;

        switch (vm_stat.nodes[i])
        {
            case 0:  // node A
            case 1:  // node B
            nodes[vm_stat.nodes[i]][server_idx]->cores_used += vm_specs.cores[vm_type];
            nodes[vm_stat.nodes[i]][server_idx]->mem_used += vm_specs.memcap[vm_type];
            break;

            case 2:  // both
            for (int j = 0; j < 2; ++j)
            {
                nodes[j][server_idx]->cores_used += vm_specs.cores[vm_type] / 2;
                nodes[j][server_idx]->mem_used += vm_specs.memcap[vm_type] / 2;
            }
        }
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
    types.push_back(server_specs.name_id[server_type]);
    ids.push_back(ids.size());
    cores_used.push_back(0);
    mem_used.push_back(0);
    for (int i = 0; i < 2; ++i)
        nodes[i].push_back(new NodeStat(0, 0));
    is_running.push_back(false);
}

void ServerStat::_change_server_stat(int op, int server_id, int core, int mem, int node)
{
    int server_idx = 0;

    if (op == 0)  // reduce resources
    {
        server_idx = _get_idx_by_id(server_id);
        cores_used[server_idx] -= core;
        mem_used[server_idx] -= mem;
        if (cores_used[server_idx] == 0)
            is_running[server_idx] = false;
    }

    if (op == 1)  // occupy resources
    {
        server_idx = _get_idx_by_id(server_id);
        cores_used[server_idx] += core;
        mem_used[server_idx] += mem;
        is_running[server_idx] = true;
    }

    _change_node_stat(op, server_id, core, mem, node);
}

void ServerStat::_change_node_stat(int op, int server_id, int core, int mem, int node)
{
    int server_idx = _get_idx_by_id(server_id);

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

bool ServerStat::add_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    int server_idx = _get_idx_by_id(server_id);
    if (server_specs.cores[types[server_idx]] - cores_used[server_idx] < vm_specs.cores[vm_type])
        return false;
    if (server_specs.memcap[types[server_idx]] - mem_used[server_idx] < vm_specs.memcap[vm_type])
        return false;
    if (server_specs.cores[types[server_idx]] - cores_used[server_idx])
    _change_server_stat(1, server_id, vm_specs.cores[vm_type], vm_specs.memcap[vm_type], node);
    return true; 
}

bool ServerStat::del_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    int server_idx = _get_idx_by_id(server_id);
    if (cores_used[server_idx] < vm_specs.cores[vm_type])
        return false;
    if (mem_used[server_idx] < vm_specs.memcap[vm_type])
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