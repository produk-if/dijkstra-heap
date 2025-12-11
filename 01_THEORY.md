# DIJKSTRA'S ALGORITHM: TEORI & KONSEP
**Fokus: Fibonacci Heap vs Binary Heap dengan Parallelisasi**

---

## 1. DIJKSTRA'S ALGORITHM OVERVIEW

### 1.1 Problem Statement
**Single-Source Shortest Path:**
Diberikan directed graph G=(V,E) dengan non-negative edge weights w(u,v), 
temukan shortest path dari source vertex s ke semua vertex lainnya.

### 1.2 Algorithm Pseudocode

```
DIJKSTRA(Graph G, Vertex s):
    dist[s] ← 0
    for each vertex v ≠ s:
        dist[v] ← ∞
    
    PQ ← MakeQueue(all vertices)
    
    while PQ is not empty:
        u ← ExtractMin(PQ)
        
        for each edge (u,v) with weight w:
            if dist[u] + w < dist[v]:
                dist[v] ← dist[u] + w
                DecreaseKey(PQ, v, dist[v])
    
    return dist[]
```

### 1.3 Correctness (Greedy Choice Property)

**Theorem:**
Ketika vertex u di-extract dari priority queue, dist[u] adalah final shortest distance.

**Proof Sketch:**
1. Assume dist[u] > shortest[u]
2. Maka ada path P lebih pendek: s → ... → x → y → ... → u
3. Ketika x di-extract, dist[x] = shortest[x] (by induction)
4. Ketika edge (x,y) direlax: dist[y] ≤ dist[x] + w(x,y)
5. Ini ≤ shortest[u] (karena P lebih pendek)
6. Maka y seharusnya di-extract sebelum u, bukan u
7. **Contradiction! ∴ dist[u] = shortest[u]** ✓

---

## 2. PRIORITY QUEUE OPERATIONS

### 2.1 Interface

```cpp
class PriorityQueue {
    virtual void Insert(Vertex v, Key key);
    virtual Vertex ExtractMin();
    virtual void DecreaseKey(Vertex v, Key new_key);
};
```

### 2.2 Frequency Analysis (untuk V vertices, E edges)

| Operation     | Frequency | Impact |
|---------------|-----------|--------|
| Insert()      | V times   | Initialization |
| ExtractMin()  | V times   | Visit each vertex |
| DecreaseKey() | E times   | Relax each edge |

**Total Dijkstra Time:**
```
T = V·T_ExtractMin + E·T_DecreaseKey
```

---

## 3. BINARY HEAP IMPLEMENTATION

### 3.1 Structure & Properties

**Definition:**
- Complete binary tree dalam array
- Min-heap property: parent ≤ children
- Height: h = ⌊log₂ n⌋

**Array Representation:**
```
For node at index i:
  - parent(i) = ⌊i/2⌋
  - left(i)   = 2i
  - right(i)  = 2i + 1
```

### 3.2 Operations

#### Insert(v, key) - O(log n)
```
1. Append v to end of array
2. i ← size
3. while i > 1 and key < parent_key:
       Swap(i, parent(i))
       i ← parent(i)
```

**Cost:** O(log n) comparisons + swaps

#### ExtractMin() - O(log n)
```
1. min ← array[1]
2. array[1] ← array[size]
3. Decrease size
4. BubbleDown(1)
5. return min
```

**BubbleDown(i):**
```
while i has children:
    j ← index of min child
    if array[i] > array[j]:
        Swap(i, j)
        i ← j
    else break
```

**Cost:** O(log n) comparisons + swaps

#### DecreaseKey(v, new_key) - O(log n)
```
1. i ← position of v
2. key[v] ← new_key
3. while i > 1 and key[i] < key[parent(i)]:
       Swap(i, parent(i))
       i ← parent(i)
```

**Cost:** O(log n) comparisons + swaps

### 3.3 Dijkstra Complexity dengan Binary Heap

```
T = V·log V + E·log V
  = O((V + E) log V)

For sparse graph (E ≈ V):  O(V log V)
For dense graph (E ≈ V²):  O(V² log V)
```

### 3.4 Characteristics

**Advantages:**
✓ Simple implementation (~100 lines)
✓ Good cache locality (array-based)
✓ Low constant factors
✓ Practical untuk most real-world cases
✓ Easy to parallelize (array access pattern)

**Disadvantages:**
✗ DecreaseKey bukan O(1)
✗ Tidak ada amortized improvement possible
✗ Constant log factors

---

## 4. FIBONACCI HEAP IMPLEMENTATION (FOKUS!)

### 4.1 Structure

