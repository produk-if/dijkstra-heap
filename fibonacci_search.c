/*
================================================================================
    FIBONACCI SEARCH DENGAN OPENMP & CILK
    Studi Kasus: Pencarian Data pada Array/Database Besar
================================================================================

Compile : gcc -O2 -fopenmp -o fibonacci_search fibonacci_search.c -lm
Run     : ./fibonacci_search

================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define MAX_FIB 50
#define CILK_THRESHOLD 1000

// =============================================================================
//  BAGIAN 1: DERET FIBONACCI MURNI
// =============================================================================
/*
    RUMUS FIBONACCI:
    
    F(0) = 0
    F(1) = 1
    F(n) = F(n-1) + F(n-2)   untuk n >= 2
    
    Deret: 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233...
    
    Sifat penting untuk Fibonacci Search:
    - F(k) = F(k-1) + F(k-2)
    - Digunakan untuk membagi array tanpa operasi division
*/

// Global array untuk menyimpan deret Fibonacci
long long fib[MAX_FIB];
int fib_count = 0;

/*
    GENERATE FIBONACCI - SEQUENTIAL
    
    Cara kerja:
    fib[0] = 0
    fib[1] = 1
    fib[2] = fib[1] + fib[0] = 1 + 0 = 1
    fib[3] = fib[2] + fib[1] = 1 + 1 = 2
    fib[4] = fib[3] + fib[2] = 2 + 1 = 3
    fib[5] = fib[4] + fib[3] = 3 + 2 = 5
    ...
*/
void generate_fibonacci_sequential(int n) {
    fib[0] = 0;
    fib[1] = 1;
    
    for (int i = 2; i < n && i < MAX_FIB; i++) {
        fib[i] = fib[i-1] + fib[i-2];  // RUMUS FIBONACCI!
    }
    fib_count = n < MAX_FIB ? n : MAX_FIB;
}

/*
    GENERATE FIBONACCI - OPENMP
    
    Catatan: Karena ada dependency (fib[i] butuh fib[i-1] dan fib[i-2]),
    kita tidak bisa langsung paralelisasi loop utama.
    
    Tapi kita bisa menghitung Fibonacci secara paralel untuk NILAI BERBEDA
    menggunakan formula matrix atau formula langsung.
*/
void generate_fibonacci_openmp(int n) {
    fib[0] = 0;
    fib[1] = 1;
    
    // Sequential karena dependency
    for (int i = 2; i < n && i < MAX_FIB; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    fib_count = n < MAX_FIB ? n : MAX_FIB;
}

/*
    FIBONACCI REKURSIF - CILK STYLE
    
    Ini adalah contoh KLASIK parallelisasi dengan Cilk:
    
    fib(n) = fib(n-1) + fib(n-2)
           = [TASK 1]  + [TASK 2]
           
    Kedua task bisa dijalankan PARALEL karena independen!
*/
long long fibonacci_cilk(int n, int depth) {
    if (n <= 1) return n;
    
    // Jika sudah cukup dalam, jalankan sequential
    if (depth > 20 || n < 20) {
        return fibonacci_cilk(n-1, depth+1) + fibonacci_cilk(n-2, depth+1);
    }
    
    long long x, y;
    
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            x = fibonacci_cilk(n-1, depth+1);  // TASK 1
        }
        #pragma omp section
        {
            y = fibonacci_cilk(n-2, depth+1);  // TASK 2
        }
    }
    
    return x + y;  // Gabungkan hasil
}

/*
    FIBONACCI REKURSIF - CILK STYLE DENGAN TASK
*/
long long fibonacci_cilk_task(int n) {
    if (n <= 1) return n;
    
    if (n < 20) {
        // Sequential untuk n kecil (menghindari overhead)
        return fibonacci_cilk_task(n-1) + fibonacci_cilk_task(n-2);
    }
    
    long long x, y;
    
    #pragma omp task shared(x)
    x = fibonacci_cilk_task(n-1);
    
    #pragma omp task shared(y)
    y = fibonacci_cilk_task(n-2);
    
    #pragma omp taskwait
    
    return x + y;
}

