#include "bitnet_kernel.hpp"
#include <iostream>

int main() {
    std::cout << "--- Phase 19: Sequential Swarm Inference (Data Flow Integration) ---" << std::endl;
    
    // Bounds parameters
    SandboxKernel sandbox(10 * 1024 * 1024);
    AVX2_Engine engine; 
    
    // Load HuggingFace Matrix
    try {
        sandbox.mount_global_fabric("real_weights.bin");
    } catch(const std::exception& e) {
        std::cerr << "Weight Failure: " << e.what() << std::endl;
        return 1;
    }
    
    // Spawn two sequential identical test agents mimicking post-Mitosis
    std::cout << "[KERNEL] Spawning Swarm Node 1 and Node 2." << std::endl;
    AgentPayload& alpha = sandbox.spawn_agent(1, 12, 16);
    AgentPayload& beta = sandbox.spawn_agent(2, 12, 16);
    
    // Simulate initial latent conditions for Alpha
    for(size_t i=0; i<16; ++i) { alpha.contextual_state[i] = 1.0f; } // All ones native
    
    std::cout << "[SWARM] Commencing Execution Pipeline (Node 1 -> Node 2)..." << std::endl;
    try {
        sandbox.execute_swarm_inference(1, 2, engine, 16);
    } catch(const std::exception& e) {
        std::cerr << "Inference Exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Test the output cascade
    std::cout << "Calculated Latent Matrix output off Node 2: " << std::endl;
    for(size_t i=0; i<std::min(alpha.contextual_state.size(), (size_t)8); i++) {
         std::cout << beta.contextual_state[i] << " ";
    }
    std::cout << std::endl;

    // Node 1's contextual_state SHOULD NOT equal Node 2's contextual_state,
    // because Node 2 takes Node 1's output state and recursively runs AVX2 on it AGAIN.
    bool inference_compounded = (alpha.contextual_state[0] != beta.contextual_state[0]);

    if (inference_compounded) {
         std::cout << "Verification: SWARM COMPOUND INFERENCE CASCADE PASSED." << std::endl;
    } else {
         std::cout << "Verification: EXECUTION FAULT (Matrix Static)." << std::endl;
         return 1;
    }

    return 0;
}
