#include "reduce_indirect.hh"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#ifdef ANNOTATE
extern "C" {
#include "annotate.h"
}
#endif // ANNOTATE

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <index_file> <data_file>" << std::endl;
        return 1;
    }

#ifdef ANNOTATE
    annotate_init_();
#endif // ANNOTATE

    std::vector<uint64_t> index;
    std::vector<double> data_a;

    read_array_from_file<uint64_t>(argv[1], index);
    read_array_from_file<double>(argv[2], data_a);

    int num_elements = static_cast<int>(index.size());
    std::cout << "Read " << num_elements << " elements." << std::endl;

    std::vector<double> data_b(num_elements, 0.0);

#ifdef ANNOTATE
    roi_begin_();
#endif // ANNOTATE
    reduce_indirect(index.data(), data_a.data(), data_b.data(), num_elements);
#ifdef ANNOTATE
    roi_end_();
#endif // ANNOTATE

    // Compute sum of elements in data_b to verify correctness
    double sum = 0.0;
    for (int i = 0; i < num_elements; ++i) {
        sum += data_b[i];
    }
    std::cout << "Total sum: " << sum << std::endl;

#ifdef ANNOTATE
    annotate_term_();
#endif // ANNOTATE

    return 0;
}