// =============================================================================
//  BAGIAN 2: FIBONACCI SEARCH - ALGORITMA
// =============================================================================
/*
    FIBONACCI SEARCH
    
    Algoritma pencarian pada SORTED ARRAY menggunakan deret Fibonacci
    untuk menentukan posisi pembagian (bukan bagi 2 seperti Binary Search).
    
    KEUNGGULAN:
    - Hanya pakai operasi + dan - (tidak perlu division)
    - Lebih baik untuk storage dengan access time berbeda (tape, disk)
    
    CARA KERJA:
    1. Cari F(k) >= n (panjang array)
    2. offset = -1
    3. Selama F(k) > 1:
       - i = min(offset + F(k-2), n-1)
       - Jika arr[i] < target: potong bagian kiri
       - Jika arr[i] > target: potong bagian kanan
       - Jika arr[i] == target: KETEMU!
    4. Cek elemen terakhir
*/

int fibonacci_search(int arr[], int n, int target) {
    // Cari F(k) terkecil >= n
    int k = 0;
    while (fib[k] < n) k++;
    
    // Offset untuk elemen yang sudah dieliminasi
    int offset = -1;
    
    // Selama masih ada elemen untuk dicek
    while (k > 1) {
        // Pastikan index valid
        int i = offset + fib[k-2];
        if (i >= n) i = n - 1;
        
        if (arr[i] < target) {
            // Target di bagian KANAN
            // Potong F(k-2) elemen dari kiri
            k = k - 1;
            offset = i;
        }
        else if (arr[i] > target) {
            // Target di bagian KIRI
            // Potong F(k-1) elemen dari kanan
            k = k - 2;
        }
        else {
            // KETEMU!
            return i;
        }
    }
    
    // Cek elemen terakhir
    if (k == 1 && offset + 1 < n && arr[offset + 1] == target) {
        return offset + 1;
    }
    
    return -1;  // Tidak ditemukan
}

// =============================================================================
//  BAGIAN 3: BATCH SEARCH - SEQUENTIAL
// =============================================================================
/*
    BATCH SEARCH SEQUENTIAL
    
    Mencari banyak target dalam satu array.
    Setiap pencarian dilakukan satu per satu.
*/
void batch_search_sequential(int arr[], int n, int targets[], int num_targets, int results[]) {
    for (int i = 0; i < num_targets; i++) {
        results[i] = fibonacci_search(arr, n, targets[i]);
    }
}

// =============================================================================
//  BAGIAN 4: BATCH SEARCH - OPENMP
// =============================================================================
/*
    BATCH SEARCH DENGAN OPENMP
    
    Setiap pencarian INDEPENDEN, jadi bisa diparalelkan!
    
    #pragma omp parallel for:
    - Membagi loop ke beberapa thread
    - Setiap thread cari target berbeda
    - Tidak perlu sinkronisasi (hasil disimpan di index berbeda)
*/
void batch_search_openmp(int arr[], int n, int targets[], int num_targets, int results[]) {
    #pragma omp parallel for
    for (int i = 0; i < num_targets; i++) {
        results[i] = fibonacci_search(arr, n, targets[i]);
    }
}

// =============================================================================
//  BAGIAN 5: BATCH SEARCH - CILK STYLE
// =============================================================================
/*
    BATCH SEARCH DENGAN CILK STYLE (RECURSIVE TASK)
    
    Cilk menggunakan pendekatan divide-and-conquer:
    1. Bagi batch menjadi 2 bagian
    2. Spawn task untuk bagian pertama
    3. Spawn task untuk bagian kedua
    4. Tunggu semua selesai (sync)
    
    Ini disimulasikan dengan OpenMP tasks.
*/
void batch_search_cilk_recursive(int arr[], int n, int targets[], int start, int end, int results[]) {
    int count = end - start;
    
    // Base case: sequential jika batch kecil
    if (count <= CILK_THRESHOLD) {
        for (int i = start; i < end; i++) {
            results[i] = fibonacci_search(arr, n, targets[i]);
        }
        return;
    }
    
    // Divide: bagi menjadi 2 bagian
    int mid = start + count / 2;
    
    // Conquer: spawn 2 task paralel
    #pragma omp task
    batch_search_cilk_recursive(arr, n, targets, start, mid, results);
    
    #pragma omp task
    batch_search_cilk_recursive(arr, n, targets, mid, end, results);
    
    // Sync: tunggu semua task selesai
    #pragma omp taskwait
}

