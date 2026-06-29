import argparse
import random
from pathlib import Path

def generate_inputs(num_elements, seed, output_dir):
    random.seed(seed)

    out_path = Path(output_dir)
    out_path.mkdir(parents=True, exist_ok=True)

    index_a = list(range(num_elements))
    random.shuffle(index_a)
    index_b = list(range(num_elements))
    random.shuffle(index_b)

    data_a = [round(random.uniform(1.0, 10.0), 2) for _ in range(num_elements)]
    data_b = [round(random.uniform(1.0, 10.0), 2) for _ in range(num_elements)]

    with open(out_path / "index_a.txt", "w") as f:
        for i in index_a:
            f.write(f"{i}\n")
    with open(out_path / "index_b.txt", "w") as f:
        for i in index_b:
            f.write(f"{i}\n")
    with open(out_path / "data_a.txt", "w") as f:
        for val in data_a:
            f.write(f"{val}\n")
    with open(out_path / "data_b.txt", "w") as f:
        for val in data_b:
            f.write(f"{val}\n")

    with open(out_path / "info", "w") as info_file:
        info_file.write(f"num_elements: {num_elements}\n")
        info_file.write(f"seed: {seed}\n")

    print(f"✅ Wrote index_a/b and data_a/b to {out_path} with {num_elements} entries each")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate test inputs for dot_indirect")
    parser.add_argument("num_elements", type=int, help="Number of entries to generate")
    parser.add_argument("seed", type=int, help="Random seed")
    parser.add_argument("--output-dir", default=".", help="Output directory (default: current directory)")
    args = parser.parse_args()

    generate_inputs(args.num_elements, args.seed, args.output_dir)
