//
//  main.c
//  mknapsack
//
//  Created by Bai on 14/03/2020.
//  Copyright © 2019 UNNC. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

/* global parameters */
int RAND_SEED[] = {1,20,30,40,50,60,70,80,90,100,110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
int NUM_OF_RUNS = 2;
int MAX_TIME = 30;  //max amount of time permited (in sec)
int num_of_problems;
enum ALG{SA=1,TS=2,VNS=3};
enum ALG alg=VNS;


/* parameters for evlutionary algorithms*/
static int POP_SIZE = 50;   //please modify these parameters according to your problem
int MAX_NUM_OF_GEN = 10000; //max number of generations
float CROSSOVER_RATE = 0.8;
float MUTATION_RATE = 0.1;

/* declare parameters for simulated annealing here */
float SA_TS =500;
float SA_TF =10;
float SA_BETA = 0.00000001;
int SA_MAX_ITER = 200000000; //total number of runs.
int SA_ITER_PER_T = 1; //number of runs per temperature

/* declare parameteres for tabu search here*/
int TABU_TENURE = 20;
int TS_MAX_ITER = 2000000;
struct tabu_struct{
    int item1;
    int item2;
    int tabu_tenure; //iterations to remain in tabu
};

struct move_struct{
    int item1;
    int item2;
};

/* declare parameters for variable neighbourhood search here*/
int K= 3; // k-opt is used
int SHAKE_STRENGTH = 12;
struct solution_struct best_sln;  //global best solution

//return a random number between 0 and 1
float rand_01()
{
    float number;
    number = (float) rand();
    number = number/RAND_MAX;
    //printf("rand01=%f\n", number);
    return number;
}

//return a random nunber ranging from min to max (inclusive)
int rand_int(int min, int max)
{
    int div = max-min+1;
    int val =rand() % div + min;
    //printf("rand_range= %d \n", val);
    return val;
}


struct item_struct{
    int dim; //no. of dimensions
    int* size; //volume of item in all dimensions
    int p;
    double ratio;
    int indx;
};

struct problem_struct{
    int n; //number of items
    int dim; //number of dimensions
    struct item_struct* items;
    int* capacities;  //knapsack capacities
    double best_obj; //best known objective
};

struct solution_struct{
    struct problem_struct* prob; //maintain a shallow copy of the problem data
    float objective;
    int feasibility; //indicate the feasiblity of the solution
    int* x; // solution encoding vector 代表有没有被选中
    int* cap_left; //capacity left in all dimensions
};

void free_problem(struct problem_struct* prob)
{
    if(prob!=NULL)
    {
        if(prob->capacities !=NULL) free(prob->capacities);
        if(prob->items!=NULL)
        {
            for(int j=0; j<prob->n; j++)
            {
                if(prob->items[j].size != NULL)
                    free(prob->items[j].size);
            }
            free(prob->items);
        }
        free(prob);
    }
}

void init_problem(int n, int dim, struct problem_struct** my_prob)
{
    struct problem_struct* new_prob = malloc(sizeof(struct problem_struct));
    new_prob->n=n; new_prob->dim=dim;
    new_prob->items=malloc(sizeof(struct item_struct)*n);
    for(int j=0; j<n; j++)
        new_prob->items[j].size= malloc(sizeof(int)*dim);
    new_prob->capacities = malloc(sizeof(int)*dim);
    *my_prob = new_prob;
}


//example to create problem instances, actual date should come from file
struct problem_struct** load_problems(char* data_file)
{
    int i,j,k;
    //int num_of_probs;
    FILE* pfile = fopen(data_file, "r");
    if(pfile==NULL)
        {printf("Data file %s does not exist. Please check!\n", data_file); exit(2); }
    fscanf (pfile, "%d", &num_of_problems);
 
    struct problem_struct** my_problems = malloc(sizeof(struct problem_struct*)*num_of_problems);
    for(k=0; k<num_of_problems; k++)
    {
        int n, dim, obj_opt;
        fscanf (pfile, "%d", &n);
        fscanf (pfile, "%d", &dim); fscanf (pfile, "%d", &obj_opt);
        
        init_problem(n, dim, &my_problems[k]);  //allocate data memory
        my_problems[k]->best_obj = obj_opt;
        for(j=0; j<n; j++)
        {
            my_problems[k]->items[j].dim=dim;
            my_problems[k]->items[j].indx=j;
            fscanf(pfile, "%d", &(my_problems[k]->items[j].p)); //read profit data
            //printf("item[j].p=%d\n",my_problems[k]->items[j].p);
        }
        for(i=0; i<dim; i++)
        {
            for(j=0; j<n; j++)
            {
                fscanf(pfile, "%d", &(my_problems[k]->items[j].size[i])); //read size data
                //printf("my_problems[%i]->items[%i].size[%i]=%d\n",k,j,i,my_problems[k]->items[j].size[i]);
            }
        }
        for(i=0; i<dim; i++){
            fscanf(pfile, "%d", &(my_problems[k]->capacities[i]));
            //printf("capacities[i]=%d\n",my_problems[k]->capacities[i] );
        }
    }
    fclose(pfile); //close file
    return my_problems;
}

void free_solution(struct solution_struct* sln)
{
    if(sln!=NULL)
    {
        free(sln->x);
        free(sln->cap_left);
        sln->objective=0;
        sln->prob=NULL;
        sln->feasibility=false;
    }
}

//copy a solution from another solution
bool copy_solution(struct solution_struct* dest_sln, struct solution_struct* source_sln)
{
    if(source_sln ==NULL) return false;
    if(dest_sln==NULL)
    {
        dest_sln = malloc(sizeof(struct solution_struct));
    }
    else{
        free(dest_sln->cap_left);
        free(dest_sln->x);
    }
    int n = source_sln->prob->n;
    int m =source_sln->prob->dim;
    dest_sln->x = malloc(sizeof(int)*n);
    dest_sln->cap_left=malloc(sizeof(int)*m);
    for(int i=0; i<m; i++)
        dest_sln->cap_left[i]= source_sln->cap_left[i];
    for(int j=0; j<n; j++)
        dest_sln->x[j] = source_sln->x[j];
    dest_sln->prob= source_sln->prob;
    dest_sln->feasibility=source_sln->feasibility;
    dest_sln->objective=source_sln->objective;
    return true;
}



void evaluate_solution(struct solution_struct* sln)
{
    //evaluate the feasibility and objective of the solution
    sln->objective =0; sln->feasibility = 1;
    struct item_struct* items_p = sln->prob->items;
    
    for(int i=0; i< items_p->dim; i++)
    {
        sln->cap_left[i]=sln->prob->capacities[i];
        for(int j=0; j<sln->prob->n; j++)
        {
            sln->cap_left[i] -= items_p[j].size[i]*sln->x[j];
            if(sln->cap_left[i]<0) {
                sln->feasibility = -1*i; //exceeding capacity
                return;
            }
        }
    }
    if(sln->feasibility>0)
    {
        for(int j=0; j<sln->prob->n; j++)
        {
            sln->objective += sln->x[j] * items_p[j].p;
        }
    }
}

//output a given solution to a file
void output_solution(struct solution_struct* sln, char* out_file)
{
    if(out_file !=NULL){
        FILE* pfile = fopen(out_file, "a"); //append solution data
        double gap=1000; //set to an arbitrarily large number if best known solution is not availabe.
        if(best_sln.prob->best_obj!=0) gap=  100*(best_sln.prob->best_obj - best_sln.objective)/best_sln.prob->best_obj;
        fprintf(pfile, "%i \t %0.2f\n", (int)sln->objective, gap);
        for(int i=0; i<sln->prob->n; i++)
        {
            fprintf(pfile, "%i ", sln->x[i]);
        }
        fprintf(pfile, "\n");
        /*for(int j=0; j<sln->prob->n; j++)
            fprintf(pfile, "%i ", sln->prob->items[j].p);
        fprintf(pfile, "\n");*/
        fclose(pfile);
    }
    else
        printf("sln.feas=%d, sln.obj=%f\n", sln->feasibility, sln->objective);
}


//check the  feasiblity and obj values of solutons from solution_file.
//return 0 is all correct or output infeasible solutions to files.
int check_solutions(struct problem_struct** my_problems, char* solution_file)
{
    FILE * pfile= fopen(solution_file, "r");
    if(pfile==NULL)
    {
        printf("Solution file %s does not exist. Please check!\n", solution_file);
        exit(2);
    }
    float val_obj;
    int val;
    fscanf (pfile, "%i", &val);
    if(val != num_of_problems)
    {
        printf("The stated number of solutions does not match the number of problems.\n");
        exit(3);
    }
    
    fscanf(pfile, "%d", &val); //get rid of gap information.
    struct solution_struct temp_sln;
    int count=0, k=0;
    int n, dim;
    while(fscanf (pfile, "%f", &val_obj)!=EOF && k<num_of_problems)
    {
        //val_obj = val;
        n= my_problems[k]->n;  dim= my_problems[k]->dim;
        temp_sln.x = malloc(sizeof(int)*n);
        temp_sln.cap_left=malloc(sizeof(int)*dim);
        temp_sln.prob = my_problems[k];
        while(fscanf (pfile, "%i", &val)!=EOF)
        {
            if(val<0 || val>1) {fclose(pfile);  return k+1;} //illeagal values
            temp_sln.x[count] = val;
            count++;
            if(count==n)
            {
                evaluate_solution(&temp_sln);
                if(!temp_sln.feasibility || fabs(temp_sln.objective - val_obj)>0.01)
                {
                    fclose(pfile);
                    //printf("feasb=%i, obj= %f, val=%i\n",temp_sln.feasibility, temp_sln.objective, val_obj);
                    //output_solution(&temp_sln, "my_debug.txt");
                    return k+1;  //infeasible soltuion or wrong obj
                }
                else{
                    break;
                }
            }
        }
        count=0; k++;
        
        free(temp_sln.x); free(temp_sln.cap_left);
    }
    fclose(pfile);
    return 0;
}

//modify the solutions that violate the capacity constraints
void feasibility_repair(struct solution_struct* pop)
{
    //todo
}

//local search
void local_search_first_descent(struct solution_struct* pop)
{
    //todo
}


//update global best solution from sln
void update_best_solution(struct solution_struct* sln)
{
    if(best_sln.objective < sln->objective)
    copy_solution(&best_sln, sln);
}

int cmpfunc1(const void* a, const void* b){
    const struct item_struct* item1 = a;
    const struct item_struct* item2 = b;
    if(item1->ratio>item2->ratio) return -1;
    if(item1->ratio<item2->ratio) return 1;
    return 0;
    }
int cmpfunc2 (const void * a, const void * b) {
        const struct item_struct* item1 = a;
        const struct item_struct* item2 = b;
        if(item1->indx>item2->indx) return 1;
        if(item1->indx<item2->indx) return -1;
        return 0;
    }

int cmpfunc_sln (const void * a, const void * b) {
    const struct solution_struct* sln1 = a;
    const struct solution_struct* sln2 = b;
    if(sln1->objective > sln2 ->objective) return -1;
    if(sln1->objective < sln2 ->objective) return 1;
    return 0;
}

//a greedy heuristic solution based on profit/volume ratio
struct solution_struct* greedy_heuristic(struct problem_struct* prob)
{
    for(int i=0; i<prob->n;i++){
        double avg_size=0;
        struct item_struct* item_i = &prob->items[i];
        for(int d=0; d< prob->dim; d++){
            avg_size += (double)item_i->size[d]/prob->capacities[d];
        }
        item_i->ratio = item_i->p/avg_size;
    }
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc1);
    
    struct solution_struct* init_sln = malloc(sizeof(struct solution_struct));
    init_sln->prob=prob;    init_sln->objective =0;
    init_sln->x = malloc(sizeof(int)*prob->n);
    init_sln->cap_left = malloc(sizeof(int)*prob->dim);
    int* cap = malloc(sizeof(int)*prob->dim);
    int i=0, d=0;
    for(d=0; d<prob->dim; d++) cap[d]=0; //aggregated volume
    for(i=0; i<prob->n; i++)
    {
        struct item_struct* item_i = &prob->items[i];
        //printf("item[%d].ratio = %.3f\t",item_i->indx,prob->items[i].ratio);
        for(d=0; d<prob->dim; d++){
            if(cap[d] + item_i->size[d] > prob->capacities[d])
                break; //infeasible to pack this item, try next
        }
        if(d>=prob->dim){
            init_sln->x[item_i->indx] = 1;
            init_sln->objective += item_i->p;
            for(d=0; d<prob->dim; d++){
                cap[d] += item_i->size[d];
            }
            //printf("packing item %d\n", item_i->indx);
        }
        else init_sln->x[item_i->indx] =0;
    }
    for(d=0; d<prob->dim; d++){
        init_sln->cap_left[d] = prob->capacities[d]- cap[d];
    }
    free(cap);
    //restore item original order by sorting by index.
    qsort(prob->items, prob->n, sizeof(struct item_struct), cmpfunc2);
    
    evaluate_solution(init_sln);
    //output_solution(init_sln, "greedy_sln.txt");
    printf("Init_sln obj=\t%0.0f\tfeasiblity = %d.\n", init_sln->objective, init_sln->feasibility);
    return init_sln;
}
//Simulated Annealing
int SimulatedAnnealing(struct problem_struct* prob)
{
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int iter =0;
    double temperature=SA_TS;
    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);
    struct solution_struct* rand_neighb=malloc(sizeof(struct solution_struct));
    rand_neighb->cap_left= malloc(sizeof(int)*prob->dim);
    rand_neighb->x = malloc(sizeof(int)*prob->n);
    while(iter<SA_MAX_ITER && time_spent < MAX_TIME && temperature > SA_TF)
    {
        //add your SA code here
        copy_solution(rand_neighb, curt_sln);
        int item1, item2;
        item1 = rand_int(0, prob->n-1);
        if(curt_sln->x[item1] ==1){
            item2 = rand_int(0, prob->n-1);
            while(curt_sln->x[item2] ==1){//careful, this might lead to deadloop
                item2 = rand_int(0, prob->n-1);
            }
        }
        else{
            item2 = rand_int(0, prob->n-1);
            while(curt_sln->x[item2] ==0){//careful, this might lead to deadloop
                item2 = rand_int(0, prob->n-1);
            }
            int temp = item1;
            item1 = item2;
            item2 = temp;
        }
        
        //testing potential constraint violations after swap
        bool flag=true;
        for(int d=0; d<prob->dim; d++){
            if(rand_neighb->cap_left[d] + prob->items[item1].size[d] <
               prob->items[item2].size[d]){
                flag=false;
                break;
            }
        }
        if(flag){//can swap
            float delta = prob->items[item2].p - prob->items[item1].p;
            if(delta>=0 || (delta<0 && exp(delta/temperature)> rand_01())){
                rand_neighb->x[item1]=0;
                rand_neighb->x[item2]=1;
                rand_neighb->objective += delta;
                for(int d=0; d<prob->dim; d++){
                    rand_neighb->cap_left[d] +=  prob->items[item1].size[d] - prob->items[item2].size[d];
                }
            }
            copy_solution(curt_sln, rand_neighb);
            update_best_solution(curt_sln);
            if(iter%100 ==0)
                printf("tempereature=%0.2f, curt obj =%0.0f,\t best obj=%0.0f\n",temperature, curt_sln->objective, best_sln.objective);
        }
        temperature = temperature/(1+SA_BETA*temperature);
        iter++;
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
    }

    //output_solution(&best_sln, "SA_debug.txt");
    free_solution(curt_sln);
    free_solution(rand_neighb);
    free(rand_neighb);
    return 0;
}

