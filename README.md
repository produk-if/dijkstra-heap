# 🎓 Dijkstra's Algorithm: Fibonacci Heap vs Binary Heap

**Studi Perbandingan Teoritis & Praktis dengan Parallelisasi OpenMP & Cilk**

---

## 📋 Overview

Project ini mengimplementasikan dan membandingkan **Algoritma Dijkstra** dengan dua struktur data priority queue:

1. **Binary Heap** - Praktis dan cepat untuk graf kecil-menengah
2. **Fibonacci Heap** - Optimal secara teoritis dengan O(1) amortized decrease_key

Juga mengeksplorasi **parallelisasi** dengan:
- **OpenMP** - Loop-based parallelism
- **Cilk** - Task-based parallelism dengan work stealing

**Fokus:** Analisis trade-off antara kompleksitas asimptotik vs performa praktis.

---

## 🎯 Tujuan

✅ Implementasi Dijkstra dengan Binary Heap dan Fibonacci Heap  
✅ Perbandingan kompleksitas teoritis O(big-O) vs waktu eksekusi praktis  
✅ Studi kasus: Routing jaringan Data Center  
✅ Analisis jumlah operasi (operation counting)  
✅ Parallelisasi dengan OpenMP dan Cilk  
✅ Memahami Amdahl's Law dan batasan speedup  

---

## 📁 Struktur Project

```
dijkstra-heap/
├── README.md                           # Dokumentasi utama
├── README_STUDI_KASUS.md               # Dokumentasi studi kasus
│
├── main.cpp                            # Implementasi dasar Dijkstra
│                                       # Binary Heap vs Fibonacci Heap
│                                       # Graf: sparse, dense, grid
│
├── routing_datacenter.cpp              # Serial implementation + operation counting
│
└── routing_parallel.cpp                # OpenMP & Cilk parallelization
```

---

## 🚀 Cara Menjalankan

### 1. Implementasi Dasar (main.cpp)

```bash
g++ -O2 -o dijkstra main.cpp
./dijkstra
```

**Output:** Perbandingan Binary Heap vs Fibonacci Heap pada 3 jenis graf:
- Sparse Graph (E ≈ 3V)
- Dense Graph (E ≈ V²/2)
- Grid Graph (structured network)

### 2. Studi Kasus: Data Center Routing (Serial)

```bash
g++ -O2 -o routing routing_datacenter.cpp
./routing
```

**Output:** 
- Operation counting: Binary vs Fibonacci
- Menunjukkan Fibonacci melakukan 24x lebih sedikit decrease_key!
- Wall-clock time comparison

### 3. Studi Kasus: Parallelisasi (OpenMP & Cilk)

```bash
g++ -O2 -fopenmp -o routing_parallel routing_parallel.cpp
./routing_parallel
```

**Output:**
- Perbandingan Serial vs OpenMP vs Cilk
- Speedup analysis
- Amdahl's Law demonstration

---

## 📊 Hasil Utama

### 1. Fibonacci Heap: Teoritis vs Praktis

| Metrik | Binary Heap | Fibonacci Heap | Insight |
|--------|-------------|----------------|---------|
| **Kompleksitas Teoritis** | O((V+E) log V) | O(E + V log V) | Fibonacci menang |
| **Operation Count** | 6,248 decrease_key | 257 decrease_key | **24x lebih sedikit!** |
| **Wall-clock Time** | **0.276 ms** | 0.466 ms | Binary 1.7x lebih cepat |

**Kesimpulan:** Fibonacci Heap unggul secara **teoritis**, tapi Binary Heap lebih cepat dalam **praktik**.

### 2. Kenapa Binary Heap Lebih Cepat?

1. **Cache Locality** - Array-based (sequential memory access)
2. **Constant Factors** - Operasi lebih sederhana
3. **Memory Overhead** - 16 bytes/node vs 56 bytes/node
4. **Implementasi** - 100 lines vs 400 lines

### 3. Parallelisasi: Batasan Speedup

Test pada 500 servers dengan 4 threads:

| Metode | Waktu | Speedup |
|--------|-------|---------|
| Serial Binary | 0.276 ms | baseline |
| OpenMP Binary | 31.9 ms | **0.01x** (overhead dominan!) |
| Cilk Binary | 198.7 ms | **0.001x** |

