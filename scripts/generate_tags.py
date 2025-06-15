import struct
import sys

def write_binary_indexes(num_vectors, dim, output_file):
    if dim != 1:
        raise ValueError("For index vectors, dim must be 1.")

    with open(output_file, "wb") as f:
        f.write(struct.pack("I", num_vectors))  # uint32
        f.write(struct.pack("I", dim))          # uint32

        for i in range(num_vectors):
            f.write(struct.pack("I", i))        # index as uint32

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <num_vectors> <output_file>")
        sys.exit(1)

    num_vectors = int(sys.argv[1])
    dim = 1  # fixed as requested
    output_file = sys.argv[2]

    write_binary_indexes(num_vectors, dim, output_file)
