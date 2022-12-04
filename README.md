

## Contents

In this report, the following topics will be covered:

[toc]

​																																									Words count: 1989

## Algorithm explanation

In this section, I will first introduce how I encoded the solution. And then, I would explain how I chose the fitness function. Finally, I will illustrate how I come up with these neighborhoods and how they perform. 

#### Solution encoding

Four structures are used in total, representing an **item**, **bin**, **solution**, and **problem**, respectively. 

```c++
// struct of the item
struct item_struct{
    int size;   // size of the item
    int index;  // index of the item
};

// struct of the bin
struct bin_struct {
    vector<item_struct> packed_items;   // vector of the items it contains
    int cap_left;                       // the capacity left
};

// struct of the problem
struct problem_struct{
    char name[10];              // name of the problem
    int n;                      // number of items
    int capacities;             // capacitie of each bin
    int known_best;             // the best objective of this problem
    struct item_struct* items;  // a pointer to an array of items
};

// struct of the solution
struct solution_struct {
    struct problem_struct* prob;    // a pointer to the problem that it is going to solve
    int objective;                  // current solution's objective
    vector<bin_struct> bins;
};
```



Besides, what's worth mentioning is that an array is used to represent the swap that will be carried out. 

It could be simply considered as **[bin1_index, item1_index, bin2_index, item2_index, bin3_index, item3_index...]**.

```c++
int curt_move[] = {-1,-1, -1,-1, -1,-1, -1,-1};
int best_move[] = {-1,-1, -1,-1, -1,-1, -1,-1};
```





#### Fitness function

When designing all these algorithms, my overall purpose is to **minimize the capacity left in the lower-indexed bins** and **maximize the capacity left in higher-indexed bins**. So under this circumstance, for most cases, I would use the capacity **change on the lower-indexed bin** to denote the fitness value.

Specific fitness functions would be mentioned in each neighborhood's introduction.



#### Initial solution gathering

For the sake of reducing time consumption, my initial idea is to use first-fit algorithms to get the initial solution. It works, but the result does not seem to be good. For easy instances, the gap is around 2 bin2. So I wondered how best-fit works and how long it may take. I experimented on it, and the results show that it takes approximately 0.01 seconds to get the initial solution for both first-fit and the best-fit algorithms. And the best-fit algorithm's performance is a little bit better. The gap for easy instances is almost reduced to 1 bin. Though for medium and hard instances, the gap is still around 3 to 4 bins. 

It is worth mentioning that, before applying both first-fit and best-fit, I sort the items in descending order first, according to their size. This unintentional act did bring me some hints later on about choosing the neighborhood.

The table below displays the performance of the initial solution. The performance for hard and medium instances is poor, as half of the instances reach the gap of 3 bins.

| Performance (gap with best best-known objective) | 0 bin | 1 bin | 2 bin | 3 bin | 4 bin |
| ------------------------------------------------ | ----- | ----- | ----- | ----- | ----- |
| **Easy** (20 instances in total)                 | 6     | 14    | /     | /     | /     |
| **Medium** (20 instances in total)               | /     | 1     | 4     | 15    | /     |
| **Hard** (10 instances in total)                 | /     | /     | /     | 6     | 4     |



#### Neighborhood choosing (intensification part)

In this subsection, I would introduce the neighborhood in the order of time that I come up with them, which is not the actual order in code. The final order is redesigned, and the order with the best performance is selected, which is noted in the bracket after each neighborhood. Moreover, for all the neighborhood I applied best descent local search algorithm to achieve a better performance.

##### 1-1-1 swap (neighborhood 3)

After I got the initial solution, my main focus was optimizing medium and hard instances. I've checked what the initial solution looks like, and I found something interesting. As the items are sorted before applying the best-fit algorithm, the dataset for the instances presents a similar pattern. For the small indexed bin, it is always a huge item coupled with several tiny items. For medium and small indexed bins, it is most likely to have several medium-sized or small-sized items. I came up with my first algorithm, 1-1-1 swap, according to this pattern.

