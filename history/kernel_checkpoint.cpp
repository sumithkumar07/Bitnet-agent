#include "bitnet_kernel.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "--- Phase 11: Continuous Daemon Checkpointing ---" << std::endl;
    std::string trace_file = "agent_checkpoint.bin";
    
    // Part 1: Sandbox Alpha (The original execution state)
    std::vector<float> original_state_copy;
    {
        std::cout << "[KERNEL] Booting Sandbox Alpha..." << std::endl;
        SandboxKernel sandbox_alpha(10 * 1024 * 1024);
        Tokenizer tokenizer;
        AgentPayload agent_alpha(505, 512, 1024);
        
        sandbox_alpha.ingest_text(agent_alpha, tokenizer, "Sovereign Context Sentence 1.");
        sandbox_alpha.ingest_text(agent_alpha, tokenizer, "Sovereign Context Sentence 2.");
        
        // Simulating the AVX Engine doing work by modifying the hidden state manually
        for (size_t i = 0; i < agent_alpha.contextual_state.size(); ++i) {
            agent_alpha.contextual_state[i] = static_cast<float>(i) * 0.158f; 
        }
        
        std::cout << "[KERNEL] Freezing Agent 505. Serializing state... " << std::endl;
        original_state_copy = agent_alpha.contextual_state;
        agent_alpha.serialize_to_disk(trace_file);
        
        std::cout << "[KERNEL] Terminating Sandbox Alpha. Clearing RAM." << std::endl;
    } // Agent Alpha falls entirely out of scope, memory destroyed natively by C++.

    // Part 2: Sandbox Beta (The separate execution state weeks later, or on a different server)
    {
        std::cout << "\n[KERNEL] Booting Sandbox Beta..." << std::endl;
        AgentPayload agent_beta(0, 0, 0); // Blank dummy instantiation
        
        std::cout << "[KERNEL] Reconstituting Agent via Trace file..." << std::endl;
        try {
            agent_beta.reconstruct_from_disk(trace_file);
        } catch (const std::exception& e) {
            std::cerr << "LOAD FAILED: " << e.what() << std::endl;
            return 1;
        }

        std::cout << "Reconstructed Agent ID: " << agent_beta.agent_id << std::endl;
        std::cout << "Reconstructed Tokens:   " << agent_beta.current_tokens << " / " << agent_beta.max_capacity << std::endl;

        // Verify mathematical equivalence (MSE against the old Alpha Agent state copy)
        float mse = 0.0f;
        for (size_t i = 0; i < original_state_copy.size(); ++i) {
            float diff = original_state_copy[i] - agent_beta.contextual_state[i];
            mse += diff * diff;
        }
        mse /= original_state_copy.size();

        std::cout << "Mathematical State Variance (MSE): " << mse << std::endl;

        if (mse == 0.0f && agent_beta.agent_id == 505) {
            std::cout << "Verification: PERFECT STATE RECONSTITUTION." << std::endl;
        } else {
            std::cout << "Verification: STATE CORRUPTION." << std::endl;
            return 1;
        }
    }

    return 0;
}
