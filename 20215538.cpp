#include <iostream>
#include <vector>

#include <algorithm>
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
    char name[10];
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

/* declare parameters for variable neighbourhood search here*/
int K= 4; // k-opt is used
int SHAKE_STRENGTH = 12;
struct solution_struct best_sln;  //global best solution
int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 1;
int MAX_TIME = 30;  //max amount of time permited (in sec)
int num_of_problems;
bool isImproving = false;


int cmpfunc1(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

int cmpfunc2(const void* a, const void* b){
    const struct item_struct* item1 = (const struct item_struct*) a;
    const struct item_struct* item2 = (const struct item_struct*) b;
    if(item1->size>item2->size) return 1;
    if(item1->size<item2->size) return -1;
    return 0;
}

void init_problem(char* name, int n, int capacities, int known_best, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = (struct problem_struct*) malloc(sizeof(struct problem_struct));
    strcpy(new_prob->name, name);

    new_prob->n=n; new_prob->capacities=capacities; new_prob->known_best=known_best;
    new_prob->items= (struct item_struct*) malloc(sizeof(struct item_struct)*n);
    // for(int j=0; j<n; j++)
        // new_prob->items[j].index = j;
    *my_prob = new_prob;
}

void free_problem(struct problem_struct* prob)
{
    if(prob!=NULL)
    {
        // if(prob->capacities !=NULL) free(prob->capacities);
        if(prob->items!=NULL)
        {
            free(prob->items);
        }
        free(prob);
    }
}

//copy可能存在很多问题

//copy a solution from another solution
bool copy_solution(struct solution_struct* dest_sln, struct solution_struct* source_sln)
{
    if(source_sln ==NULL) return false;
    if(dest_sln==NULL)
    {
        dest_sln = (struct solution_struct*) malloc(sizeof(struct solution_struct));
    }
    dest_sln->prob = source_sln->prob;
    dest_sln->feasibility = source_sln->feasibility;
    dest_sln->objective = source_sln->objective;
    dest_sln->bins = source_sln->bins;
    return true;
}

//update global best solution from sln
void update_best_solution(struct solution_struct* sln)
{
    if(best_sln.objective >= sln->objective) { //这边特意放宽条件
        copy_solution(&best_sln, sln);
    }
}

struct problem_struct** load_problems(char* data_file)
{
    int i,j,k;

    // Open data file and read out data
    FILE* pfile = fopen(data_file, "r");
    if(pfile==NULL) {
        printf("Data file %s does not exist. Please check!\n", data_file); exit(2); 
    }

    fscanf (pfile, "%d", &num_of_problems);

    // For test purpose
    printf("Number of problem: %d\n", num_of_problems);
    
    struct problem_struct** my_problems = (struct problem_struct**) malloc(sizeof(struct problem_struct*)*num_of_problems);

    for(k=0; k<num_of_problems; k++)
    {
        int capacities, n, known_best;
        char name[10];

        // Read in relevant data of a problem
        fscanf(pfile, "%s", name);
        fscanf (pfile, "%d", &capacities);
        fscanf (pfile, "%d", &n); 
        fscanf (pfile, "%d", &known_best);
        // printf("Name: %s, capacities: %d, item number: %d, known_best: %d\n", name, capacities, n, known_best);

        // Initialize problem
        init_problem(name, n, capacities, known_best, &my_problems[k]);  //allocate data memory
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].index=j;
            my_problems[k]->items[j].isPacked = false;
            fscanf(pfile, "%d", &(my_problems[k]->items[j].size)); //read profit data
            // printf("item[%d].p=%d\n", j, my_problems[k]->items[j].size);
        }
    }
    fclose(pfile); //close file

    return my_problems;
}


//output a given solution to a file
void output_solution(struct solution_struct* sln, char* out_file) {
    FILE* pfile = fopen(out_file, "a"); //open a new file

    fprintf(pfile, "%s\n", sln->prob->name); 
    fprintf(pfile, " obj=   %d   %d\n", sln->objective, sln->objective - sln->prob->known_best); 
    for (int i=0; i<sln->bins.size(); i++) {
        // fprintf(pfile, "bin %d: ", i);
        for (int j=0; j<sln->bins[i].packed_items.size(); j++) {
            fprintf(pfile, "%d ", sln->bins[i].packed_items[j].index); 
        }
        // fprintf(pfile, "%d", sln->bins[i].cap_left);
        fprintf(pfile, "\n");  // 如果跟据bin的数量来判断会多出空行，因为有空bin没被删去
    }

    fclose(pfile);

}