bool can_swap(struct solution_struct* sln, int out, int in)
{
    for(int d =0; d<sln->prob->dim; d++)
    {
        if(sln->cap_left[d]+sln->prob->items[out].size[d] < sln->prob->items[in].size[d])
            return false;
    }
    return true;
}

struct move_struct get_move(struct solution_struct* new_sln, struct solution_struct* orig_sln){
    struct move_struct mv;
    int count=0;
    for(int i=0; i<orig_sln->prob->n; i++)
    {
        if(new_sln->x[i] != orig_sln->x[i]){
            if(count==0){ mv.item1 = i; count =1;}
            else mv.item2=i;
        }
    }
    return mv;
}

//return the position of the move in the tabu lift. return -1 if not in the list
int get_tabu_pos(struct tabu_struct* tabu_list, struct move_struct mv, int count){
    for(int i=0; i<count; i++)
    {
        if(mv.item1==tabu_list[i].item1 && mv.item2 == tabu_list[i].item2) return i;
        if(mv.item1==tabu_list[i].item2 && mv.item2 == tabu_list[i].item1) return i;
    }
    return -1;
}

void update_tabu_list(struct tabu_struct* tabu_list, struct move_struct mv, int count){
    //update
    for(int i=0; i<count; i++)
    {
        tabu_list[i].tabu_tenure -= 1;
        if(tabu_list[i].tabu_tenure<=0)
        {
            tabu_list[i].item1=-1; tabu_list[i].item2=-1; //remove from tabu
        }
    }
        
    //insert mv
    int p=get_tabu_pos(tabu_list, mv, count);
    if(p>=0)
    {
        tabu_list[p].tabu_tenure= TABU_TENURE;
    }
    else{
        for(int i=0; i<count; i++)
        {
            if(tabu_list[i].item1<0 || tabu_list[i].item2<0) { //insert in first empty entry
                tabu_list[i].item1=mv.item1; tabu_list[i].item2=mv.item2;
                tabu_list[i].tabu_tenure = TABU_TENURE;
                break;
            }
        }
    }
}

