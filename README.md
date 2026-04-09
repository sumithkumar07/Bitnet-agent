# BitNet-Agent: 1.58-bit C++ Neural Swarm Simulation

BitNet-Agent is a C++ implementation of a 1.58-bit (ternary) neural simulation utilizing a distributed agent mesh. The engine is optimized for AVX2 SIMD hardware and focuses on memory-efficient weight paging and parallel head processing.

## Technical Specifications

- **Bit-Packing Protocol:** Weights and states are stored using 2-bit packing (4 values per `uint8_t`), achieving a 75% reduction in disk and memory footprint compared to 8-bit storage.
- **Just-in-Time weight Paging:** Individual agent weights are loaded from disk only during active inference and purged immediately after, capping global RAM usage at the size of a single agent's parameters plus metadata.
- **Dynamic Node Mitosis:** The simulation spawns or merges logical agents based on the bit-entropy of the input data stream.
- **Parallel Head Processing:** Multi-head attention operations are distributed across parallel CPU threads using `std::async`.
- **Ternary Quantization:** On-disk persistence utilizes a `{-1, 0, 1}` mapping for agent state serialization.

## Implementation Workflow

1. **Tokenization:** Input stream is processed into localized character-integer mappings.
2. **Mesh Expansion:** Nodes are allocated dynamically based on complexity thresholds.
3. **Council Inference:** Sub-agents execute parallel weight-sum operations using AVX2 intrinsics.
4. **Resonance Handoff:** Inter-agent data transfer is gated by attention scores (scalar dot-product).
5. **Serialization:** Agent states are bit-packed and written to disk for persistence.

## Performance Profile

| Parameter | 8-bit Baseline | 1.58-bit (Packed) | Efficiency |
| :--- | :--- | :--- | :--- |
| **Storage per 512 Weights** | 512 Bytes | 128 Bytes | 75% Reduction |
| **RAM Scaling** | O(N) | O(1) + Meta | Constant Floor |
| **Execution Latency** | ~2.5 ms | 0.6 - 0.9 ms | AVX2 Optimized |

## Usage

### Build Requirements
- Windows OS
- MSVC Compiler (`cl.exe`)
- Python 3.x

### 1. Build Engine
Execute `build.bat` to compile with `/arch:AVX2`.
```cmd
build.bat
```

### 2. Weight Conversion
Use the included bridge to prepare binary payloads from external float matrices.
```bash
python scripts/bridge.py --mock --output real_weights.bin
```

## Development Protocol
Refer to `docs/RULES.md` for the technical constitution and phase-by-phase verification logs.

## License
MIT License. See `LICENSE` for details.
