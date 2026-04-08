#include "bitnet_kernel.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "--- Phase 16: Inter-Agent Data Trading ---" << std::endl;
    
    SandboxKernel sandbox(10 * 1024 * 1024);
    Tokenizer tokenizer;
    
    // Spawn Agent Alpha and Beta
    std::cout << "[KERNEL] Instantiating Swarm Node 1 (Alpha) and Node 2 (Beta)..." << std::endl;
    AgentPayload& alpha = sandbox.spawn_agent(1, 1024, 256);
    AgentPayload& beta = sandbox.spawn_agent(2, 1024, 256);
    
    // Mutate Alpha
    std::cout << "[SWARM] Node 1 digesting string input..." << std::endl;
    sandbox.ingest_text(alpha, tokenizer, "Sovereign Local Swarm Pipeline Test.");
    for (size_t i = 0; i < alpha.contextual_state.size(); ++i) {
        alpha.contextual_state[i] = static_cast<float>(i) * 0.99f;
    }
    
    // Execute Native Trade
    std::cout << "[SWARM] Node 1 executing memory pointer pass to Node 2..." << std::endl;
    try {
        sandbox.trade_state(1, 2);
    } catch (const std::exception& e) {
        std::cerr << "TRADE FAILED: " << e.what() << std::endl;
        return 1;
    }
    
    // Verify mathematical variance natively
    float mse = 0.0f;
    for (size_t i = 0; i < alpha.contextual_state.size(); ++i) {
        float diff = alpha.contextual_state[i] - beta.contextual_state[i];
        mse += diff * diff;
    }
    mse /= alpha.contextual_state.size();

    // Verify token counts identically matched
    std::cout << "Node 1 Token Load: " << alpha.current_tokens << std::endl;
    std::cout << "Node 2 Token Load: " << beta.current_tokens << std::endl;
    std::cout << "Memory Pass Error Rate (MSE): " << mse << std::endl;

    if (mse == 0.0f && alpha.current_tokens == beta.current_tokens) {
        std::cout << "Verification: PURE LATENT TRANSFER ACHIEVED (No Cache Misses)." << std::endl;
    } else {
        std::cout << "Verification: MEMORY LEAK DETECTED." << std::endl;
        return 1;
    }

    return 0;
}
