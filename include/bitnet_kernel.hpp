#ifndef BITNET_KERNEL_HPP
#define BITNET_KERNEL_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <immintrin.h>
#include <cmath>

// ---------------------------------------------------------
// COMPONENT 1: AVX2 SIMD Hardware Overdrive (Phase 6)
// ---------------------------------------------------------
struct AVX2_Engine {
    __m128 mask_add_tbl[256];
    __m128 mask_sub_tbl[256];

    AVX2_Engine() {
        for (int i = 0; i < 256; ++i) {
            uint32_t add_mask[4] = {0, 0, 0, 0};
            uint32_t sub_mask[4] = {0, 0, 0, 0};
            for (int pos = 0; pos < 4; ++pos) {
                uint8_t bits = (i >> (pos * 2)) & 0b11;
                if (bits == 1) add_mask[pos] = 0xFFFFFFFF;
                if (bits == 2) sub_mask[pos] = 0xFFFFFFFF;
            }
            mask_add_tbl[i] = _mm_castsi128_ps(_mm_set_epi32(add_mask[3], add_mask[2], add_mask[1], add_mask[0]));
            mask_sub_tbl[i] = _mm_castsi128_ps(_mm_set_epi32(sub_mask[3], sub_mask[2], sub_mask[1], sub_mask[0]));
        }
    }

    std::vector<float> forward_pass(const std::vector<float>& x, const std::vector<int8_t>& w_packed, size_t rows, size_t cols) {
        std::vector<float> y(rows, 0.0f);
        size_t col_chunks = cols / 4; 
        for (size_t r = 0; r < rows; ++r) {
            __m128 sum_vec = _mm_setzero_ps();
            size_t row_start_byte = (r * cols) / 4;
            for (size_t chunk = 0; chunk < col_chunks; ++chunk) {
                uint8_t packed = static_cast<uint8_t>(w_packed[row_start_byte + chunk]);
                __m128 x_vec = _mm_loadu_ps(&x[chunk * 4]);
                __m128 to_add = _mm_and_ps(x_vec, mask_add_tbl[packed]);
                __m128 to_sub = _mm_and_ps(x_vec, mask_sub_tbl[packed]);
                sum_vec = _mm_add_ps(sum_vec, to_add);
                sum_vec = _mm_sub_ps(sum_vec, to_sub);
            }
            float sum_array[4];
            _mm_storeu_ps(sum_array, sum_vec);
            y[r] = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3];
        }
        return y;
    }

    // Phase 24: Mathematical Non-Linearity Verification
    void apply_relu(std::vector<float>& x) {
        __m128 zero = _mm_setzero_ps();
        size_t size = x.size();
        size_t i = 0;
        for (; i + 3 < size; i += 4) {
            __m128 vec = _mm_loadu_ps(&x[i]);
            vec = _mm_max_ps(vec, zero);
            _mm_storeu_ps(&x[i], vec);
        }
        for (; i < size; ++i) {
            if (x[i] < 0.0f) x[i] = 0.0f;
        }
    }

    // Phase 26: RMS Normalization
    void apply_rmsnorm(std::vector<float>& x) {
        float eps = 1e-6f;
        float sum_sq = 0.0f;
        size_t size = x.size();
        for (size_t i = 0; i < size; ++i) {
            sum_sq += x[i] * x[i];
        }
        float rms = std::sqrt(sum_sq / size + eps);
        for (size_t i = 0; i < size; ++i) {
            x[i] /= rms;
        }
    }

    // Phase 27: Mathematical Residual Connections
    void apply_residual(std::vector<float>& target, const std::vector<float>& skip) {
        size_t size = target.size();
        for (size_t i = 0; i < size; ++i) {
            target[i] += skip[i];
        }
    }
    
    // Phase 28: Intrinsic QKV Array Pipeline Extracts
    void extract_qkv(std::vector<float>& input_state, std::vector<float>& q, std::vector<float>& k, std::vector<float>& v, const std::vector<int8_t>& w_packed, size_t dim) {
        q = forward_pass(input_state, w_packed, dim, dim);
        k = forward_pass(input_state, w_packed, dim, dim);
        v = forward_pass(input_state, w_packed, dim, dim);
    }

    // Phase 29/30: Scaled Dot-Product Self-Attention (Neural Gating Support)
    // Returns {output_vector, attention_score}
    struct AttentionResult { std::vector<float> output; float weight; };
    
    AttentionResult compute_attention(const std::vector<float>& q, const std::vector<float>& k, const std::vector<float>& v, size_t dim) {
        // Step 1: Q . K^T dot product
        float qk_dot = 0.0f;
        for (size_t i = 0; i < dim; ++i) { qk_dot += q[i] * k[i]; }

        // Step 2-3: Scale and Sigmoid-Softmax
        float scale = 1.0f / std::sqrt(static_cast<float>(dim));
        float weight = 1.0f / (1.0f + std::exp(-(qk_dot * scale)));

        // Step 4: Weight the Value vector
        std::vector<float> output(dim, 0.0f);
        for (size_t i = 0; i < dim; ++i) { output[i] = weight * v[i]; }
        
        return {output, weight};
    }
};