**Amdahl's Law:** extract_min() serial (30-50%) membatasi speedup maksimal ≈ 2x.

---

## 💡 Key Insights

### 1. Big-O Bukan Segalanya

Fibonacci Heap memiliki kompleksitas asimptotik lebih baik (O(1) vs O(log n) untuk decrease_key), tapi:
- Constant factors lebih besar
- Overhead pointer
- Cache-unfriendly

### 2. Operation Count ≠ Execution Time

Meskipun Fibonacci melakukan 24x lebih sedikit operasi, execution time lebih lambat karena:
- Setiap operasi Fibonacci lebih kompleks
- Pointer chasing merusak cache
- Memory allocation overhead

### 3. Parallelisasi Tidak Selalu Menguntungkan

Untuk Dijkstra:
- Bagian serial (extract_min) tidak bisa diparalel
- Synchronization overhead tinggi (#pragma omp critical)
- Hanya efektif untuk graf sangat besar (>100K nodes)

### 4. Teori vs Praktik

Ini adalah **pembelajaran penting** dalam Computer Science:
- Analisis kompleksitas asimptotik penting untuk **understanding**
- Empirical testing penting untuk **real performance**
- Trade-off engineering: kompleksitas vs kesederhanaan

---

## 🎓 Nilai Akademis

Project ini mendemonstrasikan:

✅ **Analisis Algoritma** - Pemahaman mendalam kompleksitas asimptotik  
✅ **Amortized Analysis** - Fibonacci Heap O(1) decrease_key  
✅ **Struktur Data Lanjut** - Binary Heap, Fibonacci Heap  
✅ **Parallelisasi** - OpenMP, Cilk, Amdahl's Law  
✅ **Performance Engineering** - Cache locality, memory layout  
✅ **Critical Thinking** - Teori vs praktik, trade-off engineering  

---

## 📖 Referensi

1. **Cormen et al.** - "Introduction to Algorithms" (CLRS)
   - Chapter 19: Fibonacci Heaps
   - Chapter 24: Single-Source Shortest Paths

2. **Fredman & Tarjan (1987)** - "Fibonacci Heaps and Their Uses in Improved Network Optimization Algorithms"
   - Paper original Fibonacci Heap

3. **Amdahl's Law** - Gene Amdahl (1967)
   - Theoretical maximum speedup with parallel computing

---

## 🔧 Requirements

- **Compiler:** g++ dengan support C++11 atau lebih baru
- **OpenMP:** Untuk parallelisasi (optional)
- **OS:** Linux, macOS, atau Windows dengan WSL

---

## 📞 Pertanyaan untuk Diskusi

### Level Algoritma
- Bagaimana cara kerja Dijkstra step-by-step?
- Kenapa greedy choice di Dijkstra benar?
- Apa peran priority queue dalam Dijkstra?

### Level Implementasi
- Kenapa Binary Heap pakai array, Fibonacci pakai pointer?
- Apa itu cascading cuts dan kenapa perlu?
- Bagaimana amortized analysis bekerja?

### Level Performa
- Kenapa Fibonacci lebih lambat meski O() lebih baik?
- Kapan Fibonacci Heap lebih menguntungkan?
- Bagaimana cache locality mempengaruhi performa?

### Level Parallelisasi
- Apa yang membatasi speedup OpenMP?
- Kenapa Fibonacci Heap lebih sulit diparalel?
- Bagaimana Amdahl's Law berlaku di sini?

---

## ✨ Kesimpulan

Project ini membuktikan bahwa:

1. **Fibonacci Heap secara teoritis lebih efisien** dengan O(1) amortized decrease_key
2. **Binary Heap lebih praktis** untuk sebagian besar kasus nyata
3. **Parallelisasi memiliki batasan** sesuai Amdahl's Law
4. **Teori dan praktik berbeda** - keduanya penting untuk dipelajari!

**Rekomendasi:**
- Gunakan **Binary Heap** untuk production code
- Pelajari **Fibonacci Heap** untuk pemahaman teoritis
- **Profile sebelum optimize** - jangan asumsikan performa

---

**© 2025 - Studi Kasus Project S2**

