#include "basicconstraints.h"
#include <QJsonArray>

BasicConstraints::BasicConstraints(DBData *dbdata)
{
    // Collect the atomic subgroups
    QHash<int, QString> bmi2sg;
    QList<int> indexvec{0};
    QHash<QString, int> sg2bmi;

    for (int gid : dbdata->Tables.value("GROUPS")) {
        auto node = dbdata->Nodes.value(gid).DATA;

        if (node.value("ID").toString().isEmpty()) {
            // A whole-class group
            int cl = node.value("CLASS").toInt();
            auto sglist = node.value("SUBGROUPS").toArray();
            for (auto sg : sglist) {
                auto sgstr = sg.toString();
                int bmi = indexvec.length();
                sg2bmi[sgstr] = bmi;
                bmi2sg[bmi] = sgstr;
                indexvec.append(cl);
            }
        }
    }

    QHash<int, int> t2bmi;
    QHash<int, QString> bmi2t;
    for (int tid : dbdata->Tables.value("TEACHERS")) {
        int bmi = indexvec.length();
        t2bmi[tid] = bmi;
        bmi2t[bmi] = dbdata->get_tag(tid);
        indexvec.append(tid);
    }

    qDebug() << " ** indexvec:" << indexvec;
    qDebug() << " ** bmi2sg:" << bmi2sg;
    qDebug() << " ** sg2bmi:" << sg2bmi;
    qDebug() << " ** bmi2t:" << bmi2t;
    qDebug() << " ** t2bmi:" << t2bmi;
}
