# 🖥️ STUDI KASUS: ROUTING JARINGAN DATA CENTER

## 📋 Deskripsi Singkat

Studi kasus ini mensimulasikan **sistem routing pada jaringan Data Center** yang mencari jalur dengan **latency (waktu tunda) terendah** antar server.

---

## 🏢 Apa itu Data Center?

Data Center adalah fasilitas yang berisi **ribuan server komputer** yang saling terhubung. Contoh Data Center terkenal:
- Google Data Center
- Amazon Web Services (AWS)
- Microsoft Azure
- Data Center Tokopedia/Gojek

```
┌─────────────────────────────────────────────────────────────────┐
│                         DATA CENTER                              │
│                                                                  │
│   ┌─────┐    2ms    ┌─────┐    1ms    ┌─────┐                  │
│   │ S1  │───────────│ S2  │───────────│ S3  │                  │
│   └──┬──┘           └──┬──┘           └──┬──┘                  │
│      │ 3ms             │ 1ms             │ 2ms                  │
│      │                 │                 │                      │
│   ┌──┴──┐    1ms    ┌──┴──┐    3ms    ┌──┴──┐                  │
│   │ S4  │───────────│ S5  │───────────│ S6  │                  │
│   └──┬──┘           └──┬──┘           └──┬──┘                  │
│      │ 2ms             │ 1ms             │ 4ms                  │
│      │                 │                 │                      │
│   ┌──┴──┐    2ms    ┌──┴──┐    1ms    ┌──┴──┐                  │
│   │ S7  │───────────│ S8  │───────────│ S9  │                  │
│   └─────┘           └─────┘           └─────┘                  │
│                                                                  │
│   S = Server, angka = latency dalam milidetik (ms)              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Masalah yang Diselesaikan

### Pertanyaan Utama:
> **"Bagaimana cara mengirim data dari Server A ke Server B dengan waktu tercepat?"**

### Contoh:
- User di Jakarta mengakses website
- Request masuk ke Server 1
- Data yang dibutuhkan ada di Server 9
- **Jalur mana yang paling cepat?**

```
Opsi 1: S1 → S2 → S3 → S6 → S9 = 2 + 1 + 2 + 4 = 9 ms
Opsi 2: S1 → S4 → S5 → S8 → S9 = 3 + 1 + 1 + 1 = 6 ms  ✓ LEBIH CEPAT!
Opsi 3: S1 → S2 → S5 → S8 → S9 = 2 + 1 + 1 + 1 = 5 ms  ✓ PALING CEPAT!
```

---

## 🔧 Mengapa Perlu Algoritma Dijkstra?

### Tanpa Algoritma:
- Harus cek **SEMUA** kemungkinan jalur
- Dengan 100 server: jutaan kemungkinan!
- **TIDAK PRAKTIS**

### Dengan Dijkstra:
- Secara cerdas mencari jalur terpendek
- Tidak perlu cek semua jalur
- **EFISIEN**

---

## ⚡ Mengapa Fibonacci Heap Cocok?

### Karakteristik Jaringan Data Center:
1. **Graf PADAT** - Setiap server terhubung ke banyak server lain
2. **Banyak koneksi** - Ribuan kabel jaringan
3. **Sering update** - Latency berubah-ubah

### Operasi yang Sering Dilakukan:
| Operasi | Penjelasan | Binary Heap | Fibonacci Heap |
|---------|------------|-------------|----------------|
| Cari server terdekat | `extract_min` | O(log n) | O(log n) |
| Update latency | `decrease_key` | O(log n) | **O(1)** ✓ |

### Keunggulan Fibonacci:
```
Jika ada 10,000 koneksi dan 500 server:

Binary Heap:
  - 10,000 × O(log 500) = 10,000 × 9 = ~90,000 operasi

Fibonacci Heap:
  - 10,000 × O(1) = ~10,000 operasi
  
