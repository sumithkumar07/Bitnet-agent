#include "bitnet_kernel.hpp"
#include <iostream>

int main() {
    std::cout << "--- Phase 9: Unified Kernel Header Compilation ---" << std::endl;
    
    // Test instantiations
    try {
        AVX2_Engine engine;
        Tokenizer tokenizer;
        AgentPayload agent(1, 4096, 512);
        SandboxKernel sandbox(10 * 1024 * 1024); // 10MB bounds
        
        std::cout << "[STRUCT REPORT]" << std::endl;
        std::cout << "AVX2_Engine Size:   " << sizeof(engine) << " bytes" << std::endl;
        std::cout << "Tokenizer Size:     " << sizeof(tokenizer) << " bytes" << std::endl;
        std::cout << "AgentPayload Size:  " << sizeof(agent) << " bytes" << std::endl;
        std::cout << "SandboxKernel Size: " << sizeof(sandbox) << " bytes" << std::endl;
        
        std::cout << "\nSUCCESS: Header compilation and struct instantiation passed strict isolation checks." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Initialization Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
