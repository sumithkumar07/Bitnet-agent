#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <random>
#include <chrono>
#include <immintrin.h>

// Lookup tables fitting neatly into the L1 CPU Cache (4KB each)
__m128 mask_add_tbl[256];
__m128 mask_sub_tbl[256];

void init_simd_tables() {
    for (int i = 0; i < 256; ++i) {
        uint32_t add_mask[4] = {0, 0, 0, 0};
        uint32_t sub_mask[4] = {0, 0, 0, 0};
        for (int pos = 0; pos < 4; ++pos) {
            uint8_t bits = (i >> (pos * 2)) & 0b11;
            if (bits == 1) add_mask[pos] = 0xFFFFFFFF; // All 1s mask
            if (bits == 2) sub_mask[pos] = 0xFFFFFFFF;
        }
        mask_add_tbl[i] = _mm_castsi128_ps(_mm_set_epi32(add_mask[3], add_mask[2], add_mask[1], add_mask[0]));
        mask_sub_tbl[i] = _mm_castsi128_ps(_mm_set_epi32(sub_mask[3], sub_mask[2], sub_mask[1], sub_mask[0]));
    }
}

// SSE/AVX hardware-accelerated bitwise pass (Zero FP Multipliers used)
std::vector<float> simd_bitwise_forward_pass(const std::vector<float>& x, 
                                             const std::vector<int8_t>& w_packed, 
                                             size_t rows, size_t cols) {
    std::vector<float> y(rows, 0.0f);
    size_t col_chunks = cols / 4; 
    
    for (size_t r = 0; r < rows; ++r) {
        __m128 sum_vec = _mm_setzero_ps(); // Hardware accumulator
        size_t row_start_byte = (r * cols) / 4;
        
        for (size_t chunk = 0; chunk < col_chunks; ++chunk) {
            uint8_t packed = static_cast<uint8_t>(w_packed[row_start_byte + chunk]);
            
            // Load 4 floats from array x into AVX register
            __m128 x_vec = _mm_loadu_ps(&x[chunk * 4]);
            
            // L1 cache mask generation (Zero processing cycles, instant map)
            __m128 to_add = _mm_and_ps(x_vec, mask_add_tbl[packed]);
            __m128 to_sub = _mm_and_ps(x_vec, mask_sub_tbl[packed]);
            
            // Hardware level conditional addition/subtraction without multipliers
            sum_vec = _mm_add_ps(sum_vec, to_add);
            sum_vec = _mm_sub_ps(sum_vec, to_sub);
        }
        
        // Horizontal add to collapse the SIMD vector into a single float
        float sum_array[4];
        _mm_storeu_ps(sum_array, sum_vec);
        y[r] = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3];
    }
    return y;
}

// Standard FP pass for benchmark anchoring
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
    init_simd_tables(); // Initialize L1 logic blocks
    
    size_t rows = 2048;
    size_t cols = 2048;
    size_t total_weights = rows * cols;
    int iterations = 100;
    
    std::cout << "--- Phase 6: AVX Hardware Overdrive ---" << std::endl;
    std::cout << "Matrix: " << rows << "x" << cols << " (" << iterations << " passes)" << std::endl;
    
    std::vector<float> x(cols);
    std::vector<float> w_float(total_weights);
    std::vector<int8_t> w_packed((total_weights + 3) / 4, 0);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dis_x(0.0f, 1.0f);
    std::uniform_int_distribution<int> dis_w(-1, 1);
    
    for(size_t i=0; i<cols; ++i) x[i] = dis_x(gen);
    
    for(size_t i=0; i<total_weights; ++i) {
        int w = dis_w(gen);
        w_float[i] = static_cast<float>(w);
        pack_weight(w_packed, i, static_cast<int8_t>(w));
    }

    auto start_fp = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i) {
        volatile auto r = float_forward_pass(x, w_float, rows, cols);
    }
    auto stop_fp = std::chrono::high_resolution_clock::now();
    
    auto start_bw = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i) {
        volatile auto r = simd_bitwise_forward_pass(x, w_packed, rows, cols);
    }
    auto stop_bw = std::chrono::high_resolution_clock::now();
    
    auto duration_fp = std::chrono::duration_cast<std::chrono::milliseconds>(stop_fp - start_fp).count();
    auto duration_bw = std::chrono::duration_cast<std::chrono::milliseconds>(stop_bw - start_bw).count();
    
    std::cout << "Standard FP16 Duration: " << duration_fp << " ms" << std::endl;
    std::cout << "SIMD Bitwise Duration:  " << duration_bw << " ms" << std::endl;
    
    return 0;
}