The primary purpose of 1-1-1 swap is to **minimize the capacity left in bin1 and bin2** and **maximize the capacity left in bin3**. To achieve this, it picks three bins, bin1, bin2, and bin3. Then, from each bin, pick an item out, and name them as item1, item2, item3. Assuming item1 is from bin1, item2 from bin2, and so on. Our goal is to **put item2 and item3 into bin1** and **put item1 into bin2**. After this kind of swap, bin3 lost an item, and the capacity left for both bin1 and bin2 is reduced. To better explain it, let me express the requirements for swap in a formal way:

```
item2.size + item3.size <= item1.size + bin1.cap_left (make sure item2 and item3 could be packed into bin1)
```

```
item1.size <= item2.size + bin2.cap_left              (make sure item1 could be packed into bin2)
```

The fitness function is as below:

```
delta = item2.size + item3.size - item1.size
```



##### 1-2 swap (neighborhood 3)

When applying 1-1-1 swap, I figured out this algorithm could be further improved. There is an algorithm that is relatively similar to 1-1-1 swap, that is 1-2 swap. Surprisingly, 1-2 swap could be added to 1-1-1 swap with just a little bit of modification on the code. **So I combined these two algorithms into one neighborhood**. To implement 1-2 swap, we would firstly pick two bins, bin1 and bin2. From bin1, pick item1; from bin2, pick two items, item2 and item3. Then swap item1 with item2 and item3. The requirements could be expressed as follows:

```
item2.size + item3.size <= item1.size + bin1.cap_left (make sure item2 and item3 could be packed into bin1)
```

```
item1.size <= item2.size + item3.size + bin2.cap_left (make sure item1 could be packed into bin2)
```

The fitness value of this algorithm could be expressed similarly with 1-1-1 swap:

```
delta = item2.size + item3.size - item1.size
```



After applying these two algorithms, the results are displayed below. The combination of these two algorithms has a significant effect on medium-level and hard-level instances.
(Number in the bracket denotes the performance before this operation.)

| Performance (gap with best-known objective) | 0 bin | 1 bin  | 2 bin     | 3 bin  | 4 bin |
| ------------------------------------------- | ----- | ------ | --------- | ------ | ----- |
| **Easy** (20 instances in total)            | 6     | 14     | /         | /      | /     |
| **Medium** (20 instances in total)          | /     | **17** | **3** (4) | / (15) | /     |
| **Hard** (10 instances in total)            | /     | **9**  | **2**     | / (6)  | / (4) |



##### Shaking (diversification part)

After reaching a not bad result through the first neighbor, I first implemented shaking instead of finding other useful neighborhoods, e.g., the diversification part. Because though there do exist some other neighborhoods that could help improve the performance, they are too specific. In other words, they could only help with the particular cases, and I think I'm not yet going to dig that deep into one algorithm. I want to try something more general that could help with more cases.

The algorithm for shaking is quite straightforward. Randomly pick two bins and from each bin, pick an item. If two items could swap, then swap, else pick another pair of items. Shaking would happen if all the neighborhoods could not generate a better solution. And **applying shaking would help the algorithm to get out of the local optimum**. 

Below is the table for shaking's performance. It has a significant improvement in easy, medium, and hard instances. After shaking, the gap for most of the instances dropped to 1 bin.

| Performance (gap with best-known objective) | 0 bin       | 1 bin      | 2 bin     | 3 bin  | 4 bin |
| ------------------------------------------- | ----------- | ---------- | --------- | ------ | ----- |
| **Easy** (20 instances in total)            | **14**  (6) | **6** (14) | /         | /      | /     |
| **Medium** (20 instances in total)          | /           | **17**     | **3** (4) | / (15) | /     |
| **Hard** (10 instances in total)            | **4** (0)   | **5** (9)  | **1** (2) | /      | /     |



##### One item transfer (neighborhood 1)

