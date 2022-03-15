#include <stdio.h>
#include <stdlib.h>

int QUESTION = 3;


struct item{
    int indx; //remember the index of items after sorting
    int p;
    int v;
    double ratio;
};

struct problem{
    int C; //knapsack capacity
    int num_of_items;
    struct item* items;
};

struct solution{
    int objective;
    int cap_left;
    struct problem* prob;
    int* x;
};


int main() {
    FILE * ptr;

    ptr = fopen("mknapcb1.txt", "r");
    char* buffer = NULL;

    if (ptr) {
        printf("successful open\n");
        fseek (ptr, 0, SEEK_END);
        int length = ftell(ptr);
        printf("file length:%d\n", length);
        buffer = malloc(sizeof(char) * length);

        if (buffer) {
            fread(buffer, 1, length, ptr);
        }
        fclose(ptr);
    }

    if (buffer) {
        printf("successful malloc\n");
    }

    printf("%c", buffer);
}