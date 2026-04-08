#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <numeric>

// 1. Memory Block
struct AgentPayload {
    uint32_t agent_id;
    std::vector<float> context_state; // A moving accumulator representing learned data
    
    AgentPayload(uint32_t id) : agent_id(id) {
        context_state.resize(64, 0.0f); // Small state for test
    }
};

// 2. The Agent wrapper
class BitnetAgent {
public:
    AgentPayload memory;
    
    BitnetAgent(uint32_t id) : memory(id) {}
    
    // Simulate some deterministic math on the data (mimicking inference Phase 2)
    void process_data(int step_multiplier) {
        for (size_t i = 0; i < memory.context_state.size(); ++i) {
            // Pseudo-random but deterministic shift based on step
            memory.context_state[i] += (static_cast<float>(i) * step_multiplier) / 10.0f;
        }
    }
    
    // Phase 3 Serialization implementation attached to Agent
    std::vector<uint8_t> freeze() const {
        size_t size = sizeof(memory.agent_id) + (memory.context_state.size() * sizeof(float));
        std::vector<uint8_t> buffer(size);
        
        std::memcpy(buffer.data(), &memory.agent_id, sizeof(memory.agent_id));
        std::memcpy(buffer.data() + sizeof(memory.agent_id), memory.context_state.data(), memory.context_state.size() * sizeof(float));
        
        return buffer;
    }
    
    void load_frozen(const std::vector<uint8_t>& buffer) {
        std::memcpy(&memory.agent_id, buffer.data(), sizeof(memory.agent_id));
        
        size_t float_size = (buffer.size() - sizeof(memory.agent_id)) / sizeof(float);
        memory.context_state.resize(float_size);
        std::memcpy(memory.context_state.data(), buffer.data() + sizeof(memory.agent_id), float_size * sizeof(float));
    }
};

int main() {
    std::cout << "--- Phase 5: Atomic Handoff ---" << std::endl;
    
    // ------------- TEST A: CONTINUOUS RUN -------------
    BitnetAgent agent_continuous(101);
    agent_continuous.process_data(1); // Step 1
    agent_continuous.process_data(2); // Step 2
    // Output is locked.
    
    // ------------- TEST B: HANDOFF RUN -------------
    BitnetAgent agent_A(201);
    agent_A.process_data(1); // Step 1
    
    // The "Wire" transmission
    std::vector<uint8_t> frozen_wire = agent_A.freeze(); 
    
    // Agent A dies. We spawn Agent B.
    BitnetAgent agent_B(301);
    agent_B.load_frozen(frozen_wire); // Agent B inherits Agent A's mind
    
    agent_B.process_data(2); // Step 2
    
    // Output must perfectly match Agent Continuous
    bool match = true;
    for(size_t i=0; i < agent_continuous.memory.context_state.size(); ++i) {
        if(agent_continuous.memory.context_state[i] != agent_B.memory.context_state[i]) {
            match = false;
        }
    }
    
    std::cout << (match ? "HANDOFF SUCCESS" : "CORRUPTION DETECTED") << std::endl;

    return 0;
}
