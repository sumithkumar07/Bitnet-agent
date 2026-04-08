#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>

// Mock Payload from Phase 3
struct AgentPayload {
    uint32_t agent_id;
    uint32_t active_tokens;
    std::vector<float> kv_cache;

    AgentPayload(uint32_t id, uint32_t tokens) : agent_id(id), active_tokens(tokens) {
        kv_cache.resize(tokens * 64, 0.0f);
    }
};

// ==========================================
// THE SANDBOX ENGINE
// ==========================================

class SovereignSandbox {
private:
    uint32_t max_token_limit;
    bool thread_active;

public:
    SovereignSandbox(uint32_t max_limit) : max_token_limit(max_limit), thread_active(true) {}

    // Attempts to ingest incoming data into the agent's payload.
    // Enforces strict sandbox memory constraints.
    bool safe_ingest(AgentPayload& agent, uint32_t incoming_tokens) {
        if (!thread_active) return false;

        try {
            // Check boundary before allocation
            if (agent.active_tokens + incoming_tokens > max_token_limit) {
                // Rule: Catch overflow, do NOT crash the host thread.
                throw std::length_error("Sandbox Overflow: Context limit exceeded.");
            }

            // Safe to allocate
            agent.active_tokens += incoming_tokens;
            agent.kv_cache.resize(agent.active_tokens * 64, 0.5f);
            return true;

        } catch (const std::length_error& e) {
            // Sandbox catches the isolated agent overflow
            std::cerr << "[SANDBOX KERNEL CATCH] " << e.what() << std::endl;
            
            // Truncation Routine: Force agent to maximize safely instead of crashing
            uint32_t safe_remainder = max_token_limit - agent.active_tokens;
            agent.active_tokens += safe_remainder;
            agent.kv_cache.resize(agent.active_tokens * 64, 0.5f);
            
            std::cout << "[SANDBOX] Truncation applied. Thread survived. Tokens strictly clamped to: " 
                      << agent.active_tokens << std::endl;
            return false; // Indicates partial ingest
        }
    }
    
    bool is_alive() const { return thread_active; }
};

int main() {
    std::cout << "--- Phase 4: Sandbox Memory Enforcement ---" << std::endl;
    
    // Strict container boundary: 1024 tokens max memory.
    SovereignSandbox sandbox(1024);
    AgentPayload agent(101, 500); // Starts with 500 tokens
    
    // Test safe ingest
    sandbox.safe_ingest(agent, 200); // 700 total. Safe.
    
    // Test malicious / overflow ingest (Trying to push 1000 tokens into remaining 324 slots)
    sandbox.safe_ingest(agent, 1000); 
    
    // Thread must still be alive here.
    return 0;
}