**Components:**
```cpp
struct Node {
    Vertex v;           // Vertex identifier
    Key key;            // Priority value
    Node* parent;       // Parent dalam tree
    Node* child;        // Salah satu child
    Node* left, *right; // Circular doubly-linked list of siblings
    int degree;         // Number of children
    bool marked;        // Mark flag untuk cascading cuts
};
```

**Fibonacci Heap:**
```cpp
struct FibonacciHeap {
    Node* min;          // Min element
    int n;              // Total nodes
    vector<Node*> rank; // Consolidation array
};
```

### 4.2 Key Insight: Amortized Analysis

**Potential Function:**
```
Φ(H) = t(H) + 2·m(H)

where:
  t(H) = number of trees dalam root list
  m(H) = number of marked nodes
```

**Amortized Cost = Actual Cost + ΔΦ**

### 4.3 Operations

#### Insert(v, key) - **O(1) Amortized**

```
1. Create new Node n dengan (v, key)
2. if min == null:
       min ← n
   else:
       Add n ke root list sebelum min
       if key < min.key:
           min ← n
3. H.n++
4. return n
```

**Cost Analysis:**
```
Actual cost: O(1)
ΔΦ = (t+1) + 2m - (t + 2m) = 1
Amortized: O(1) + 1 = O(1) ✓
```

#### ExtractMin() - **O(log n) Amortized**

```
1. z ← min
2. if z ≠ null:
       3. for each child c of z:
              Remove c dari child list z
              Add c ke root list
          4. Remove z dari root list
          5. if z == z.right:  // z is the only root
                min ← null
             else:
                min ← z.right
                Consolidate(H)
          6. H.n--
   7. return z
```

**Consolidate(H):** Merge trees dengan same degree
```
1. Initialize rank array (size = max possible degree)
2. for each node w dalam root list:
       3. d ← w.degree
       4. while rank[d] ≠ null:
              x ← rank[d]
              if w.key > x.key: Swap(w, x)
              FibonacciHeapLink(x, w)
              rank[d] ← null
              d++
          5. rank[d] ← w
   6. Rebuild min pointer from root list
```

**FibonacciHeapLink(y, x):** Make y child of x
```
1. Remove y dari root list
2. Make y child of x
3. x.degree++
4. y.marked ← false
```

**Cost Analysis:**
```
Actual cost: O(t + log n) untuk consolidate
ΔΦ ≤ O(log n) - t

Amortized: O(t + log n) + O(log n) - t = O(log n) ✓
```

#### DecreaseKey(v, new_key) - **O(1) Amortized** ⭐

```
1. if new_key > v.key:
       error "new key is larger than current key"
   
   2. v.key ← new_key
   3. y ← v.parent
   
   4. if y ≠ null and v.key < y.key:
          5. Cut(H, v, y)
          6. CascadingCut(H, y)
```

**Cut(H, v, y):** Remove v dari child list y, add ke root list
```
1. Remove v dari child list of y
2. y.degree--
3. Add v ke root list
4. v.parent ← null
5. v.marked ← false
```

**CascadingCut(H, y):** Propagate cuts up tree
```
1. z ← y.parent
2. if z ≠ null:
       3. if y.marked == false:
              y.marked ← true
          else:
              Cut(H, y, z)
              CascadingCut(H, z)
```

**Cost Analysis:**
```
Actual cost: O(c) dimana c = number of cascading cuts
ΔΦ = O(c) untuk marked flags
    - c untuk trees removed

Amortized: O(c) + (marks - cuts) ≤ O(1) ✓
```

### 4.4 Dijkstra Complexity dengan Fibonacci Heap

```
T = V·log V + E·O(1)
  = V·log V + E
  = O(E + V·log V)

For sparse graph (E ≈ V):  O(V log V)
For dense graph (E ≈ V²):  O(V² log V) vs O(V² + V log V) ✓
```

**Improvement vs Binary Heap:**
```
Binary Heap: O((V+E) log V)
Fibonacci:   O(E + V log V)

When E >> V log V, Fibonacci wins!
Example: E = V^1.5  →  Binary: O(V^2.5 log V), Fib: O(V^1.5)
```

### 4.5 Characteristics

**Advantages:**
✓ O(1) DecreaseKey (asymptotically optimal!)
✓ Better theoretical complexity
✓ E = dominates untuk dense graphs
✓ Interesting algorithm properties

**Disadvantages:**
✗ Very complex implementation (~400 lines)
✗ Large constant factors
✗ Pointer-heavy (poor cache locality)
✗ Many allocations/deallocations
✗ Rarely faster in practice than Binary Heap
✗ Harder to parallelize (pointer chasing)

