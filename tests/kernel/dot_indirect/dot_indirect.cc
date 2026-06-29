#include "dot_indirect.hh"

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
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " <index_a> <index_b> <data_a> <data_b>" << std::endl;
        return 1;
    }

#ifdef ANNOTATE
    annotate_init_();
#endif // ANNOTATE

    std::vector<uint64_t> index_a;
    std::vector<uint64_t> index_b;
    std::vector<double> data_a;
    std::vector<double> data_b;

    read_array_from_file<uint64_t>(argv[1], index_a);
    read_array_from_file<uint64_t>(argv[2], index_b);
    read_array_from_file<double>(argv[3], data_a);
    read_array_from_file<double>(argv[4], data_b);

    int num_elements = static_cast<int>(index_a.size());
    std::cout << "Read " << num_elements << " elements." << std::endl;

#ifdef ANNOTATE
    roi_begin_();
#endif // ANNOTATE
    double sum = dot_indirect(index_a.data(), index_b.data(), data_a.data(), data_b.data(), num_elements);
#ifdef ANNOTATE
    roi_end_();
#endif // ANNOTATE
    std::cout << "Total sum: " << sum << std::endl;
#ifdef ANNOTATE
    annotate_term_();
#endif // ANNOTATE

    return 0;
}
