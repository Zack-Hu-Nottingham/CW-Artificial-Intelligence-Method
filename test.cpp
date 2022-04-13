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

    vector<int> vect1{1, 2, 3, 4};
    vector<int> vect2{3,4,5,6};
    vect2 = vect1;
    for (int i=0; i<vect2.size(); i++) {
        cout << vect2[i] << " ";
    }
}