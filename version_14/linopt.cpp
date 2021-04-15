#include "linopt.h"
#include <vector>
#include <set>
#include <cfloat>
#include <deque>
#include <iostream>
#include <unordered_set>
#include <map>
using namespace std;

void pivot(vector<vector<float>> &A, vector<float> &b, vector<float> &c, int col_pivot, int row_pivot,
    map<int, int> &idx_basic_vars, map<int, int> &idx_nonbasic_vars,
    map<int, int> &idx_basic_var_to_idx_row, map<int, int> &idx_nonbasic_var_to_idx_col)
{
    int num_rows = b.size();
    int num_cols = c.size();

    float coe_pivot_var = A.at(row_pivot).at(col_pivot);
    A.at(row_pivot).at(col_pivot) = 1;
    for (int i = 0; i < num_cols; ++i)
    {
        A.at(row_pivot).at(i) = A.at(row_pivot).at(i) / coe_pivot_var;
    }
    b.at(row_pivot) = b.at(row_pivot) / coe_pivot_var;

    float multiple;
    for (int i = 0; i < num_rows; ++i)
    {
        if (i == row_pivot)
            continue;
        multiple = A.at(i).at(col_pivot);
        A.at(i).at(col_pivot) = 0;
        for (int j = 0; j < num_cols; ++j)
        {
            A.at(i).at(j) = A.at(i).at(j) - A.at(row_pivot).at(j) * multiple;
        }
        b.at(i) = b.at(i) - b.at(row_pivot) * multiple;
    }

    multiple = c.at(col_pivot);
    c.at(col_pivot) = 0;
    for (int i = 0; i < num_cols; ++i)
        c.at(i) = c.at(i) - A.at(row_pivot).at(i) * multiple;

    // 映射序号关系
    int temp_idx_nonbasic_var = idx_nonbasic_vars.at(col_pivot);
    int temp_idx_basic_var = idx_basic_vars.at(row_pivot);
    idx_basic_var_to_idx_row.erase(temp_idx_basic_var);
    idx_basic_var_to_idx_row.emplace(temp_idx_nonbasic_var, row_pivot);
    idx_nonbasic_var_to_idx_col.erase(temp_idx_nonbasic_var);
    idx_nonbasic_var_to_idx_col.emplace(temp_idx_basic_var, col_pivot);

    int temp_val = idx_nonbasic_vars.at(col_pivot);
    idx_nonbasic_vars.at(col_pivot) = idx_basic_vars.at(row_pivot);
    idx_basic_vars.at(row_pivot) = temp_val;
}