//TS method
int TabuSearch(struct problem_struct* prob){
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int iter =0;
    struct tabu_struct tabu_list[100];  //arbitrially large enough
    for(int i=0; i<100; i++) {
        tabu_list[i].item1=-1; tabu_list[i].item2=-1; tabu_list[i].tabu_tenure=0;
    }
    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);
    
    int neighb_size_max = prob->n*prob->n/4; //maximum num of neighbouring solutions
    struct solution_struct* N_S=malloc(sizeof(struct solution_struct)*neighb_size_max);
    for(int k=0; k<neighb_size_max; k++)
    {
        N_S[k].cap_left= malloc(sizeof(int)*prob->dim);
        N_S[k].x = malloc(sizeof(int)*prob->n);
        copy_solution(&N_S[k], curt_sln);
        N_S[k].objective = -10000;
    }
    
    
    while(iter< TS_MAX_ITER && time_spent < MAX_TIME)
    {
        int count=0; //count the number of feasible neighbours
        int i, j;
        for(i=0; i<prob->n; i++){
            if(curt_sln->x[i]>0){
                //item1 =i;
                for(j=0; j<prob->n; j++){
                    //item2 = j;
                    if(i!=j && curt_sln->x[j]==0 && can_swap(curt_sln,i, j))
                    {
                        copy_solution(&N_S[count], curt_sln);
                        for(int d=0; d<prob->dim; d++){
                            N_S[count].cap_left[d] = N_S[count].cap_left[d]+ prob->items[i].size[d]-prob->items[j].size[d];
                        }
                        int delta =curt_sln->prob->items[j].p -curt_sln->prob->items[i].p;
                        N_S[count].objective = curt_sln->objective + delta;
                        N_S[count].x[i] = 0; //swap
                        N_S[count].x[j] = 1;
                        /*evaluate_solution(&N_S[count]);
                        if(N_S[count].feasibility<0){
                            printf("infeasible sln=%d\n", N_S[count].feasibility);
                            output_solution(&N_S[count], "TS_debug.txt");
                        }*/
                        
                        count++;
                    }
                }
            }
        }
        
        qsort(N_S, count, sizeof(struct solution_struct), cmpfunc_sln);
        for(int k =0; k<count; k++){
            //printf("sln[%d].obj = %.0f\n",count,N_S[k].objective);
            struct move_struct mv = get_move(&N_S[k], curt_sln);
            if(get_tabu_pos(tabu_list, mv, 100)>=0)
            {
                if(N_S[k].objective > best_sln.objective){
                    copy_solution(curt_sln, &N_S[k]); //aspiration criteria
                    update_tabu_list(tabu_list, mv,100);
                    break;
                }
            }
            else{
                copy_solution(curt_sln, &N_S[k]);
                update_tabu_list(tabu_list, mv,100);
                break;
            }
        }
        update_best_solution(curt_sln);
        if(iter%100 ==0)
        printf("iter=%d, curt obj =%0.0f,\t best obj=%0.0f\n",iter, curt_sln->objective, best_sln.objective);
        iter++;
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
    }

    output_solution(&best_sln, "TS_debug.txt");
    free_solution(curt_sln);
    for(int k=0; k<neighb_size_max; k++)
        free_solution(&N_S[k]);
    free(N_S);
    return 0;
    
}

