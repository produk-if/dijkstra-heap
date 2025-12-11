/*
 * ═══════════════════════════════════════════════════════════════════════════
 * DIJKSTRA'S ALGORITHM - FIBONACCI HEAP vs BINARY HEAP
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Program ini membandingkan performa 2 jenis priority queue:
 * 1. Binary Heap  - Simple, praktis, O(log n) per operasi
 * 2. Fibonacci Heap - Kompleks, teoritis lebih baik, O(1) amortized decrease_key
 * 
 * Kompilasi: g++ -O2 -fopenmp main.cpp -o dijkstra
 * Jalankan:  ./dijkstra
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <vector>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <algorithm>

using namespace std;

// ═══════════════════════════════════════════════════════════════════════════
// TIPE DATA DASAR
// ═══════════════════════════════════════════════════════════════════════════

const double INF = numeric_limits<double>::infinity();

// Struktur untuk menyimpan edge (sisi graf)
struct Edge {
    int to;         // Vertex tujuan
    double weight;  // Bobot/jarak
    
    Edge(int t, double w) : to(t), weight(w) {}
};

// ═══════════════════════════════════════════════════════════════════════════
// KELAS GRAPH - Representasi Graf dengan Adjacency List
// ═══════════════════════════════════════════════════════════════════════════

class Graph {
public:
    int num_vertices;
    int num_edges;
    vector<vector<Edge>> adj_list;  // Adjacency list
    
    // Constructor
    Graph(int n) : num_vertices(n), num_edges(0), adj_list(n) {}
    
    // Tambah edge dari 'from' ke 'to' dengan bobot 'weight'
    void addEdge(int from, int to, double weight) {
        adj_list[from].push_back(Edge(to, weight));
        num_edges++;
    }
    
    // Tambah edge dua arah (untuk graf tidak berarah)
    void addUndirectedEdge(int u, int v, double weight) {
        adj_list[u].push_back(Edge(v, weight));
        adj_list[v].push_back(Edge(u, weight));
        num_edges += 2;
    }
    
    // Generate graf sparse (jarang) secara random
    static Graph generateSparse(int n, int edges_per_vertex = 3) {
        Graph g(n);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < edges_per_vertex; j++) {
                int to = rand() % n;
                if (to != i) {
                    double w = 1.0 + (rand() % 100);
                    g.addEdge(i, to, w);
                }
            }
        }
        return g;
    }
    
    // Generate graf dense (padat) secara random
    static Graph generateDense(int n, double density = 0.5) {
        Graph g(n);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j && (rand() / (double)RAND_MAX) < density) {
                    double w = 1.0 + (rand() % 100);
                    g.addEdge(i, j, w);
                }
            }
        }
        return g;
    }
    
    // Generate graf grid (seperti papan catur)
    static Graph generateGrid(int rows, int cols) {
        Graph g(rows * cols);
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int v = r * cols + c;
                
                // Tetangga kanan
                if (c + 1 < cols) {
                    double w = 1.0 + (rand() % 100);
                    g.addUndirectedEdge(v, v + 1, w);
                }
                
                // Tetangga bawah
                if (r + 1 < rows) {
                    double w = 1.0 + (rand() % 100);
                    g.addUndirectedEdge(v, v + cols, w);
                }
            }
        }
        return g;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// KELAS BINARY HEAP - Priority Queue Sederhana
// ═══════════════════════════════════════════════════════════════════════════
// 
// Binary Heap adalah struktur data berbasis array.
// - Insert: O(log n)
// - Extract Min: O(log n)  
// - Decrease Key: O(log n)
//
// Dijkstra dengan Binary Heap: O((V + E) log V)
//
// ═══════════════════════════════════════════════════════════════════════════

class BinaryHeap {
private:
    // Node dalam heap
    struct Node {
        int vertex;
        double key;
        Node(int v = 0, double k = INF) : vertex(v), key(k) {}
    };
    
    vector<Node> heap;              // Array heap
    vector<int> vertex_to_index;    // Mapping vertex -> posisi di heap
    vector<bool> in_heap;           // Apakah vertex ada di heap
    int heap_size;
    
    // Geser node ke atas sampai property min-heap terpenuhi
    void bubble_up(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            
            // Jika node lebih kecil dari parent, tukar
            if (heap[idx].key < heap[parent].key) {
                swap(heap[idx], heap[parent]);
                vertex_to_index[heap[idx].vertex] = idx;
                vertex_to_index[heap[parent].vertex] = parent;
                idx = parent;
            } else {
                break;
            }
        }
    }
    
    // Geser node ke bawah sampai property min-heap terpenuhi
    void bubble_down(int idx) {
        while (true) {
            int smallest = idx;
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            
            // Cari yang terkecil antara node, left child, right child
            if (left < heap_size && heap[left].key < heap[smallest].key) {
                smallest = left;
            }
            if (right < heap_size && heap[right].key < heap[smallest].key) {
                smallest = right;
            }
            
            // Jika bukan node current yang terkecil, tukar
            if (smallest != idx) {
                swap(heap[idx], heap[smallest]);
                vertex_to_index[heap[idx].vertex] = idx;
                vertex_to_index[heap[smallest].vertex] = smallest;
                idx = smallest;
            } else {
                break;
            }
        }
    }

public:
    BinaryHeap(int n) : heap(n), vertex_to_index(n, -1), in_heap(n, false), heap_size(0) {}
    
    bool isEmpty() { return heap_size == 0; }
    
    // Masukkan vertex dengan key tertentu
    void insert(int v, double key) {
        if (in_heap[v]) {
            decreaseKey(v, key);
            return;
        }
        
        int idx = heap_size++;
        heap[idx] = Node(v, key);
        vertex_to_index[v] = idx;
        in_heap[v] = true;
        
        bubble_up(idx);
    }
    
    // Ambil vertex dengan key terkecil
    int extractMin() {
        int min_v = heap[0].vertex;
        in_heap[min_v] = false;
        vertex_to_index[min_v] = -1;
        
        heap_size--;
        if (heap_size > 0) {
            heap[0] = heap[heap_size];
            vertex_to_index[heap[0].vertex] = 0;
            bubble_down(0);
        }
        
        return min_v;
    }
    
    // Kurangi key dari vertex (untuk update jarak)
    void decreaseKey(int v, double new_key) {
        if (!in_heap[v]) {
            insert(v, new_key);
            return;
        }
        
        int idx = vertex_to_index[v];
        if (new_key < heap[idx].key) {
            heap[idx].key = new_key;
            bubble_up(idx);
        }
    }
    
    bool contains(int v) { return in_heap[v]; }
};

// ═══════════════════════════════════════════════════════════════════════════
// KELAS FIBONACCI HEAP - Priority Queue Kompleks (FOKUS UTAMA!)
// ═══════════════════════════════════════════════════════════════════════════
//
// Fibonacci Heap menggunakan struktur pohon dengan circular linked list.
// - Insert: O(1) amortized
// - Extract Min: O(log n) amortized
// - Decrease Key: O(1) amortized  <-- KEUNGGULAN UTAMA!
//
// Dijkstra dengan Fibonacci Heap: O(E + V log V)
// Lebih baik secara teori, tapi lebih lambat di praktik!
//
// ═══════════════════════════════════════════════════════════════════════════

class FibonacciHeap {
private:
    // Node dalam Fibonacci Heap (lebih kompleks dari Binary Heap)
    struct Node {
        int vertex;
        double key;
        
        Node* parent;    // Pointer ke parent
        Node* child;     // Pointer ke salah satu child
        Node* left;      // Sibling kiri (circular list)
        Node* right;     // Sibling kanan (circular list)
        
        int degree;      // Jumlah children
        bool marked;     // Flag untuk cascading cut
        
        Node(int v, double k) 
            : vertex(v), key(k), parent(nullptr), child(nullptr),
              left(this), right(this), degree(0), marked(false) {}
    };
    
    Node* min_node;              // Pointer ke node minimum
    int n_nodes;                 // Jumlah node
    vector<Node*> vertices;      // Direct access ke node berdasarkan vertex ID
    
    // Tambah node ke root list
    void addToRootList(Node* node) {
        if (min_node == nullptr) {
            min_node = node;
            node->left = node;
            node->right = node;
        } else {
            // Sisipkan ke kanan min_node
            node->left = min_node;
            node->right = min_node->right;
            min_node->right->left = node;
            min_node->right = node;
        }
        node->parent = nullptr;
    }
    
    // Hapus node dari sibling list
    void removeFromList(Node* node) {
        node->left->right = node->right;
        node->right->left = node->left;
    }
    
    // Jadikan y sebagai child dari x
    void link(Node* y, Node* x) {
        removeFromList(y);
        y->parent = x;
        
        if (x->child == nullptr) {
            x->child = y;
            y->left = y;
            y->right = y;
        } else {
            y->left = x->child;
            y->right = x->child->right;
            x->child->right->left = y;
            x->child->right = y;
        }
        
        x->degree++;
        y->marked = false;
    }
    
    // Consolidate: gabungkan pohon dengan degree sama
    // Ini yang membuat extract_min menjadi O(log n)
    void consolidate() {
        if (min_node == nullptr) return;
        
        int max_degree = (int)(log2(n_nodes + 1)) + 2;
        vector<Node*> A(max_degree, nullptr);
        
        // Kumpulkan semua root
        vector<Node*> roots;
        Node* current = min_node;
        do {
            roots.push_back(current);
            current = current->right;
        } while (current != min_node);
        
        // Proses setiap root
        for (Node* w : roots) {
            Node* x = w;
            int d = x->degree;
            
            // Gabungkan pohon dengan degree sama
            while (d < max_degree && A[d] != nullptr) {
                Node* y = A[d];
                
                if (x->key > y->key) {
                    swap(x, y);
                }
                
                link(y, x);  // y jadi child dari x
                A[d] = nullptr;
                d++;
            }
            
            if (d < max_degree) {
                A[d] = x;
            }
        }
        
        // Bangun ulang root list dan cari min baru
        min_node = nullptr;
        for (int i = 0; i < max_degree; i++) {
            if (A[i] != nullptr) {
                if (min_node == nullptr) {
                    min_node = A[i];
                    A[i]->left = A[i];
                    A[i]->right = A[i];
                } else {
                    A[i]->left = min_node;
                    A[i]->right = min_node->right;
                    min_node->right->left = A[i];
                    min_node->right = A[i];
                    
                    if (A[i]->key < min_node->key) {
                        min_node = A[i];
                    }
                }
            }
        }
    }
    
    // Cut: potong node dari parent dan tambahkan ke root list
    void cut(Node* node, Node* parent) {
        if (parent->child == node) {
            if (node->right == node) {
                parent->child = nullptr;
            } else {
                parent->child = node->right;
            }
        }
        removeFromList(node);
        parent->degree--;
        
        addToRootList(node);
        node->marked = false;
    }
    
    // Cascading Cut: potong secara berantai ke atas
    // Ini yang membuat decrease_key O(1) amortized
    void cascadingCut(Node* node) {
        Node* parent = node->parent;
        if (parent != nullptr) {
            if (!node->marked) {
                node->marked = true;  // Tandai pertama kali kehilangan child
            } else {
                cut(node, parent);    // Sudah ditandai, potong!
                cascadingCut(parent); // Lanjutkan ke atas
            }
        }
    }
    
    // Hapus semua node (cleanup memory)
    void cleanup(Node* node) {
        if (node == nullptr) return;
        
        vector<Node*> to_delete;
        Node* current = node;
        do {
            to_delete.push_back(current);
            if (current->child) {
                cleanup(current->child);
            }
            current = current->right;
        } while (current != node);
        
        for (Node* n : to_delete) {
            delete n;
        }
    }

public:
    FibonacciHeap(int n) : min_node(nullptr), n_nodes(0), vertices(n, nullptr) {}
    
    ~FibonacciHeap() {
        if (min_node) {
            cleanup(min_node);
        }
    }
    
    bool isEmpty() { return min_node == nullptr; }
    
    // Insert: O(1) - cukup tambahkan ke root list
    void insert(int v, double key) {
        if (vertices[v] != nullptr) {
            decreaseKey(v, key);
            return;
        }
        
        Node* node = new Node(v, key);
        vertices[v] = node;
        
        addToRootList(node);
        
        if (min_node == nullptr || key < min_node->key) {
            min_node = node;
        }
        
        n_nodes++;
    }
    
    // Extract Min: O(log n) amortized
    int extractMin() {
        Node* z = min_node;
        int min_v = z->vertex;
        
        // Tambahkan semua children ke root list
        if (z->child != nullptr) {
            vector<Node*> children;
            Node* child = z->child;
            do {
                children.push_back(child);
                child = child->right;
            } while (child != z->child);
            
            for (Node* c : children) {
                addToRootList(c);
            }
        }
        
        // Hapus z dari root list
        if (z == z->right) {
            min_node = nullptr;
        } else {
            min_node = z->right;
            removeFromList(z);
            consolidate();  // Gabungkan pohon-pohon
        }
        
        vertices[min_v] = nullptr;
        delete z;
        n_nodes--;
        
        return min_v;
    }
    
    // Decrease Key: O(1) amortized - KEUNGGULAN UTAMA!
    void decreaseKey(int v, double new_key) {
        Node* node = vertices[v];
        
        if (node == nullptr) {
            insert(v, new_key);
            return;
        }
        
        if (new_key >= node->key) return;
        
        node->key = new_key;
        Node* parent = node->parent;
        
        // Jika melanggar min-heap property, cut!
        if (parent != nullptr && node->key < parent->key) {
            cut(node, parent);
            cascadingCut(parent);
        }
        
        // Update min jika perlu
        if (node->key < min_node->key) {
            min_node = node;
        }
    }
    
    bool contains(int v) { return vertices[v] != nullptr; }
};

// ═══════════════════════════════════════════════════════════════════════════
// ALGORITMA DIJKSTRA
// ═══════════════════════════════════════════════════════════════════════════

// Hasil dari Dijkstra
struct DijkstraResult {
    vector<double> dist;      // Jarak terpendek dari source
    vector<int> parent;       // Parent untuk rekonstruksi path
    int operations;           // Jumlah operasi (untuk analisis)
};

// Dijkstra dengan Binary Heap
DijkstraResult dijkstraBinaryHeap(Graph& g, int source) {
    int n = g.num_vertices;
    DijkstraResult result;
    result.dist.assign(n, INF);
    result.parent.assign(n, -1);
    result.operations = 0;
    
    result.dist[source] = 0;
    
    BinaryHeap pq(n);
    pq.insert(source, 0);
    
    while (!pq.isEmpty()) {
        int u = pq.extractMin();
        result.operations++;
        
        // Relax semua edge dari u
        for (const Edge& e : g.adj_list[u]) {
            int v = e.to;
            double new_dist = result.dist[u] + e.weight;
            
            if (new_dist < result.dist[v]) {
                result.dist[v] = new_dist;
                result.parent[v] = u;
                pq.decreaseKey(v, new_dist);
                result.operations++;
            }
        }
    }
    
    return result;
}

// Dijkstra dengan Fibonacci Heap
DijkstraResult dijkstraFibonacciHeap(Graph& g, int source) {
    int n = g.num_vertices;
    DijkstraResult result;
    result.dist.assign(n, INF);
    result.parent.assign(n, -1);
    result.operations = 0;
    
    result.dist[source] = 0;
    
    FibonacciHeap pq(n);
    pq.insert(source, 0);
    
    while (!pq.isEmpty()) {
        int u = pq.extractMin();
        result.operations++;
        
        // Relax semua edge dari u
        for (const Edge& e : g.adj_list[u]) {
            int v = e.to;
            double new_dist = result.dist[u] + e.weight;
            
            if (new_dist < result.dist[v]) {
                result.dist[v] = new_dist;
                result.parent[v] = u;
                pq.decreaseKey(v, new_dist);
                result.operations++;
            }
        }
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNGSI UTILITAS
// ═══════════════════════════════════════════════════════════════════════════

// Verifikasi apakah dua hasil sama
bool verifyResults(const DijkstraResult& r1, const DijkstraResult& r2) {
    if (r1.dist.size() != r2.dist.size()) return false;
    
    for (size_t i = 0; i < r1.dist.size(); i++) {
        if (abs(r1.dist[i] - r2.dist[i]) > 1e-9) {
            return false;
        }
    }
    return true;
}

// Cetak garis pemisah
void printSeparator(char c = '═', int len = 78) {
    for (int i = 0; i < len; i++) cout << c;
    cout << endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNGSI TEST
// ═══════════════════════════════════════════════════════════════════════════

void runTest(const string& name, Graph& g, int source) {
    cout << "\n";
    printSeparator('-', 78);
    cout << "TEST: " << name << endl;
    printSeparator('-', 78);
    cout << "  Vertices: " << g.num_vertices << endl;
    cout << "  Edges:    " << g.num_edges << endl;
    cout << endl;
    
    // Test Binary Heap
    auto start1 = chrono::high_resolution_clock::now();
    DijkstraResult result1 = dijkstraBinaryHeap(g, source);
    auto end1 = chrono::high_resolution_clock::now();
    double time1 = chrono::duration<double, milli>(end1 - start1).count();
    
    // Test Fibonacci Heap
    auto start2 = chrono::high_resolution_clock::now();
    DijkstraResult result2 = dijkstraFibonacciHeap(g, source);
    auto end2 = chrono::high_resolution_clock::now();
    double time2 = chrono::duration<double, milli>(end2 - start2).count();
    
    // Print results
    cout << fixed << setprecision(4);
    cout << "  BINARY HEAP:" << endl;
    cout << "    Waktu:   " << setw(10) << time1 << " ms" << endl;
    cout << "    Operasi: " << setw(10) << result1.operations << endl;
    cout << endl;
    
    cout << "  FIBONACCI HEAP:" << endl;
    cout << "    Waktu:   " << setw(10) << time2 << " ms" << endl;
    cout << "    Operasi: " << setw(10) << result2.operations << endl;
    cout << endl;
    
    // Comparison
    double ratio = time2 / time1;
    cout << "  PERBANDINGAN:" << endl;
    cout << "    Rasio (Fib/Binary): " << setw(6) << ratio << "x" << endl;
    
    if (ratio > 1.0) {
        cout << "    >> Binary Heap " << ratio << "x LEBIH CEPAT!" << endl;
    } else {
        cout << "    >> Fibonacci Heap " << (1.0/ratio) << "x lebih cepat" << endl;
    }
    
    // Verify correctness
    if (verifyResults(result1, result2)) {
        cout << "\n  ✓ Hasil TERVERIFIKASI (kedua algoritma menghasilkan jarak yang sama)" << endl;
    } else {
        cout << "\n  ✗ ERROR: Hasil berbeda!" << endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN PROGRAM
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    srand(42);  // Seed untuk reproducibility
    
    cout << endl;
    printSeparator();
    cout << "     DIJKSTRA'S ALGORITHM: FIBONACCI HEAP vs BINARY HEAP" << endl;
    printSeparator();
    cout << endl;
    cout << "  Program ini membandingkan performa dua jenis priority queue:" << endl;
    cout << "  1. Binary Heap    - Simple, O(log n) per operasi" << endl;
    cout << "  2. Fibonacci Heap - Kompleks, O(1) amortized decrease_key" << endl;
    cout << endl;
    cout << "  TEORI:" << endl;
    cout << "    Binary Heap:    Dijkstra = O((V + E) log V)" << endl;
    cout << "    Fibonacci Heap: Dijkstra = O(E + V log V)  <- lebih baik!" << endl;
    cout << endl;
    cout << "  KENYATAAN:" << endl;
    cout << "    Binary Heap biasanya 2-5x lebih CEPAT karena:" << endl;
    cout << "    - Cache locality lebih baik (array vs pointer)" << endl;
    cout << "    - Overhead konstanta lebih kecil" << endl;
    cout << "    - Graf nyata biasanya sparse (E ~ V)" << endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 1: Graf Kecil Sparse
    // ═══════════════════════════════════════════════════════════════════════
    {
        Graph g = Graph::generateSparse(100, 3);
        runTest("Graf Kecil Sparse (n=100, ~3 edge per vertex)", g, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 2: Graf Menengah Sparse
    // ═══════════════════════════════════════════════════════════════════════
    {
        Graph g = Graph::generateSparse(1000, 5);
        runTest("Graf Menengah Sparse (n=1000, ~5 edge per vertex)", g, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 3: Graf Grid
    // ═══════════════════════════════════════════════════════════════════════
    {
        Graph g = Graph::generateGrid(50, 50);  // 2500 vertices
        runTest("Graf Grid 50x50 (n=2500, ~4 edge per vertex)", g, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 4: Graf Lebih Besar Sparse
    // ═══════════════════════════════════════════════════════════════════════
    {
        Graph g = Graph::generateSparse(5000, 10);
        runTest("Graf Besar Sparse (n=5000, ~10 edge per vertex)", g, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 5: Graf Dense (dimana Fibonacci MUNGKIN lebih cepat)
    // ═══════════════════════════════════════════════════════════════════════
    {
        Graph g = Graph::generateDense(500, 0.3);  // 30% density
        runTest("Graf Dense (n=500, density=30%)", g, 0);
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // KESIMPULAN
    // ═══════════════════════════════════════════════════════════════════════
    
    cout << endl;
    printSeparator();
    cout << "     KESIMPULAN" << endl;
    printSeparator();
    cout << endl;
    cout << "  KEY INSIGHT - FIBONACCI HEAP PARADOX:" << endl;
    cout << endl;
    cout << "  ┌─────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │  TEORI:    Fibonacci Heap lebih baik O(E + V log V)        │" << endl;
    cout << "  │  PRAKTIK:  Binary Heap biasanya 2-5x LEBIH CEPAT!          │" << endl;
    cout << "  └─────────────────────────────────────────────────────────────┘" << endl;
    cout << endl;
    cout << "  MENGAPA FIBONACCI LEBIH LAMBAT?" << endl;
    cout << "  1. Memory: ~56 bytes per node (vs 16 bytes Binary)" << endl;
    cout << "  2. Pointer chasing: banyak cache miss" << endl;
    cout << "  3. Overhead operasi: cascading cut, consolidate, dll" << endl;
    cout << "  4. Graf nyata: E tidak >> V log V" << endl;
    cout << endl;
    cout << "  REKOMENDASI:" << endl;
    cout << "  - Gunakan BINARY HEAP untuk project nyata" << endl;
    cout << "  - Gunakan FIBONACCI HEAP untuk belajar amortized analysis" << endl;
    cout << endl;
    printSeparator();
    cout << "     SELESAI!" << endl;
    printSeparator();
    cout << endl;
    
    return 0;
}
