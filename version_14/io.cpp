#include "io.h"
#include <iostream>
#include <cstring>
using namespace std;

#define BUFFERSIZE 50

// util functions
string server_id_to_name(int id)
{
    return server_names_vec[id];
}

string vm_id_to_name(int id)
{
    return vm_names_vec[id];
}

void read_from_stdin(ServSpecList &server_specs, VMSpecList &vm_specs, DayReqList &reqs)
{
    int num_serv;
    scanf("%d", &num_serv);
    char serv_name[BUFFERSIZE];
    int serv_core, serv_mem, serv_cost, serv_consume;
    for (int i = 0; i < num_serv; ++i)
    {
        scanf("%s", serv_name);
        serv_name[strlen(serv_name)-1] = '\0';
        scanf("%d,", &serv_core);
        scanf("%d,", &serv_mem);
        scanf("%d,", &serv_cost);
        scanf("%d)", &serv_consume);
        // scanf("(%s, %d, %d, %d, %d)", serv_name, &serv_core, &serv_mem, &serv_cost, &serv_consume);
        server_names_map.insert(make_pair(string(serv_name+1), server_names_id_top));
        ++server_names_id_top;
        server_names_vec.push_back(serv_name+1);
        server_specs.push_back(ServSpec(serv_core, serv_mem, serv_cost, serv_consume));
    }

    int num_vm;
    scanf("%d", &num_vm);
    char vm_name[BUFFERSIZE];
    int vm_core, vm_mem, vm_node;
    for (int i = 0; i < num_vm; ++i)
    {
        scanf("%s", vm_name);
        vm_name[strlen(vm_name)-1] = '\0';
        scanf("%d,", &vm_core);
        scanf("%d,", &vm_mem);
        scanf("%d)", &vm_node);
        vm_names_map.insert(make_pair(vm_name+1, vm_names_id_top));
        ++vm_names_id_top;
        vm_specs.push_back(VMSpec(vm_name+1, vm_core, vm_mem, vm_node+1));
    }

    int num_req_day;
    scanf("%d", &num_req_day);
    int num_req;
    char buf_req[BUFFERSIZE];
    char buf_req_vm_name[BUFFERSIZE];
    int vm_id;
    DayReq *day_req = NULL;
    for (int i = 0; i < num_req_day; ++i)
    {
        day_req = new DayReq;
        scanf("%d", &num_req);
        for (int j = 0; j < num_req; ++j)
        {
            scanf("%s", buf_req);
            if (buf_req[1] == 'a')
            {
                scanf("%s", buf_req_vm_name);
                buf_req_vm_name[strlen(buf_req_vm_name)-1] = '\0';
                scanf("%d)", &vm_id);
                day_req->id.push_back(vm_id);
                day_req->vm_type.push_back(vm_names_map.find(buf_req_vm_name)->second);
                day_req->op.push_back(1);
            }
            else
            {
                scanf("%d)", &vm_id);
                day_req->op.push_back(0);
                day_req->vm_type.push_back(-1);
                day_req->id.push_back(vm_id);
            }
        }
        reqs.push_back(day_req);
    }
}

void read_serv_vm_specs(ServSpecList &serv_specs, VMSpecList &vm_specs)
{
    int num_serv;
    scanf("%d", &num_serv);
    char serv_name[BUFFERSIZE];
    int serv_core, serv_mem, serv_cost, serv_consume;
    for (int i = 0; i < num_serv; ++i)
    {
        scanf("%s", serv_name);
        serv_name[strlen(serv_name)-1] = '\0';
        scanf("%d,", &serv_core);
        scanf("%d,", &serv_mem);
        scanf("%d,", &serv_cost);
        scanf("%d)", &serv_consume);
        // scanf("(%s, %d, %d, %d, %d)", serv_name, &serv_core, &serv_mem, &serv_cost, &serv_consume);
        server_names_map.insert(make_pair(string(serv_name+1), server_names_id_top));
        ++server_names_id_top;
        server_names_vec.push_back(serv_name+1);
        serv_specs.push_back(ServSpec(serv_core, serv_mem, serv_cost, serv_consume));
    }

    int num_vm;
    scanf("%d", &num_vm);
    char vm_name[BUFFERSIZE];
    int vm_core, vm_mem, vm_node;
    for (int i = 0; i < num_vm; ++i)
    {
        scanf("%s", vm_name);
        vm_name[strlen(vm_name)-1] = '\0';
        scanf("%d,", &vm_core);
        scanf("%d,", &vm_mem);
        scanf("%d)", &vm_node);
        vm_names_map.insert(make_pair(vm_name+1, vm_names_id_top));
        ++vm_names_id_top;
        vm_specs.push_back(VMSpec(vm_name+1, vm_core, vm_mem, vm_node+1));
    }
}

void read_num_days_k(int &num_days, int &k)
{
    scanf("%d", &num_days);
    scanf("%d", &k);
}


