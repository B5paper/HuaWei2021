#ifndef SELECT
#define SELECT

#include "status.h"
#include "migrate.h"


// OneAssignScheme mig_serv_select_vm_bf(int serv_id, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);
OneAssignScheme mig_vm_select_serv_first_fit(int vm_id, int old_serv_id, unordered_set<int> &except_servs);
OneAssignScheme mig_vm_select_serv_balance_fit(int vm_id, int old_serv_id, unordered_set<int> &except_servs);
OneAssignScheme mig_vm_select_serv_worst_fit(int vm_id, int old_serv_id, unordered_set<int> &except_servs);

// best fit
OneAssignScheme mig_vm_select_serv_bf(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched);
OneAssignScheme mig_vm_select_serv_bf_hash(int vm_id, int vm_type, int old_serv_id, unordered_set<int> &serv_searched);
OneAssignScheme add_vm_select_serv_bf_hash(int vm_id, int vm_type);
OneAssignScheme mig_serv_select_vm_full_load(int serv_id, int node, int vm_node);

OneAssignScheme mig_serv_select_vm_best_fit(int core_ream, int mem_ream, int node, int serv_id, unordered_set<int> &except_servs);


#endif