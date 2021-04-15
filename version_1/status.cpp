#include "status.h"

ServerStat::~ServerStat()
{
    for (int i = 0; i < node_a.size(); ++i)
    {
        delete node_a[i];
        delete node_b[i];
    }
}

void ServerStat::clear()
{
    types.clear();
    ids.clear();
    cores_used.clear();
    cores_ream.clear();
    mem_used.clear();
    mem_ream.clear();
    for (int i = 0; i < node_a.size(); ++i)
    {
        delete node_a[i];
        delete node_b[i];
    }
    node_a.clear();
    node_b.clear();
}

void ServerStat::reset_stat(ServerSpecList &server_specs)
{
    for (int i = 0; i < cores_used.size(); ++i)
    {
        cores_used[i] = 0;
        cores_ream[i] = server_specs.cores[types[i]];  // require the server_specs to store servers as order 
        mem_used[i] = 0;
        mem_ream[i] = server_specs.memcap[types[i]];  // the same as above
        node_a[i]->cores_used = 0;
        node_a[i]->cores_ream = cores_ream[i] / 2;
        node_a[i]->mem_used = 0;
        node_a[i]->mem_ream = mem_ream[i] / 2;
        node_b[i]->cores_used = 0;
        node_b[i]->cores_ream = cores_ream[i] / 2;
        node_b[i]->mem_used = 0;
        node_b[i]->mem_ream = mem_ream[i] / 2;
    }
}

void ServerStat::update_stat(VMStat &vm_stat, VMSpecList &vm_specs)
{
    int idx = 0;
    int vm_type = 0;
    for (int i = 0; i < vm_stat.types.size(); ++i)
    {
        for (int j = 0; j < ids.size(); ++j)
        {
            if (ids[j] == vm_stat.server_ids[i])
                idx = j;
        }
        vm_type = vm_stat.types[i];

        cores_used[idx] += vm_specs.cores[vm_type];
        cores_ream[idx] -= vm_specs.cores[vm_type];
        mem_used[idx] += vm_specs.memcap[vm_type];
        mem_ream[idx] -= vm_specs.memcap[vm_type];

        switch (vm_stat.nodes[i])
        {
            case 0:  // node A
            node_a[idx]->cores_used += vm_specs.cores[vm_type];
            node_a[idx]->cores_ream -= vm_specs.cores[vm_type];
            node_a[idx]->mem_used += vm_specs.memcap[vm_type];
            node_a[idx]->mem_ream -= vm_specs.memcap[vm_type];

            case 1:  // node B
            node_b[idx]->cores_used += vm_specs.cores[vm_type];
            node_b[idx]->cores_ream -= vm_specs.cores[vm_type];
            node_b[idx]->mem_used += vm_specs.memcap[vm_type];
            node_b[idx]->mem_ream -= vm_specs.memcap[vm_type];

            case 2:  // both
            node_a[idx]->cores_used += vm_specs.cores[vm_type] / 2;
            node_a[idx]->cores_ream -= vm_specs.cores[vm_type] / 2;
            node_a[idx]->mem_used += vm_specs.memcap[vm_type] / 2;
            node_a[idx]->mem_ream -= vm_specs.memcap[vm_type] / 2;

            node_b[idx]->cores_used += vm_specs.cores[vm_type] / 2;
            node_b[idx]->cores_ream -= vm_specs.cores[vm_type] / 2;
            node_b[idx]->mem_used += vm_specs.memcap[vm_type] / 2;
            node_b[idx]->mem_ream -= vm_specs.memcap[vm_type] / 2;
        }
    }
}

void ServerStat::update_stat(const VMSingle &vm, VMSpecList &vm_specs)
{
    int idx = _get_idx_by_id(vm.server_id);
    int vm_type = vm.type;

    cores_used[idx] += vm_specs.cores[vm_type];
    cores_ream[idx] -= vm_specs.cores[vm_type];
    mem_used[idx] += vm_specs.memcap[vm_type];
    mem_ream[idx] -= vm_specs.memcap[vm_type];

    switch (vm.node)
    {
        case 0:  // node A
        node_a[idx]->cores_used += vm_specs.cores[vm_type];
        node_a[idx]->cores_ream -= vm_specs.cores[vm_type];
        node_a[idx]->mem_used += vm_specs.memcap[vm_type];
        node_a[idx]->mem_ream -= vm_specs.memcap[vm_type];

        case 1:  // node B
        node_b[idx]->cores_used += vm_specs.cores[vm_type];
        node_b[idx]->cores_ream -= vm_specs.cores[vm_type];
        node_b[idx]->mem_used += vm_specs.memcap[vm_type];
        node_b[idx]->mem_ream -= vm_specs.memcap[vm_type];

        case 2:  // both
        node_a[idx]->cores_used += vm_specs.cores[vm_type] / 2;
        node_a[idx]->cores_ream -= vm_specs.cores[vm_type] / 2;
        node_a[idx]->mem_used += vm_specs.memcap[vm_type] / 2;
        node_a[idx]->mem_ream -= vm_specs.memcap[vm_type] / 2;

        node_b[idx]->cores_used += vm_specs.cores[vm_type] / 2;
        node_b[idx]->cores_ream -= vm_specs.cores[vm_type] / 2;
        node_b[idx]->mem_used += vm_specs.memcap[vm_type] / 2;
        node_b[idx]->mem_ream -= vm_specs.memcap[vm_type] / 2;
    }
}

int ServerStat::_get_idx_by_id(int id)
{
    for (int i = 0; i < ids.size(); ++i)
        if (ids[i] == id)
            return i;
    return -1;
}

void ServerStat::add(ServerSpec &server_spec)
{
    types.push_back(server_spec.name_id);
    ids.push_back(ids.size());
    cores_used.push_back(0);
    cores_ream.push_back(server_spec.cores);
    mem_used.push_back(0);
    mem_ream.push_back(server_spec.memcap);
    node_a.push_back(new NodeStat(0, server_spec.cores / 2, 0, server_spec.memcap / 2));
    node_b.push_back(new NodeStat(0, server_spec.cores / 2, 0, server_spec.memcap / 2));
}

void ServerStat::add(int server_type, ServerSpecList &server_specs)
{
    types.push_back(server_specs.name_id[server_type]);
    ids.push_back(ids.size());
    cores_used.push_back(0);
    cores_ream.push_back(server_specs.cores[server_type]);
    mem_used.push_back(0);
    mem_ream.push_back(server_specs.memcap[server_type]);
    node_a.push_back(new NodeStat(0, server_specs.cores[server_type] / 2, 0, server_specs.memcap[server_type] / 2));
    node_b.push_back(new NodeStat(0, server_specs.cores[server_type] / 2, 0, server_specs.memcap[server_type] / 2));
}


int ServerStat::get_cores_total()
{
    int num_cores = 0;
    for (int i = 0; i < types.size(); ++i)
        num_cores += cores_used[i] + cores_ream[i];
    return num_cores;
}

int ServerStat::get_cores_ream()
{
    int num_cores;
    for (int i = 0; i < types.size(); ++i)
        num_cores += cores_ream[i];
    return num_cores;
}

int ServerStat::get_mem_total()
{
    int num_mem;
    for (int i = 0; i < types.size(); ++i)
        num_mem += mem_used[i] + mem_ream[i];
    return num_mem;
}

int ServerStat::get_mem_ream()
{
    int num_mem;
    for (int i = 0; i < types.size(); ++i)
        num_mem += mem_used[i] + mem_ream[i];
    return num_mem;
}
