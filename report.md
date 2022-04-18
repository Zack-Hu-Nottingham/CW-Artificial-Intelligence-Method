##### Initial solution gathering

The algorithm for gathering the initial solution is relatively primitive and intuitive. Firstly, sort the items according to their size, from biggest to smallest. Then apply the first-fit algorithm, initialize a new bin, and put the first item (also the biggest, as items are sorted by their size) that could fit in. If none of the items could fit in, then initialize a new bin. Through these operation, the performance of initial solution is relatively high:

##### About neighborhood



For this problem, I choose to let k start with 3, then decrease. Because for the initial solution we get, it is meaningless to apply 

The reason that k starts from 3 is that it could be proved that for k = 1, or k = 2, no improvement could be made.

​	k = 1

​	k = 2

​	k = 3



##### Performance evaluation

Statistical results (avg, best, worst of 5 runs) of the algorithm for all the problem instances, in comparison with the best published results (i.e. the absolute gap to the best results).