void read_one_day_reqs(DayReqList &reqs)
{
    static char buf_req[BUFFERSIZE];
    static char buf_req_vm_name[BUFFERSIZE];
    int vm_id;
    int num_req;
    DayReq *day_req = NULL;
    
    day_req = new DayReq;
    scanf("%d", &num_req);
    for (int j = 0; j < num_req; ++j)
    {
        scanf("%s", buf_req);
        if (buf_req[1] == 'a')
        {
            scanf("%s", buf_req_vm_name);
            buf_req_vm_name[strlen(buf_req_vm_name)-1] = '\0';
            scanf("%d)", &vm_id);
            day_req->id.push_back(vm_id);
            day_req->vm_type.push_back(vm_names_map.find(buf_req_vm_name)->second);
            day_req->op.push_back(1);
        }
        else
        {
            scanf("%d)", &vm_id);
            day_req->op.push_back(0);
            day_req->vm_type.push_back(-1);
            day_req->id.push_back(vm_id);
        }
    }
    reqs.push_back(day_req);
}

void write_to_stdout(DayDispList &disps)
{
    for (int i = 0; i < disps.disp_list.size(); ++i)
    {
        // purchase
        int serv_type_num = 0;

        serv_type_num = disps.disp_list[i]->pur_server_name_ids.size();
        printf("(purchase, %d)\n", serv_type_num);
        for (int j = 0; j < disps.disp_list[i]->pur_server_name_ids.size(); ++j)
            printf("(%s, %d)\n", server_id_to_name(disps.disp_list[i]->pur_server_name_ids[j]).c_str(), disps.disp_list[i]->pur_server_nums[j]);

        // migrate
        printf("(migration, %d)\n", disps.disp_list[i]->mig_vm_ids.size());
        for (int j = 0; j < disps.disp_list[i]->mig_vm_ids.size(); ++j)
        {
            printf("(%d, %d", disps.disp_list[i]->mig_vm_ids[j], disps.disp_list[i]->mig_server_ids[j]);

            if (disps.disp_list[i]->mig_server_nodes[j] == 2)
                printf(")\n");
            else
                printf(", %c)\n", disps.disp_list[i]->mig_server_nodes[j] == 0 ? 'A' : 'B');
        }

        // process requirements
        for (int j = 0; j < disps.disp_list[i]->req_server_ids.size(); ++j)
        {          
            if (disps.disp_list[i]->req_server_nodes[j] == 2)
                printf("(%d)\n", disps.disp_list[i]->req_server_ids[j]);
            else
                printf("(%d, %c)\n", disps.disp_list[i]->req_server_ids[j], disps.disp_list[i]->req_server_nodes[j] == 0 ? 'A' : 'B');
        }
    }
    fflush(stdout);
}

void DayDispList::push_add_vm(int day, int serv_id, int serv_node)
{
    // if (idx_day >= add_nums.size() || idx_add > add_nums[idx_day])
    //     cout << "in day " << idx_day << ", req " << idx_add << ", error occored" << endl;
    disp_list[day]->req_server_ids.push_back(serv_id);
    disp_list[day]->req_server_nodes.push_back(serv_node);
    // disp_list[day]->add_serv_id_nodes.emplace(vm_id, make_pair(serv_id, serv_node));
}

void DayDispList::push_mig_vm(int day, int vm_id, int server_id, int server_node)
{
    disp_list[day]->mig_vm_ids.push_back(vm_id);
    disp_list[day]->mig_server_ids.push_back(server_id);
    disp_list[day]->mig_server_nodes.push_back(server_node);
}

void DayDispList::push_pur_server(int day, int type, int num)
{
    disp_list[day]->pur_server_name_ids.push_back(type);
    disp_list[day]->pur_server_nums.push_back(num);
}


void DayDispList::clear()
{
    for (int i = 0; i < disp_list.size(); ++i)
    {
        delete disp_list[i];
    }
    disp_list.clear();
    add_nums.clear();
}


DayDispList::~DayDispList()
{
    clear();
}

void DayDispList::output_day(int idx_day_start, int idx_day_end)
{
    for (int i = idx_day_start; i != idx_day_end; ++i)
    {
        // purchase
        int serv_type_num = 0;
        serv_type_num = disp_list[i]->pur_server_name_ids.size();
        printf("(purchase, %d)\n", serv_type_num);
        for (int j = 0; j < disp_list[i]->pur_server_name_ids.size(); ++j)
            printf("(%s, %d)\n", server_id_to_name(disp_list[i]->pur_server_name_ids[j]).c_str(), disp_list[i]->pur_server_nums[j]);

        // migrate
        printf("(migration, %d)\n", disp_list[i]->mig_vm_ids.size());
        for (int j = 0; j < disp_list[i]->mig_vm_ids.size(); ++j)
        {
            printf("(%d, %d", disp_list[i]->mig_vm_ids[j], disp_list[i]->mig_server_ids[j]);

            if (disp_list[i]->mig_server_nodes[j] == 2)
                printf(")\n");
            else
                printf(", %c)\n", disp_list[i]->mig_server_nodes[j] == 0 ? 'A' : 'B');
        }

        // process requirements
        for (int j = 0; j < disp_list[i]->req_server_ids.size(); ++j)
        {          
            if (disp_list[i]->req_server_nodes[j] == 2)
                printf("(%d)\n", disp_list[i]->req_server_ids[j]);
            else
                printf("(%d, %c)\n", disp_list[i]->req_server_ids[j], disp_list[i]->req_server_nodes[j] == 0 ? 'A' : 'B');
        }
    }
    fflush(stdout);
}