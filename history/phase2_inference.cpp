#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <random>
#include <chrono>

// SIMULATED engine for Bitwise Multiply-Accumulate optimized for unrolled AVX2
std::vector<float> bitwise_forward_pass(const std::vector<float>& x, 
                                        const std::vector<int8_t>& w_packed, 
                                        size_t rows, size_t cols) {
    std::vector<float> y(rows, 0.0f);
    
    // Process in chunks of 4 (1 byte at a time) to eliminate division and modulus overhead.
    size_t col_chunks = cols / 4; 
    
    for (size_t r = 0; r < rows; ++r) {
        float sum = 0.0f;
        size_t row_start_byte = (r * cols) / 4;
        
        for (size_t chunk = 0; chunk < col_chunks; ++chunk) {
            int8_t packed = w_packed[row_start_byte + chunk];
            size_t base_c = chunk * 4;
            
            // Extract bits rapidly without functions or modulo
            uint8_t b0 = packed & 0b11;
            uint8_t b1 = (packed >> 2) & 0b11;
            uint8_t b2 = (packed >> 4) & 0b11;
            uint8_t b3 = (packed >> 6) & 0b11;
            
            // Apply additions and subtractions avoiding multipliers
            if (b0 == 1) sum += x[base_c]; else if (b0 == 2) sum -= x[base_c];
            if (b1 == 1) sum += x[base_c + 1]; else if (b1 == 2) sum -= x[base_c + 1];
            if (b2 == 1) sum += x[base_c + 2]; else if (b2 == 2) sum -= x[base_c + 2];
            if (b3 == 1) sum += x[base_c + 3]; else if (b3 == 2) sum -= x[base_c + 3];
        }
        y[r] = sum;
    }
    return y;
}

// Traditional float multiplication reference
std::vector<float> float_forward_pass(const std::vector<float>& x,
                                      const std::vector<float>& w_float,
                                      size_t rows, size_t cols) {
    std::vector<float> y(rows, 0.0f);
    for (size_t r = 0; r < rows; ++r) {
        float sum = 0.0f;
        for (size_t c = 0; c < cols; ++c) {
            sum += w_float[r * cols + c] * x[c]; 
        }
        y[r] = sum;
    }
    return y;
}

void pack_weight(std::vector<int8_t>& w_packed, size_t flat_idx, int8_t w) {
    size_t byte_idx = flat_idx / 4;
    int bit_pos = flat_idx % 4;
    uint8_t bits = 0;
    if (w == 1) bits = 1;
    else if (w == -1) bits = 2;
    
    w_packed[byte_idx] &= ~(0b11 << (bit_pos * 2));
    w_packed[byte_idx] |= (bits << (bit_pos * 2));
}

int main() {
    size_t rows = 2048;
    size_t cols = 2048;
    size_t total_weights = rows * cols;
    int iterations = 100;
    
    std::cout << "--- Phase 6: Hardware Compiler Hookup & Timing ---" << std::endl;
    std::cout << "Matrix: " << rows << "x" << cols << " (" << iterations << " passes)" << std::endl;
    
    std::vector<float> x(cols);
    std::vector<float> w_float(total_weights);
    std::vector<int8_t> w_packed((total_weights + 3) / 4, 0);

    // Setup pseudo random data mimicking exactly equivalent arrays
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dis_x(0.0f, 1.0f);
    std::uniform_int_distribution<int> dis_w(-1, 1);
    
    for(size_t i=0; i<cols; ++i) x[i] = dis_x(gen);
    
    for(size_t i=0; i<total_weights; ++i) {
        int w = dis_w(gen);
        w_float[i] = static_cast<float>(w);
        pack_weight(w_packed, i, static_cast<int8_t>(w));
    }

    // Benchmark FP
    auto start_fp = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i) {
        volatile auto r = float_forward_pass(x, w_float, rows, cols);
    }
    auto stop_fp = std::chrono::high_resolution_clock::now();
    
    // Benchmark Bitwise
    auto start_bw = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i) {
        volatile auto r = bitwise_forward_pass(x, w_packed, rows, cols);
    }
    auto stop_bw = std::chrono::high_resolution_clock::now();
    
    auto duration_fp = std::chrono::duration_cast<std::chrono::milliseconds>(stop_fp - start_fp).count();
    auto duration_bw = std::chrono::duration_cast<std::chrono::milliseconds>(stop_bw - start_bw).count();
    
    std::cout << "Standard FP16 Duration: " << duration_fp << " ms" << std::endl;
    std::cout << "Bitwise Exec Duration:  " << duration_bw << " ms" << std::endl;
    
    return 0;
}