bool initialize_simplex(vector<vector<float>> &A, vector<float> &b, vector<float> &c,
    map<int, int> &idx_basic_vars, map<int, int> &idx_nonbasic_vars,
    map<int, int> &idx_basic_var_to_idx_row, map<int, int> &idx_nonbasic_var_to_idx_col)
{
    int idx_min_b = -1;
    float min_b = FLT_MAX;
    for (int i = 0; i < b.size(); ++i)
    {
        if (b[i] < min_b)
        {
            min_b = b[i];
            idx_min_b = i;
        }
    }

    if (min_b >= 0)
        return true;

    // 加入辅助变量
    int idx_aux_var = idx_nonbasic_vars.size() + idx_basic_vars.size();
    idx_nonbasic_vars.emplace(idx_nonbasic_vars.size(), idx_aux_var);
    idx_nonbasic_var_to_idx_col.emplace(idx_aux_var, idx_nonbasic_var_to_idx_col.size());
    for (int i = 0; i < A.size(); ++i)
        A[i].push_back(-1);
    
    // 构建辅助目标函数
    vector<float> c_copy(c);
    c.clear();
    c.assign(idx_nonbasic_vars.size(), 0);
    c.back() = -1;

    pivot(A, b, c, idx_nonbasic_vars.size()-1, idx_min_b, idx_basic_vars, idx_nonbasic_vars,
        idx_basic_var_to_idx_row, idx_nonbasic_var_to_idx_col);

    while (true)
    {
        // find pivot column
        int col_pivot = -1;
        float max_val = -FLT_MAX;
        for (int i = 0; i < c.size(); ++i)
        {
            if (c.at(i) <= 0)
                continue;

            if (c.at(i) > max_val)
            {
                max_val = c.at(i);
                col_pivot = i;
            }
        }

        // found optimal
        if (col_pivot == -1)
            break;

        // find the pivot row
        int row_pivot = -1;
        float min_val_cons = FLT_MAX;
        float val_cons;
        for (int i = 0; i < b.size(); ++i)
        {
            if (A.at(i).at(col_pivot) <= 0)
                continue;
            val_cons = b.at(i) / A.at(i).at(col_pivot);
            if (val_cons < min_val_cons)
            {
                min_val_cons = val_cons;
                row_pivot = i;
            }
        }

        // unbounded
        if (row_pivot == -1)
            break;

        // pivot
        pivot(A, b, c, col_pivot, row_pivot, idx_basic_vars, idx_nonbasic_vars,
            idx_basic_var_to_idx_row, idx_nonbasic_var_to_idx_col);
    }

    // 若没有出现在非基本变量中，出现在基本变量中
    if (idx_nonbasic_var_to_idx_col.find(idx_aux_var) == idx_nonbasic_var_to_idx_col.end())
    {
        // 若出现在基本变量中，且其不为零，那么无可行解
        auto iter = idx_basic_var_to_idx_row.find(idx_aux_var);
        // if (iter == idx_basic_var_to_idx_row.end())
        //     cout << "error in initialize_simplex" << endl;
        if (abs(b.at(iter->second) - 0) > 1e-4)
            return false;  // infeasible
        // 若出现在基本变量中，且其为零，那么对其进行转动，让其变成非基本变量
        else
        {
            int row_pivot = iter->second;
            for (int j = 0; j < A[0].size(); ++j)
            {
                if (A[row_pivot][j] != 0)  // 找到矩阵 A 中 row_pivot 行某个非零元素即可
                {
                    pivot(A, b, c, j, row_pivot, idx_basic_vars, idx_nonbasic_vars, idx_basic_var_to_idx_row,
                        idx_nonbasic_var_to_idx_col);
                    break;
                }
            }
        }
    }

    // 若辅助变量出现在非基本变量中，那么说明一定有可行解 
    // 只需要修改线性规划问题的形式即可

    // 将辅助变量从 A 和 c 中删除
    auto iter_aux_col = idx_nonbasic_var_to_idx_col.find(idx_aux_var);
    // if (iter_aux_col == idx_nonbasic_var_to_idx_col.end())
    //     cout << "error" << endl;
    int col_aux_var = iter_aux_col->second;
    for (int i = 0; i < A.size(); ++i)
        A[i].erase(A[i].begin() + col_aux_var);
    c.erase(c.begin() + col_aux_var);

    // 调整变量序号和行列数的映射关系，所有列号大于 idx_aux_var 所在列的变量都应该把列号减一
    idx_nonbasic_var_to_idx_col.erase(idx_aux_var);
    idx_nonbasic_vars.clear();
    for (auto &iter: idx_nonbasic_var_to_idx_col)
    {
        if (iter.second > col_aux_var)
            iter.second -= 1;
        idx_nonbasic_vars.emplace(iter.second, iter.first);
    }
    
    // 替换目标函数中的基本变量
    c = c_copy;
    vector<float> c_new(c.size(), 0);
    unordered_set<int> idx_vars_in_c;
    // 原始的 c 的变量序号是从 0 到 n-1
    for (int i = 0; i < idx_nonbasic_vars.size(); ++i)
        idx_vars_in_c.insert(i);
    // 若 c 的变量序号中有基本变量，那么将基本变量通过等式替换为非基本变量
    // 在矩阵上的操作即为找到 c 的变量中的基本变量所在的行，然后让 c 消去 c 中这一行 1 所在列的数即可
    // 对于 c 中的非基本变量，把它添加到 c 对应列的位置即可
    c_copy.assign(c.size(), 0);  // 循环利用 ^_^
    for (int i = 0; i < idx_nonbasic_vars.size(); ++i)
    {
        auto iter = idx_basic_var_to_idx_row.find(i);
        // 若第 i 号变量是基本变量
        if (iter != idx_basic_var_to_idx_row.end())
        {
            for (int j = 0; j < c.size(); ++j)
                c_copy[j] -= c[i] * A[iter->second][j];
        }
        else  // 若第 i 号变量是非基本变量
            c_copy[i] += c[i];
    }
    c = c_copy;
    return true;
}