// 
void three_item_swap(bin_struct* bin1, bin_struct* bin2, bin_struct* bin3, int a, int b, int c,  struct solution_struct* sln) {

    int cap1, cap2, cap3;
    int cap1_after, cap2_after, cap3_after;

    item_struct item1, item2, item3;

    for (int i=0; i<bin1->packed_items.size(); i++) {
        for (int j=0; j<bin2->packed_items.size(); j++) {
            for (int k=0; k<bin3->packed_items.size(); k++) {
                item1 = bin1->packed_items[i];
                item2 = bin2->packed_items[j];
                item3 = bin3->packed_items[k];
                
                cap1 = bin1->cap_left;
                cap2 = bin2->cap_left;
                cap3 = bin3->cap_left;

                int cap_left1 = bin1->cap_left;
                int cap_left2 = bin2->cap_left;

                // tell if could swap
                if (item2.size + item3.size <= cap_left1 + item1.size && item2.size + item3.size > item1.size) {
                    if (item1.size <= cap_left2 + item2.size) {                        
                        
                        // cout << endl << "bin number: " << a << " " << b << " " << c << endl;

                        // // print the content of bins
                        // cout << "Before:" << endl;
                        // cout << "bin1: ";
                        // for (int i=0; i<bin1->packed_items.size(); i++) {
                        //     cout << bin1->packed_items[i].size << " ";
                        // }
                        // cout << endl;

                        // cout << "bin2: ";
                        // for (int i=0; i<bin2->packed_items.size(); i++) {
                        //     cout << bin2->packed_items[i].size << " ";
                        // }
                        // cout << endl;

                        // cout << "bin3: ";
                        // for (int i=0; i<bin3->packed_items.size(); i++) {
                        //     cout << bin3->packed_items[i].size << " ";
                        // }
                        // cout << endl;


                        // cout << "i j k: " << i << " " << j << " " << k << endl;
                        // cout << "before: " << cap1 << " " << cap2 << " " << cap3 << endl;

                        // cout << "item 1: " << item1.size << "  item 2: " << item2.size << "  item 3: " << item3.size << endl;   



                        bin1->cap_left = bin1->cap_left - item2.size - item3.size + item1.size;
                        bin2->cap_left = bin2->cap_left - item1.size + item2.size;
                        bin3->cap_left = bin3->cap_left + item3.size;

                        // all satisfied, then swap
                        bin1->packed_items.erase(bin1->packed_items.begin() + i);
                        bin2->packed_items.erase(bin2->packed_items.begin() + j);
                        bin3->packed_items.erase(bin3->packed_items.begin() + k);

                        bin1->packed_items.push_back(item2); // add item2 to bin1
                        bin1->packed_items.push_back(item3); // add item3 to bin1
                        bin2->packed_items.push_back(item1); // add item1 to bin2


                        cap1_after = bin1->cap_left;
                        cap2_after = bin2->cap_left;
                        cap3_after = bin3->cap_left;


                        
                        // // print the content of bins
                        // cout << endl << "After:" << endl;
                        // cout << "bin1: ";
                        // for (int i=0; i<bin1->packed_items.size(); i++) {
                        //     cout << bin1->packed_items[i].size << " ";
                        // }
                        // cout << endl;

                        // cout << "bin2: ";
                        // for (int i=0; i<bin2->packed_items.size(); i++) {
                        //     cout << bin2->packed_items[i].size << " ";
                        // }
                        // cout << endl;

                        // cout << "bin3: ";
                        // for (int i=0; i<bin3->packed_items.size(); i++) {
                        //     cout << bin3->packed_items[i].size << " ";
                        // }
                        // cout << endl;

                        // cout << "after: " << cap1_after << " " << cap2_after << " " << cap3_after << endl;


                        if (bin3->packed_items.size() == 0) {
                            cout << "objective -1 " << endl;
                            sln->bins.erase(sln->bins.begin() + c);
                            sln->objective -= 1;
                        }
                    }
                }
            }
        }
    }
    return ;
}

