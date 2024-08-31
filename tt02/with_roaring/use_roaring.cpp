#include "use_roaring.h"
#include <iostream>
#include "roaring.hh" // the amalgamated roaring.hh includes roaring64map.hh
#include "roaring.c"

void roar() {
    roaring::Roaring r1;
    for (uint32_t i = 100; i < 1000; i++) {
        r1.add(i);
    }
    std::cout << "cardinality = " << r1.cardinality() << std::endl;

    roaring::Roaring64Map r2;
    for (uint64_t i = 18000000000000000100ull; i < 18000000000000001000ull; i++) {
        r2.add(i);
    }
    std::cout << "cardinality = " << r2.cardinality() << std::endl;

    std::vector<std::vector<roaring::Roaring64Map>> vec(
        5 , std::vector<roaring::Roaring64Map> (9));

    std::vector<roaring::Roaring64Map> vec2(1000);
}
