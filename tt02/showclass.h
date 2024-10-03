#ifndef SHOWCLASS_H
#define SHOWCLASS_H

#include "timetabledata.h"
#include "tt_grid.h"

class ShowClass
{
public:
    ShowClass(TT_Grid *grid, TimetableData *tt_data, int class_id);
};

#endif // SHOWCLASS_H