I hadn't expected that shaking would bring me such a massive improvement in the performance. After implementing shaking, I observed the results. I figured out that for higher indexed bins, there may occur the case that several bins only have one item, and these items could be packed into one bin. Therefore, I implemented one item transfer. The algorithm is relatively easy. It picks two bins, takes an item from the second bin, and puts it into the first bin.

In this case, the fitness value could be denoted as item2's size:

```
delta = item2.size
```



##### 1-1 swap (neighborhood 2) 

Another algorithm that I thought might be helpful is 1-1 swap. Just like its name refers, it directly swaps two items.

1-1 swap uses the same algorithm with shaking, but the bins and items are not randomly picked. 1-1 swap would only happen when the two items could swap, and item2's size is bigger than item1's size.

In this case, the fitness value could be denoted as the capacity change on bin1:

```
delta = item2.size - item1.size
```



##### 2-2 swap (neighborhood 4) & 1-3 swap & 2-3 swap

After designing and implementing the first three algorithms, I found that there is no obvious way to improve the current situation. So I tried on some variants of 1-1 swap: 2-2 swap, 1-3 swap, and 2-3 swap. Unfortunately, they did not have a significant improvement on the current solution. 

Implementing any of these algorithms could only make a difference on easy instances, e.g., making the easy instances all 0. I think the reason behind this is that they are all variants of 1-1 swap. They could be replaced by 1-1 swap. 

Finally, I chose 2-2 swap as the fourth neighborhood. Though in the code you may find there exist neighborhoods 5 and 6, I did not choose to make use of them, i.e., these parts of codes would be escaped.

Below is the overall performance after adding one item transfer algorithm (neighborhood 1), 1-1 swap algorithm (neighborhood 2), and 2-2 swap (neighborhood 4). It could be seen that the combination of these three algorithms is quite efficient and performs well.

| Performance (gap with best-known objective) | 0 bin       | 1 bin      | 2 bin | 3 bin | 4 bin |
| ------------------------------------------- | ----------- | ---------- | ----- | ----- | ----- |
| **Easy** (20 instances in total)            | **20** (14) | **0** (6)  | /     | /     | /     |
| **Medium** (20 instances in total)          | **16** (0)  | **4** (17) | /     | /     | /     |
| **Hard** (10 instances in total)            | **10** (4)  | **0** (5)  | /     | /     | /     |





## Performance evaluation

Below are the results of five runs with random seeds 3, 10, 20, 30, and 100. In a total of five runs, the best result means the minimum gap of the specific instance, and the worst result means the maximum gap of the specific instance. For example, the gap for instance u500_03 are 1, 0, 0, 0, 1 in five runs. Here, the best result is 0, the worst is 1, and the average is 0.4 ((1+1) / 5).

##### Best result

