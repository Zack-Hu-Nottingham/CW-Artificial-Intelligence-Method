#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <time.h>
#include <vector>
using namespace std;

/* global parameters */
int MAX_TIME = 30;  // max amount of time permited (in sec)
int num_of_problems; // number of problems

// the struct for item
struct item{
    int size; // size of item
    int index; // id of item
};

// the struct for problem
struct problem{
    int n; // number of items
    item* items; // array of items
    int capacity; // max capacity of a bin in the problem
    int best_obj; // best known objective
    char name[20]; // the name of problem
};

// the struct for bin
struct bin{
    vector<item> items; // using vector to store items for each bin
    int cap_left; // the left capacity
};

// the struct for solution
struct solution{
    problem* prob; // maintain a reference to its problem
    int objective; // the objective that this solution ca get
    bool feasibility; //indicate the feasibility of the solution
    vector<bin> bins;
};

/* declare parameters for variable neighbourhood search here*/
int K = 3; // k-opt is used
int SHAKE_MAX_STRENGTH = 12; // define random shaking strength for vns shaking
solution best_sln;  // global best solution

// return a random number ranging from min to max (inclusive)
int rand_int(int min, int max)
{
    int div = max-min+1;
    int val =rand() % div + min;
    //printf("rand_range= %d \n", val);
    return val;
}

// free the allocated data for struct problem
void free_problem(problem* prob, int len)
{
    if(prob!=NULL)
    {
        for (int i = 0; i < len; i++)
        {
            delete[] (prob[i].items);
        }
        
    }
}

// initialize struct problem
void init_problem(int n, problem* my_prob)
{
    problem new_prob;
    new_prob.n=n; 
    new_prob.items=new item[n];
    *my_prob = new_prob;
}


// load problem instances from file
problem* load_problems(char* data_file)
{
    int i,j,k;
    FILE* pfile = fopen(data_file, "r"); // open source file
    if(pfile==NULL)
        {printf("Data file %s does not exist. Please check!\n", data_file); exit(2); }
    fscanf (pfile, "%d", &num_of_problems); // get number of problems
    
    problem* my_problems = new problem[num_of_problems]; // initializeing the array of problems
    for(k=0; k<num_of_problems; k++)
    {
        int n, obj_opt,capacity;
        char name[20];
        fscanf (pfile, "%s", name);
        fscanf (pfile, "%d", &capacity);
        fscanf (pfile, "%d", &n);
        fscanf (pfile, "%d", &obj_opt);

        // load data to the problems
        init_problem(n, &my_problems[k]);  
        my_problems[k].best_obj = obj_opt;
        my_problems[k].capacity = capacity;
        strcpy(my_problems[k].name,name);
        for(j=0; j<n; j++)
        {
            my_problems[k].items[j].index=j;
            fscanf(pfile, "%d", &(my_problems[k].items[j].size));
        }
    }
    fclose(pfile); //close file
    return my_problems;
}

//copy a solution from another solution, return true if copy successfully
bool copy_solution(solution* dest_sln, solution* source_sln)
{
    if(source_sln == NULL) return false;
    if(dest_sln == NULL)
    {
        dest_sln = new solution();
    }
    dest_sln->bins = source_sln->bins;
    dest_sln->prob = source_sln->prob;
    dest_sln->feasibility = source_sln->feasibility;
    dest_sln->objective = source_sln->objective;
    return true;
}


// output a given solution to a file
void output_solution(struct solution* sln, char* out_file)
{
    if(out_file !=NULL){
        FILE* pfile = fopen(out_file, "a"); //append solution data
        int gap = sln->objective - sln->prob->best_obj;
        fprintf(pfile, "%s\n", sln->prob->name); // print problem name
        fprintf(pfile, " obj=   %i   %i\n", sln->objective, gap); // print objective

        // print item indexes
        for(int i=0; i<sln->bins.size(); i++)
        {
            for (int j = 0; j < sln->bins[i].items.size(); j++)
            {
                fprintf(pfile, "%i ", sln->bins[i].items[j].index);
            }
            fprintf(pfile, "\n");
        }
        fclose(pfile); // close file
    }
    else
        printf("sln.feas=%d, sln.obj=%f\n", sln->feasibility, sln->objective);
}


