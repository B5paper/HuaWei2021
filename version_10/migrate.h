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

// 如果发现某个服务器有虚拟机迁移不掉，那么就放弃这台服务器！反正也腾不空！
struct IdNum
{
    int serv_id;
    int num_vm;
    int num_cant_hold;

    IdNum(int serv_id, int num_vm)
    : serv_id(serv_id), num_vm(num_vm), num_cant_hold(0) {}
};
void migrate_stop_early(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

// 对服务器进行遍历，让每个服务器都填得尽量满
void migrate_serv_first(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

void migrate_2(MigScheme &mig_scheme, ServStatList &serv_stats, ServSpecList &serv_specs, VMSpecList &vm_specs);

#endif