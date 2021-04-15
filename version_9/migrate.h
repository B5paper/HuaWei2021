#ifndef MIGRATE
#define MIGRATE

#include "types.h"
#include "status.h"

struct OneMigScheme
{
    int vm_id;
    int serv_id;
    int serv_node;

    OneMigScheme(int vm_id, int serv_id, int serv_node)
    : vm_id(vm_id), serv_id(serv_id), serv_node(serv_node) {}
};

typedef vector<OneMigScheme> MigScheme;

void migrate(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

// 对服务器进行遍历，让每个服务器都填得尽量满
void migrate_serv_first(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

#endif