void best_descent_3_swap(vector<bin_struct>* bins, struct solution_struct* curt_sln) {

    // int bin1_id, bin2_id, bin3_id, item1_id, item2_id, item3_id;
    int delta=0, best_delta=0;
    
    bin_struct* bin1;
    bin_struct* bin1_best;

    bin_struct* bin2;
    bin_struct* bin2_best;
    
    bin_struct* bin3;
    bin_struct* bin3_best;
    
    int cap1, cap2, cap3;
    int cap1_after, cap2_after, cap3_after;

    item_struct item1, item2, item3;
    item_struct* item1_best;
    item_struct* item2_best;
    item_struct* item3_best;
    int a, b, c, d;
    
    for (int i=0; i<bins->size(); i++) {
        bin1 = &(*(bins->begin() + i));
        if (bin1->cap_left == 0) continue;
        for (int j=i+1; j<bins->size(); j++) {
            bin2 = &(*(bins->begin() + j));
            if (bin2->cap_left == 0) continue;
            for (int k=j; k<bins->size(); k++) {
                bin3 = &(*(bins->begin() + k));
                if (bin3->cap_left == 0) continue;
                // 到此为止，获得到所有的bin

                
                for (int o=0; o<bin1->packed_items.size(); o++) {
                    for (int p=0; p<bin2->packed_items.size(); p++) {
                        
                        item1 = bin1->packed_items[o];
                        item2 = bin2->packed_items[p];
                        
                        cap1 = bin1->cap_left;
                        cap2 = bin2->cap_left;

                        int cap_left1 = bin1->cap_left;
                        int cap_left2 = bin2->cap_left;

                        if (k == j) { // 如果是2-1 swap
                            for (int q=p+1; q<bin2->packed_items.size(); q++) {
                                item3 = bin2->packed_items[q];

                                if (item2.size + item3.size <= cap_left1 + item1.size && item2.size + item3.size > item1.size ) {
                                    delta = cap_left1 + item1.size - item2.size - item3.size;
                                    if (delta >= best_delta) {
                                        bin1_best = bin1;
                                        bin2_best = bin2;
                                        bin3_best = bin3;
                                        item1_best = &item1;
                                        item2_best = &item2;
                                        item3_best = &item3;
                                        a = o, b = p, c = q, d = k;
                                    }
                                }

                            }

                        } else { // 1-1-1 swap
                            for (int q=0; q<bin3->packed_items.size(); q++) {
                                item3 = bin3->packed_items[q];

                                if (item2.size + item3.size <= cap_left1 + item1.size && item2.size + item3.size > item1.size && item1.size <= cap_left2 + item2.size) {
                                    delta = cap_left1 + item1.size - item2.size - item3.size;
                                    if (delta >= best_delta) {
                                        bin1_best = bin1;
                                        bin2_best = bin2;
                                        bin3_best = bin3;
                                        item1_best = &item1;
                                        item2_best = &item2;
                                        item3_best = &item3;
                                        a = o, b = p, c = q, d = k;

                                    }
                                }
                                                            
               
                            }
                        }
                        
                    }
                }

            }
        }
    }

    if (best_delta >= 0) {
        
        isImproving = true;

        if (bin2_best != bin3_best) {
            bin1_best->cap_left = bin1_best->cap_left - item2_best->size - item3_best->size + item1_best->size;
            bin2_best->cap_left = bin2_best->cap_left - item1_best->size + item2_best->size;
            bin3_best->cap_left = bin3_best->cap_left + item3_best->size;

                        
            // all satisfied, then swap

            bin1_best->packed_items.push_back(*item2_best); // add item2 to bin1
            bin1_best->packed_items.push_back(*item3_best); // add item3 to bin1
            bin2_best->packed_items.push_back(*item1_best); // add item1 to bin2

            bin1_best->packed_items.erase(bin1_best->packed_items.begin() + a);
            bin2_best->packed_items.erase(bin2_best->packed_items.begin() + b);
            bin3_best->packed_items.erase(bin3_best->packed_items.begin() + c);
            if (bin3_best->packed_items.size() == 0) {
                cout << "objective -1 " << endl;
                curt_sln->bins.erase(curt_sln->bins.begin() + d);
                curt_sln->objective -= 1;
            }
            return ;

        } else {

            bin1_best->packed_items.push_back(*item2_best);
            bin1_best->packed_items.push_back(*item3_best);
            bin2_best->packed_items.push_back(*item1_best);

            // 从bin1中删去item1
            bin1_best->packed_items.erase(bin1_best->packed_items.begin() + a);
            bin1_best->cap_left = bin1_best->cap_left + item1_best->size - item2_best->size - item3_best->size;
            if (bin1_best->cap_left != 0) {
                cout << "ERROR!!!!!!!!!" << endl;
            }
            
            // delete item2 and item3 from bin2
            bin2_best->packed_items.erase(bin2_best->packed_items.begin() + b);
            bin2_best->packed_items.erase(bin2_best->packed_items.begin() + c-1);
            
            bin2_best->cap_left = bin2_best->cap_left + item2_best->size + item3_best->size - item1_best->size;
            return ;
        }
        
        
    }
}