//update global best solution from sln
void update_best_solution(struct solution* sln)
{
    if(best_sln.objective > sln->objective)
    copy_solution(&best_sln, sln);
}

// The compare function for struct item
int cmpItemSize(const void* a, const void* b){
    const item* item1 = (const item*)a;
    const item* item2 = (const item*)b;
    if(item1->size>item2->size) return -1;
    if(item1->size<item2->size) return 1;
    return 0;
}

// The greedy heuristic solution, using the best fit algorithm. Each time we 
// try to pack the item into the available bin with smallest capacity left. 
// If now such bin exists, pack the item into a new empty bin.
solution* greedy_heuristic(problem* prob)
{
    // sort the items with size in descending order
    qsort(prob->items,prob->n,sizeof(item),cmpItemSize);
    // initialize the initial solution
    solution* initialSol = new solution();
    initialSol->prob = prob;
    initialSol->feasibility = 1;

    int size = 0; // the number of bins
    int targetIndex; // the target index that the item should be packed to
    for (int i = 0; i < prob->n; i++)
    {
        targetIndex = -1; // set initial target index
        for (int j = 0; j < size; j++)
        {
            // update target index
            if (initialSol->bins[j].cap_left>=prob->items[i].size)
            {
                if (targetIndex==-1)
                {
                    targetIndex=j;
                } else {
                    if (initialSol->bins[j].cap_left<initialSol->bins[targetIndex].cap_left)
                    {
                        targetIndex=j;
                    }
                }
            }
        }

        // apply packing
        if (targetIndex==-1)
        {
            // create a new bin if no current bin is suitable
            size++;
            bin new_bin;
            new_bin.items.push_back(prob->items[i]);
            new_bin.cap_left=prob->capacity-prob->items[i].size;
            initialSol->bins.push_back(new_bin);
        } else {
            // pack the item into the target bin
            initialSol->bins[targetIndex].cap_left-=prob->items[i].size;
            initialSol->bins[targetIndex].items.push_back(prob->items[i]);
        }
    }
    initialSol->objective=size;
    return initialSol;
}

// best descent function for VNS, neighborhood index is from 1 to 5
struct solution* best_descent_vns(int nb_indx, solution* curt_sln, bool* isChanged)
{
    // initialize best neighbor
    solution* best_neighb = new solution();
    copy_solution(best_neighb, curt_sln);

    int n = curt_sln->bins.size(); // store number of bins
    // store data for bin indexes, item indexes, and best fitness value so far bestDelta
    int binIndex1, binIndex2, binIndex3, itemIndex1, itemIndex2, itemIndex3, itemIndex4, itemIndex5, bestDelta;
    bestDelta = 0;

