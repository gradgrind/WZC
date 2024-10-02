# Timetable Basics

In order to work with the timetable data, the relevant data is extracted from the [database](database.md#the-database), processed and stored in structures which are more appropriate to the operations to be performed. If any of the underlying information (in the database) is changed, these timetable-specific structures must be rebuilt.

The basis of the timetable is a [*BasicConstraints*](basicconstraints.md#the-basicconstraints-structure) structure. This ties together a lot of the basic data and keeps track of the lesson and resource placements. Also the [*LessonData*](lessondata.md#the-lessondata-structure) structure, representing a single lesson, plays a key role in the timetable.