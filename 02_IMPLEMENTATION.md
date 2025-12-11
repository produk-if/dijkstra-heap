# DIJKSTRA'S ALGORITHM: IMPLEMENTASI GUIDE

---

## 1. OVERVIEW

Proyek ini mengimplementasikan Dijkstra's Algorithm dengan 2 priority queue backends:
1. **Binary Heap** - praktis, cepat
2. **Fibonacci Heap** - teoritis optimal, kompleks

Plus parallelization dengan **OpenMP** dan **Cilk** paradigm.

---

## 2. PROJECT STRUCTURE

```
dijkstra_project/
├── include/
│   ├── graph.h              # Graph data structure
│   ├── binary_heap.h        # Binary Heap implementation
│   ├── fibonacci_heap.h     # Fibonacci Heap implementation
│   └── dijkstra.h           # Dijkstra algorithms (4 variants)
│
├── src/
│   └── main.cpp             # Testing & benchmarking
│
├── docs/
│   ├── 01_THEORY.md         # Theory documentation
│   └── 02_IMPLEMENTATION.md # This file
│
├── analysis/
│   └── results.csv          # Performance results (generated)
│
└── CMakeLists.txt           # Build configuration
```

---

## 3. GRAPH DATA STRUCTURE (graph.h)

### Design Decisions

**Adjacency List Representation:**
```cpp
vector<vector<Edge>> adj_list;
```

**Advantages:**
- Efficient untuk sparse graphs
- O(degree) iteration untuk neighbors
- Memory-efficient untuk most real graphs

**Edge Structure:**
```cpp
struct Edge {
    Vertex to;      // Target vertex
    Weight weight;  // Edge weight
};
```

### Usage

```cpp
Graph g(5, true);  // 5 vertices, directed

g.addEdge(0, 1, 2.5);  // Add edge 0->1 dengan weight 2.5
g.addEdge(1, 2, 3.0);

const auto& neighbors = g.getNeighbors(0);
for (const Edge& e : neighbors) {
    cout << "To: " << e.to << ", Weight: " << e.weight << endl;
}
```

---

## 4. BINARY HEAP (binary_heap.h)

### Implementation Details

**Internal Structure:**
```cpp
struct Node {
    Vertex v;           // Vertex ID
    Weight key;         // Priority (distance)
    int heap_index;     // Current position dalam array
};

vector<Node> heap;              // Min-heap array
vector<int> vertex_to_index;    // Vertex ID -> heap index mapping
```

**Why vertex_to_index?**
- Memungkinkan O(1) lookup posisi vertex
- Essential untuk O(log n) decrease_key

### Key Operations

#### Insert(v, key) - O(log n)
```cpp
void insert(Vertex v, Weight key) {
    // 1. Append ke end of array
    int idx = heap_size++;
    heap[idx] = Node(v, key, idx);
    
    // 2. Bubble up until min-heap property restored
    bubble_up(idx);
}
```

**Bubble Up:**
```
Before: [5, 3, 7, 2, 1]  inserting 0
                └─┬─┘
                  idx=4
                  
After bubbling up: [0, 3, 1, 2, 5, 7]
                    └──────────┘
                    (1 bubbles up, 0 becomes min)
```

#### Extract Min() - O(log n)
```cpp
Vertex extract_min() {
    // 1. Save minimum vertex
    Vertex min_v = heap[0].v;
    
    // 2. Move last element ke root
    heap[0] = heap[--heap_size];
    
    // 3. Bubble down to restore min-heap property
    bubble_down(0);
    
    return min_v;
}
```

#### Decrease Key(v, new_key) - O(log n)
```cpp
void decrease_key(Vertex v, Weight new_key) {
    int idx = vertex_to_index[v];
    
    // 1. Update key
    heap[idx].key = new_key;
    
    // 2. Bubble up jika lebih kecil dari parent
    bubble_up(idx);
}
```

### Characteristics

**Strengths:**
✓ Simple implementation
✓ Array-based = good cache locality
✓ Low constant factors
✓ Easy to understand/debug

**Weaknesses:**
✗ Both extract_min dan decrease_key O(log n)
✗ Tidak ada O(1) improvement possible
✗ Tidak ideal untuk dense graphs

---

## 5. FIBONACCI HEAP (fibonacci_heap.h)

### Implementation Details

