#ifndef STATECACHE_H
#define STATECACHE_H

struct LessonState {
    // The members are 0-based indexes, a negative value specifies "none".
    signed char day{-1}, hour{-1};
    signed short room{-1}; // room chosen from list
};

class StateCache
{
public:
    StateCache();
};

#endif // STATECACHE_H