bool can_move(int nb_indx, int* move, struct solution_struct* curt_sln ){
    bool ret=true;
    if(nb_indx==1)
    {
        int i = move[0];
        if(i<0) return false;
        for(int d=0; d<curt_sln->prob->dim; d++){
            if(curt_sln->cap_left[d] < curt_sln->prob->items[i].size[d])
                return false;
        }
    }
    else if(nb_indx==2){
        ret=can_swap(curt_sln, move[0], move[1]);
    }
    else if(nb_indx==3){//3-item swap
        int i= move[0], j= move[1], k= move[2];
        if(i<0 || j<0 || k<0) return false;
        if(curt_sln->x[j]>0) {//2-1 swap
            for(int d=0; d<curt_sln->prob->dim; d++){
                if(curt_sln->cap_left[d] + curt_sln->prob->items[i].size[d] +
                   curt_sln->prob->items[j].size[d] < curt_sln->prob->items[k].size[d])
                    return false;
            }
        }
        else {//1-2 swap
            for(int d=0; d<curt_sln->prob->dim; d++){
                if(curt_sln->cap_left[d] + curt_sln->prob->items[i].size[d] <
                   curt_sln->prob->items[j].size[d] + curt_sln->prob->items[k].size[d])
                    return false;
            }
        }
        
    }
    else ret=false;
    return ret;
}