// This 
void one_swap(bin_struct* bin1, bin_struct* bin2, int index, struct solution_struct* sln) {
    item_struct item;
    for (int i=0; i<bin1->packed_items.size(); i++) {
        item = bin1->packed_items[i];
        if (item.size <= bin2->cap_left) {
            // 把这个item放到bin2里
            bin1->packed_items.erase(bin1->packed_items.begin() + i);
            bin2->packed_items.push_back(item);
            bin1->cap_left += item.size;
            bin2->cap_left -= item.size;
            // 判断bin1是否为空，若空则删去这个bin并且obejctive - 1
            if (bin1->packed_items.size() == 0) {
                sln->bins.erase(sln->bins.begin() + index);
                sln->objective -= 1;
                cout << "Objective -1" << endl;
            }
        }
    }
}

void two_one_swap(bin_struct* bin1, bin_struct* bin2, struct solution_struct* sln) {
    bool isFound = false;
    item_struct item1, item2, item3;

    for (int i=0; i<bin1->packed_items.size(); i++) {
        item1 = bin1->packed_items[i];
        for (int j=0; j<bin2->packed_items.size(); j++) {
            item2 = bin2->packed_items[j];
            for (int k=j+1; k<bin2->packed_items.size(); k++) {
                item3 = bin2->packed_items[k];
                if (item2.size + item3.size == item1.size + bin1->cap_left) {

                    // 从bin1中删去item1
                    bin1->packed_items.erase(bin1->packed_items.begin() + i);
                    bin1->packed_items.push_back(item2);
                    bin1->packed_items.push_back(item3);
                    bin1->cap_left = bin1->cap_left + item1.size - item2.size - item3.size;
                    if (bin1->cap_left != 0) {
                        cout << "ERROR!!!!!!!!!" << endl;
                    }
                    
                    // delete item2 and item3 from bin2
                    bin2->packed_items.erase(bin2->packed_items.begin() + j);
                    bin2->packed_items.erase(bin2->packed_items.begin() + k-1);
                    bin2->packed_items.push_back(item1);
                    bin2->cap_left = bin2->cap_left + item2.size + item3.size - item1.size;
                    return ;
                }
            }
        }
    }
}

//nb_indx <=3
struct solution_struct* best_descent_vns(int nb_indx, struct solution_struct* curt_sln)
{
    struct solution_struct* best_neighb = curt_sln;
    
    int n=curt_sln->prob->n;

    vector<bin_struct> * bins = & curt_sln->bins;

    //storing best neighbourhood moves
    // int curt_move[] ={-1,-1,-1,-1}, best_move []={-1,-1,-1,-1};
    // int delta=0, best_delta=0;  
    
    bin_struct* bin1;
    bin_struct* bin2;
    bin_struct* bin3;