// maximum the objective z = c^Tx, subject to Ax <= b
vector<float> simplex(vector<vector<float>> &A, vector<float> &b, vector<float> &c)
{
    // 建立行、列序号与变量序号的映射关系
    map<int, int> idx_basic_vars;  // idx_row -> idx_basic_var
    map<int, int> idx_nonbasic_vars;  // idx_col -> idx_nonbasic_var
    map<int, int> idx_basic_var_to_idx_row;  // idx_basic_var -> idx_row
    map<int, int> idx_nonbasic_var_to_idx_col;  // idx_nonbasic_var -> idx_col
    for (int i = 0; i < c.size(); ++i)
    {
        idx_nonbasic_vars[i] = i;
        idx_nonbasic_var_to_idx_col[i] = i;
    }
    for (int i = 0; i < b.size(); ++i)
    {
        idx_basic_vars[i] = i + c.size();
        idx_basic_var_to_idx_row[i + c.size()] = i;
    }

    // 初始化单纯形法
    if (!initialize_simplex(A, b, c, idx_basic_vars, idx_nonbasic_vars, idx_basic_var_to_idx_row, idx_nonbasic_var_to_idx_col))
        return vector<float> ();

    // 正式的单纯形法
    while (true)
    {
        // find pivot column
        int col_pivot = -1;
        float max_val = -FLT_MAX;
        for (int i = 0; i < c.size(); ++i)
        {
            if (c.at(i) <= 0)
                continue;

            if (c.at(i) > max_val)
            {
                max_val = c.at(i);
                col_pivot = i;
            }
        }

        // found optimal
        if (col_pivot == -1)
            break;

        // find the pivot row
        int row_pivot = -1;
        float min_val_cons = FLT_MAX;
        float val_cons;
        for (int i = 0; i < b.size(); ++i)
        {
            if (A.at(i).at(col_pivot) <= 0)
                continue;
            val_cons = b.at(i) / A.at(i).at(col_pivot);
            if (val_cons < min_val_cons)
            {
                min_val_cons = val_cons;
                row_pivot = i;
            }
        }

        // unbounded
        if (row_pivot == -1)
            break;

        // pivot
        pivot(A, b, c, col_pivot, row_pivot, idx_basic_vars, idx_nonbasic_vars, idx_basic_var_to_idx_row,
            idx_nonbasic_var_to_idx_col);
    }

    // 提取最优解
    vector<float> x(c.size(), 0);
    map<int, int>::iterator iter;
    for (int i = 0; i < c.size(); ++i)
    {
        iter = idx_basic_var_to_idx_row.find(i);
        if (iter != idx_basic_var_to_idx_row.end())
            x.at(i) = b.at(iter->second);
        else
            x[i] = 0;
    }

    return x;
}


void linpro(LP &lp)
{
    LP lp_copy(lp);
    lp.x = simplex(lp_copy.A, lp_copy.b, lp_copy.c);
    if (lp.x.empty())
    {
        lp.feasible = false;
        return;
    }
    float max_val = 0;
    for (int i = 0; i < lp.x.size(); ++i)
        max_val += lp.c.at(i) * lp.x.at(i);
    lp.max_val = max_val;
}