| DataSet | Objective | Best know | Gap  | DataSet | Objective | Best know | Gap  |
| ------- | --------- | --------- | ---- | ------- | --------- | --------- | ---- |
| u120_00 | 48        | 48        | 0    | u500_05 | 206       | 206       | 0    |
| u120_01 | 49        | 49        | 0    | u500_06 | 208       | 207       | 1    |
| u120_02 | 46        | 46        | 0    | u500_07 | 205       | 204       | 1    |
| u120_03 | 49        | 49        | 0    | u500_08 | 196       | 196       | 0    |
| u120_04 | 50        | 50        | 0    | u500_09 | 202       | 202       | 0    |
| u120_05 | 48        | 48        | 0    | u500_10 | 200       | 200       | 0    |
| u120_06 | 48        | 48        | 0    | u500_11 | 200       | 200       | 0    |
| u120_07 | 49        | 49        | 0    | u500_12 | 199       | 199       | 0    |
| u120_08 | 50        | 50        | 0    | u500_13 | 196       | 196       | 0    |
| u120_09 | 46        | 46        | 0    | u500_14 | 204       | 204       | 0    |
| u120_10 | 52        | 52        | 0    | u500_15 | 201       | 201       | 0    |
| u120_11 | 49        | 49        | 0    | u500_16 | 202       | 202       | 0    |
| u120_12 | 48        | 48        | 0    | u500_17 | 198       | 198       | 0    |
| u120_13 | 49        | 49        | 0    | u500_18 | 202       | 202       | 0    |
| u120_14 | 50        | 50        | 0    | u500_19 | 196       | 196       | 0    |
| u120_15 | 48        | 48        | 0    | HARD0   | 56        | 56        | 0    |
| u120_16 | 52        | 52        | 0    | HARD1   | 57        | 57        | 0    |
| u120_17 | 52        | 52        | 0    | HARD2   | 56        | 56        | 0    |
| u120_18 | 49        | 49        | 0    | HARD3   | 55        | 55        | 0    |
| u120_19 | 49        | 49        | 0    | HARD4   | 57        | 57        | 0    |
| u500_00 | 198       | 198       | 0    | HARD5   | 56        | 56        | 0    |
| u500_01 | 201       | 201       | 0    | HARD6   | 57        | 57        | 0    |
| u500_02 | 202       | 202       | 0    | HARD7   | 55        | 55        | 0    |
| u500_03 | 204       | 204       | 0    | HARD8   | 57        | 57        | 0    |
| u500_04 | 206       | 206       | 0    | HARD9   | 56        | 56        | 0    |

##### Worst result

| DataSet | Objective | Best know | Gap  | DataSet | Objective | Best know | Gap  |
| ------- | --------- | --------- | ---- | ------- | --------- | --------- | ---- |
| u120_00 | 48        | 48        | 0    | u500_05 | 206       | 206       | 0    |
| u120_01 | 49        | 49        | 0    | u500_06 | 208       | 207       | 1    |
| u120_02 | 46        | 46        | 0    | u500_07 | 205       | 204       | 1    |
| u120_03 | 49        | 49        | 0    | u500_08 | 196       | 196       | 0    |
| u120_04 | 50        | 50        | 0    | u500_09 | 202       | 202       | 0    |
| u120_05 | 48        | 48        | 0    | u500_10 | 200       | 200       | 0    |
| u120_06 | 48        | 48        | 0    | u500_11 | 200       | 200       | 0    |
| u120_07 | 49        | 49        | 0    | u500_12 | 200       | 199       | 1    |
| u120_08 | 50        | 50        | 0    | u500_13 | 196       | 196       | 0    |
| u120_09 | 46        | 46        | 0    | u500_14 | 204       | 204       | 0    |
| u120_10 | 52        | 52        | 0    | u500_15 | 201       | 201       | 0    |
| u120_11 | 49        | 49        | 0    | u500_16 | 202       | 202       | 0    |
| u120_12 | 48        | 48        | 0    | u500_17 | 198       | 198       | 0    |
| u120_13 | 49        | 49        | 0    | u500_18 | 202       | 202       | 0    |
| u120_14 | 50        | 50        | 0    | u500_19 | 197       | 196       | 1    |
| u120_15 | 48        | 48        | 0    | HARD0   | 56        | 56        | 0    |
| u120_16 | 52        | 52        | 0    | HARD1   | 57        | 57        | 0    |
| u120_17 | 52        | 52        | 0    | HARD2   | 56        | 56        | 0    |
| u120_18 | 49        | 49        | 0    | HARD3   | 55        | 55        | 0    |
| u120_19 | 49        | 49        | 0    | HARD4   | 57        | 57        | 0    |
| u500_00 | 198       | 198       | 0    | HARD5   | 56        | 56        | 0    |
| u500_01 | 201       | 201       | 0    | HARD6   | 57        | 57        | 0    |
| u500_02 | 202       | 202       | 0    | HARD7   | 55        | 55        | 0    |
| u500_03 | 205       | 204       | 1    | HARD8   | 57        | 57        | 0    |
| u500_04 | 206       | 206       | 0    | HARD9   | 56        | 56        | 0    |