void batch_search_cilk(int arr[], int n, int targets[], int num_targets, int results[]) {
    #pragma omp parallel
    {
        #pragma omp single
        {
            batch_search_cilk_recursive(arr, n, targets, 0, num_targets, results);
        }
    }
}

// =============================================================================
//  BAGIAN 6: UTILITAS
// =============================================================================

// Generate sorted array
void generate_sorted_array(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = i * 2;  // 0, 2, 4, 6, 8, ...
    }
}

// Generate random targets
void generate_random_targets(int targets[], int num_targets, int max_val) {
    for (int i = 0; i < num_targets; i++) {
        targets[i] = rand() % max_val;
    }
}

// Verifikasi hasil
int verify_results(int results1[], int results2[], int n) {
    for (int i = 0; i < n; i++) {
        if (results1[i] != results2[i]) return 0;
    }
    return 1;
}

// =============================================================================
//  BAGIAN 7: DEMO DAN BENCHMARK
// =============================================================================

void print_separator() {
    printf("════════════════════════════════════════════════════════════════════════════════\n");
}

void print_header(const char* title) {
    printf("\n");
    print_separator();
    printf("    %s\n", title);
    print_separator();
}

void demo_fibonacci_murni() {
    print_header("BAGIAN 1: DERET FIBONACCI MURNI");
    
    printf("\n  RUMUS: F(n) = F(n-1) + F(n-2)\n");
    printf("  F(0) = 0, F(1) = 1\n\n");
    
    generate_fibonacci_sequential(15);
    
    printf("  DERET FIBONACCI:\n  ");
    for (int i = 0; i < 15; i++) {
        printf("F(%d)=%lld", i, fib[i]);
        if (i < 14) printf(", ");
        if (i == 7) printf("\n  ");
    }
    printf("\n");
    
    printf("\n  CARA HITUNG:\n");
    printf("  F(2) = F(1) + F(0) = 1 + 0 = 1\n");
    printf("  F(3) = F(2) + F(1) = 1 + 1 = 2\n");
    printf("  F(4) = F(3) + F(2) = 2 + 1 = 3\n");
    printf("  F(5) = F(4) + F(3) = 3 + 2 = 5\n");
    printf("  F(6) = F(5) + F(4) = 5 + 3 = 8\n");
    printf("  ...\n");
}

void demo_fibonacci_search() {
    print_header("BAGIAN 2: FIBONACCI SEARCH - CARA KERJA");
    
    // Array contoh
    int arr[] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
    int n = 11;
    int target = 14;
    
    printf("\n  ARRAY (sorted): ");
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n  Panjang array: %d\n", n);
    printf("  Target: %d\n\n", target);
    
    // Generate Fibonacci sampai >= n
    generate_fibonacci_sequential(MAX_FIB);
    
    printf("  LANGKAH-LANGKAH PENCARIAN:\n\n");
    
    // Cari F(k) >= n
    int k = 0;
    while (fib[k] < n) k++;
    printf("  1. Cari F(k) >= %d\n", n);
    printf("     F(%d) = %lld >= %d ✓\n\n", k, fib[k], n);
    
    // Simulasi pencarian dengan trace
    int offset = -1;
    int step = 2;
    
    while (k > 1) {
        int i = offset + fib[k-2];
        if (i >= n) i = n - 1;
        
        printf("  %d. k=%d, F(k-2)=F(%d)=%lld, index=%d, arr[%d]=%d\n", 
               step, k, k-2, fib[k-2], i, i, arr[i]);
        
        if (arr[i] < target) {
            printf("     %d < %d → geser KANAN, offset=%d\n\n", arr[i], target, i);
            k = k - 1;
            offset = i;
        }
        else if (arr[i] > target) {
            printf("     %d > %d → geser KIRI\n\n", arr[i], target);
            k = k - 2;
        }
        else {
            printf("     %d == %d → KETEMU di index %d! ✓\n\n", arr[i], target, i);
            break;
        }
        step++;
    }
    
    // Hasil
    int result = fibonacci_search(arr, n, target);
    printf("  HASIL: Target %d ditemukan di index %d\n", target, result);
}

