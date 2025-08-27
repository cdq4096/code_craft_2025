#pragma once
#include "global_global.h"

void read_init();

void read_action();

void find_request(int i);

void read_and_pass(int i, int& n_rsp, vector<int>& request_finish_id);