#include <iostream>
#include <vector>
#include <cstdint>

// Baseline: A standard 16-bit float matrix requires 2 bytes per weight.
// We simulate FP16 sizing using uint16_t arrays.
struct Float16Matrix {
    size_t rows;
    size_t cols;
    std::vector<uint16_t> data; // 2 Bytes per weight

    Float16Matrix(size_t r, size_t c) : rows(r), cols(c) {
        data.resize(r * c, 0); 
    }
    
    size_t get_byte_size() const {
        return data.capacity() * sizeof(uint16_t);
    }
};

// 1.58-bit matrix. Values are -1, 0, 1.
// We require exactly 2 bits of memory to store -1, 0, 1. (00 = 0, 01 = 1, 10 = -1)
// Therefore, we can pack four weights into a single standard 8-bit integer (int8_t).
struct TernaryPackedMatrix {
    size_t rows;
    size_t cols;
    std::vector<int8_t> data; // 1 Byte holds 4 weights

    TernaryPackedMatrix(size_t r, size_t c) : rows(r), cols(c) {
        // 1 byte holds 4 weights. Divide total requested size by 4.
        size_t physical_bytes = ((r * c) + 3) / 4; 
        data.resize(physical_bytes, 0);
    }

    size_t get_byte_size() const {
        return data.capacity() * sizeof(int8_t);
    }
};

int main() {
    // Simulating a layer in a Tier 1 "Courier" Agent
    size_t test_rows = 4096;
    size_t test_cols = 4096;
    size_t total_weights = test_rows * test_cols;

    std::cout << "--- Phase 1: Ternary Allocation Test ---" << std::endl;
    std::cout << "Total parameters:       " << total_weights << std::endl;

    Float16Matrix baseline(test_rows, test_cols);
    TernaryPackedMatrix ternary(test_rows, test_cols);

    size_t fp16_size = baseline.get_byte_size();
    size_t ternary_size = ternary.get_byte_size();

    std::cout << "FP16 Matrix Size:       " << fp16_size << " bytes (" << fp16_size / (1024 * 1024) << " MB)" << std::endl;
    std::cout << "Ternary Matrix Size:    " << ternary_size << " bytes (" << ternary_size / (1024 * 1024) << " MB)" << std::endl;
    
    double compression_ratio = (double)fp16_size / (double)ternary_size;
    std::cout << "Compression Factor:     " << compression_ratio << "x smaller." << std::endl;

    return 0;
}
