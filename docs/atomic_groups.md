# Atomic Student Groups

When dealing with some aspects of timetable construction we are faced with various problems arising from the division of a school class into various groups. A class can be divided in different ways, according to the subjects to be handled. For example, a class might be divided differently in Maths and English (say groups A and B) than in Art and Woodwork (say groups X and Y). Significantly more complicated scenarios are possible, but this can be used to show how the idea of "atomic" groups arises.

It can happen that we need to know whether there are some pupils in both group A and group Y. For example, if there are common pupils, lessons for groups A and Y cannot take place at the same time. By building the Cartesian product of the divisions (A, B) and (X, Y) I get the four "atomic" groups A*X, A*Y, B*X, B*Y. These subgroups are guaranteed to share no pupils. Of course this example is rather trivial and could be handled fairly easily in other ways, but the atomic divisions provide a fairly straightforward, flexible and practical way of handling collision control, even in more complex cases.

A potential danger of the use of Cartesian products is the exponential growth with new divisions. In most normal, realistic situations it probably won't be a problem, but one should be aware of this characteristic.