##### Average result

| DataSet | Objective | Best know | Gap  | DataSet | Objective | Best know | Gap  |
| ------- | --------- | --------- | ---- | ------- | --------- | --------- | ---- |
| u120_00 | 48        | 48        | 0    | u500_05 | 206       | 206       | 0    |
| u120_01 | 49        | 49        | 0    | u500_06 | 208       | 207       | 1    |
| u120_02 | 46        | 46        | 0    | u500_07 | 205       | 204       | 1    |
| u120_03 | 49        | 49        | 0    | u500_08 | 196       | 196       | 0    |
| u120_04 | 50        | 50        | 0    | u500_09 | 202       | 202       | 0    |
| u120_05 | 48        | 48        | 0    | u500_10 | 200       | 200       | 0    |
| u120_06 | 48        | 48        | 0    | u500_11 | 200       | 200       | 0    |
| u120_07 | 49        | 49        | 0    | u500_12 | 199.2     | 199       | 0.2  |
| u120_08 | 50        | 50        | 0    | u500_13 | 196       | 196       | 0    |
| u120_09 | 46        | 46        | 0    | u500_14 | 204       | 204       | 0    |
| u120_10 | 52        | 52        | 0    | u500_15 | 201       | 201       | 0    |
| u120_11 | 49        | 49        | 0    | u500_16 | 202       | 202       | 0    |
| u120_12 | 48        | 48        | 0    | u500_17 | 198       | 198       | 0    |
| u120_13 | 49        | 49        | 0    | u500_18 | 202       | 202       | 0    |
| u120_14 | 50        | 50        | 0    | u500_19 | 196.2     | 196       | 0.2  |
| u120_15 | 48        | 48        | 0    | HARD0   | 56        | 56        | 0    |
| u120_16 | 52        | 52        | 0    | HARD1   | 57        | 57        | 0    |
| u120_17 | 52        | 52        | 0    | HARD2   | 56        | 56        | 0    |
| u120_18 | 49        | 49        | 0    | HARD3   | 55        | 55        | 0    |
| u120_19 | 49        | 49        | 0    | HARD4   | 57        | 57        | 0    |
| u500_00 | 198       | 198       | 0    | HARD5   | 56        | 56        | 0    |
| u500_01 | 201       | 201       | 0    | HARD6   | 57        | 57        | 0    |
| u500_02 | 202       | 202       | 0    | HARD7   | 55        | 55        | 0    |
| u500_03 | 204.4     | 204       | 0.4  | HARD8   | 57        | 57        | 0    |
| u500_04 | 206       | 206       | 0    | HARD9   | 56        | 56        | 0    |



## Reflections

From the table above, we can see that two medium instances, u500_06 and u500_07, can not be optimized to 0 bin for my algorithms. There's a chance that u500_03, u500_12, and u500_19 can not be optimized to 0 bin. However, the chance is low. Frankly speaking, I'm pretty satisfied with the performance. 

However, the performance could be further improved with the new distinct neighborhoods. Here, distinctiveness is of vital importance. As I mentioned in 2-2 swap, the variant of 1-1 swap (like 2-3 swap, 1-3 swap) does not perform well. That is because they could be replaced with 1-1 swap and shaking, e.g., they are not irreplaceable. However, the time consumption would be much higher with the extra neighborhood, which means the code may need further refactoring for higher efficiency.

To summarize, it is a magical and meaningful journey for me to come up with, explore, and verify all these algorithms. In the beginning, I had no idea how to develop the algorithm, though I knew the framework of VNS. After observing the dataset, together with the experience from daily life, I made some guesses and assumptions. Then I write codes to implement these assumptions. When the performance/result does not match expectations, I will verify it carefully, step by step. Trying to give it a reason why it fails, to see if something goes wrong with my code or just my assumptions are not reasonable. 

Overall, the journey of raising an assumption, implementing it, and finally verifying it is satisfying. 

Thanks for your reading!
