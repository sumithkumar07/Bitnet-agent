import torch
from transformers import AutoModelForCausalLM
import struct
import numpy as np

print("--- Phase 12: Network Model Ingestion ---")
model_id = "hf-internal-testing/tiny-random-LlamaForCausalLM"
print(f"Connecting to HuggingFace Hub -> {model_id}")

# Load a structurally perfect but tiny LLM architecture
model = AutoModelForCausalLM.from_pretrained(model_id)
print("Model Struct Loaded Successfully in PyTorch.")

# Extract a single representative Attention Projection Linear Layer
target_layer = model.model.layers[0].self_attn.v_proj.weight.data
print(f"Extracted Base Target Layer: {target_layer.shape} (FP32)")

print("\n--- Phase 13: Ternary Matrix Down-Sampling ---")
# BitNet 1.58b Quantization Math
# Scale = mean(abs(W))
scale = torch.mean(torch.abs(target_layer))
print(f"Calculated Mean-Absolute Scale: {scale.item():.4f}")

# W_quant = Round(Clamp(W / Scale, -1, 1))
scaled_tensor = target_layer / (scale + 1e-8)
quantized_tensor = torch.clamp(torch.round(scaled_tensor), min=-1.0, max=1.0)

unique_vals = torch.unique(quantized_tensor)
print(f"Quantized Constraints: {unique_vals.tolist()}")
if torch.any((quantized_tensor != -1) & (quantized_tensor != 0) & (quantized_tensor != 1)):
    print("FATAL: Floating point bleed detected.")
    exit(1)
print("Verification: STRICT TERNARY BOUNDARY CONSTRAINTS [-1, 0, 1] PASSED.")

print("\n--- Phase 14: Hardware Byte-Packing ---")
# Convert down to int8 flat array to send to C++
flat_tensor = quantized_tensor.view(-1).to(torch.int8).cpu().numpy()
rows, cols = target_layer.shape

# Calculate exact bytes required. Packing 4 weights per byte.
total_weights = len(flat_tensor)
total_bytes = (total_weights + 3) // 4
packed_array = bytearray(total_bytes)

print(f"Compressing {total_weights} items into {total_bytes} bytes...")

# Bitwise dictionary to map the hardware representation identical to our C++ struct
# -1 -> 10 (binary 2)
# 1  -> 01 (binary 1)
# 0  -> 00 (binary 0)
def to_bits(val):
    if val == 1: return 1
    if val == -1: return 2
    return 0

for i in range(total_weights):
    byte_idx = i // 4
    bit_pos = i % 4
    bits = to_bits(flat_tensor[i])
    
    # Inject shifting the bits into the bytearray natively in Python
    packed_array[byte_idx] |= (bits << (bit_pos * 2))

# Write to C++ Kernel Loader File
output_file = "real_weights.bin"
with open(output_file, "wb") as f:
    f.write(packed_array)

print(f"Verification: {total_bytes} bytes securely flushed to '{output_file}'.")
print("Python Pipeline Complete. Yielding strictly to C++ Kernel Daemon.")