bool apply_move(int nb_indx, int* move, struct solution_struct* sln ){
    bool ret=true;
    if(nb_indx==1)
    {
        int i = move[0];
        if(i<0) return false;
        for(int d=0; d<sln->prob->dim; d++){
            sln->cap_left[d] -= sln->prob->items[i].size[d];
        }
        sln->objective += sln->prob->items[i].p;
        sln->x[i]=1;
        
        //printf("success\n");
    }
    else if(nb_indx==2){
        for(int d=0; d<sln->prob->dim; d++){
            sln->cap_left[d] = sln->cap_left[d] + sln->prob->items[move[0]].size[d]-
                sln->prob->items[move[1]].size[d];
        }
        sln->objective += sln->prob->items[move[1]].p-sln->prob->items[move[0]].p;
        sln->x[move[0]]=0; sln->x[move[1]]=1;
    }
    else if(nb_indx==3){//3-item swap
        int i= move[0], j= move[1], k= move[2];
        if(i<0 || j<0 || k<0) return false;
        if(sln->x[j]>0) {//2-1 swap
            for(int d=0; d<sln->prob->dim; d++){
                sln->cap_left[d] = sln->cap_left[d]+sln->prob->items[i].size[d] +
                    sln->prob->items[j].size[d] - sln->prob->items[k].size[d];
            }
            sln->objective += sln->prob->items[k].p-sln->prob->items[i].p-sln->prob->items[j].p;
            sln->x[i]=0; sln->x[j]=0; sln->x[k]=1;
        }
        else {//1-2 swap
            for(int d=0; d<sln->prob->dim; d++){
                sln->cap_left[d] = sln->cap_left[d]+sln->prob->items[i].size[d] -
                    sln->prob->items[j].size[d] - sln->prob->items[k].size[d];
            }
            sln->objective += sln->prob->items[j].p+sln->prob->items[k].p-sln->prob->items[i].p;
            sln->x[i]=0; sln->x[j]=1; sln->x[k]=1;
        }
        
    }
    else ret=false;
    return ret;
}

