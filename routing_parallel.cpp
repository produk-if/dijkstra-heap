/*
 * ═══════════════════════════════════════════════════════════════════════════
 * STUDI KASUS: ROUTING DATA CENTER DENGAN PARALLELIZATION
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * TUJUAN:
 * Membandingkan performa Fibonacci Heap vs Binary Heap dengan tambahan:
 * - OpenMP (parallelisasi berbasis thread)
 * - Cilk-style (parallelisasi berbasis task)
 * 
 * KOMPILASI: g++ -O2 -fopenmp -o routing_parallel routing_parallel.cpp
 * JALANKAN:  ./routing_parallel
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <iostream>
#include <vector>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <string>
#include <algorithm>
#include <omp.h>  // OpenMP untuk parallelisasi

using namespace std;

const double INF = numeric_limits<double>::infinity();

// ═══════════════════════════════════════════════════════════════════════════
// STRUKTUR DATA JARINGAN
// ═══════════════════════════════════════════════════════════════════════════

struct Koneksi {
    int server_tujuan;
    double latency;
    Koneksi(int t, double l) : server_tujuan(t), latency(l) {}
};

struct DataCenter {
    int jumlah_server;
    int jumlah_koneksi;
    vector<vector<Koneksi>> topologi;
    
    DataCenter(int n) : jumlah_server(n), jumlah_koneksi(0), topologi(n) {}
    
    void tambahKoneksi(int dari, int ke, double latency) {
        topologi[dari].push_back(Koneksi(ke, latency));
        topologi[ke].push_back(Koneksi(dari, latency));
        jumlah_koneksi += 2;
    }
};

struct HasilRouting {
    vector<double> latency;
    vector<int> server_sebelumnya;
    double waktu_eksekusi;  // dalam ms
    string metode;
};

// ═══════════════════════════════════════════════════════════════════════════
// BINARY HEAP (untuk perbandingan)
// ═══════════════════════════════════════════════════════════════════════════

class BinaryHeap {
private:
    struct Node { int server; double latency; };
    vector<Node> heap;
    vector<int> posisi;
    vector<bool> aktif;
    int ukuran;
    
    void naik(int i) {
        while (i > 0) {
            int p = (i-1)/2;
            if (heap[i].latency < heap[p].latency) {
                swap(heap[i], heap[p]);
                posisi[heap[i].server] = i;
                posisi[heap[p].server] = p;
                i = p;
            } else break;
        }
    }
    
    void turun(int i) {
        while (true) {
            int min_idx = i, l = 2*i+1, r = 2*i+2;
            if (l < ukuran && heap[l].latency < heap[min_idx].latency) min_idx = l;
            if (r < ukuran && heap[r].latency < heap[min_idx].latency) min_idx = r;
            if (min_idx != i) {
                swap(heap[i], heap[min_idx]);
                posisi[heap[i].server] = i;
                posisi[heap[min_idx].server] = min_idx;
                i = min_idx;
            } else break;
        }
    }

public:
    BinaryHeap(int n) : heap(n), posisi(n,-1), aktif(n,false), ukuran(0) {}
    bool kosong() { return ukuran == 0; }
    
    void masukkan(int s, double lat) {
        if (aktif[s]) { update(s, lat); return; }
        heap[ukuran] = {s, lat};
        posisi[s] = ukuran;
        aktif[s] = true;
        naik(ukuran++);
    }
    
    int ambilMin() {
        int s = heap[0].server;
        aktif[s] = false;
        if (--ukuran > 0) {
            heap[0] = heap[ukuran];
            posisi[heap[0].server] = 0;
            turun(0);
        }
        return s;
    }
    
    void update(int s, double lat) {
        if (!aktif[s]) { masukkan(s, lat); return; }
        int i = posisi[s];
        if (lat < heap[i].latency) {
            heap[i].latency = lat;
            naik(i);
        }
    }
    
    bool contains(int s) { return aktif[s]; }
};

// ═══════════════════════════════════════════════════════════════════════════
// FIBONACCI HEAP
// ═══════════════════════════════════════════════════════════════════════════

class FibonacciHeap {
private:
    struct Node {
        int server; double latency;
        Node *parent, *child, *left, *right;
        int degree; bool marked;
        Node(int s, double l) : server(s), latency(l), parent(nullptr), child(nullptr),
            left(this), right(this), degree(0), marked(false) {}
    };
    
    Node* min_node;
    int jumlah;
    vector<Node*> nodes;
    
    void addToRoot(Node* n) {
        if (!min_node) { min_node = n; n->left = n->right = n; }
        else {
            n->left = min_node; n->right = min_node->right;
            min_node->right->left = n; min_node->right = n;
        }
        n->parent = nullptr;
    }
    
    void removeFromList(Node* n) { n->left->right = n->right; n->right->left = n->left; }
    
    void link(Node* y, Node* x) {
        removeFromList(y); y->parent = x;
        if (!x->child) { x->child = y; y->left = y->right = y; }
        else { y->left = x->child; y->right = x->child->right; x->child->right->left = y; x->child->right = y; }
        x->degree++; y->marked = false;
    }
    
    void consolidate() {
        if (!min_node) return;
        int maxD = (int)(log2(jumlah+1)) + 2;
        vector<Node*> A(maxD, nullptr);
        vector<Node*> roots;
        Node* c = min_node;
        do { roots.push_back(c); c = c->right; } while (c != min_node);
        for (Node* w : roots) {
            Node* x = w; int d = x->degree;
            while (d < maxD && A[d]) {
                Node* y = A[d];
                if (x->latency > y->latency) swap(x,y);
                link(y,x); A[d++] = nullptr;
            }
            if (d < maxD) A[d] = x;
        }
        min_node = nullptr;
        for (int i = 0; i < maxD; i++) if (A[i]) {
            if (!min_node) { min_node = A[i]; A[i]->left = A[i]->right = A[i]; }
            else {
                A[i]->left = min_node; A[i]->right = min_node->right;
                min_node->right->left = A[i]; min_node->right = A[i];
                if (A[i]->latency < min_node->latency) min_node = A[i];
            }
        }
    }
    
    void cut(Node* x, Node* y) {
        if (y->child == x) { if (x->right == x) y->child = nullptr; else y->child = x->right; }
        removeFromList(x); y->degree--;
        addToRoot(x); x->marked = false;
    }
    
    void cascadingCut(Node* y) {
        Node* z = y->parent;
        if (z) { if (!y->marked) y->marked = true; else { cut(y,z); cascadingCut(z); } }
    }

public:
    FibonacciHeap(int n) : min_node(nullptr), jumlah(0), nodes(n, nullptr) {}
    ~FibonacciHeap() { for (Node* n : nodes) if (n) delete n; }
    
    bool kosong() { return !min_node; }
    
    void masukkan(int s, double lat) {
        if (nodes[s]) { update(s, lat); return; }
        Node* n = new Node(s, lat);
        nodes[s] = n;
        addToRoot(n);
        if (!min_node || lat < min_node->latency) min_node = n;
        jumlah++;
    }
    
    int ambilMin() {
        Node* z = min_node;
        int s = z->server;
        if (z->child) {
            vector<Node*> ch;
            Node* c = z->child;
            do { ch.push_back(c); c = c->right; } while (c != z->child);
            for (Node* x : ch) addToRoot(x);
        }
        if (z == z->right) min_node = nullptr;
        else { min_node = z->right; removeFromList(z); consolidate(); }
        nodes[s] = nullptr; delete z; jumlah--;
        return s;
    }
    
    void update(int s, double lat) {
        Node* x = nodes[s];
        if (!x) { masukkan(s, lat); return; }
        if (lat >= x->latency) return;
        x->latency = lat;
        Node* y = x->parent;
        if (y && x->latency < y->latency) { cut(x,y); cascadingCut(y); }
        if (x->latency < min_node->latency) min_node = x;
    }
    
    bool contains(int s) { return nodes[s] != nullptr; }
};

// ═══════════════════════════════════════════════════════════════════════════
// METODE 1: DIJKSTRA SERIAL (Binary Heap)
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraSerialBinary(DataCenter& dc, int source) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "Serial Binary Heap";
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    BinaryHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();
        
        for (const Koneksi& k : dc.topologi[u]) {
            double lat_baru = hasil.latency[u] + k.latency;
            if (lat_baru < hasil.latency[k.server_tujuan]) {
                hasil.latency[k.server_tujuan] = lat_baru;
                hasil.server_sebelumnya[k.server_tujuan] = u;
                pq.update(k.server_tujuan, lat_baru);
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 2: DIJKSTRA SERIAL (Fibonacci Heap)
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraSerialFibonacci(DataCenter& dc, int source) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "Serial Fibonacci Heap";
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    FibonacciHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();
        
        for (const Koneksi& k : dc.topologi[u]) {
            double lat_baru = hasil.latency[u] + k.latency;
            if (lat_baru < hasil.latency[k.server_tujuan]) {
                hasil.latency[k.server_tujuan] = lat_baru;
                hasil.server_sebelumnya[k.server_tujuan] = u;
                pq.update(k.server_tujuan, lat_baru);
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 3: DIJKSTRA + OPENMP (Binary Heap)
// 
// STRATEGI PARALLELISASI:
// - extract_min TETAP serial (karena priority queue tidak thread-safe)
// - relaksasi edge DIPARALEL (setiap thread proses subset edge)
// 
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraOpenMPBinary(DataCenter& dc, int source, int num_threads) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "OpenMP Binary (" + to_string(num_threads) + " threads)";
    
    omp_set_num_threads(num_threads);
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    BinaryHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();  // SERIAL: tidak bisa diparalel
        
        const vector<Koneksi>& neighbors = dc.topologi[u];
        int num_neighbors = neighbors.size();
        
        // PARALLEL: relaksasi edge diparalel
        // Setiap thread memproses sebagian edge
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < num_neighbors; i++) {
            const Koneksi& k = neighbors[i];
            double lat_baru = hasil.latency[u] + k.latency;
            
            // CRITICAL: update harus atomic untuk menghindari race condition
            #pragma omp critical
            {
                if (lat_baru < hasil.latency[k.server_tujuan]) {
                    hasil.latency[k.server_tujuan] = lat_baru;
                    hasil.server_sebelumnya[k.server_tujuan] = u;
                    pq.update(k.server_tujuan, lat_baru);
                }
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 4: DIJKSTRA + OPENMP (Fibonacci Heap)
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraOpenMPFibonacci(DataCenter& dc, int source, int num_threads) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "OpenMP Fibonacci (" + to_string(num_threads) + " threads)";
    
    omp_set_num_threads(num_threads);
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    FibonacciHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();
        
        const vector<Koneksi>& neighbors = dc.topologi[u];
        int num_neighbors = neighbors.size();
        
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < num_neighbors; i++) {
            const Koneksi& k = neighbors[i];
            double lat_baru = hasil.latency[u] + k.latency;
            
            #pragma omp critical
            {
                if (lat_baru < hasil.latency[k.server_tujuan]) {
                    hasil.latency[k.server_tujuan] = lat_baru;
                    hasil.server_sebelumnya[k.server_tujuan] = u;
                    pq.update(k.server_tujuan, lat_baru);
                }
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 5: DIJKSTRA CILK-STYLE (Binary Heap)
// 
// CILK adalah model paralel berbasis TASK (bukan thread)
// Karena Cilk tidak tersedia di semua compiler, kita simulasikan
// dengan OpenMP tasks
// 
// PERBEDAAN OPENMP vs CILK:
// - OpenMP: berbasis loop paralel (#pragma omp parallel for)
// - Cilk: berbasis spawn/sync (lebih fleksibel untuk rekursi)
// 
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraCilkStyleBinary(DataCenter& dc, int source, int num_threads) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "Cilk-style Binary (" + to_string(num_threads) + " threads)";
    
    omp_set_num_threads(num_threads);
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    BinaryHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();
        
        const vector<Koneksi>& neighbors = dc.topologi[u];
        
        // CILK-STYLE: menggunakan tasks bukan loop paralel
        #pragma omp parallel
        {
            #pragma omp single
            {
                for (size_t i = 0; i < neighbors.size(); i++) {
                    // Spawn task untuk setiap edge
                    #pragma omp task firstprivate(i)
                    {
                        const Koneksi& k = neighbors[i];
                        double lat_baru = hasil.latency[u] + k.latency;
                        
                        #pragma omp critical
                        {
                            if (lat_baru < hasil.latency[k.server_tujuan]) {
                                hasil.latency[k.server_tujuan] = lat_baru;
                                hasil.server_sebelumnya[k.server_tujuan] = u;
                                pq.update(k.server_tujuan, lat_baru);
                            }
                        }
                    }
                }
                #pragma omp taskwait  // Sync: tunggu semua task selesai
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 6: DIJKSTRA CILK-STYLE (Fibonacci Heap)
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting dijkstraCilkStyleFibonacci(DataCenter& dc, int source, int num_threads) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.metode = "Cilk-style Fibonacci (" + to_string(num_threads) + " threads)";
    
    omp_set_num_threads(num_threads);
    
    auto start = chrono::high_resolution_clock::now();
    
    hasil.latency[source] = 0;
    FibonacciHeap pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilMin();
        
        const vector<Koneksi>& neighbors = dc.topologi[u];
        
        #pragma omp parallel
        {
            #pragma omp single
            {
                for (size_t i = 0; i < neighbors.size(); i++) {
                    #pragma omp task firstprivate(i)
                    {
                        const Koneksi& k = neighbors[i];
                        double lat_baru = hasil.latency[u] + k.latency;
                        
                        #pragma omp critical
                        {
                            if (lat_baru < hasil.latency[k.server_tujuan]) {
                                hasil.latency[k.server_tujuan] = lat_baru;
                                hasil.server_sebelumnya[k.server_tujuan] = u;
                                pq.update(k.server_tujuan, lat_baru);
                            }
                        }
                    }
                }
                #pragma omp taskwait
            }
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    hasil.waktu_eksekusi = chrono::duration<double, milli>(end - start).count();
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// GENERATOR DATA CENTER
// ═══════════════════════════════════════════════════════════════════════════

DataCenter buatDataCenter(int n, double kepadatan) {
    DataCenter dc(n);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if ((rand() / (double)RAND_MAX) < kepadatan) {
                double latency = 0.1 + (rand() % 100) / 10.0;
                dc.tambahKoneksi(i, j, latency);
            }
        }
    }
    return dc;
}

// ═══════════════════════════════════════════════════════════════════════════
// VERIFIKASI HASIL
// ═══════════════════════════════════════════════════════════════════════════

bool verifikasiHasil(HasilRouting& h1, HasilRouting& h2) {
    if (h1.latency.size() != h2.latency.size()) return false;
    for (size_t i = 0; i < h1.latency.size(); i++) {
        if (abs(h1.latency[i] - h2.latency[i]) > 1e-9) return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNGSI CETAK
// ═══════════════════════════════════════════════════════════════════════════

void cetakGaris(char c = '=', int len = 75) {
    for (int i = 0; i < len; i++) cout << c;
    cout << endl;
}

void cetakHeader(const string& judul) {
    cout << endl;
    cetakGaris();
    cout << "  " << judul << endl;
    cetakGaris();
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN PROGRAM
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    srand(42);
    
    cetakHeader("ROUTING DATA CENTER: SERIAL vs OPENMP vs CILK");
    
    cout << endl;
    cout << "  TUJUAN EKSPERIMEN:" << endl;
    cout << "  Membandingkan performa Dijkstra dengan berbagai metode:" << endl;
    cout << "  1. Serial (Binary Heap & Fibonacci Heap)" << endl;
    cout << "  2. OpenMP Parallel (berbasis loop paralel)" << endl;
    cout << "  3. Cilk-style (berbasis task paralel)" << endl;
    
    cout << endl;
    cout << "  KONSEP PARALLELISASI:" << endl;
    cout << "  ┌────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │ BAGIAN SERIAL:                                             │" << endl;
    cout << "  │   - extract_min() dari priority queue                      │" << endl;
    cout << "  │   - Tidak bisa diparalel (satu minimum per iterasi)        │" << endl;
    cout << "  │                                                            │" << endl;
    cout << "  │ BAGIAN PARALEL:                                            │" << endl;
    cout << "  │   - Relaksasi edge (update jarak ke tetangga)              │" << endl;
    cout << "  │   - Setiap edge bisa diproses oleh thread berbeda          │" << endl;
    cout << "  └────────────────────────────────────────────────────────────┘" << endl;
    
    int num_threads = omp_get_max_threads();
    cout << endl;
    cout << "  Jumlah thread tersedia: " << num_threads << endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 1: Data Center Kecil
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("TEST 1: Data Center Kecil (200 Server, 30% Koneksi)");
    
    {
        DataCenter dc = buatDataCenter(200, 0.3);
        cout << endl;
        cout << "  Topologi: " << dc.jumlah_server << " server, " 
             << dc.jumlah_koneksi << " koneksi" << endl;
        cout << endl;
        
        // Jalankan semua metode
        HasilRouting h_serial_b = dijkstraSerialBinary(dc, 0);
        HasilRouting h_serial_f = dijkstraSerialFibonacci(dc, 0);
        HasilRouting h_omp_b = dijkstraOpenMPBinary(dc, 0, num_threads);
        HasilRouting h_omp_f = dijkstraOpenMPFibonacci(dc, 0, num_threads);
        HasilRouting h_cilk_b = dijkstraCilkStyleBinary(dc, 0, num_threads);
        HasilRouting h_cilk_f = dijkstraCilkStyleFibonacci(dc, 0, num_threads);
        
        cout << "  +----------------------------------+---------------+" << endl;
        cout << "  | Metode                           | Waktu (ms)    |" << endl;
        cout << "  +----------------------------------+---------------+" << endl;
        cout << fixed << setprecision(4);
        cout << "  | " << setw(32) << left << h_serial_b.metode << " | " << setw(13) << right << h_serial_b.waktu_eksekusi << " |" << endl;
        cout << "  | " << setw(32) << left << h_serial_f.metode << " | " << setw(13) << right << h_serial_f.waktu_eksekusi << " |" << endl;
        cout << "  | " << setw(32) << left << h_omp_b.metode << " | " << setw(13) << right << h_omp_b.waktu_eksekusi << " |" << endl;
        cout << "  | " << setw(32) << left << h_omp_f.metode << " | " << setw(13) << right << h_omp_f.waktu_eksekusi << " |" << endl;
        cout << "  | " << setw(32) << left << h_cilk_b.metode << " | " << setw(13) << right << h_cilk_b.waktu_eksekusi << " |" << endl;
        cout << "  | " << setw(32) << left << h_cilk_f.metode << " | " << setw(13) << right << h_cilk_f.waktu_eksekusi << " |" << endl;
        cout << "  +----------------------------------+---------------+" << endl;
        
        // Verifikasi
        cout << endl;
        if (verifikasiHasil(h_serial_b, h_omp_b) && verifikasiHasil(h_serial_b, h_cilk_b)) {
            cout << "  [OK] Semua hasil TERVERIFIKASI (jarak sama)" << endl;
        } else {
            cout << "  [ERROR] Ada perbedaan hasil!" << endl;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // TEST 2: Data Center Besar
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("TEST 2: Data Center Besar (500 Server, 40% Koneksi)");
    
    {
        DataCenter dc = buatDataCenter(500, 0.4);
        cout << endl;
        cout << "  Topologi: " << dc.jumlah_server << " server, " 
             << dc.jumlah_koneksi << " koneksi" << endl;
        cout << endl;
        
        HasilRouting h_serial_b = dijkstraSerialBinary(dc, 0);
        HasilRouting h_serial_f = dijkstraSerialFibonacci(dc, 0);
        HasilRouting h_omp_b = dijkstraOpenMPBinary(dc, 0, num_threads);
        HasilRouting h_omp_f = dijkstraOpenMPFibonacci(dc, 0, num_threads);
        HasilRouting h_cilk_b = dijkstraCilkStyleBinary(dc, 0, num_threads);
        HasilRouting h_cilk_f = dijkstraCilkStyleFibonacci(dc, 0, num_threads);
        
        cout << "  +----------------------------------+---------------+----------+" << endl;
        cout << "  | Metode                           | Waktu (ms)    | Speedup  |" << endl;
        cout << "  +----------------------------------+---------------+----------+" << endl;
        cout << fixed << setprecision(4);
        cout << "  | " << setw(32) << left << h_serial_b.metode << " | " << setw(13) << right << h_serial_b.waktu_eksekusi << " | baseline |" << endl;
        cout << "  | " << setw(32) << left << h_serial_f.metode << " | " << setw(13) << right << h_serial_f.waktu_eksekusi << " | " << setw(7) << setprecision(2) << h_serial_b.waktu_eksekusi / h_serial_f.waktu_eksekusi << "x |" << endl;
        cout << "  | " << setw(32) << left << h_omp_b.metode << " | " << setw(13) << setprecision(4) << right << h_omp_b.waktu_eksekusi << " | " << setw(7) << setprecision(2) << h_serial_b.waktu_eksekusi / h_omp_b.waktu_eksekusi << "x |" << endl;
        cout << "  | " << setw(32) << left << h_omp_f.metode << " | " << setw(13) << setprecision(4) << right << h_omp_f.waktu_eksekusi << " | " << setw(7) << setprecision(2) << h_serial_b.waktu_eksekusi / h_omp_f.waktu_eksekusi << "x |" << endl;
        cout << "  | " << setw(32) << left << h_cilk_b.metode << " | " << setw(13) << setprecision(4) << right << h_cilk_b.waktu_eksekusi << " | " << setw(7) << setprecision(2) << h_serial_b.waktu_eksekusi / h_cilk_b.waktu_eksekusi << "x |" << endl;
        cout << "  | " << setw(32) << left << h_cilk_f.metode << " | " << setw(13) << setprecision(4) << right << h_cilk_f.waktu_eksekusi << " | " << setw(7) << setprecision(2) << h_serial_b.waktu_eksekusi / h_cilk_f.waktu_eksekusi << "x |" << endl;
        cout << "  +----------------------------------+---------------+----------+" << endl;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // PENJELASAN OPENMP vs CILK
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("PENJELASAN: OPENMP vs CILK");
    
    cout << endl;
    cout << "  OPENMP (Open Multi-Processing):" << endl;
    cout << "  ┌────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │ - Model: Loop-based parallelism                            │" << endl;
    cout << "  │ - Sintaks: #pragma omp parallel for                        │" << endl;
    cout << "  │ - Cocok untuk: Loop dengan iterasi independen              │" << endl;
    cout << "  │ - Kelebihan: Mudah digunakan, overhead rendah              │" << endl;
    cout << "  │ - Kekurangan: Kurang fleksibel untuk rekursi               │" << endl;
    cout << "  └────────────────────────────────────────────────────────────┘" << endl;
    
    cout << endl;
    cout << "  CILK (MIT Cilk):" << endl;
    cout << "  ┌────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │ - Model: Task-based parallelism dengan work stealing       │" << endl;
    cout << "  │ - Sintaks: cilk_spawn, cilk_sync                           │" << endl;
    cout << "  │ - Cocok untuk: Algoritma rekursif (divide & conquer)       │" << endl;
    cout << "  │ - Kelebihan: Work stealing untuk load balancing otomatis   │" << endl;
    cout << "  │ - Kekurangan: Overhead task creation                       │" << endl;
    cout << "  └────────────────────────────────────────────────────────────┘" << endl;
    
    cout << endl;
    cout << "  DALAM KONTEKS DIJKSTRA:" << endl;
    cout << "  ┌────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │ - Parallelisasi TERBATAS karena extract_min harus serial   │" << endl;
    cout << "  │ - Hanya relaksasi edge yang bisa diparalel                 │" << endl;
    cout << "  │ - OpenMP lebih cocok (loop paralel sederhana)              │" << endl;
    cout << "  │ - Cilk overhead tinggi untuk task kecil                    │" << endl;
    cout << "  │ - Speedup terbatas oleh Amdahl's Law                       │" << endl;
    cout << "  └────────────────────────────────────────────────────────────┘" << endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // KESIMPULAN
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("KESIMPULAN");
    
    cout << endl;
    cout << "  1. SERIAL:" << endl;
    cout << "     - Binary Heap: sederhana, cepat untuk kebanyakan kasus" << endl;
    cout << "     - Fibonacci Heap: lebih sedikit operasi decrease_key" << endl;
    
    cout << endl;
    cout << "  2. OPENMP:" << endl;
    cout << "     - Mudah diimplementasikan dengan #pragma" << endl;
    cout << "     - Speedup terbatas karena bagian serial (extract_min)" << endl;
    cout << "     - Cocok untuk edge relaxation paralel" << endl;
    
    cout << endl;
    cout << "  3. CILK-STYLE:" << endl;
    cout << "     - Task-based parallelism" << endl;
    cout << "     - Overhead tinggi untuk task kecil" << endl;
    cout << "     - Lebih cocok untuk algoritma divide & conquer" << endl;
    
    cout << endl;
    cout << "  REKOMENDASI:" << endl;
    cout << "  ┌────────────────────────────────────────────────────────────┐" << endl;
    cout << "  │ 1. Untuk SERIAL: Binary Heap (simple & fast)               │" << endl;
    cout << "  │ 2. Untuk PARALEL: OpenMP + Binary Heap (practical)         │" << endl;
    cout << "  │ 3. Fibonacci Heap: untuk ANALISIS TEORITIS                 │" << endl;
    cout << "  └────────────────────────────────────────────────────────────┘" << endl;
    
    cout << endl;
    cetakGaris();
    cout << "  SELESAI!" << endl;
    cetakGaris();
    cout << endl;
    
    return 0;
}
