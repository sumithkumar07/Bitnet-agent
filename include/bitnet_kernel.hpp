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
#include <thread>
#include <future>

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

    std::vector<float> forward_pass(const std::vector<float>& x, const std::vector<int8_t>& w_packed, size_t rows, size_t cols, size_t offset = 0) {
        std::vector<float> y(rows, 0.0f);
        size_t col_chunks = cols / 4; 
        for (size_t r = 0; r < rows; ++r) {
            __m128 sum_vec = _mm_setzero_ps();
            size_t row_start_byte = offset + (r * cols) / 4;
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
    
    // Phase 28, 34 & 36: Intrinsic QKV Array Pipeline (Fixed Offsets)
    void extract_qkv(const std::vector<float>& input_state, std::vector<float>& q, std::vector<float>& k, std::vector<float>& v, const std::vector<int8_t>& w_packed, size_t full_dim, size_t head_dim, size_t head_idx) {
        // Header (8) + (HeadIdx * 3 * LayerSize)
        size_t layer_size = (full_dim * head_dim / 4);
        size_t head_base = 8 + (head_idx * 3 * layer_size);
        
        q = forward_pass(input_state, w_packed, head_dim, full_dim, head_base);
        k = forward_pass(input_state, w_packed, head_dim, full_dim, head_base + layer_size);
        v = forward_pass(input_state, w_packed, head_dim, full_dim, head_base + 2 * layer_size);
    }

    // Phase 29/30/34: Scaled Dot-Product Self-Attention (Neural Gating & Parallel Head Support)
    struct AttentionResult { std::vector<float> output; float weight; };
    
    AttentionResult compute_attention(const std::vector<float>& q, const std::vector<float>& k, const std::vector<float>& v, size_t dim) {
        float qk_dot = 0.0f;
        for (size_t i = 0; i < dim; ++i) { qk_dot += q[i] * k[i]; }

        float scale = 1.0f / std::sqrt(static_cast<float>(dim));
        float weight = 1.0f / (1.0f + std::exp(-(qk_dot * scale)));

        std::vector<float> output(dim, 0.0f);
        for (size_t i = 0; i < dim; ++i) { output[i] = weight * v[i]; }
        
        return {output, weight};
    }
};

typedef AVX2_Engine::AttentionResult HeadResult;

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
    NodeMemory(uint32_t id, uint32_t capacity, uint32_t state_dim) 
        : agent_id(id), max_capacity(capacity), current_tokens(0) {
        token_cache.resize(max_capacity, 0);
        contextual_state.resize(state_dim, 0.0f);
    }
    
    // Phase 3, 11 & 31: Bitwise Freezing logic (1.58-bit Quantization)
    void serialize_to_disk(const std::string& filepath) {
        std::ofstream file(filepath, std::ios::binary);
        if(!file) throw std::runtime_error("Disk write exception");
        
        file.write(reinterpret_cast<char*>(&agent_id), sizeof(agent_id));
        file.write(reinterpret_cast<char*>(&max_capacity), sizeof(max_capacity));
        file.write(reinterpret_cast<char*>(&current_tokens), sizeof(current_tokens));
        file.write(reinterpret_cast<char*>(token_cache.data()), max_capacity * sizeof(uint32_t));
        
        // Phase 31 & 35: Extreme Bit-Packing (2 Bits Per Value)
        size_t state_size = contextual_state.size();
        file.write(reinterpret_cast<char*>(&state_size), sizeof(size_t));
        
        size_t packed_size = (state_size + 3) / 4;
        std::vector<uint8_t> packed_state(packed_size, 0);
        
        for (size_t i = 0; i < state_size; ++i) {
            float val = contextual_state[i];
            uint8_t code = 0; // 00 -> 0.0
            if (val > 0.5f) code = 1;      // 01 -> 1.0
            else if (val < -0.5f) code = 2; // 10 -> -1.0
            
            size_t byte_idx = i / 4;
            size_t bit_pos = (i % 4) * 2;
            packed_state[byte_idx] |= (code << bit_pos);
        }
        
        file.write(reinterpret_cast<char*>(packed_state.data()), packed_size);
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
        
        // Phase 31 & 35: Extreme Unpacking (2-bit to Float space)
        size_t packed_size = (state_size + 3) / 4;
        std::vector<uint8_t> packed_state(packed_size);
        file.read(reinterpret_cast<char*>(packed_state.data()), packed_size);
        
        contextual_state.assign(state_size, 0.0f);
        for (size_t i = 0; i < state_size; ++i) {
            size_t byte_idx = i / 4;
            size_t bit_pos = (i % 4) * 2;
            uint8_t code = (packed_state[byte_idx] >> bit_pos) & 0b11;
            
            if (code == 1) contextual_state[i] = 1.0f;
            else if (code == 2) contextual_state[i] = -1.0f;
            else contextual_state[i] = 0.0f;
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

    std::string neural_fabric_path;

public:
    SwarmSimulator(size_t hard_limit) : sandbox_memory_limit_bytes(hard_limit) {}

    // Phase 18 & 33/36: Connecting the Sovereign Neural Fabric
    void load_weight_matrix(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Neural Fabric missing at path: " + filepath);
        
        // Phase 36: SOGN Magic Number Verification
        char magic[4];
        file.read(magic, 4);
        if (std::string(magic, 4) != "SOGN") {
            throw std::runtime_error("Sovereign Violation: Invalid Neural Fabric Header (Magic Number Mismatch)");
        }
        
        uint32_t dim_check;
        file.read(reinterpret_cast<char*>(&dim_check), 4);
        
        neural_fabric_path = filepath;
        std::cout << "[SIMULATOR] Sovereign Fabric Linked: " << filepath << " (Dim: " << dim_check << ")" << std::endl;
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

    // Phase 17 & 32: Swarm Mitosis (Neural-Gated Entropy Splitting)
    // Sequentially allocates tokens across dynamically spanned agents based on Neural Pressure (Entropy)
    uint32_t simulate_array_overflow(uint32_t root_agent_id, Tokenizer& tokenizer, const std::string& input_data, uint32_t state_dim) {
        std::vector<uint32_t> incoming_tokens = tokenizer.encode(input_data);
        size_t total_tokens = incoming_tokens.size();
        size_t tokens_processed = 0;
        
        uint32_t current_id = root_agent_id;
        float current_neural_pressure = 0.0f;
        const float pressure_threshold = 5.0f; // Agentic complexity budget
        char last_char = 0;

        while (tokens_processed < total_tokens) {
            NodeMemory& current_agent = get_agent(current_id);
            
            // Phase 32: Dynamic Entropy Pressure Calculation
            char c = (tokens_processed < input_data.length()) ? input_data[tokens_processed] : 0;
            float token_pressure = 1.0f; // Default for unique context
            if (c == last_char) token_pressure = 0.1f; // Repetitive pattern low load
            else if (c == ' ') token_pressure = 0.2f;  // Structural white space
            
            last_char = c;

            // Check if this token would exceed the neural budget of the current agent
            if (current_neural_pressure + token_pressure > pressure_threshold || current_agent.current_tokens >= current_agent.max_capacity) {
                // Mitosis triggers: Spawn a new node to handle the cognitive overflow
                current_id++;
                spawn_agent(current_id, 32, state_dim); // Increased capacity buffer for variable pressure
                current_neural_pressure = 0.0f;
                continue; // Restart logic with the new agent
            }

            // Ingest token into the active agent's cache
            current_agent.token_cache[current_agent.current_tokens] = incoming_tokens[tokens_processed];
            current_agent.current_tokens++;
            current_neural_pressure += token_pressure;
            tokens_processed++;
        }
        
        return current_id;
    }

    // Phase 19/33: Hardware Sequential Inference (with JIT Hydration)
    void execute_swarm_inference(uint32_t root_agent_id, uint32_t tail_agent_id, AVX2_Engine& engine, size_t h_dim) {
        if (neural_fabric_path.empty()) throw std::runtime_error("Execution Fault: Neural Fabric not linked.");

        for (uint32_t iter = root_agent_id; iter <= tail_agent_id; ++iter) {
            NodeMemory& active = get_agent(iter);
            
            // Phase 33: JIT Weight Hydration (Memory Metabolism)
            // Load the weights required for this agent's pass only
            std::vector<int8_t> active_weights = load_weights_safe(neural_fabric_path);
            std::cout << "[SIMULATOR] Agent " << iter << " Hy-Drating weights at " << (void*)active_weights.data() << std::endl;

            // Phase 25: Bridging Memory to Compute
            for (size_t t = 0; t < active.current_tokens; ++t) {
                if (t < active.contextual_state.size()) {
                    active.contextual_state[t] += static_cast<float>(active.token_cache[t]) * 0.01f;
                }
            }
            
            // Phase 34: Inter-Agent Parallel Head Council (2 Heads)
            // Each head runs as a sovereign sub-agent on a separate CPU thread
            const size_t num_heads = 2;
            const size_t head_dim = 8;
            std::vector<std::future<HeadResult>> head_futures;

            for (size_t h = 0; h < num_heads; ++h) {
                head_futures.push_back(std::async(std::launch::async, [&engine, &active, &active_weights, h, head_dim]() {
                    std::vector<float> q, k, v;
                    engine.extract_qkv(active.contextual_state, q, k, v, active_weights, 16, head_dim, h);
                    return engine.compute_attention(q, k, v, head_dim);
                }));
            }

            // Neural Fusion: Merge parallel head outputs into the main state vector
            active.contextual_state.clear();
            float cumulative_gate = 0.0f;
            for (auto& f : head_futures) {
                auto res = f.get();
                active.contextual_state.insert(active.contextual_state.end(), res.output.begin(), res.output.end());
                cumulative_gate += res.weight;
            }
            float trade_gate = cumulative_gate / num_heads; // Average resonance for the handoff
            
            std::cout << "[SIMULATOR] Agent " << iter << " Council Emerged | Head Resonance: " << (trade_gate * 100.0f) << "%" << std::endl;
            
            // Phase 29: Post-Attention Normalization
            engine.apply_rmsnorm(active.contextual_state);
            
            // Phase 27: Isolate Matrix Cache string
            std::vector<float> residual = active.contextual_state;
            
            // Agent executes AVX2 natively on local state
            // Offset: Header (8) + QKV (num_heads * 3 * layer_size)
            size_t layer_size = (16 * 8 / 4);
            size_t ffn_offset = 8 + (num_heads * 3 * layer_size);

            active.contextual_state = engine.forward_pass(active.contextual_state, active_weights, 16, 16, ffn_offset); 
            
            // Phase 24: Bound mathematical limits
            engine.apply_relu(active.contextual_state);
            
            // Phase 26: RMS Normalization
            engine.apply_rmsnorm(active.contextual_state);
            
            // Phase 27: Combine residual history
            engine.apply_residual(active.contextual_state, residual);
            
            // Phase 30: AI Gated Inter-Agent Trade
            if (iter < tail_agent_id) {
                NodeMemory& next_agent = get_agent(iter + 1);
                for (size_t i = 0; i < active.contextual_state.size(); ++i) {
                    next_agent.contextual_state[i] = active.contextual_state[i] * trade_gate;
                }
                std::cout << "[SIMULATOR] Agent " << iter << " -> Agent " << (iter+1) 
                          << " | Neural Trade Resonance: " << (trade_gate * 100.0f) << "%" << std::endl;
            }
            
            // Phase 33: Metabolism (Purge)
            // active_weights vector goes out of scope here and is purged from RAM.
        }
    }
};

#endif // BITNET_KERNEL_HPP
