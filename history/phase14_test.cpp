#include "bitnet_kernel.hpp"
#include <iostream>

int main() {
    std::cout << "--- Phase 14: C++ Hardware Ingestion Verification ---" << std::endl;
    
    // The Sandbox bounds: We expect exactly 64 bytes to be loaded.
    SandboxKernel sandbox(1024); // Generous 1KB bound for the tiny 64 byte target
    std::string file_path = "real_weights.bin";
    
    try {
        std::cout << "[KERNEL] Booting Sandbox Interface..." << std::endl;
        std::cout << "[KERNEL] Attempting to ingest empirical PyTorch mapped binary stream..." << std::endl;
        
        std::vector<int8_t> real_weights = sandbox.load_weights_safe(file_path);
        
        std::cout << "SUCCESS: Real HuggingFace empirical parameters passed Sandbox boundaries." << std::endl;
        std::cout << "Matrix Int8 Footprint: " << real_weights.size() << " bytes securely loaded." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "LOAD FAILED: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