    switch (nb_indx)
    {
        case 1:
            // search from back to front, bin1 is the bin with higher index
            for (int i=bins->size()-1; i>0; i-- ) { 
                bin1 = &(*(bins->begin() + i));
                for (int j=i-1; j>0; j-- ) { 
                    bin2 = &(*(bins->begin() + j));
                    one_swap(bin1, bin2, i, curt_sln);
                }
            }

            

        case 3:            
            // 1 - 1 - 1 swap
            // select three bins that are not full, and try all kinds of swap between bins

            cout << "here" << endl;
            best_descent_3_swap(bins, curt_sln);

            // for (int i=0; i<bins->size(); i++) {
            //     if (bin1->cap_left == 0) continue;
            //     bin1 = &(*(bins->begin() + i));
            //     for (int j=i+1; j<bins->size(); j++) {
            //         if (bin2->cap_left == 0) continue;
            //         bin2 = &(*(bins->begin() + j));
            //         for (int k=j+1; k<bins->size(); k++) {
            //             if (bin3->cap_left == 0) continue;
            //             bin3 = &(*(bins->begin() + k));

            //             three_item_swap(bin1, bin2, bin3, i, j, k, curt_sln);
                        
            //             if (bin1->cap_left == 0 || bin2->cap_left == 0) { 
            //                 // if after swap either bin1 or bin2 full, break
            //                 break;
            //             }
            //         }
            //         if (bin1->cap_left == 0) {
            //             break;
            //         }
            //     }
            // }

            // // 两个算法之间做好隔离
            // // dif = 0;
            // bin1 = NULL;
            // bin2 = NULL;
            // bin3 = NULL;
            // // 2-1 swap
            
            // for (int i=0; i<bins->size(); i++) {
            //     bin1 = &(*(bins->begin() + i));
            //     if (bin1->cap_left == 0) continue;
            //     for (int j=i+1; j<bins->size(); j++) {
            //         bin2 = &(*(bins->begin() + j));
            //         if (bin2->cap_left == 0) continue;

            //         two_one_swap(bin1, bin2, curt_sln);
            //     }
            // }


            //2-1 swap
            // for(int i=0; i<n; i++){
            //     for(int j=0; j!=i && j<n; j++){
            //         for(int k=0;k<n;k++){
            //             curt_move[0]=i; curt_move[1]=j; curt_move[2]=k;
            //             if(can_move(nb_indx, &curt_move[0], best_neighb)){
            //                 delta = curt_sln->prob->items[k].p -curt_sln->prob->items[i].p-curt_sln->prob->items[j].p;
            //                 if(delta > best_delta){
            //                     best_delta = delta; best_move[0] = i; best_move[1] = j; best_move[2]=k;
            //                 }
            //             }
            //         }
            //     }
            // }
            // //1-2 swap
            // for(int i=0; i<n; i++){
            //     if(curt_sln->x[i]==0) continue;
            //     for(int j=0; j<n; j++){
            //         if(curt_sln->x[j]>0) continue;
            //         for(int k=0;k!=j&&k<n;k++){
            //             if(curt_sln->x[k] == 0)
            //             {
            //                 curt_move[0]=i; curt_move[1]=j; curt_move[2]=k;
            //                 if(can_move(nb_indx, &curt_move[0], curt_sln)){
            //                     delta = curt_sln->prob->items[k].p +curt_sln->prob->items[j].p-curt_sln->prob->items[i].p;
            //                     if(delta > best_delta){
            //                         best_delta = delta; best_move[0] = i; best_move[1] = j; best_move[2]=k;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }
            // if(best_delta>0) { apply_move(nb_indx, &best_move[0], best_neighb);}
            break;
        
        // case 4:

            // break;
        
        // default:
            // printf("Neighbourhood index is out of the bounds, nothing is done!\n");
    }
    return best_neighb;
}

//  best descent
struct solution_struct* greedy_heuristic (struct problem_struct* prob) {
    int total_n = prob->n;
    int n = prob->n; // unpacked items number
    int cap = prob->capacities; // capacities of each bin
    struct item_struct* items = prob->items; // all items of the problem

    struct solution_struct* sol = new solution_struct(); // initialize the solution
    sol->prob = prob;
    // struct solution_struct* sol = initialize_empty_sol(prob); // initialize the solution

    // initialize a bin
    bin_struct *bin = new bin_struct();
    bin->cap_left = cap;

    bool isFound = false;

    // qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc1); // descending
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc2); // ascending

    // let it run infinitely until some conditions are reached
    while(1) {

        if (n == 0) {
            // add the last bin
            sol->bins.push_back(*bin);
            delete(bin);
            sol->objective += 1;

            break; // if all items packed
        }
        
        item_struct* choice = NULL; // record the best descent

        for (int i=0; i<total_n; i++) {
            // if the item is small enough to be added into the bin
            if (!items[i].isPacked && bin->cap_left >= items[i].size) {
                isFound = true; 

                if (choice == NULL) { 
                    choice = & items[i]; 
                }

                // compete with the current known best descent                
                if (items[i].size > choice->size) {
                    choice = & items[i];
                }
            }
        }

        if (isFound) {
            
            bin->packed_items.push_back(*choice);
            bin->cap_left -= choice->size;
            choice->isPacked = true;
            n--; // minus the total unpacked item number

        } else { 
            
            // the current bin is too small to hold any item, push back the bin
            sol->bins.push_back(*bin);
            delete(bin);
            sol->objective += 1;

            // initialize a new empty bin
            bin = new bin_struct();
            bin->cap_left = cap;
        }
    
        isFound = false;
    }

    return sol;
}