// ---------------------------------------------------------
// COMPONENT 2: Structural Tokenizer (Phase 8)
// ---------------------------------------------------------
class Tokenizer {
private:
    std::unordered_map<char, uint32_t> vocab;
public:
    Tokenizer() {
        for (int i = 0; i < 128; ++i) { vocab[static_cast<char>(i)] = i + 1; }
    }
    std::vector<uint32_t> encode(const std::string& text) {
        std::vector<uint32_t> tokens;
        tokens.reserve(text.length());
        for (char c : text) { tokens.push_back(vocab.count(c) ? vocab[c] : 0); }
        return tokens;
    }
};

// ---------------------------------------------------------
// COMPONENT 3: Sovereign Agent Memory Payload (Phase 3 & 4)
// ---------------------------------------------------------
struct NodeMemory {
    uint32_t agent_id;
    uint32_t max_capacity;
    uint32_t current_tokens;
    std::vector<uint32_t> token_cache;
    std::vector<float> contextual_state; 
    
    // Phase 28: Structural Attention Arrays
    std::vector<float> query;
    std::vector<float> key;
    std::vector<float> value;

    NodeMemory(uint32_t id, uint32_t capacity, uint32_t state_dim) 
        : agent_id(id), max_capacity(capacity), current_tokens(0) {
        token_cache.resize(max_capacity, 0);
        contextual_state.resize(state_dim, 0.0f);
        query.resize(state_dim, 0.0f);
        key.resize(state_dim, 0.0f);
        value.resize(state_dim, 0.0f);
    }
    
    // Phase 3, 11 & 31: Bitwise Freezing logic (1.58-bit Quantization)
    void serialize_to_disk(const std::string& filepath) {
        std::ofstream file(filepath, std::ios::binary);
        if(!file) throw std::runtime_error("Disk write exception");
        
        file.write(reinterpret_cast<char*>(&agent_id), sizeof(agent_id));
        file.write(reinterpret_cast<char*>(&max_capacity), sizeof(max_capacity));
        file.write(reinterpret_cast<char*>(&current_tokens), sizeof(current_tokens));
        file.write(reinterpret_cast<char*>(token_cache.data()), max_capacity * sizeof(uint32_t));
        
        // Phase 31: Ternary Quantization Pass for Neural Persistence
        size_t state_size = contextual_state.size();
        file.write(reinterpret_cast<char*>(&state_size), sizeof(size_t));
        
        std::vector<int8_t> quantized_state(state_size);
        for (size_t i = 0; i < state_size; ++i) {
            float val = contextual_state[i];
            // Simple absolute-mean thresholding for 1.58-bit ternary mapping
            quantized_state[i] = (val > 0.5f) ? 1 : (val < -0.5f ? -1 : 0);
        }
        file.write(reinterpret_cast<char*>(quantized_state.data()), state_size * sizeof(int8_t));
    }

    // Phase 11 & 31: Bitwise Reconstitution Logic
    void reconstruct_from_disk(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if(!file) throw std::runtime_error("Disk read exception: Agent Trace missing.");

        file.read(reinterpret_cast<char*>(&agent_id), sizeof(agent_id));
        file.read(reinterpret_cast<char*>(&max_capacity), sizeof(max_capacity));
        file.read(reinterpret_cast<char*>(&current_tokens), sizeof(current_tokens));
        
        token_cache.resize(max_capacity);
        file.read(reinterpret_cast<char*>(token_cache.data()), max_capacity * sizeof(uint32_t));
        
        size_t state_size;
        file.read(reinterpret_cast<char*>(&state_size), sizeof(size_t));
        
        // Phase 31: De-quantize back to float space
        std::vector<int8_t> quantized_state(state_size);
        file.read(reinterpret_cast<char*>(quantized_state.data()), state_size * sizeof(int8_t));
        
        contextual_state.assign(state_size, 0.0f);
        for (size_t i = 0; i < state_size; ++i) {
            contextual_state[i] = static_cast<float>(quantized_state[i]);
        }
    }
};