**Node Structure (Advanced):**
```cpp
struct Node {
    Vertex v;              // Vertex ID
    Weight key;            // Priority value
    
    Node* parent;          // Parent dalam tree
    Node* child;           // Salah satu child (circular list)
    Node* left, *right;    // Siblings (circular doubly-linked)
    
    int degree;            // Number of children
    bool marked;           // Mark flag untuk cascading cuts
};
```

**Key Innovation:**
```
Min Pointer
    │
    ├─→ [4] → [7] → [10] → [4]
        │      │      │
        │      └─ [9] └─ [12]
        │
        └─ [6] → [8] → [6]
```

Circular lists for:
- Root list (all tree roots)
- Child list (children of each node)

### Key Operations

#### Insert(v, key) - **O(1) Amortized**

```cpp
void insert(Vertex v, Weight key) {
    Node* node = new Node(v, key);
    
    if (min_node == nullptr) {
        min_node = node;
    } else {
        // Add ke root list sebelum min
        node->left = min_node;
        node->right = min_node->right;
        min_node->right->left = node;
        min_node->right = node;
        
        // Update min jika perlu
        if (key < min_node->key) {
            min_node = node;
        }
    }
    
    n_nodes++;
}
```

**Cost Analysis:**
```
Actual cost: O(1) - just add ke circular list
ΔΦ = (t+1) + 2m - (t + 2m) = 1
Amortized: O(1) + 1 = O(1) ✓
```

#### Extract Min() - **O(log n) Amortized**

```
Before extract_min:
    Root list: [min(4)] → [7] → [10]
    
During:
    1. Promote all children of min ke root list
    2. Remove min dari root list
    3. Consolidate: merge trees dengan same degree
    
After consolidate:
    Root list: [6] → [7] → [10]
    (trees dengan degree 1 merged, etc)
```

**Consolidate Algorithm:**
```cpp
void consolidate() {
    // Array A[k] = root of tree dengan degree k
    vector<Node*> A(max_degree, nullptr);
    
    // For each root w:
    //   While A[degree[w]] exists:
    //     Link w dan A[degree[w]]
    //     Increment degree
    //   A[degree[w]] = w
}
```

**Why O(log n)?**
- Maximum degree = O(log n) (Fibonacci property)
- Consolidate scans O(log n) degrees
- Each link O(1)
- Total: O(log n)

#### Decrease Key(v, new_key) - **O(1) Amortized** ⭐

```cpp
void decrease_key(Vertex v, Weight new_key) {
    Node* node = vertices[v];
    node->key = new_key;
    
    Node* parent = node->parent;
    if (parent != nullptr && node->key < parent->key) {
        // Cut node dari parent
        cut(node, parent);
        
        // Cascade: if parent marked, cut it too
        cascading_cut(parent);
    }
    
    // Update min
    if (node->key < min_node->key) {
        min_node = node;
    }
}
```

**Cut Operation:**
```
Before cut:
    Parent → ... → [v] → ...
    
After cut:
    Parent → ... → ...
    Root list → [v] → ...
```

**Cascading Cuts:**
```
Tree with marked nodes:
    
        [x]
       /   \
      [y]* [z]
      /
    [w]
    
When w's key decreased:
    - Cut w, add to root
    - y marked, so cut y too
    - x marked, so cut x too
    
Result: All promoted to roots
```

**Cost Analysis:**
```
Actual: O(c) dimana c = cascading cuts
ΔΦ: +c untuk marked flags, -c untuk trees
Amortized: O(c) + ΔΦ = O(1) ✓
```

### Why Fibonacci Slower in Practice?

**Constant Factors:**
```
Binary Heap: simple array access
  - Single indirection: heap[i]
  - Cache-friendly

Fibonacci: pointer-heavy
  - Multiple indirections: node→parent→child
  - Cache misses
  - Memory fragmentation
```

**Overhead:**
```
Binary:     O(1) per node metadata
Fibonacci:  O(1) per node but much larger:
  - parent pointer
  - child pointer
  - left, right pointers
  - degree field
  - marked flag
  - Total: 8 pointers + 2 ints = ~80 bytes per node!

Binary node: ~24 bytes (vertex + key + index)
```

**Marked Array:**
```
Marking nodes adds overhead
Cascading cuts not "cheap" despite O(1) amortized
Real graphs: E not >> V log V enough
```

---

## 6. DIJKSTRA IMPLEMENTATIONS (dijkstra.h)

### 6.1 Sequential - Binary Heap