//nb_indx <=3
struct solution_struct* best_descent_vns(int nb_indx, struct solution_struct* curt_sln)
{
    struct solution_struct* best_neighb = malloc(sizeof(struct solution_struct));
    best_neighb->cap_left = malloc(sizeof(int)*curt_sln->prob->dim);
    best_neighb->x = malloc(sizeof(int)*curt_sln->prob->n);
    copy_solution(best_neighb, curt_sln);
    int n=curt_sln->prob->n;
    int curt_move[] ={-1,-1,-1}, best_move []={-1,-1,-1}, delta=0, best_delta=0;  //storing best neighbourhood moves
    switch (nb_indx)
    {
        case 1: //check whether any items can be inserted.
            for(int i=0; i<n; i++){
                if(curt_sln->x[i]>0) continue;
                curt_move[0]=i;
                if(can_move(nb_indx, &curt_move[0], best_neighb)){
                    delta = curt_sln->prob->items[i].p;
                    if(delta> best_delta) {
                        best_delta = delta; best_move[0] = i;
                    }
                }
            }
            if(best_delta>0) {    apply_move(nb_indx, &best_move[0], best_neighb);}
            break;
        case 2:
            for(int i=0; i<n; i++){
                if(curt_sln->x[i]<=0) continue;
                for(int j=0; j<n; j++){
                    if(curt_sln->x[j]==0)
                    {
                        curt_move[0]= i; curt_move[1]= j; curt_move[2]=-1;
                        if(can_move(nb_indx, &curt_move[0], best_neighb)){
                            delta = curt_sln->prob->items[j].p -curt_sln->prob->items[i].p;
                            if(delta > best_delta){
                                best_delta = delta; best_move[0] = i; best_move[1] = j; best_move[2]=-1;
                            }
                        }
                    }
                }
            }
            if(best_delta>0) { apply_move(nb_indx, &best_move[0], best_neighb);}
            break;
        case 3:
            //2-1 swap
            for(int i=0; i<n; i++){
                if(curt_sln->x[i]==0) continue;
                for(int j=0; j!=i&&j<n; j++){
                    if(curt_sln->x[j]==0) continue;
                    for(int k=0;k<n;k++){
                        if(curt_sln->x[k] == 0)
                        {
                            curt_move[0]=i; curt_move[1]=j; curt_move[2]=k;
                            if(can_move(nb_indx, &curt_move[0], best_neighb)){
                                delta = curt_sln->prob->items[k].p -curt_sln->prob->items[i].p-curt_sln->prob->items[j].p;
                                if(delta > best_delta){
                                    best_delta = delta; best_move[0] = i; best_move[1] = j; best_move[2]=k;
                                }
                            }
                        }
                    }
                }
            }
            //1-2 swap
            for(int i=0; i<n; i++){
                if(curt_sln->x[i]==0) continue;
                for(int j=0; j<n; j++){
                    if(curt_sln->x[j]>0) continue;
                    for(int k=0;k!=j&&k<n;k++){
                        if(curt_sln->x[k] == 0)
                        {
                            curt_move[0]=i; curt_move[1]=j; curt_move[2]=k;
                            if(can_move(nb_indx, &curt_move[0], curt_sln)){
                                delta = curt_sln->prob->items[k].p +curt_sln->prob->items[j].p-curt_sln->prob->items[i].p;
                                if(delta > best_delta){
                                    best_delta = delta; best_move[0] = i; best_move[1] = j; best_move[2]=k;
                                }
                            }
                        }
                    }
                }
            }
            if(best_delta>0) { apply_move(nb_indx, &best_move[0], best_neighb);}
            break;
        default:
            printf("Neighbourhood index is out of the bounds, nothing is done!\n");
    }
    return best_neighb;
}