// ---------------------------------------------------------
// COMPONENT 4: Sandbox Interface (Phase 7, 15, 18)
// ---------------------------------------------------------
class SwarmSimulator {
private:
    size_t sandbox_memory_limit_bytes;
    std::unordered_map<uint32_t, NodeMemory> swarm_registry;
    uint32_t active_agent_count = 0;

public:
    std::vector<int8_t> global_weights;  // Phase 18: Constant Matrix Fabric

    SwarmSimulator(size_t hard_limit) : sandbox_memory_limit_bytes(hard_limit) {}

    // Phase 18: Hydrating the Neural Fabric centrally
    void load_weight_matrix(const std::string& filepath) {
        global_weights = load_weights_safe(filepath);
    }
    
    // Phase 15: Swarm Spawning Mechanics
    NodeMemory& spawn_agent(uint32_t id, uint32_t capacity, uint32_t state_dim) {
        if (swarm_registry.find(id) != swarm_registry.end()) {
            throw std::runtime_error("Sandbox Registry Fault: Agent ID Collision");
        }

        
        // Strict boundary checking simulated roughly via capacity counts
        size_t proposed_allocation = (capacity * sizeof(uint32_t)) + (state_dim * sizeof(float));
        if (proposed_allocation > sandbox_memory_limit_bytes) {
            throw std::length_error("Sandbox Violation: Swarm payload requested exceeds global heap.");
        }
        
        // Emplace avoids standard constructor copying for fast initialization
        swarm_registry.emplace(id, NodeMemory(id, capacity, state_dim));
        active_agent_count++;
        return swarm_registry.at(id);
    }
    
    NodeMemory& get_agent(uint32_t id) {
        return swarm_registry.at(id);
    }
    
    uint32_t swarm_size() const {
        return active_agent_count;
    }

