#include "types.h"

unordered_map<string, int> server_names_map;
int server_names_id_top = 0;
vector<string> server_names_vec;

unordered_map<string, int> vm_names_map;
int vm_names_id_top = 0;
vector<string> vm_names_vec;

bool ServerSpecList::can_hold_vm(int server_type, int vm_type, ServerSpecList &server_specs, VMSpecList &vm_specs)
{
    if (vm_specs.nodes[vm_type] == 1)
    {
        if (server_specs.cores[server_type] / 2 > vm_specs.cores[vm_type] &&
            server_specs.memcap[server_type] / 2 > vm_specs.memcap[vm_type])
            return true;
    }
    else
    {
        if (server_specs.cores[server_type] > vm_specs.cores[vm_type] &&
            server_specs.memcap[server_type] > vm_specs.memcap[vm_type])
            return true;
    }
    return false;
}