#ifndef SHOWTEACHER_H
#define SHOWTEACHER_H

#include "timetabledata.h"
#include "tt_grid.h"

class ShowTeacher
{
public:
    ShowTeacher(TT_Grid *grid, TimetableData *tt_data, int teacher_id);
};

#endif // SHOWTEACHER_H