    // switch according to different neighborhood index
    switch (nb_indx)
    {
        // For the first neighborhood, check whether any item in the bin with larger index can
        // be packed to the bin with smaller index. The fitness value is simply the size of
        // item we can move which we mentioned above.
        case 1: 
            // traverse bins, let i denotes the index of the first bin
            for(int i=0; i<n; i++){
                int left_cap = best_neighb->bins[i].cap_left;
                if (left_cap<=0)
                {
                    continue; // only focus on the bin which is not full
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i + 1; j < n; j++)
                {
                    // traverse all items in bin j
                    for (int k = 0; k < best_neighb->bins[j].items.size(); k++)
                    {
                        // check if item k can be packed in to bin i and whether we can have a better fitness value
                        if (best_neighb->bins[j].items[k].size<=left_cap&&best_neighb->bins[j].items[k].size>bestDelta)
                        {
                            bestDelta = best_neighb->bins[j].items[k].size; // update best fitness value
                            // update the data for the move that generate best fitness value
                            binIndex1 = i;
                            binIndex2 = j;
                            itemIndex1 = k;
                        }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if(bestDelta>0) {
                
                *isChanged = true; // denotes the best_neighb is different from the original solution
                // copy the target item
                item copyItem;
                copyItem.index = best_neighb->bins[binIndex2].items[itemIndex1].index;
                copyItem.size = best_neighb->bins[binIndex2].items[itemIndex1].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem.size;
                best_neighb->bins[binIndex2].cap_left+=copyItem.size;
                // pack the item into new bin
                best_neighb->bins[binIndex1].items.push_back(copyItem);
                // delete the rubbish item
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex1);
                // check if we have an empty bin now
                if (best_neighb->bins[binIndex2].items.size()==0)
                {
                    best_neighb->bins.erase(best_neighb->bins.begin()+binIndex2); // delete empty bin
                    best_neighb->objective--; // update objective
                }
                
            }
            break;

        // For the second neighborhood, check whether we can interchange an item in the bin 
        // with smaller index and an item in the bin with larger index. The fitness value is 
        // the decreased capacity of the bin with smaller index we mentioned above.
        case 2:
            // traverse bins, let i denotes the index of the first bin
            for (int i = 0; i < n; i++)
            {
                if (best_neighb->bins[i].cap_left==0)
                {
                    continue; // skip the bin which is already full
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i+1; j < n; j++)
                {
                    // traverse all items in bin i, let a denotes its index
                    for (int a = 0; a < best_neighb->bins[i].items.size(); a++)
                    {
                        // traverse all items in bin j, let b denotes its index
                        for (int b = 0; b < best_neighb->bins[j].items.size(); b++)
                        {
                            // check if we can swap the item a and item b
                            if (best_neighb->bins[i].items[a].size<best_neighb->bins[j].items[b].size
                                && best_neighb->bins[j].items[b].size-best_neighb->bins[i].items[a].size<=best_neighb->bins[i].cap_left)
                            {
                                // check whether we can have a better fitness value
                                if (best_neighb->bins[j].items[b].size-best_neighb->bins[i].items[a].size>bestDelta)
                                {
                                    // update best fitness value
                                    bestDelta = best_neighb->bins[j].items[b].size-best_neighb->bins[i].items[a].size;

                                    // update the data for the move that generate best fitness value
                                    binIndex1 = i;
                                    binIndex2 = j;
                                    itemIndex1 = a;
                                    itemIndex2 = b;
                                }
                            }
                        }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if(bestDelta>0) {
                *isChanged = true; // denotes the best_neighb is different from the original solution
                // copy the target items
                item copyItem1, copyItem2;
                copyItem1.index = best_neighb->bins[binIndex1].items[itemIndex1].index;
                copyItem1.size = best_neighb->bins[binIndex1].items[itemIndex1].size;
                copyItem2.index = best_neighb->bins[binIndex2].items[itemIndex2].index;
                copyItem2.size = best_neighb->bins[binIndex2].items[itemIndex2].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem2.size-copyItem1.size;
                best_neighb->bins[binIndex2].cap_left+=copyItem2.size-copyItem1.size;
                // swap two items in two bins correspondingly
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex1);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex2);
                best_neighb->bins[binIndex1].items.push_back(copyItem2);
                best_neighb->bins[binIndex2].items.push_back(copyItem1);
            }
            break;
        
        // For the third neighborhood, say we have bin i, j, k (i<j<=k). We are trying to find item1
        // in bin i, item2 in bin j, item3 in bin k, so that we can change the positions of them.
        // We now pack the item2 and item3 to bin i, pack item3 to bin j. The fitness value is the
        // decreased capacity of the bin i.
        case 3:
            // traverse bins, let i denotes the index of the first bin
            for(int i=0; i<n; i++){
                if (best_neighb->bins[i].cap_left==0)
                {
                    continue; // skip the bin which is already full
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i + 1; j < n; j++)
                {
                    if (best_neighb->bins[j].cap_left==0)
                    {
                        continue; // skip the bin which is already full
                    }
                    // traverse bins, let k denotes the index of the third bin
                    for (int k = j; k < n; k++)
                    {
                        // traverse all items in bin i, let a denotes its index
                        for (int a = 0; a < best_neighb->bins[i].items.size(); a++)
                        {
                            // traverse all items in bin j, let b denotes its index
                            for (int b = 0; b < best_neighb->bins[j].items.size(); b++)
                            {
                                // traverse all items in bin k, let c denotes its index
                                for (int c = 0; c < best_neighb->bins[k].items.size(); c++)
                                {
                                    // in case bin j and bin k are the same bin, we need to make sure item b,c are different
                                    if (j==k&&b==c)
                                    {
                                        continue;
                                    }
                                    // Check whether we can apply the swap, making sure bin i,j has enough capacity. Check 
                                    // whether we can generate a better fitness value as well.
                                    if (best_neighb->bins[i].items[a].size<best_neighb->bins[j].items[b].size+best_neighb->bins[k].items[c].size
                                        &&best_neighb->bins[j].items[b].size+best_neighb->bins[k].items[c].size-best_neighb->bins[i].items[a].size<=best_neighb->bins[i].cap_left
                                        &&(best_neighb->bins[i].items[a].size-best_neighb->bins[j].items[b].size<=best_neighb->bins[j].cap_left
                                        ||j==k&&best_neighb->bins[i].items[a].size-best_neighb->bins[j].items[b].size-best_neighb->bins[k].items[c].size<=best_neighb->bins[j].cap_left)
                                        &&best_neighb->bins[j].items[b].size+best_neighb->bins[k].items[c].size-best_neighb->bins[i].items[a].size>bestDelta)
                                    {
                                        // update best fitness value
                                        bestDelta = best_neighb->bins[j].items[b].size+best_neighb->bins[k].items[c].size-best_neighb->bins[i].items[a].size;
                                        // update the data for the move that generate best fitness value
                                        binIndex1 = i;
                                        binIndex2 = j;
                                        binIndex3 = k;
                                        itemIndex1 = a;
                                        itemIndex2 = b;
                                        itemIndex3 = c;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // update best neighbor to the neighbor which has the best fitness value so far
            if (bestDelta>0)
            {   
                *isChanged = true; // denotes the best_neighb is different from the original solution
                // copy the target items
                item copyItem1,copyItem2,copyItem3;
                copyItem1.index = best_neighb->bins[binIndex1].items[itemIndex1].index;
                copyItem1.size = best_neighb->bins[binIndex1].items[itemIndex1].size;
                copyItem2.index = best_neighb->bins[binIndex2].items[itemIndex2].index;
                copyItem2.size = best_neighb->bins[binIndex2].items[itemIndex2].size;
                copyItem3.index = best_neighb->bins[binIndex3].items[itemIndex3].index;
                copyItem3.size = best_neighb->bins[binIndex3].items[itemIndex3].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem2.size+copyItem3.size-copyItem1.size;
                best_neighb->bins[binIndex2].cap_left-=copyItem1.size-copyItem2.size;
                best_neighb->bins[binIndex3].cap_left+=copyItem3.size;
                // swap items
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex1);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex2);
                if (binIndex2==binIndex3&&itemIndex2<itemIndex3)
                {
                    best_neighb->bins[binIndex3].items.erase(best_neighb->bins[binIndex3].items.begin()+itemIndex3-1);
                } else {
                    best_neighb->bins[binIndex3].items.erase(best_neighb->bins[binIndex3].items.begin()+itemIndex3);
                }
                best_neighb->bins[binIndex1].items.push_back(copyItem2);
                best_neighb->bins[binIndex1].items.push_back(copyItem3);
                best_neighb->bins[binIndex2].items.push_back(copyItem1);
                // update objective when we get get an empty bin
                if (best_neighb->bins[binIndex3].items.size()==0)
                {
                    best_neighb->bins.erase(best_neighb->bins.begin()+binIndex3);
                    best_neighb->objective--;
                }
            }
            break;

        // For the fourth neighborhood, we are going to swap two items in a bin with other two items
        // in another bin. In other words, we are trying to find item1, item2 in bin i, item3, item4
        // in bin j, so that we can change the positions of them. We can try to pack the item3 and 
        // item4 to bin i, and pack item1, item2 to bin j. The fitness value is the decreased capacity
        // of the bin i.
        case 4:
            // traverse bins, let i denotes the index of the first bin
            for (int i = 0; i < n; i++)
            {
                if (best_neighb->bins[i].cap_left==0||best_neighb->bins[i].items.size()<2)
                {
                    continue; // skip the bin which is already full or has no more than two items
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i+1; j < n; j++)
                {
                    if (best_neighb->bins[j].items.size()<2)
                    {
                        continue; // skip the bin which is already full or has no more than two items
                    }
                    // traverse items in bin i, let a denotes the index of the first item in bin i
                    for (int a = 0; a < best_neighb->bins[i].items.size(); a++)
                    {
                        // traverse items in bin i, let b denotes the index of the second item in bin i
                        for (int b = a+1; b < best_neighb->bins[i].items.size(); b++)
                        {
                            // traverse items in bin j, let c denotes the index of the first item in bin j
                            for (int c = 0; c < best_neighb->bins[j].items.size(); c++)
                            {
                                // traverse items in bin j, let d denotes the index of the second item in bin j
                                for (int d = c+1; d < best_neighb->bins[j].items.size(); d++)
                                {
                                    // calculate the fitness value of this new solution (neighbor)
                                    int delta = best_neighb->bins[j].items[c].size+best_neighb->bins[j].items[d].size-best_neighb->bins[i].items[a].size-best_neighb->bins[i].items[b].size;
                                    // check if the moves are feasible and whether we can have a better fitness value
                                    if (delta>bestDelta&&delta<=best_neighb->bins[i].cap_left)
                                    {
                                        // update best fitness value
                                        bestDelta = best_neighb->bins[j].items[c].size+best_neighb->bins[j].items[d].size-best_neighb->bins[i].items[a].size-best_neighb->bins[i].items[b].size;
                                        // update the data for the move that generate best fitness value
                                        binIndex1 = i;
                                        binIndex2 = j;
                                        itemIndex1 = a;
                                        itemIndex2 = b;
                                        itemIndex3 = c;
                                        itemIndex4 = d;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if (bestDelta>0)
            {
                *isChanged = true; // denotes the best_neighb is different from the original solution
                // copy the target items
                item copyItem1,copyItem2,copyItem3,copyItem4;
                copyItem1.index = best_neighb->bins[binIndex1].items[itemIndex1].index;
                copyItem1.size = best_neighb->bins[binIndex1].items[itemIndex1].size;
                copyItem2.index = best_neighb->bins[binIndex1].items[itemIndex2].index;
                copyItem2.size = best_neighb->bins[binIndex1].items[itemIndex2].size;
                copyItem3.index = best_neighb->bins[binIndex2].items[itemIndex3].index;
                copyItem3.size = best_neighb->bins[binIndex2].items[itemIndex3].size;
                copyItem4.index = best_neighb->bins[binIndex2].items[itemIndex4].index;
                copyItem4.size = best_neighb->bins[binIndex2].items[itemIndex4].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                best_neighb->bins[binIndex2].cap_left+=copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                // swap items
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex2);
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex1);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex4);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex3);
                best_neighb->bins[binIndex1].items.push_back(copyItem3);
                best_neighb->bins[binIndex1].items.push_back(copyItem4);
                best_neighb->bins[binIndex2].items.push_back(copyItem1);
                best_neighb->bins[binIndex2].items.push_back(copyItem2);
            }
            break;

        // For the fifth neighborhood, we are going to swap two items in a bin with other three items
        // in another bin. In other words, we are trying to find item1, item2 in bin i, item3, item4, 
        // item5 in bin j, so that we can change the positions of them. We can try to pack the item3 and 
        // item4, item5 to bin i, and pack item1, item2 to bin j. The fitness value is the decreased
        // capacity of the bin i.
        case 5:
            // traverse bins, let i denotes the index of the first bin
            for (int i = 0; i < n; i++)
            {
                if (best_neighb->bins[i].cap_left==0||best_neighb->bins[i].items.size()<2)
                {
                    continue; // skip the bin which is already full or has no more than two items
                }
                // traverse bins, let j denotes the index of the second bin
                for (int j = i+1; j < n; j++)
                {
                    if (best_neighb->bins[j].items.size()<3)
                    {
                        continue; // skip the bin which is already full or has no more than three items
                    }
                    // traverse items in bin i, let a denotes the index of the first item in bin i
                    for (int a = 0; a < best_neighb->bins[i].items.size(); a++)
                    {
                        // traverse items in bin i, let b denotes the index of the second item in bin i
                        for (int b = a+1; b < best_neighb->bins[i].items.size(); b++)
                        {
                            // traverse items in bin j, let c denotes the index of the first item in bin j
                            for (int c = 0; c < best_neighb->bins[j].items.size(); c++)
                            {
                                // traverse items in bin j, let d denotes the index of the second item in bin j
                                for (int d = c+1; d < best_neighb->bins[j].items.size(); d++)
                                {
                                    // traverse items in bin j, let e denotes the index of the third item in bin j
                                    for (int e = d+1; e < best_neighb->bins[j].items.size(); e++)
                                    {
                                        // calculate the fitness value of this new solution (neighbor)
                                        int delta = best_neighb->bins[j].items[c].size+best_neighb->bins[j].items[d].size+best_neighb->bins[j].items[e].size-best_neighb->bins[i].items[a].size-best_neighb->bins[i].items[b].size;
                                        // check if the moves are feasible and whether we can have a better fitness value
                                        if (delta>bestDelta&&delta<=best_neighb->bins[i].cap_left)
                                        {
                                            // update best fitness value
                                            bestDelta = best_neighb->bins[j].items[c].size+best_neighb->bins[j].items[d].size+best_neighb->bins[j].items[e].size-best_neighb->bins[i].items[a].size-best_neighb->bins[i].items[b].size;
                                            // update the data for the move that generate best fitness value
                                            binIndex1 = i;
                                            binIndex2 = j;
                                            itemIndex1 = a;
                                            itemIndex2 = b;
                                            itemIndex3 = c;
                                            itemIndex4 = d;
                                            itemIndex5 = e;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // update best neighbor to the neighbor which has the best fitness value so far
            if (bestDelta>0)
            {
                *isChanged = true; // denotes the best_neighb is different from the original solution
                // copy the target items
                item copyItem1,copyItem2,copyItem3,copyItem4,copyItem5;
                copyItem1.index = best_neighb->bins[binIndex1].items[itemIndex1].index;
                copyItem1.size = best_neighb->bins[binIndex1].items[itemIndex1].size;
                copyItem2.index = best_neighb->bins[binIndex1].items[itemIndex2].index;
                copyItem2.size = best_neighb->bins[binIndex1].items[itemIndex2].size;
                copyItem3.index = best_neighb->bins[binIndex2].items[itemIndex3].index;
                copyItem3.size = best_neighb->bins[binIndex2].items[itemIndex3].size;
                copyItem4.index = best_neighb->bins[binIndex2].items[itemIndex4].index;
                copyItem4.size = best_neighb->bins[binIndex2].items[itemIndex4].size;
                copyItem5.index = best_neighb->bins[binIndex2].items[itemIndex5].index;
                copyItem5.size = best_neighb->bins[binIndex2].items[itemIndex5].size;
                // update left capacities for bins
                best_neighb->bins[binIndex1].cap_left-=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                best_neighb->bins[binIndex2].cap_left+=copyItem5.size+copyItem4.size+copyItem3.size-copyItem1.size-copyItem2.size;
                // swap items
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex2);
                best_neighb->bins[binIndex1].items.erase(best_neighb->bins[binIndex1].items.begin()+itemIndex1);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex5);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex4);
                best_neighb->bins[binIndex2].items.erase(best_neighb->bins[binIndex2].items.begin()+itemIndex3);
                best_neighb->bins[binIndex1].items.push_back(copyItem3);
                best_neighb->bins[binIndex1].items.push_back(copyItem4);
                best_neighb->bins[binIndex1].items.push_back(copyItem5);
                best_neighb->bins[binIndex2].items.push_back(copyItem1);
                best_neighb->bins[binIndex2].items.push_back(copyItem2);
            }
            
            break;
        default:
            printf("Neighbourhood index is out of the bounds, nothing is done!\n");
    }
    return best_neighb; // return the best neighbor
}

// diversification function (shaking)
void vns_shaking(struct solution* sln, int strength)
{
    //using random pair-wise swap, strength is the number of times we need to swap
    int n = sln->prob->n;
    int m = 0, cnt = 0;
    int index1, index2;
    while(m<strength && cnt<200)
    {
        // pick bins randomly
        index1 = rand_int(0,sln->bins.size()-1);
        index2 = rand_int(0,sln->bins.size()-1);
        while (index1==index2&&cnt<200)
        {
            index2 = rand_int(0,sln->bins.size()-1);
            cnt++;
        }
        // pick items randomly
        int itemIndex1 = rand_int(0,sln->bins[index1].items.size()-1);
        int itemIndex2 = rand_int(0,sln->bins[index2].items.size()-1);
        if (sln->bins[index2].items[itemIndex2].size-sln->bins[index1].items[itemIndex1].size<=sln->bins[index1].cap_left
            && sln->bins[index1].items[itemIndex1].size-sln->bins[index2].items[itemIndex2].size<=sln->bins[index2].cap_left)
        {
            // copy target items
            item copyItem1, copyItem2;
            copyItem1.index = sln->bins[index1].items[itemIndex1].index;
            copyItem1.size = sln->bins[index1].items[itemIndex1].size;
            copyItem2.index = sln->bins[index2].items[itemIndex2].index;
            copyItem2.size = sln->bins[index2].items[itemIndex2].size;
            // update capacities for bins
            sln->bins[index1].cap_left-=copyItem2.size-copyItem1.size;
            sln->bins[index2].cap_left+=copyItem2.size-copyItem1.size;
            // apply swap
            sln->bins[index1].items.erase(sln->bins[index1].items.begin()+itemIndex1);
            sln->bins[index2].items.erase(sln->bins[index2].items.begin()+itemIndex2);
            sln->bins[index1].items.push_back(copyItem2);
            sln->bins[index2].items.push_back(copyItem1);
            m++;
        }
        cnt++;
    }
}

//VNS
int varaible_neighbourhood_search(struct problem* prob){
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int nb_indx = 0; //neighbourhood index
    
    best_sln.objective=99999999; // set initial objective

    // apply best fit greedy heuristic
    struct solution* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);
    return 0;
    // start neighborhood searching
    int shaking_count = 0;
    bool isUpdated;
    while(time_spent < MAX_TIME) // check whether exceeds time limit
    {
        while(nb_indx<K){
            isUpdated = false;
            struct solution* neighb_s=best_descent_vns(nb_indx+1, curt_sln, &isUpdated); //best solution in neighbourhood nb_indx
            if(isUpdated){
                copy_solution(curt_sln, neighb_s); // update current solution
                nb_indx=0; // reset neighborhood index
            }
            else nb_indx++;
            delete neighb_s;
        }
        update_best_solution(curt_sln);
        int gap=-1; //set to -1 if best known solution is not availabe.
        if(best_sln.prob->best_obj!=0) gap = best_sln.objective-best_sln.prob->best_obj;
        printf("shaking_count=%d, curt obj=%d, target obj=%d, gap=%d\n",shaking_count, curt_sln->objective, best_sln.prob->best_obj, gap);

        // stop searching when optimal solution is generated
        if (best_sln.objective-best_sln.prob->best_obj==0) {
            break;
        } 

        // diversification stage (shaking)
        copy_solution(curt_sln,&best_sln);
        // vns_shaking(curt_sln, rand_int(1,SHAKE_MAX_STRENGTH)); //shaking at a random strength
        shaking_count++;
        nb_indx=0;
        
        // update time
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
        
    }

    delete curt_sln;
    return 0;
}

int main(int argc, const char * argv[]) {
    srand(3); // initialize random seed
    printf("Starting the run...\n");
    char data_file[50]={"somefile"}, out_file[50], solution_file[50];  //max 50 problem instances per run

    // handle command line arguments
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
            if(strcmp(argv[i],"-s")==0) {
                strcpy(data_file, argv[i+1]);
            }
            else if(strcmp(argv[i],"-o")==0) {
                strcpy(out_file, argv[i+1]);
            }
            else if(strcmp(argv[i],"-c")==0) {
                strcpy(solution_file, argv[i+1]);
            }
            else if(strcmp(argv[i],"-t")==0) {
                MAX_TIME = atoi(argv[i+1]);
            }
        }
    }

    struct problem* my_problems = load_problems(data_file); // load problems
    
    if (strlen(solution_file)>=0)
    {
        if(strcmp(out_file,"")==0) strcpy(out_file, "my_solutions.txt"); //default output
        FILE* pfile = fopen(out_file, "w"); //open a new file
        fprintf(pfile, "%d\n", num_of_problems); 
        fclose(pfile);
        for(int k=0; k<num_of_problems; k++)
        {
            printf("Problem_%d:\n",k);
            varaible_neighbourhood_search(&my_problems[k]); // apply vns
            output_solution(&best_sln,out_file); // output solution
        }
    }

    //free memory
    free_problem(my_problems,num_of_problems); 
    delete[] my_problems;

    return 0;
}