---

## 5. COMPLEXITY COMPARISON

### 5.1 Single Operations

| Operation      | Binary Heap | Fibonacci Heap |
|----------------|-------------|----------------|
| Insert         | O(log n)    | O(1) amortized |
| ExtractMin     | O(log n)    | O(log n) amortized |
| DecreaseKey    | O(log n)    | **O(1) amortized** |
| Delete         | O(log n)    | O(log n) amortized |

### 5.2 Dijkstra Complete Algorithm

```
Binary Heap:     O((V+E) log V)
Fibonacci Heap:  O(E + V log V)

Sparse (E = O(V)):
  Binary:        O(V log V)
  Fibonacci:     O(V log V)
  Winner:        Same ≈

Medium (E = O(V^1.5)):
  Binary:        O(V^1.5 log V)
  Fibonacci:     O(V^1.5)
  Winner:        Fibonacci ✓

Dense (E = O(V^2)):
  Binary:        O(V^2 log V)
  Fibonacci:     O(V^2)
  Winner:        Fibonacci ✓✓
```

### 5.3 Practical Performance

Despite theoretical advantage, Fibonacci often slower!

**Why?**
- Large constant factors (8x-10x overhead)
- Cache misses (pointer-based structure)
- Memory fragmentation
- Marked array overhead
- Cascading cuts overhead

**Rule of thumb:**
- V ≤ 10^5: Binary Heap faster
- V ≈ 10^6: Depends on E
- E >> V log V: Fibonacci might win (rarely)

---

## 6. PARALLELIZATION CONCEPTS

### 6.1 Dijkstra Parallelization Challenges

**Sequential Dependency:**
```
dist[v] bisa berubah kapan saja
Aman untuk parallelize:
  - Hanya relax edge (u,v) ketika u sudah di-extract
  
Tidak aman (data race):
  - Multiple threads updating dist[v] simultaneously
  - Multiple threads accessing priority queue
```

### 6.2 OpenMP Strategy

**Approach 1: Parallel Edge Relaxation**
```cpp
for each extracted vertex u:
    #pragma omp parallel for
    for each edge (u,v):
        // Relaxation dengan atomic operations
        #pragma omp atomic
        if (dist[u] + w < dist[v]):
            dist[v] = dist[u] + w
```

**Cost:** Good untuk E >> V, bad overhead untuk small graphs

**Approach 2: Parallel Priority Queue**
```cpp
// Use thread-safe priority queue
// Cilk or OpenMP task queues
```

### 6.3 Cilk Strategy (Task-Based)

**Cilk Spawning:**
```cpp
cilk_for each edge (u,v):
    Relax edge atomically
```

**Advantage:**
- Better work stealing
- Dynamic load balancing
- Nested parallelism support

---

## 7. THEORETICAL EXPECTATIONS

### 7.1 Serial Performance

```
Binary Heap:
  - Time: O((V+E) log V)
  - Space: O(V)
  - Fast dalam practice
  - Cache-friendly

Fibonacci Heap:
  - Time: O(E + V log V) better theo
  - Space: O(V)
  - Slower dalam practice (constants)
  - Cache-unfriendly
```

### 7.2 Parallel Performance

```
OpenMP Binary Heap:
  - Can parallelize edge relaxation
  - Speedup ≈ 2-4x pada 4 cores (untuk E >> V)
  - Atomic overhead limits scaling
  
OpenMP Fibonacci Heap:
  - Harder to parallelize (pointer structures)
  - Potential for task-level parallelism
  - Marked operations konflikt dengan threading
  
Cilk Binary Heap:
  - Natural work stealing
  - Better scaling potential
  - Nested parallelism possible
  
Cilk Fibonacci Heap:
  - Difficult coordination of cascading cuts
  - Potential deadlocks dari marked updates
  - Not recommended for heavy parallelism
```

---

## SUMMARY

| Aspect              | Binary Heap | Fibonacci Heap |
|---------------------|-------------|----------------|
| Theory Complexity   | O((V+E)log V) | **O(E+V log V)** |
| Practical Speed     | **FAST**    | SLOW |
| Implementation      | **SIMPLE**  | COMPLEX |
| Cache Locality      | **GOOD**    | BAD |
| Parallelization     | **EASIER**  | HARDER |
| Use Cases           | **GENERAL** | Dense graphs (rare) |

**Project Approach:**
- Implement BOTH untuk comparison
- Parallel dengan OpenMP (simpler)
- Parallel dengan Cilk (advanced)
- Measure performance empirically
- Analyze trade-offs

