/*
 * ═══════════════════════════════════════════════════════════════════════════
 * STUDI KASUS: ROUTING JARINGAN DATA CENTER
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * SKENARIO:
 * Sebuah Data Center memiliki ratusan server yang saling terhubung.
 * Kita perlu mencari jalur dengan LATENCY TERENDAH untuk mengirim data.
 * 
 * TUJUAN:
 * Membandingkan efisiensi Fibonacci Heap vs Binary Heap untuk masalah ini.
 * 
 * KOMPILASI: g++ -O2 -o routing routing_datacenter.cpp
 * JALANKAN:  ./routing
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

using namespace std;

const double INF = numeric_limits<double>::infinity();

// ═══════════════════════════════════════════════════════════════════════════
// STRUKTUR DATA JARINGAN
// ═══════════════════════════════════════════════════════════════════════════

// Koneksi antar server
struct Koneksi {
    int server_tujuan;      // ID server tujuan
    double latency;         // Waktu tunda (ms)
    
    Koneksi(int tujuan, double lat) : server_tujuan(tujuan), latency(lat) {}
};

// Jaringan Data Center
struct DataCenter {
    int jumlah_server;                      // Total server
    int jumlah_koneksi;                     // Total koneksi
    vector<vector<Koneksi>> topologi;       // Adjacency list
    vector<string> nama_server;             // Nama server (opsional)
    
    DataCenter(int n) : jumlah_server(n), jumlah_koneksi(0), topologi(n), nama_server(n) {
        // Beri nama default untuk setiap server
        for (int i = 0; i < n; i++) {
            nama_server[i] = "Server_" + to_string(i);
        }
    }
    
    // Tambah koneksi dua arah antar server
    void tambahKoneksi(int dari, int ke, double latency) {
        topologi[dari].push_back(Koneksi(ke, latency));
        topologi[ke].push_back(Koneksi(dari, latency));
        jumlah_koneksi += 2;
    }
};

// Hasil pencarian rute
struct HasilRouting {
    vector<double> latency;             // Latency ke setiap server
    vector<int> server_sebelumnya;      // Untuk rekonstruksi jalur
    long long operasi_update;           // Jumlah operasi decrease_key
    long long operasi_extract;          // Jumlah operasi extract_min
};

// ═══════════════════════════════════════════════════════════════════════════
// METODE 1: NAIVE (Tanpa Struktur Data Khusus)
// 
// Kompleksitas: O(V^2)
// Setiap iterasi harus scan SEMUA server untuk cari minimum
// SANGAT LAMBAT untuk Data Center besar!
// ═══════════════════════════════════════════════════════════════════════════

HasilRouting routingNaive(DataCenter& dc, int source) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.operasi_update = 0;
    hasil.operasi_extract = 0;
    
    vector<bool> sudah_diproses(n, false);
    hasil.latency[source] = 0;
    
    for (int i = 0; i < n; i++) {
        // MASALAH: Harus scan semua server! O(V) per iterasi
        int u = -1;
        double min_lat = INF;
        for (int v = 0; v < n; v++) {
            hasil.operasi_extract++;  // Hitung setiap perbandingan
            if (!sudah_diproses[v] && hasil.latency[v] < min_lat) {
                min_lat = hasil.latency[v];
                u = v;
            }
        }
        
        if (u == -1) break;
        sudah_diproses[u] = true;
        
        // Update latency ke tetangga
        for (const Koneksi& k : dc.topologi[u]) {
            double lat_baru = hasil.latency[u] + k.latency;
            if (lat_baru < hasil.latency[k.server_tujuan]) {
                hasil.latency[k.server_tujuan] = lat_baru;
                hasil.server_sebelumnya[k.server_tujuan] = u;
                hasil.operasi_update++;
            }
        }
    }
    
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 2: BINARY HEAP
// 
// Kompleksitas: O((V + E) log V)
// - extract_min: O(log V)
// - decrease_key: O(log V)  <-- Kurang efisien untuk graf padat!
// ═══════════════════════════════════════════════════════════════════════════

class BinaryHeapRouter {
private:
    struct Node {
        int server;
        double latency;
    };
    
    vector<Node> heap;
    vector<int> posisi;
    vector<bool> aktif;
    int ukuran;
    
    void naik(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
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
            int terkecil = i;
            int kiri = 2 * i + 1;
            int kanan = 2 * i + 2;
            
            if (kiri < ukuran && heap[kiri].latency < heap[terkecil].latency)
                terkecil = kiri;
            if (kanan < ukuran && heap[kanan].latency < heap[terkecil].latency)
                terkecil = kanan;
            
            if (terkecil != i) {
                swap(heap[i], heap[terkecil]);
                posisi[heap[i].server] = i;
                posisi[heap[terkecil].server] = terkecil;
                i = terkecil;
            } else break;
        }
    }

public:
    long long operasi_bubble = 0;  // Hitung operasi bubble (O(log n) per update)
    
    BinaryHeapRouter(int n) : heap(n), posisi(n, -1), aktif(n, false), ukuran(0) {}
    
    bool kosong() { return ukuran == 0; }
    
    void masukkan(int server, double latency) {
        if (aktif[server]) {
            kurangiLatency(server, latency);
            return;
        }
        heap[ukuran] = {server, latency};
        posisi[server] = ukuran;
        aktif[server] = true;
        naik(ukuran++);
    }
    
    int ambilTercepat() {
        int server = heap[0].server;
        aktif[server] = false;
        posisi[server] = -1;
        if (--ukuran > 0) {
            heap[0] = heap[ukuran];
            posisi[heap[0].server] = 0;
            turun(0);
        }
        return server;
    }
    
    // OPERASI KUNCI: O(log n) - harus bubble up!
    void kurangiLatency(int server, double lat_baru) {
        if (!aktif[server]) {
            masukkan(server, lat_baru);
            return;
        }
        int i = posisi[server];
        if (lat_baru < heap[i].latency) {
            heap[i].latency = lat_baru;
            
            // Hitung level yang di-bubble (untuk statistik)
            int temp = i;
            while (temp > 0) {
                operasi_bubble++;  // Setiap level = 1 operasi
                temp = (temp - 1) / 2;
            }
            
            naik(i);
        }
    }
};

HasilRouting routingBinaryHeap(DataCenter& dc, int source) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.operasi_extract = 0;
    
    hasil.latency[source] = 0;
    
    BinaryHeapRouter pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilTercepat();
        hasil.operasi_extract++;
        
        for (const Koneksi& k : dc.topologi[u]) {
            double lat_baru = hasil.latency[u] + k.latency;
            if (lat_baru < hasil.latency[k.server_tujuan]) {
                hasil.latency[k.server_tujuan] = lat_baru;
                hasil.server_sebelumnya[k.server_tujuan] = u;
                pq.kurangiLatency(k.server_tujuan, lat_baru);
            }
        }
    }
    
    hasil.operasi_update = pq.operasi_bubble;
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// METODE 3: FIBONACCI HEAP
// 
// Kompleksitas: O(E + V log V)
// - extract_min: O(log V) amortized
// - decrease_key: O(1) amortized  <-- KEUNGGULAN UTAMA!
// 
// Untuk graf padat dengan banyak decrease_key, ini JAUH lebih efisien!
// ═══════════════════════════════════════════════════════════════════════════

class FibonacciHeapRouter {
private:
    struct Node {
        int server;
        double latency;
        
        Node* parent;
        Node* child;
        Node* left;
        Node* right;
        
        int degree;
        bool marked;
        
        Node(int s, double l) : server(s), latency(l), parent(nullptr), child(nullptr),
            left(this), right(this), degree(0), marked(false) {}
    };
    
    Node* min_node;
    int jumlah;
    vector<Node*> nodes;
    
    void addToRoot(Node* n) {
        if (!min_node) {
            min_node = n;
            n->left = n->right = n;
        } else {
            n->left = min_node;
            n->right = min_node->right;
            min_node->right->left = n;
            min_node->right = n;
        }
        n->parent = nullptr;
    }
    
    void removeFromList(Node* n) {
        n->left->right = n->right;
        n->right->left = n->left;
    }
    
    void link(Node* y, Node* x) {
        removeFromList(y);
        y->parent = x;
        if (!x->child) {
            x->child = y;
            y->left = y->right = y;
        } else {
            y->left = x->child;
            y->right = x->child->right;
            x->child->right->left = y;
            x->child->right = y;
        }
        x->degree++;
        y->marked = false;
    }
    
    void consolidate() {
        if (!min_node) return;
        
        int maxD = (int)(log2(jumlah + 1)) + 2;
        vector<Node*> A(maxD, nullptr);
        
        vector<Node*> roots;
        Node* c = min_node;
        do {
            roots.push_back(c);
            c = c->right;
        } while (c != min_node);
        
        for (Node* w : roots) {
            Node* x = w;
            int d = x->degree;
            while (d < maxD && A[d]) {
                Node* y = A[d];
                if (x->latency > y->latency) swap(x, y);
                link(y, x);
                A[d++] = nullptr;
            }
            if (d < maxD) A[d] = x;
        }
        
        min_node = nullptr;
        for (int i = 0; i < maxD; i++) {
            if (A[i]) {
                if (!min_node) {
                    min_node = A[i];
                    A[i]->left = A[i]->right = A[i];
                } else {
                    A[i]->left = min_node;
                    A[i]->right = min_node->right;
                    min_node->right->left = A[i];
                    min_node->right = A[i];
                    if (A[i]->latency < min_node->latency) min_node = A[i];
                }
            }
        }
    }
    
    void cut(Node* x, Node* y) {
        if (y->child == x) {
            if (x->right == x) y->child = nullptr;
            else y->child = x->right;
        }
        removeFromList(x);
        y->degree--;
        addToRoot(x);
        x->marked = false;
    }
    
    void cascadingCut(Node* y) {
        Node* z = y->parent;
        if (z) {
            if (!y->marked) y->marked = true;
            else {
                cut(y, z);
                cascadingCut(z);
            }
        }
    }

public:
    long long operasi_cut = 0;  // Hitung operasi cut (tetap O(1) amortized!)
    
    FibonacciHeapRouter(int n) : min_node(nullptr), jumlah(0), nodes(n, nullptr) {}
    
    ~FibonacciHeapRouter() {
        for (Node* n : nodes) if (n) delete n;
    }
    
    bool kosong() { return !min_node; }
    
    void masukkan(int server, double latency) {
        if (nodes[server]) {
            kurangiLatency(server, latency);
            return;
        }
        Node* n = new Node(server, latency);
        nodes[server] = n;
        addToRoot(n);
        if (!min_node || latency < min_node->latency) min_node = n;
        jumlah++;
    }
    
    int ambilTercepat() {
        Node* z = min_node;
        int server = z->server;
        
        if (z->child) {
            vector<Node*> children;
            Node* c = z->child;
            do {
                children.push_back(c);
                c = c->right;
            } while (c != z->child);
            for (Node* x : children) addToRoot(x);
        }
        
        if (z == z->right) min_node = nullptr;
        else {
            min_node = z->right;
            removeFromList(z);
            consolidate();
        }
        
        nodes[server] = nullptr;
        delete z;
        jumlah--;
        return server;
    }
    
    // OPERASI KUNCI: O(1) amortized - TIDAK perlu bubble up!
    void kurangiLatency(int server, double lat_baru) {
        Node* x = nodes[server];
        if (!x) {
            masukkan(server, lat_baru);
            return;
        }
        
        if (lat_baru >= x->latency) return;
        
        x->latency = lat_baru;
        Node* y = x->parent;
        
        // Hanya cut jika melanggar heap property - O(1)!
        if (y && x->latency < y->latency) {
            cut(x, y);
            cascadingCut(y);
            operasi_cut++;  // Hitung, tapi tetap O(1) amortized
        }
        
        if (x->latency < min_node->latency) min_node = x;
    }
};

HasilRouting routingFibonacci(DataCenter& dc, int source) {
    int n = dc.jumlah_server;
    HasilRouting hasil;
    hasil.latency.assign(n, INF);
    hasil.server_sebelumnya.assign(n, -1);
    hasil.operasi_extract = 0;
    
    hasil.latency[source] = 0;
    
    FibonacciHeapRouter pq(n);
    pq.masukkan(source, 0);
    
    while (!pq.kosong()) {
        int u = pq.ambilTercepat();
        hasil.operasi_extract++;
        
        for (const Koneksi& k : dc.topologi[u]) {
            double lat_baru = hasil.latency[u] + k.latency;
            if (lat_baru < hasil.latency[k.server_tujuan]) {
                hasil.latency[k.server_tujuan] = lat_baru;
                hasil.server_sebelumnya[k.server_tujuan] = u;
                pq.kurangiLatency(k.server_tujuan, lat_baru);
            }
        }
    }
    
    hasil.operasi_update = pq.operasi_cut;
    return hasil;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNGSI PEMBUAT DATA CENTER
// ═══════════════════════════════════════════════════════════════════════════

DataCenter buatDataCenter(int jumlah_server, double kepadatan_koneksi) {
    DataCenter dc(jumlah_server);
    
    // Buat koneksi berdasarkan kepadatan
    // kepadatan 0.5 = 50% dari semua pasangan server terhubung
    for (int i = 0; i < jumlah_server; i++) {
        for (int j = i + 1; j < jumlah_server; j++) {
            if ((rand() / (double)RAND_MAX) < kepadatan_koneksi) {
                // Latency random antara 0.1 - 10 ms
                double latency = 0.1 + (rand() % 100) / 10.0;
                dc.tambahKoneksi(i, j, latency);
            }
        }
    }
    
    return dc;
}

// Rekonstruksi jalur dari source ke target
vector<int> rekonstruksiJalur(HasilRouting& hasil, int target) {
    vector<int> jalur;
    int current = target;
    
    while (current != -1) {
        jalur.push_back(current);
        current = hasil.server_sebelumnya[current];
    }
    
    // Balik urutan (dari source ke target)
    reverse(jalur.begin(), jalur.end());
    return jalur;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNGSI CETAK
// ═══════════════════════════════════════════════════════════════════════════

void cetakGaris(char c = '=', int len = 70) {
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
    srand(42);  // Seed untuk hasil reproducible
    
    cetakHeader("STUDI KASUS: ROUTING JARINGAN DATA CENTER");
    
    cout << endl;
    cout << "  SKENARIO:" << endl;
    cout << "  ---------" << endl;
    cout << "  Sebuah Data Center memiliki ratusan server yang saling" << endl;
    cout << "  terhubung. Kita perlu mencari jalur dengan LATENCY TERENDAH" << endl;
    cout << "  untuk mengirim data dari satu server ke server lain." << endl;
    
    cout << endl;
    cout << "  METODE YANG DIBANDINGKAN:" << endl;
    cout << "  1. Naive      - Scan semua server, O(V^2)" << endl;
    cout << "  2. Binary Heap - decrease_key O(log V)" << endl;
    cout << "  3. Fibonacci  - decrease_key O(1) amortized" << endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DEMO 1: Data Center Kecil (untuk visualisasi)
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("DEMO 1: Data Center Kecil (10 Server)");
    
    {
        DataCenter dc = buatDataCenter(10, 0.5);
        int source = 0;
        int target = 9;
        
        cout << endl;
        cout << "  Topologi:" << endl;
        cout << "    Server: " << dc.jumlah_server << endl;
        cout << "    Koneksi: " << dc.jumlah_koneksi << endl;
        cout << "    Source: Server " << source << endl;
        cout << "    Target: Server " << target << endl;
        
        HasilRouting hasil = routingFibonacci(dc, source);
        
        cout << endl;
        cout << "  HASIL:" << endl;
        cout << "    Latency optimal: " << fixed << setprecision(2) 
             << hasil.latency[target] << " ms" << endl;
        
        vector<int> jalur = rekonstruksiJalur(hasil, target);
        cout << "    Jalur: ";
        for (size_t i = 0; i < jalur.size(); i++) {
            cout << "S" << jalur[i];
            if (i < jalur.size() - 1) cout << " -> ";
        }
        cout << endl;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // DEMO 2: Perbandingan Performa
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("DEMO 2: Perbandingan Performa");
    
    cout << endl;
    cout << "  Menguji dengan berbagai ukuran dan kepadatan..." << endl;
    cout << endl;
    
    cout << "  +--------+---------+----------+------------+------------+-----------+" << endl;
    cout << "  | Server | Density | Koneksi  | Naive Ops  | Binary Ops | Fib Ops   |" << endl;
    cout << "  +--------+---------+----------+------------+------------+-----------+" << endl;
    
    vector<pair<int, double>> tests = {
        {100, 0.2},
        {200, 0.3},
        {300, 0.4},
        {400, 0.5},
    };
    
    for (auto& test : tests) {
        int n = test.first;
        double density = test.second;
        
        DataCenter dc = buatDataCenter(n, density);
        
        HasilRouting h1 = routingNaive(dc, 0);
        HasilRouting h2 = routingBinaryHeap(dc, 0);
        HasilRouting h3 = routingFibonacci(dc, 0);
        
        cout << "  | " << setw(6) << n
             << " | " << setw(6) << fixed << setprecision(0) << (density * 100) << "%"
             << " | " << setw(8) << dc.jumlah_koneksi
             << " | " << setw(10) << h1.operasi_extract
             << " | " << setw(10) << h2.operasi_update
             << " | " << setw(9) << h3.operasi_update << " |" << endl;
    }
    
    cout << "  +--------+---------+----------+------------+------------+-----------+" << endl;
    
    // ═══════════════════════════════════════════════════════════════════════
    // DEMO 3: Analisis Detail
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("DEMO 3: Analisis Detail (300 Server, 50% Koneksi)");
    
    {
        DataCenter dc = buatDataCenter(300, 0.5);
        
        cout << endl;
        cout << "  TOPOLOGI:" << endl;
        cout << "    Server: " << dc.jumlah_server << endl;
        cout << "    Koneksi: " << dc.jumlah_koneksi << endl;
        cout << "    Rata-rata koneksi per server: " 
             << dc.jumlah_koneksi / dc.jumlah_server << endl;
        
        // Jalankan ketiga metode
        auto t1 = chrono::high_resolution_clock::now();
        HasilRouting h1 = routingNaive(dc, 0);
        auto t2 = chrono::high_resolution_clock::now();
        double waktu_naive = chrono::duration<double, milli>(t2 - t1).count();
        
        t1 = chrono::high_resolution_clock::now();
        HasilRouting h2 = routingBinaryHeap(dc, 0);
        t2 = chrono::high_resolution_clock::now();
        double waktu_binary = chrono::duration<double, milli>(t2 - t1).count();
        
        t1 = chrono::high_resolution_clock::now();
        HasilRouting h3 = routingFibonacci(dc, 0);
        t2 = chrono::high_resolution_clock::now();
        double waktu_fib = chrono::duration<double, milli>(t2 - t1).count();
        
        cout << endl;
        cout << "  HASIL PERBANDINGAN:" << endl;
        cout << "  +------------------+---------------+------------------+" << endl;
        cout << "  | Metode           | Waktu (ms)    | Operasi Update   |" << endl;
        cout << "  +------------------+---------------+------------------+" << endl;
        cout << "  | Naive (Array)    | " << setw(13) << fixed << setprecision(4) << waktu_naive
             << " | " << setw(16) << h1.operasi_update << " |" << endl;
        cout << "  | Binary Heap      | " << setw(13) << waktu_binary
             << " | " << setw(16) << h2.operasi_update << " |" << endl;
        cout << "  | Fibonacci Heap   | " << setw(13) << waktu_fib
             << " | " << setw(16) << h3.operasi_update << " |" << endl;
        cout << "  +------------------+---------------+------------------+" << endl;
        
        cout << endl;
        cout << "  ANALISIS:" << endl;
        cout << "    Fibonacci melakukan " << h2.operasi_update / max(1LL, h3.operasi_update) 
             << "x lebih SEDIKIT operasi update!" << endl;
        cout << endl;
        cout << "    Kompleksitas per operasi decrease_key:" << endl;
        cout << "      - Binary Heap:    O(log " << dc.jumlah_server << ") = O(" 
             << (int)log2(dc.jumlah_server) << ")" << endl;
        cout << "      - Fibonacci Heap: O(1) amortized" << endl;
    }
    
    // ═══════════════════════════════════════════════════════════════════════
    // KESIMPULAN
    // ═══════════════════════════════════════════════════════════════════════
    
    cetakHeader("KESIMPULAN");
    
    cout << endl;
    cout << "  1. NAIVE METHOD:" << endl;
    cout << "     - Kompleksitas: O(V^2)" << endl;
    cout << "     - TIDAK cocok untuk Data Center besar" << endl;
    cout << "     - Hanya untuk pembelajaran" << endl;
    
    cout << endl;
    cout << "  2. BINARY HEAP:" << endl;
    cout << "     - Kompleksitas: O((V + E) log V)" << endl;
    cout << "     - Bagus untuk kebanyakan kasus" << endl;
    cout << "     - Implementasi sederhana" << endl;
    
    cout << endl;
    cout << "  3. FIBONACCI HEAP:" << endl;
    cout << "     - Kompleksitas: O(E + V log V)" << endl;
    cout << "     - decrease_key O(1) vs O(log V)" << endl;
    cout << "     - TERBAIK untuk graf padat dengan banyak update" << endl;
    
    cout << endl;
    cout << "  KEUNGGULAN FIBONACCI HEAP:" << endl;
    cout << "  +---------------------------------------------------------+" << endl;
    cout << "  | Untuk jaringan Data Center dengan banyak koneksi,       |" << endl;
    cout << "  | Fibonacci Heap melakukan LEBIH SEDIKIT operasi karena   |" << endl;
    cout << "  | decrease_key hanya O(1) amortized, bukan O(log V).      |" << endl;
    cout << "  +---------------------------------------------------------+" << endl;
    
    cout << endl;
    cetakGaris();
    cout << "  SELESAI!" << endl;
    cetakGaris();
    cout << endl;
    
    return 0;
}
