#include "bitnet_kernel.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "--- Phase 17: Swarm Mitosis (Payload Spanning Analysis) ---" << std::endl;
    
    SandboxKernel sandbox(50 * 1024 * 1024);
    Tokenizer tokenizer;
    
    // We create Agent #1 with a tiny cap of exactly 12 tokens
    uint32_t standard_capacity = 12;
    uint32_t state_dim = 256;
    
    std::cout << "[KERNEL] Booting Root Node 1 [Capacity: " << standard_capacity << " Tokens]." << std::endl;
    sandbox.spawn_agent(1, standard_capacity, state_dim);
    
    // We send a massive string to Agent 1: 54 characters (so exactly 54 tokens under our naive string tokenizer)
    // 54 tokens / 12 capacity = 4.5
    // Therefore, the Sandbox should mathematically construct exactly 5 Agents to absorb this hit cleanly.
    std::string heavy_payload = "This string exceeds the Agent bound, forcing mitoses."; 
    
    std::cout << "[SWARM] Injecting heavy 54-token network payload directly into Node 1 boundary..." << std::endl;
    
    uint32_t tail_id;
    try {
        tail_id = sandbox.ingest_swarm_text(1, tokenizer, heavy_payload, standard_capacity, state_dim);
    } catch (const std::exception& e) {
        std::cerr << "MITOSIS FAILED: " << e.what() << std::endl;
        return 1;
    }
    
    // Validate Swarm Spanning Geometry
    std::cout << "Tail Agent Node ID Generated: " << tail_id << std::endl;
    std::cout << "Total Active Agents in Mesh:  " << sandbox.swarm_size() << std::endl;

    // Verify token drops
    uint32_t total_tokens_captured = 0;
    for (uint32_t i = 1; i <= sandbox.swarm_size(); ++i) {
        uint32_t caught = sandbox.get_agent(i).current_tokens;
        std::cout << " - Node " << i << " holds " << caught << " / 12 tokens." << std::endl;
        total_tokens_captured += caught;
    }

    std::cout << "Summation Total Tokens Tracked: " << total_tokens_captured << " / " << heavy_payload.length() << std::endl;

    if (sandbox.swarm_size() == 5 && total_tokens_captured == heavy_payload.length()) {
         std::cout << "\nVerification: PAYLOAD SPLITTING (MITOSIS) SWARM MATH PERFECT." << std::endl;
    } else {
         std::cout << "\nVerification: BOUNDARY LEAK." << std::endl;
         return 1;
    }
    
    return 0;
}