void demo_parallel_comparison() {
    print_header("BAGIAN 3: PERBANDINGAN SEQUENTIAL vs OPENMP vs CILK");
    
    int array_size = 1000000;    // 1 juta elemen
    int num_searches = 100000;   // 100 ribu pencarian
    
    printf("\n  SKENARIO:\n");
    printf("  - Array size    : %d elemen (sorted)\n", array_size);
    printf("  - Jumlah search : %d pencarian\n", num_searches);
    printf("  - OpenMP threads: %d\n\n", omp_get_max_threads());
    
    // Alokasi memory
    int* arr = (int*)malloc(array_size * sizeof(int));
    int* targets = (int*)malloc(num_searches * sizeof(int));
    int* results_seq = (int*)malloc(num_searches * sizeof(int));
    int* results_omp = (int*)malloc(num_searches * sizeof(int));
    int* results_cilk = (int*)malloc(num_searches * sizeof(int));
    
    // Generate data
    generate_sorted_array(arr, array_size);
    srand(42);
    generate_random_targets(targets, num_searches, array_size * 2);
    
    // Generate Fibonacci
    generate_fibonacci_sequential(MAX_FIB);
    
    // Benchmark SEQUENTIAL
    double start = omp_get_wtime();
    batch_search_sequential(arr, array_size, targets, num_searches, results_seq);
    double time_seq = omp_get_wtime() - start;
    
    // Benchmark OPENMP
    start = omp_get_wtime();
    batch_search_openmp(arr, array_size, targets, num_searches, results_omp);
    double time_omp = omp_get_wtime() - start;
    
    // Benchmark CILK
    start = omp_get_wtime();
    batch_search_cilk(arr, array_size, targets, num_searches, results_cilk);
    double time_cilk = omp_get_wtime() - start;
    
    // Verifikasi
    int valid_omp = verify_results(results_seq, results_omp, num_searches);
    int valid_cilk = verify_results(results_seq, results_cilk, num_searches);
    
    // Print hasil
    printf("  ┌──────────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("  │ Metode           │ Waktu        │ Speedup      │ Status       │\n");
    printf("  ├──────────────────┼──────────────┼──────────────┼──────────────┤\n");
    printf("  │ Sequential       │ %9.3f ms │     1.00x    │      ✓       │\n", time_seq * 1000);
    printf("  │ OpenMP           │ %9.3f ms │ %9.2fx   │      %s       │\n", 
           time_omp * 1000, time_seq / time_omp, valid_omp ? "✓" : "✗");
    printf("  │ Cilk-style       │ %9.3f ms │ %9.2fx   │      %s       │\n", 
           time_cilk * 1000, time_seq / time_cilk, valid_cilk ? "✓" : "✗");
    printf("  └──────────────────┴──────────────┴──────────────┴──────────────┘\n");
    
    // Hitung throughput
    printf("\n  THROUGHPUT (pencarian per detik):\n");
    printf("  - Sequential : %.0f pencarian/detik\n", num_searches / time_seq);
    printf("  - OpenMP     : %.0f pencarian/detik\n", num_searches / time_omp);
    printf("  - Cilk-style : %.0f pencarian/detik\n", num_searches / time_cilk);
    
    // Cleanup
    free(arr);
    free(targets);
    free(results_seq);
    free(results_omp);
    free(results_cilk);
}

void demo_cilk_fibonacci() {
    print_header("BAGIAN 4: CILK-STYLE FIBONACCI REKURSIF");
    
    printf("\n  KONSEP CILK:\n");
    printf("  F(n) = F(n-1) + F(n-2)\n");
    printf("       = [TASK 1] + [TASK 2]\n");
    printf("  Kedua task bisa dijalankan PARALEL!\n\n");
    
    printf("  VISUALISASI F(5):\n");
    printf("                    F(5)\n");
    printf("                   /    \\\n");
    printf("               F(4)  +   F(3)      <- 2 task paralel\n");
    printf("               / \\        / \\\n");
    printf("           F(3)+F(2)  F(2)+F(1)   <- 4 task paralel\n");
    printf("           ...\n\n");
    
    int n = 35;
    printf("  Menghitung F(%d)...\n\n", n);
    
    // Sequential
    generate_fibonacci_sequential(n + 1);
    double start = omp_get_wtime();
    long long result_seq = fib[n];
    double time_gen = omp_get_wtime() - start;
    
    // Cilk parallel
    start = omp_get_wtime();
    long long result_cilk;
    #pragma omp parallel
    {
        #pragma omp single
        {
            result_cilk = fibonacci_cilk_task(n);
        }
    }
    double time_cilk = omp_get_wtime() - start;
    
    printf("  ┌──────────────────┬──────────────┬──────────────┐\n");
    printf("  │ Metode           │ Hasil        │ Waktu        │\n");
    printf("  ├──────────────────┼──────────────┼──────────────┤\n");
    printf("  │ Iteratif (DP)    │ %12lld │ %9.3f ms │\n", result_seq, time_gen * 1000);
    printf("  │ Cilk Rekursif    │ %12lld │ %9.3f ms │\n", result_cilk, time_cilk * 1000);
    printf("  └──────────────────┴──────────────┴──────────────┘\n");
    
    printf("\n  CATATAN:\n");
    printf("  - Iteratif (DP) JAUH lebih cepat karena O(n)\n");
    printf("  - Rekursif adalah O(2^n) tapi bisa diparalelkan\n");
    printf("  - Cilk berguna untuk mendemonstrasikan task parallelism\n");
}

void print_kesimpulan() {
    print_header("KESIMPULAN");
    
    printf("\n  1. DERET FIBONACCI:\n");
    printf("     Rumus: F(n) = F(n-1) + F(n-2)\n");
    printf("     Digunakan untuk menentukan posisi pembagian array\n");
    
    printf("\n  2. FIBONACCI SEARCH:\n");
    printf("     - Alternatif Binary Search\n");
    printf("     - Hanya pakai + dan - (tanpa division)\n");
    printf("     - Kompleksitas: O(log n)\n");
    
    printf("\n  3. PARALLELISASI:\n");
    printf("     - OpenMP: #pragma omp parallel for (untuk batch search)\n");
    printf("     - Cilk: Task-based dengan spawn/sync (untuk rekursif)\n");
    
    printf("\n  4. KAPAN PARALEL BERGUNA?\n");
    printf("     ✓ Data BESAR (jutaan elemen)\n");
    printf("     ✓ Query BANYAK (ribuan pencarian)\n");
    printf("     ✗ Data kecil: overhead > benefit\n");
    
    printf("\n");
    print_separator();
    printf("\n");
}

// =============================================================================
//  MAIN
// =============================================================================

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║         FIBONACCI SEARCH DENGAN OPENMP & CILK                                ║\n");
    printf("║         Studi Kasus: Pencarian Data pada Array Besar                         ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    
    // Demo 1: Deret Fibonacci Murni
    demo_fibonacci_murni();
    
    // Demo 2: Cara Kerja Fibonacci Search
    demo_fibonacci_search();
    
    // Demo 3: Perbandingan Parallel
    demo_parallel_comparison();
    
    // Demo 4: Cilk Fibonacci Rekursif
    demo_cilk_fibonacci();
    
    // Kesimpulan
    print_kesimpulan();
    
    return 0;
}