```cpp
class DijkstraBinaryHeap {
    static DijkstraResult run(const Graph& graph, Vertex source) {
        // Initialize
        vector<Weight> dist(V, INF);
        dist[source] = 0;
        
        BinaryHeap pq(V);
        pq.insert(source, 0);
        
        // Main loop
        while (!pq.is_empty()) {
            Vertex u = pq.extract_min();
            
            for (const Edge& e : graph.getNeighbors(u)) {
                if (dist[u] + e.weight < dist[e.to]) {
                    dist[e.to] = dist[u] + e.weight;
                    pq.decrease_key(e.to, dist[e.to]);
                }
            }
        }
        
        return result;
    }
};
```

**Complexity: O((V+E) log V)**

### 6.2 Sequential - Fibonacci Heap

```cpp
class DijkstraFibonacciHeap {
    static DijkstraResult run(const Graph& graph, Vertex source) {
        // Same as binary, tapi dengan FibonacciHeap
        // extract_min() O(log n)
        // decrease_key() O(1)
        
        // Total: O(E * 1 + V * log V) = O(E + V log V)
    }
};
```

**Complexity: O(E + V log V)**

### 6.3 OpenMP - Binary Heap

```cpp
class DijkstraBinaryHeapOMP {
    while (!pq.is_empty()) {
        Vertex u = pq.extract_min();  // Sequential
        
        const auto& neighbors = graph.getNeighbors(u);
        
        // Parallelize edge relaxation
        #pragma omp parallel for
        for (size_t i = 0; i < neighbors.size(); i++) {
            const Edge& e = neighbors[i];
            
            if (dist[u] + e.weight < dist[e.to]) {
                #pragma omp critical  // Protect update
                {
                    if (dist[u] + e.weight < dist[e.to]) {
                        dist[e.to] = dist[u] + e.weight;
                        pq.decrease_key(e.to, dist[e.to]);
                    }
                }
            }
        }
    }
};
```

**Parallelization Strategy:**
- Sequential: extract_min (V times, V* log V)
- Parallel: Edge relaxation (E times, E / num_threads)

**Speedup:**
- Ideal: num_threads × untuk parallel portion
- Reality: overhead, diminishing returns

### 6.4 Cilk-Style - Fibonacci Heap

```cpp
class DijkstraFibonacciHeapCilk {
    // Same as sequential
    // Placeholder for actual Cilk parallel version
    // Would use: cilk_for, cilk_spawn, etc
};
```

---

## 7. COMPILATION & EXECUTION

### Build

```bash
cd dijkstra_project
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./dijkstra_benchmark
```

**Output:**
```
════════════════════════════════════════════════════════════════════════════
DIJKSTRA'S ALGORITHM: FIBONACCI vs BINARY HEAP COMPARISON
════════════════════════════════════════════════════════════════════════════

TEST 1: Small Sparse Graph (n=100)
────────────────────────────────────────────────────────────────────────────
  Vertices: 100
  Edges:    300
  Algorithm:              Binary Heap         | Time:   0.0012 ms | Ops: 4.00e+02
  Algorithm:         Fibonacci Heap          | Time:   0.0045 ms | Ops: 4.00e+02
  ✓ Results verified (identical)
```

---

## 8. KEY INSIGHTS

### Why Binary Heap Usually Wins

1. **Better Constants:** O(log n) with small constants vs O(1) with huge constants
2. **Cache Locality:** Array-based vs pointer-heavy
3. **Real Graphs:** E rarely >> V log V in practice
4. **Implementation Complexity:** Easier = fewer bugs = faster

### When Fibonacci Might Win

1. **Very Dense Graphs:** E ≈ V^2 (rare)
2. **Theoretical Analysis:** Not practical measurement
3. **Specific Graph Patterns:** Highly degree-variable graphs

### Parallelization Insights

**OpenMP Binary Heap:**
- Good untuk sparse graphs (more edges to parallelize)
- Atomic operations overhead limits scaling
- ~2-4x speedup on 4 cores typical

**Cilk Fibonacci:**
- Complex to parallelize correctly
- Cascading cuts hard to coordinate
- Work stealing not ideal untuk priority queue

---

## 9. FURTHER OPTIMIZATION

### Possible Improvements

**Binary Heap:**
- Cache-aware layout
- Multiway heap (better cache)
- Pairing heap (simpler Fibonacci properties)

**Fibonacci:**
- Ranked Fibonacci (removes marked flag)
- Eager consolidation (reduce worst-case)
- Lazy deletion (defer removals)

**General:**
- Parallel graph partitioning
- Multilevel approaches
- Specialized structures untuk specific graphs

