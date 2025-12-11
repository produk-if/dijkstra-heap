# Fibonacci Search dengan OpenMP & Cilk

Studi Kasus: Pencarian Data pada Array/Database Besar menggunakan deret Fibonacci

## File
```
├── README.md              # Dokumentasi ini
├── fibonacci_search.c     # Program C (OpenMP & Cilk)
└── penjelasan.html        # Penjelasan konsep & cara kerja
```

## Konsep Fibonacci
```
F(0) = 0
F(1) = 1
F(n) = F(n-1) + F(n-2)

Deret: 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89...
```

## Compile & Run
```bash
gcc -O2 -fopenmp -o fibonacci_search fibonacci_search.c -lm
./fibonacci_search
```

## Isi Program
1. **Deret Fibonacci Murni** - Generate dan tampilkan deret dengan rumus F(n) = F(n-1) + F(n-2)
2. **Fibonacci Search** - Algoritma pencarian dengan demo step-by-step
3. **OpenMP Parallel** - Batch search dengan `#pragma omp parallel for`
4. **Cilk-style Parallel** - Task-based dengan `#pragma omp task`
5. **Benchmark** - Perbandingan waktu Sequential vs OpenMP vs Cilk

## Output
- Penjelasan deret Fibonacci murni
- Demo langkah-langkah Fibonacci Search
- Tabel perbandingan performa (waktu, speedup, throughput)

