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

// ServerStat::~ServerStat()
// {
//     for (int i = 0; i < 2; ++i)
//         for (int j = 0; j < nodes[i].size(); ++j)
//             delete nodes[i][j];
// }

// ServerStatRecorder::ServerStatRecorder()
// : current_day(0)
// {

// }


// void ServerStatRecorder::update_one_day(ServerStat &server_stat)
// {
//     for (int i = run_days.size(); i < server_stat.ids.size(); ++i)
//     {
//         types.push_back(server_stat.types[i]);
//         ids.push_back(server_stat.ids[i]);
//         run_days.push_back(new vector<int>);
//     }

//     for (int i = 0; i < server_stat.ids.size(); ++i)
//     {
//         if (server_stat.cores_used[i] != 0 || server_stat.mem_used[i] != 0)   // if (server_stat.is_running[i])
//         {
//             run_days[i]->push_back(current_day);
//         }
//     }

//     ++current_day;
// }


// ServerStatRecorder::~ServerStatRecorder()
// {
//     for (int i = 0; i < run_days.size(); ++i)
//     {
//         delete run_days[i];
//     }
// }

ServerStat::ServerStat(int type)
: type(type)
{
    cores_used = 0;
    mem_used = 0;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = 0;
        nodes[i].mem_used = 0;
    }
    is_running = false;
}

ServerStat::ServerStat(const ServerStat &obj)
{
    type = obj.type;
    cores_used = obj.cores_used;
    mem_used = obj.mem_used;
    is_running = obj.is_running;

    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = obj.nodes[i].cores_used;
        nodes[i].mem_used = obj.nodes[i].mem_used;
    }
}

void ServerStat::operator= (ServerStat &obj)
{
    type = obj.type;
    cores_used = obj.cores_used;
    mem_used = obj.mem_used;
    is_running = obj.is_running;
    for (int i = 0; i < 2; ++i)
    {
        nodes[i].cores_used = obj.nodes[i].cores_used;
        nodes[i].mem_used = obj.nodes[i].mem_used;
    }
}

bool ServerStat::can_hold_vm(int vm_core, int vm_mem, int server_node, ServerSpecList &server_specs)
{
    int server_core = server_specs.cores[type];
    int server_mem = server_specs.memcap[type];

    switch (server_node)
    {
        case 0:
        case 1:
        if (server_core / 2 - nodes[server_node].cores_used < vm_core)
            return false;
        if (server_mem / 2 - nodes[server_node].mem_used < vm_mem)
            return false;
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            if (server_core / 2 - nodes[i].cores_used < vm_core / 2)
                return false;
            if (server_mem / 2 - nodes[i].mem_used < vm_mem / 2)
                return false;
        }
        break;

        default:
        cout << "error server node" << endl;       
    }
    return true;
}

void ServerStatList::add_server(int server_id, int server_type)
{
    servs.insert(make_pair(server_id, ServerStat(server_type)));
}

bool ServerStatList::can_hold_vm(int vm_type, int server_id, int server_node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (servs.find(server_id) == servs.end())
    {
        cout << "server does not exist." << endl;
    }
    switch (server_node)
    {
        case 0:
        case 1:
        if (server_specs.cores[servs[server_id].type] / 2 - servs[server_id].nodes[server_node].cores_used < vm_specs.cores[vm_type])
            return false;
        if (server_specs.memcap[servs[server_id].type] / 2 - servs[server_id].nodes[server_node].mem_used < vm_specs.memcap[vm_type])
            return false;
        break;

        case 2:
        for (int i = 0; i < 2; ++i)
        {
            int serv_spec_cores = server_specs.cores[servs[server_id].type];
            int cores_used = servs[server_id].nodes[i].cores_used;
            int vm_spec_cores = vm_specs.cores[vm_type];

            int serv_spec_mem = server_specs.memcap[servs[server_id].type];
            int mem_used = servs[server_id].nodes[i].mem_used;
            int vm_spec_mem = vm_specs.memcap[vm_type];

            if (serv_spec_cores / 2 - cores_used < vm_spec_cores / 2)
                return false;
            if (serv_spec_mem / 2 - mem_used < vm_spec_mem / 2)
                return false;
        }
        break;
    }
    return true;
} 

void ServerStatList::_change_node_stat(int op, int server_id, int core, int mem, int node)
{
    int server_idx = server_id;

    if (op == 0)  // reduce resources
    {
        switch (node)
        {
            case 0:
            case 1:
            servs[server_idx].nodes[node].cores_used -= core;
            servs[server_idx].nodes[node].mem_used -= mem;
            break;

            case 2:
            for (int i = 0; i < 2; ++i)
            {
                servs[server_idx].nodes[i].cores_used -= core / 2;
                servs[server_idx].nodes[i].mem_used -= mem / 2; 
            }
        }
    }

    if (op == 1)  // occupy resources
    {
        switch (node)
        {
            case 0:
            case 1:
            servs[server_idx].nodes[node].cores_used += core;
            servs[server_idx].nodes[node].mem_used += mem;
            break;

            case 2:
            for (int i = 0; i < 2; ++i)
            {
                servs[server_idx].nodes[i].cores_used += core / 2;
                servs[server_idx].nodes[i].mem_used += mem / 2;
            }
            break;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        if (servs[server_idx].nodes[i].cores_used < 0 || servs[server_idx].nodes[i].mem_used < 0)
            cout << "error" << endl;
    }
}

void ServerStatList::_change_server_stat(int op, int server_id, int core, int mem, int node)
{
    if (op == 0)  // reduce resources
    {
        servs[server_id].cores_used -= core;
        servs[server_id].mem_used -= mem;
        if (servs[server_id].cores_used == 0)
            servs[server_id].is_running = false;
    }

    if (op == 1)  // occupy resources
    {
        servs[server_id].cores_used += core;
        servs[server_id].mem_used += mem;
        servs[server_id].is_running = true;
    }

    _change_node_stat(op, server_id, core, mem, node);
}

bool ServerStatList::add_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (!can_hold_vm(vm_type, server_id, node, server_specs, vm_specs))
        return false;
    _change_server_stat(1, server_id, vm_specs.cores[vm_type], vm_specs.memcap[vm_type], node);
    return true;
}

void ServerStatList::del_vm(int vm_type, int server_id, int node, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (servs.find(server_id) == servs.end())
        cout << "error" << endl;
    _change_server_stat(0, server_id, vm_specs.cores[vm_type], vm_specs.memcap[vm_type], node);
}
