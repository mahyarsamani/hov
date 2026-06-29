import argparse
import random
from pathlib import Path

def generate_inputs(num_elements, seed, output_dir):
    random.seed(seed)

    out_path = Path(output_dir)
    out_path.mkdir(parents=True, exist_ok=True)

    # Generate index as a random permutation of 0..n-1
    index = list(range(num_elements))
    random.shuffle(index)

    # Generate n random float values for data
    data = [round(random.uniform(1.0, 10.0), 2) for _ in range(num_elements)]

    # Write index.txt
    with open(out_path / "index.txt", "w") as index_file:
        for i in index:
            index_file.write(f"{i}\n")

    # Write data.txt
    with open(out_path / "data.txt", "w") as data_file:
        for val in data:
            data_file.write(f"{val}\n")

    with open(out_path / "info", "w") as info_file:
        info_file.write(f"num_elements: {num_elements}\n")
        info_file.write(f"seed: {seed}\n")

    print(f"✅ Wrote data.txt and index.txt to {out_path} with {num_elements} entries each")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate test inputs for reduce_indirect")
    parser.add_argument("num_elements", type=int, help="Number of entries to generate")
    parser.add_argument("seed", type=int, help="Random seed")
    parser.add_argument("--output-dir", default=".", help="Output directory (default: current directory)")
    args = parser.parse_args()

    generate_inputs(args.num_elements, args.seed, args.output_dir)