// maximum the objective z = c^Tx, subject to Ax <= b
// with x as integers
vector<int> branch_and_cut_max(vector<vector<float>> &A, vector<float> &b, vector<float> &c, float &max_val, float max_obj)
{
    deque<LP> active_list;
    LP init_prob(A, b, c);
    active_list.push_back(init_prob);
    Solution lower_bound(c.size(), -FLT_MAX), upper_bound(c.size(), -FLT_MAX);

    linpro(init_prob);
    upper_bound.max_val = init_prob.max_val;
    upper_bound.x = init_prob.x;

    while (!active_list.empty())
    {
        LP &curr_prob = active_list.front();

        linpro(curr_prob);

        if (!curr_prob.feasible)
        {
            active_list.pop_front();
            continue;
        }

        // cut
        if (curr_prob.max_val <= lower_bound.max_val)
        {
            active_list.pop_front();
            continue;
        }

        // 定界
        if (curr_prob.is_integer())
        {
            lower_bound.max_val = curr_prob.max_val;
            lower_bound.x = curr_prob.x;

            if (lower_bound.max_val >= upper_bound.max_val)
                break;

            // stop early
            if (abs(lower_bound.max_val - max_obj) < 1e-4)
                break;

            active_list.pop_front();
            continue;
        }

        // 找到第一个非整数变量，并开始分枝
        for (int i = 0; i < curr_prob.x.size(); ++i)
        {
            if (abs(curr_prob.x[i] - floor(curr_prob.x[i] + 0.5)) > 1e-5)
            {
                float x_val = curr_prob.x.at(i);
                curr_prob.clear_state();
                curr_prob.mod_cons(i, floor(x_val), 's');
                active_list.push_back(curr_prob);
                curr_prob.mod_cons(i, ceil(x_val), 'g');
                active_list.push_back(curr_prob);
                break;
            }
        }

        active_list.pop_front();
    }

    vector<int> int_sol(c.size());
    for (int i = 0; i < int_sol.size(); ++i)
        int_sol[i] = floor(lower_bound.x[i] + 0.5);
    max_val = lower_bound.max_val;
    return int_sol;
}

vector<int> branch_and_cut_min(vector<vector<float>> &A, vector<float> &b, vector<float> &c, float &min_val, float min_obj)
{
    // 修改目标函数，使其变成一个最大化问题
    for (int i = 0; i < c.size(); ++i)
        c[i] = -c[i];

    deque<LP> active_list;
    LP init_prob(A, b, c);
    active_list.push_back(init_prob);
    Solution lower_bound(c.size(), -FLT_MAX), upper_bound(c.size(), -FLT_MAX);

    linpro(init_prob);
    init_prob.max_val = - init_prob.max_val;
    upper_bound.max_val = init_prob.max_val;
    upper_bound.x = init_prob.x;

    while (!active_list.empty())
    {
        LP &curr_prob = active_list.front();

        linpro(curr_prob);
        curr_prob.max_val = - curr_prob.max_val;

        if (!curr_prob.feasible)
        {
            active_list.pop_front();
            continue;
        }

        // cut
        if (curr_prob.max_val <= lower_bound.max_val)
        {
            active_list.pop_front();
            continue;
        }

        // 定界
        if (curr_prob.is_integer())
        {
            lower_bound.max_val = curr_prob.max_val;
            lower_bound.x = curr_prob.x;

            if (lower_bound.max_val >= upper_bound.max_val)
                break;

            // stop early
            if (abs(lower_bound.max_val <= min_obj) < 1e-4)
                break;

            active_list.pop_front();
            continue;
        }

        // 找到第一个非整数变量，并开始分枝
        for (int i = 0; i < curr_prob.x.size(); ++i)
        {
            if (abs(curr_prob.x.at(i) - floor(curr_prob.x.at(i) + 0.5)) > 1e-4)
            {
                float x_val = curr_prob.x.at(i);
                curr_prob.clear_state();
                curr_prob.mod_cons(i, floor(x_val), 's');
                active_list.push_back(curr_prob);
                curr_prob.mod_cons(i, ceil(x_val), 'g');
                active_list.push_back(curr_prob);
                break;
            }
        }

        active_list.pop_front();
    }

    vector<int> int_sol(c.size());
    for (int i = 0; i < int_sol.size(); ++i)
        int_sol[i] = floor(lower_bound.x[i] + 0.5);
    
    min_val = lower_bound.max_val;
    return int_sol;
}

