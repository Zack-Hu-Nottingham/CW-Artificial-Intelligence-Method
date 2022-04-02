#include <iostream>
#include <vector>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

using namespace std;

struct item_struct{
    int size;
    int index;
    bool isPacked;
};

struct bin_struct {
    vector<item_struct> packed_items;
    int cap_left;
};

struct problem_struct{
    int n; //number of items
    int capacities;  //knapsack capacities
    int known_best;
    struct item_struct* items;
};

struct solution_struct {
    struct problem_struct* prob;
    int objective; // float objective;
    int feasibility;
    vector<bin_struct> bins;
};

struct solution_struct* initialize_empty_sol (struct problem_struct* prob) {
    
    struct solution_struct* sol = (struct solution_struct*) malloc (sizeof(solution_struct));
    sol->prob = prob;
    sol->objective = 0;
    sol->feasibility = 0;
    // sol->bins = vector<bin_struct>;

    return sol;
}

int main() {
    bin_struct* bin = new bin_struct();
    bin->cap_left = 150;
    solution_struct sol;
    sol.bins.push_back(*bin);
    bin_struct bin1 = sol.bins.back();
    cout << bin1.cap_left;
    delete(bin);
}