Fibonacci 9x LEBIH EFISIEN untuk operasi update!
```

---

## 📊 Hasil Eksperimen

Dari program yang dijalankan:

| Server | Kepadatan | Koneksi | Binary Heap | Fibonacci Heap |
|--------|-----------|---------|-------------|----------------|
| 500 | 10% | 24,782 | 1,130 ops | **335 ops** |
| 500 | 30% | 74,866 | 1,527 ops | **424 ops** |
| 500 | 50% | 124,254 | 1,645 ops | **459 ops** |
| 300 | 80% | 71,858 | 983 ops | **278 ops** |

**Kesimpulan: Fibonacci Heap melakukan 3-4x lebih sedikit operasi!**

---

## 🎓 Relevansi untuk Presentasi S2

### Poin-poin Penting:

1. **Masalah Nyata**
   - Routing di Data Center adalah masalah real
   - Digunakan oleh perusahaan teknologi besar

2. **Kompleksitas Teoritis**
   - Menunjukkan O(1) vs O(log n) dalam praktik
   - Membuktikan keunggulan Fibonacci Heap

3. **Trade-off**
   - Fibonacci: lebih sedikit operasi, tapi overhead lebih besar
   - Binary: lebih banyak operasi, tapi implementasi sederhana

---

## 📁 File dalam Folder Ini

```
studi_kasus_routing_datacenter/
├── README.md                    ← File ini (penjelasan)
├── routing_datacenter.cpp       ← Kode program lengkap (serial)
├── routing_parallel.cpp         ← Kode dengan OpenMP + Cilk (paralel)
└── cara_menjalankan.md          ← Instruksi kompilasi & run
```

---

## 🚀 Cara Menjalankan

### Program Serial (tanpa paralel):
```bash
cd studi_kasus_routing_datacenter
g++ -O2 -o routing routing_datacenter.cpp
./routing
```

### Program Paralel (dengan OpenMP + Cilk):
```bash
cd studi_kasus_routing_datacenter
g++ -O2 -fopenmp -o routing_parallel routing_parallel.cpp
./routing_parallel
```

---

## 💡 Kesimpulan

Studi kasus ini menunjukkan bahwa:

1. **Fibonacci Heap unggul** untuk graf padat dengan banyak operasi `decrease_key`
2. **Jumlah operasi lebih sedikit** (3-4x dibanding Binary Heap)
3. **Cocok untuk jaringan Data Center** karena banyak koneksi antar server

---

## 🔄 Hubungan dengan OpenMP dan Cilk

### 🎯 Konsep Parallelisasi pada Dijkstra

Dijkstra memiliki 2 bagian utama:

```
┌─────────────────────────────────────────────────────────────┐
│ BAGIAN SERIAL (tidak bisa diparalel):                       │
│   while (!pq.empty()) {                                     │
│       u = pq.extract_min();  ← HARUS satu per satu         │
│   }                                                         │
├─────────────────────────────────────────────────────────────┤
│ BAGIAN PARALEL (bisa diparalel):                            │
│   for (edge : tetangga[u]) {                                │
│       relax(edge);  ← Bisa dikerjakan bersamaan             │
│   }                                                         │
└─────────────────────────────────────────────────────────────┘
```

### 🔧 Implementasi dengan OpenMP

**OpenMP** = Loop-based parallelism (paralel berbasis loop)

```cpp
// Bagian yang diparalel dengan OpenMP
#pragma omp parallel for
for (int i = 0; i < neighbors.size(); i++) {
    Edge& e = neighbors[i];
    
    #pragma omp critical  // Protect shared data
    {
        if (dist[u] + e.weight < dist[e.to]) {
            dist[e.to] = dist[u] + e.weight;
            pq.decrease_key(e.to, dist[e.to]);  // Fibonacci O(1) vs Binary O(log n)
        }
    }
}
```

**Keunggulan:**
- Mudah digunakan dengan `#pragma`
- Overhead rendah untuk loop sederhana
- Cocok untuk edge relaxation

### ⚡ Implementasi dengan Cilk

**Cilk** = Task-based parallelism dengan work stealing

```cpp
// Cilk-style (simulasi dengan thread management)
cilk_for (int i = 0; i < neighbors.size(); i++) {
    // Setiap iterasi jadi task terpisah
    // Work stealing: thread idle bisa ambil task dari thread lain
}
```

**Keunggulan:**
- Work stealing untuk load balancing otomatis
- Bagus untuk algoritma divide & conquer (seperti merge sort)

**Kelemahan untuk Dijkstra:**
- Overhead task creation tinggi
- Kurang cocok untuk task kecil seperti edge relaxation

### 📊 Perbandingan Hasil

Dari hasil program `routing_parallel`:

| Metode | Waktu (500 server) | Catatan |
|--------|-------------------|---------|
| Serial Binary | 0.28 ms | Baseline |
| Serial Fibonacci | 0.47 ms | Lebih banyak overhead |
| OpenMP Binary | 31.94 ms | Overhead paralelisasi dominan |
| OpenMP Fibonacci | 41.74 ms | Overhead + Fibonacci complexity |
| Cilk Binary | 198.78 ms | Overhead task terlalu besar |
| Cilk Fibonacci | 117.57 ms | Fibonacci lebih efisien |

### 💡 Mengapa Paralel Lebih Lambat?

**Amdahl's Law:**
```
Speedup maksimal = 1 / (P + (1-P)/N)

P = Porsi serial (extract_min)
N = Jumlah thread
```

Pada Dijkstra:
- **P ≈ 0.3-0.5** (30-50% waktu untuk extract_min)
- Dengan 4 core: Speedup maksimal hanya **~2x**
- **Overhead** (thread management, synchronization) bisa lebih besar dari benefit!

### ✅ Kapan Fibonacci + OpenMP Unggul?

1. **Graf SANGAT BESAR** (>100,000 node)
2. **Graf SANGAT PADAT** (banyak edge per node)
3. **Banyak decrease_key** (Fibonacci O(1) advantage)
4. **Hardware dengan banyak core** (>8 core)

### 🎓 Kesimpulan untuk Presentasi S2

**Poin-poin Penting:**

1. **Fibonacci Heap** = Efisien secara teoritis (O(1) decrease_key)
2. **OpenMP** = Mudah untuk paralelisasi loop
3. **Cilk** = Bagus untuk divide & conquer, kurang cocok untuk Dijkstra
4. **Trade-off** = Overhead bisa lebih besar dari benefit untuk graf kecil-menengah
5. **Kombinasi terbaik** = Tergantung ukuran & karakteristik graf

---

*Studi kasus ini adalah bagian dari project S2 tentang perbandingan Fibonacci Heap vs Binary Heap pada Algoritma Dijkstra dengan Parallelisasi OpenMP dan Cilk.*
