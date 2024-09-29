# Constraints

Constraints indicate the properties of and relationships in the timetable which determine its quality. They can be broadly divided into "hard" and "soft" constraints. Basically, all hard constraints must be fulfilled before a timetable can be deemed at all worthy of consideration. Soft constraints are applied to features that are desirable, but perhaps not essential. They can be weighted to indicate their importance.

## Hard Constraints

The main hard constraints are those that arise from physical necessity. If a particular teacher is not present on Tuesday afternoons, there is no way they can teach during that time; if class 10A has Maths in the third period on Wednesdays, there is no way they can have English at the same time.

This sort of constraint could, for example, be named "Essential Resource Constraints". To use a resource (here that refers in most cases to a teacher, student group or room), that resource must be present and available. These constraints are regarded as non-negotiable, if one is not fulfilled no timetable is possible.

## Soft Constraints

Soft constraints are evaluated when a (complete) timetable suggestion has been built, with all hard constraints fulfilled.


## "Firm" Constraints

Some constraints lie somewhere between the very hard ones mentioned above and normal soft ones. In some settings they could be regarded as critical conditions, in others just highly desirable. Among these could be lessons which should not take place on the same day (e.g. multiple lessons in the same subject), or two lessons which should start at the same time, or where lessons have limits on their starting times. Those just mentioned are special cases because they are concerned with local properties of lessons and they could be evaluated before the whole timetable is complete. Another example would be the handling of "double" (etc.) lessons. Is it permissible to split them? In these cases, it might be possible to optimize the algorithm if the constraints are "hard".

Otherwise constraints which can be hard or soft can be handled as soft constraints. If they can abort the evaluation, they should be put at the front of the soft-constraints list to avoid unnecessary processing of the other constraints.

## The Question of Completion

The purpose of any timetable software is to deliver a complete, high quality timetable as quickly as possible. In general there will be a trade-off between time and quality. In some cases the constraints may be such that all the results are of limited quality, however long the program works at them.

It may also be that the combination of hard constraints makes it impossible to build any complete timetable at all, or only after a very long time. In such cases, it would be good to know as soon as possible that there is a problem and, if possible, where it lies. The nature of timetabling means, however, that this may be difficult.

In a real-life situation, knowing that timetabling is inherently difficult possibly won't help the person responsible for having a timetable ready by a particular date very much.

The underlying question is, when it is (practically) impossible to construct a valid timetable with the given constraints, the constraints (or the lessons) must be altered. What, then, should be altered and in which way?

In the case of unavoidable conflicts concerning the constraints I called Essential Resource Constraints above, the only real option is to change something about the lessons. It seems to me that is probably not something that a timetabler can normally decide alone, and certainly not something that could be delegated to an algorithm. The timetabler could run tests with changes that they feel are likely to improve the situation, but the final decision might need to be taken at another level. It is to be hoped that this situation, where the data is not amenable to a solution, is something of an exception, and it should normally be possible to detect such problems fairly early in the process. Of course, if the conflicts arise as a result of a last-minute change, a rather stressful situation is likely.

If there are other hard constraints preventing the generation of a complete timetable, these could be made slightly soft, at least temporarily, so that a complete, though possibly very problematic, timetable can be produced.

## How much can be automated?

In view of the complicated nature of the problem and the need for not only deductive powers and troubleshooting skills, but also pedagogical knowledge and awareness of the school organism, I would not expect too much of any algorithm which tries to manipulate the constraints or lessons in an attempt to "improve" the timetable. It may be possible to implement some empirical rules derived from practical experience with the process, but the timetabler should always be aware of any adjustments that are tried. It might be better to offer suggestions, perhaps with aids to implementing them.