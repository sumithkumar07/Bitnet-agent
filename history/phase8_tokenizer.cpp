#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

// Simulated structural BPE / Character mapping tokenizer
class StructuralTokenizer {
private:
    std::unordered_map<char, uint32_t> vocab;

public:
    StructuralTokenizer() {
        // Minimal ASCII character mapping for structural proof
        for (int i = 0; i < 128; ++i) {
            vocab[static_cast<char>(i)] = i + 1; // 0 reserved for padding/null
        }
    }

    std::vector<uint32_t> encode(const std::string& text) {
        std::vector<uint32_t> tokens;
        tokens.reserve(text.length());
        for (char c : text) {
            if (vocab.find(c) != vocab.end()) {
                tokens.push_back(vocab[c]);
            } else {
                tokens.push_back(0); // Unknown token
            }
        }
        return tokens;
    }
};

// Agent memory payload (The Backpack)
struct AgentPayload {
    uint32_t agent_id;
    uint32_t max_capacity;
    uint32_t current_tokens;
    std::vector<uint32_t> token_cache;

    AgentPayload(uint32_t id, uint32_t limit) : agent_id(id), max_capacity(limit), current_tokens(0) {
        token_cache.resize(max_capacity, 0);
    }
};

// The Sandbox constraint wrapper
class SandboxInterface {
public:
    void ingest_text(AgentPayload& agent, StructuralTokenizer& tokenizer, const std::string& input_data) {
        std::vector<uint32_t> new_tokens = tokenizer.encode(input_data);
        
        // Strict Sandbox Memory Enforcement
        uint32_t requested_size = agent.current_tokens + new_tokens.size();
        if (requested_size > agent.max_capacity) {
            throw std::length_error("Sandbox Violation: Tokenized data payload exceeds contiguous memory boundary.");
        }

        // Safe injection
        for (size_t i = 0; i < new_tokens.size(); ++i) {
            agent.token_cache[agent.current_tokens + i] = new_tokens[i];
        }
        agent.current_tokens += new_tokens.size();
        
        std::cout << "[SANDBOX] Successfully ingested " << new_tokens.size() << " tokens. Current capacity: " 
                  << agent.current_tokens << "/" << agent.max_capacity << std::endl;
    }
};

int main() {
    std::cout << "--- Phase 8: Data Ingestion & Tokenization Matrix ---" << std::endl;

    StructuralTokenizer tokenizer;
    SandboxInterface sandbox;
    
    // Create an agent bounded to exactly 50 tokens of memory cache
    AgentPayload agent(101, 50);

    // Test A: Safe Ingestion
    std::string safe_string = "Hello BitNet!";
    try {
        std::cout << "\nTest A: Attempting safe string..." << std::endl;
        sandbox.ingest_text(agent, tokenizer, safe_string);
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
    }

    // Test B: Malignant Memory Overflow Ingestion
    std::string danger_string = "This represents an unexpectedly large payload of text data from the network that attempts to overwrite the internal bounds of the agent's pre-allocated memory matrix by sending a massively long sequence.";
    try {
        std::cout << "\nTest B: Attempting context overflow string..." << std::endl;
        sandbox.ingest_text(agent, tokenizer, danger_string);
    } catch (const std::length_error& e) {
        std::cout << "Verification: SANDBOX SAFELY CAUGHT OVERFLOW." << std::endl;
        std::cout << "Exception: " << e.what() << std::endl;
        std::cout << "Agent memory integrity preserved at: " << agent.current_tokens << " tokens." << std::endl;
    }

    return 0;
}
