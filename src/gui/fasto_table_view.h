#pragma once

#include <QTableView>

namespace fastonosql
{
    class FastoTableView
            : public QTableView
    {        
        Q_OBJECT
    public:
        explicit FastoTableView(QWidget* parent = 0);

    private Q_SLOTS:
        void showContextMenu(const QPoint& point);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
    };
}

