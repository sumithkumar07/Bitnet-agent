#include "bitnet_kernel.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "--- Phase 10: Continuous Executive Daemon ---" << std::endl;
    std::cout << "Booting Sovereign Kernel..." << std::endl;

    try {
        // 1. Immutable Engine Initialization
        AVX2_Engine engine;
        Tokenizer tokenizer;
        SandboxKernel sandbox(10 * 1024 * 1024); // 10MB bounds

        // 2. Initialize the persistent agent in memory
        uint32_t context_tokens = 512;
        AgentPayload agent(1, context_tokens, 1024);
        
        std::cout << "[KERNEL] System Active. Agent Memory Allocated." << std::endl;

        // Metric to track memory stability (capacity should never change)
        size_t initial_cache_capacity = agent.token_cache.capacity();
        size_t initial_state_capacity = agent.contextual_state.capacity();
        
        bool memory_leak_detected = false;
        
        std::cout << "Simulating continuous REPL stream (100 injection hits)..." << std::endl;

        // 3. Continuous Execution Simulation (Testing bounds without exiting)
        for (int i = 0; i < 100; ++i) {
            std::string prompt = "User input cycle " + std::to_string(i) + " simulating network traffic.";
            
            // Clean state manually to simulate rolling context (preventing legitimate bounds overflow for test)
            agent.current_tokens = 0; 
            
            // Ingest against boundaries
            sandbox.ingest_text(agent, tokenizer, prompt);
            
            // Critical Metric: Assert that vectors did not dynamically resize beneath us (which causes heap fragmentation)
            if (agent.token_cache.capacity() != initial_cache_capacity || 
                agent.contextual_state.capacity() != initial_state_capacity) {
                memory_leak_detected = true;
                break;
            }
        }

        if (memory_leak_detected) {
            std::cerr << "\n[KERNEL PANIC] Vector reallocation detected. Memory stability broken." << std::endl;
            return 1;
        } else {
            std::cout << "\n[KERNEL DIAGNOSTIC] 100 loops executed." << std::endl;
            std::cout << "Memory bloat reallocations: 0" << std::endl;
            std::cout << "Verification: SUCCESSFUL DAEMON UPTIME WITHOUT FRAGMENTATION" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[DAEMON EXCEPTION] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