void vns_shaking(struct solution_struct* sln, int strength)
{//using random pair-wise swap

  
    int n= sln->prob->n;
    int m =0, try=0;
    while(m<strength && try<200)
    {
        int move[2];
        int i = rand_int(0, n-1);
        if(sln->x[i]>0){
            move[0] = i;
            do{
                move[1] =rand_int(0, n-1);
            }while(sln->x[move[1]]>0);
        }
        else{
            move[1]=i;
            do{
                move[0] = rand_int(0, n-1);
            }while(sln->x[move[0]]<=0);
        }
        if(can_swap(sln, move[0], move[1]))
        {
            apply_move(2, &move[0], sln);
            m++;
        }
        try++;
    }
}
//VNS
int varaible_neighbourhood_search(struct problem_struct* prob){
    clock_t time_start, time_fin;
    time_start = clock();
    double time_spent=0;
    int nb_indx =0; //neighbourhood index
    
    best_sln.prob = prob;
    struct solution_struct* curt_sln = greedy_heuristic(prob);
    update_best_solution(curt_sln);
    
    int shaking_count =0;
    while(time_spent < MAX_TIME) //note that final computational time can be way beyond the MAX_TIME if best_descent is time consuming
    {
        while(nb_indx<K){
            struct solution_struct* neighb_s=best_descent_vns(nb_indx+1, curt_sln); //best solution in neighbourhood nb_indx
            if(neighb_s->objective > curt_sln->objective){
                copy_solution(curt_sln, neighb_s);
                nb_indx=1;
            }
            else nb_indx++;
            free_solution(neighb_s);free(neighb_s);
        }
        update_best_solution(curt_sln);
        double gap=1000; //set to an arbitrarily large number if best known solution is not availabe.
        if(best_sln.prob->best_obj!=0) gap=  100*(best_sln.prob->best_obj - best_sln.objective)/best_sln.prob->best_obj;
        printf("shaking_count=%d, curt obj =%0.0f,\t best obj=%0.0f,\t gap= %0.2f%%\n",shaking_count, curt_sln->objective, best_sln.objective, gap);
        vns_shaking(curt_sln, SHAKE_STRENGTH); //shaking at a given strength. This can be made adaptive
        //vns_shaking(curt_sln, shaking_count/100+1); //re-active shaking
        shaking_count++;
        nb_indx=0;
        
        time_fin=clock();
        time_spent = (double)(time_fin-time_start)/CLOCKS_PER_SEC;
    }

    //output_solution(&best_sln, "vns_results.txt");
    free_solution(curt_sln); free(curt_sln);
    return 0;
}

