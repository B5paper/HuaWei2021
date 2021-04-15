#include "types.h"

unordered_map<string, int> server_names_map;
int server_names_id_top = 0;
vector<string> server_names_vec;

unordered_map<string, int> vm_names_map;
int vm_names_id_top = 0;
vector<string> vm_names_vec;

int min_core_vm_type = -1;
int min_mem_vm_type = -1;



bool ServSpec::can_hold_vm(int vm_type, VMSpecList &vm_specs)
{
    int vm_core = vm_specs[vm_type].cores * 1.4;
    int vm_mem = vm_specs[vm_type].memcap * 1.4;
    int vm_node = vm_specs[vm_type].nodes;
    
    if (vm_node == 1)
    {
        if (cores / 2 < vm_core || memcap / 2 < vm_mem)
            return false; 
    }

    if (vm_node == 2)
    {
        if (cores < vm_core || memcap < vm_mem)
            return false;
    }

    return true;
}