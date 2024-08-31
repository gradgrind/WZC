#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "database.h"
#include "with_roaring/roaring.hh"

class BasicConstraints
{
public:
    BasicConstraints(DBData *dbdata);

    std::vector<int> item_vec;
    std::vector<roaring::Roaring64Map> items;
    std::vector<std::vector<roaring::Roaring64Map>> week;
    //(5 , std::vector<roaring::Roaring64Map> (9));
};

#endif // BASICCONSTRAINTS_H