int main(int argc, const char * argv[]) {
    
    printf("Starting the run...\n");
    char data_file[50]={"somefile"}, out_file[50], solution_file[50];  //max 50 problem instances per run
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
        //printf("data_file= %s, output_file= %s, sln_file=%s, max_time=%d", data_file, out_file, solution_file, MAX_TIME);
    }
    struct problem_struct** my_problems = load_problems(data_file);
    
    if(strlen(solution_file)<=0)
    {
        if(strcmp(out_file,"")==0) strcpy(out_file, "my_solutions.txt"); //default output
        FILE* pfile = fopen(out_file, "w"); //open a new file
        fprintf(pfile, "%d\n", num_of_problems); fclose(pfile);
        for(int k=0; k<num_of_problems; k++)
        {
            best_sln.objective=0; best_sln.feasibility=0;
            for(int run=0; run<NUM_OF_RUNS; run++)
            {
                srand(RAND_SEED[run]);
                switch (alg)
                {
                case SA:
                    printf("Running Simulated Annealing...\n");
                    SimulatedAnnealing(my_problems[k]); // call SA method
                    break;
                case TS:
                    printf("Running Tabu Search...\n");
                    TabuSearch(my_problems[k]);
                    break;
                case VNS:
                    printf("Running VNS...\n");
                        varaible_neighbourhood_search(my_problems[k]);
                    break;
                default:
                    printf("No algorithm selected. Please select an algorithm!\n");
                    return 1;
                }
            }
            output_solution(&best_sln,out_file);
        }
    }
    for(int k=0; k<num_of_problems; k++)
    {
       free_problem(my_problems[k]); //free problem data memory
    }
    free(my_problems); //free problems array
    if(best_sln.x!=NULL && best_sln.cap_left!=NULL){ free(best_sln.cap_left); free(best_sln.x);} //free global
    return 0;
}
