#ifndef SHOWCLASS_H
#define SHOWCLASS_H

#include "database.h"
#include "tt_grid.h"

class ShowClass
{
public:
    ShowClass(TT_Grid *grid, DBData *db_data, int class_id);
};

#endif // SHOWCLASS_H