// void varaible_neighbourhood_search(struct problem_struct* prob) {
void varaible_neighbourhood_search(struct problem_struct* prob){

    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int nb_indx = 0; //neighbourhood index, start from 2, 0 or 1 is meaningless
    
    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);

    // Test code here
    cout << "Initialize a possible answer: " << endl;
    cout << "Objectives: " << best_sln.objective << endl;
    cout << "Known best: " << prob->known_best << endl << endl;


    int shaking_count =0;
    while(time_spent < MAX_TIME) {

        // while(nb_indx < K) {
            // cout << "nb_indx=" << nb_indx << endl;
            struct solution_struct* neighb_s=best_descent_vns(nb_indx+1, curt_sln);

            if (isImproving) {
                cout << "is improving" << endl;
                copy_solution(curt_sln, neighb_s);
                cout << curt_sln->objective << endl;
                nb_indx=0;
            }
            else {
                cout << "no improment, nb+1" << endl;
                // nb_indx ++;
                ////
                break;
                ////
            }
            isImproving = false;

            // cout << endl;

            if (curt_sln->objective == curt_sln->prob->known_best) {
                break;
            }
            // delete(neighb_s);
        // }
        update_best_solution(curt_sln);

        // update_best_solution(curt_sln);
        // double gap = 1000;
        // gap = (best_sln.prob->known_best - best_sln.objective) / best_sln.prob->known_best;
        // vns_shaking(curt_sln, SHAKE_STRENGTH);
        // shaking_count ++;
        // nb_indx = 0;
        
    //     break;

        time_fin = clock();
        time_spent = (double) (time_fin - time_start) /CLOCKS_PER_SEC;
    }


    delete(curt_sln);

}


int main(int argc, const char * argv[]) {
    
    printf("Starting the run...\n");

    char data_file[50]={"somefile"}, out_file[50]={}, solution_file[50]={};  //max 50 problem instances per run

    if(argc<3)
    {
        printf("Insufficient arguments. Please use the following options:\n   -s data_file (compulsory)\n   -o out_file (default my_solutions.txt)\n   -c solution_file_to_check\n   -t max_time (in sec)\n");
        return 1;
    }
    else if(argc>9)
    {
        printf("Too many arguments.\n");
        return 2;
    }
    else
    {
        for(int i=1; i<argc; i=i+2)
        {
            if(strcmp(argv[i],"-s")==0)
                strcpy(data_file, argv[i+1]);
            else if(strcmp(argv[i],"-o")==0)
                strcpy(out_file, argv[i+1]);
            else if(strcmp(argv[i],"-c")==0)
                strcpy(solution_file, argv[i+1]);
            else if(strcmp(argv[i],"-t")==0)
                MAX_TIME = atoi(argv[i+1]);
        }
        printf("data_file= %s, output_file= %s, sln_file=%s, max_time=%d\n", data_file, out_file, solution_file, MAX_TIME);
    }
    struct problem_struct** my_problems = load_problems(data_file);
    
    if(strlen(solution_file)<=0)
    {
        if(strcmp(out_file,"")==0) strcpy(out_file, "my_solutions.txt"); //default output
        FILE* pfile = fopen(out_file, "w"); //open a new file

        fprintf(pfile, "%d\n", num_of_problems); fclose(pfile);

        for(int k=0; k<num_of_problems; k++)
        {
            cout << "Number of question: " << k << endl;
            best_sln.objective=1000; best_sln.feasibility=0;
            for(int run=0; run<NUM_OF_RUNS; run++) {
                printf("Running VNS...\n");
                varaible_neighbourhood_search(my_problems[k]);
            }
            output_solution(&best_sln,out_file);
        }
    }

    for(int k=0; k<num_of_problems; k++)
    {
        // printf("free problem %d\n", k);
        free_problem(my_problems[k]); //free problem data memory
    }

    free(my_problems); //free problems array
    
    // if (best_sln.x!=NULL && best_sln.cap_left != NULL) {
    //     free(best_sln.cap_left); 
    //     free(best_sln.x);
    // } 
    
    //free global
    return 0;
}