    // Phase 7 loader
    std::vector<int8_t> load_weights_safe(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) throw std::runtime_error("File not found");

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size > sandbox_memory_limit_bytes) {
            throw std::length_error("Sandbox Violation: File exceeds matrix RAM bound");
        }

        std::vector<int8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            throw std::runtime_error("Stream interrupt");
        }
        return buffer;
    }

    // Phase 8 ingest bounds checking
    void ingest_text(NodeMemory& agent, Tokenizer& tokenizer, const std::string& input_data) {
        std::vector<uint32_t> new_tokens = tokenizer.encode(input_data);
        if (agent.current_tokens + new_tokens.size() > agent.max_capacity) {
            throw std::length_error("Sandbox Violation: Tokenized payload overflow");
        }
        for (size_t i = 0; i < new_tokens.size(); ++i) {
            agent.token_cache[agent.current_tokens + i] = new_tokens[i];
        }
        agent.current_tokens += new_tokens.size();
    }
    
    // Phase 16: Inter-Agent Data Trading (In-Memory Passing)
    void trade_state(uint32_t sender_id, uint32_t receiver_id) {
        if (swarm_registry.find(sender_id) == swarm_registry.end() || 
            swarm_registry.find(receiver_id) == swarm_registry.end()) {
            throw std::runtime_error("Trade Fault: Target agent does not exist in registry.");
        }
        NodeMemory& sender = swarm_registry.at(sender_id);
        NodeMemory& receiver = swarm_registry.at(receiver_id);
        
        if (sender.token_cache.size() != receiver.token_cache.size() || 
            sender.contextual_state.size() != receiver.contextual_state.size()) {
            throw std::length_error("Trade Fault: Agent payload capacities do not match.");
        }
        
        // Deep copy memory blocks directly inside the heap completely avoiding the OS/Disk
        std::copy(sender.token_cache.begin(), sender.token_cache.end(), receiver.token_cache.begin());
        std::copy(sender.contextual_state.begin(), sender.contextual_state.end(), receiver.contextual_state.begin());
        receiver.current_tokens = sender.current_tokens;
    }

    // Phase 17: Swarm Mitosis (Payload Splitting)
    // Sequentially allocates tokens across dynamically spanned agents if data exceeds the target's capacity
    uint32_t simulate_array_overflow(uint32_t root_agent_id, Tokenizer& tokenizer, const std::string& input_data, uint32_t standard_capacity, uint32_t state_dim) {
        std::vector<uint32_t> incoming_tokens = tokenizer.encode(input_data);
        size_t total_tokens = incoming_tokens.size();
        size_t tokens_processed = 0;
        
        uint32_t current_id = root_agent_id;
        
        while (tokens_processed < total_tokens) {
            NodeMemory& current_agent = get_agent(current_id);
            size_t available_space = current_agent.max_capacity - current_agent.current_tokens;
            
            size_t chunk_size = std::min(available_space, total_tokens - tokens_processed);
            
            for (size_t i = 0; i < chunk_size; ++i) {
                current_agent.token_cache[current_agent.current_tokens + i] = incoming_tokens[tokens_processed + i];
            }
            
            current_agent.current_tokens += chunk_size;
            tokens_processed += chunk_size;
            
            // If there's still tokens left in the pipeline, Mitosis triggers
            if (tokens_processed < total_tokens) {
                current_id++; // The simplest sequential spanning ID mechanism for Mitosis 
                spawn_agent(current_id, standard_capacity, state_dim); // Spawns the clone cleanly
            }
        }
        
        return current_id; // Returns the ID of the final agent who holds the tail of the data
    }

    // Phase 19: Hardware Sequential Inference
    // Iterates across all agents sequentially (Data Flow), injecting State into AVX2 Engine.
    void execute_swarm_inference(uint32_t root_agent_id, uint32_t tail_agent_id, AVX2_Engine& engine, size_t h_dim) {
        if (global_weights.empty()) throw std::runtime_error("Execution Fault: Hypervisor Neural Fabric not mapped.");

        for (uint32_t iter = root_agent_id; iter <= tail_agent_id; ++iter) {
            NodeMemory& active = get_agent(iter);
            
            // Phase 25: Bridging Memory to Compute (Native Integer to Float Embed)
            for (size_t t = 0; t < active.current_tokens; ++t) {
                if (t < active.contextual_state.size()) {
                    active.contextual_state[t] += static_cast<float>(active.token_cache[t]) * 0.01f;
                }
            }
            
            // Phase 28: Formally Extract Structural Logic for Attention Loop Native Bounds
            engine.extract_qkv(active.contextual_state, active.query, active.key, active.value, global_weights, 16);
            
            // Phase 29/30: Neural-Gated Attention Scoring
            auto attn = engine.compute_attention(active.query, active.key, active.value, 16);
            active.contextual_state = attn.output;
            float trade_gate = attn.weight;
            
            // Phase 29: Post-Attention Normalization (prevents attention output explosion)
            engine.apply_rmsnorm(active.contextual_state);
            
            // Phase 27: Isolate Matrix Cache string prior to matrix multiplier loop explicitly
            std::vector<float> residual = active.contextual_state;
            
            // Agent executes AVX2 natively on local state
            // Dummy structural rows=16, cols(weights)=16 parameter dimension for the 64-byte HuggingFace empirical block
            active.contextual_state = engine.forward_pass(active.contextual_state, global_weights, 16, 16); 
            
            // Phase 24: Bound mathematical limits using isolated ReLU sequence
            engine.apply_relu(active.contextual_state);
            
            // Phase 26: RMS Normalization stabilizing mathematical explosions
            engine.apply_rmsnorm(active.contextual_state);
            
            // Phase 27: Combine residual history structurally bypassing historical decay loop natively
            engine.apply_residual(active.contextual_state, residual);
            
            // Phase 30: AI Gated Inter-Agent Trade (Knowledge Handoff)
            // Instead of a mindless copy, we use the captured 'trade_gate' (attention score) 
            // to project the sender's knowledge into the receiver's state.
            if (iter < tail_agent_id) {
                NodeMemory& next_agent = get_agent(iter + 1);
                for (size_t i = 0; i < active.contextual_state.size(); ++i) {
                    // Signal Resilience: The next agent starts with a 'Neural Resonance' of the previous state
                    next_agent.contextual_state[i] = active.contextual_state[i] * trade_gate;
                }
                std::cout << "[SIMULATOR] Agent " << iter << " -> Agent " << (iter+1) 
                          << " | Neural Trade Resonance: " << (trade_gate * 100.0f) << "%" << std::endl;
            }
        }
    }
};

#endif // BITNET_KERNEL_HPP
