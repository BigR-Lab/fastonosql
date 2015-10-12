#include "gui/dialogs/property_server_dialog.h"

#include <QHBoxLayout>
#include <QTableView>

#include "fasto/qt/gui/glass_widget.h"

#include "gui/gui_factory.h"
#include "gui/property_table_model.h"
#include "core/iserver.h"

#include "translations/global.h"

namespace fastonosql
{
    PropertyServerDialog::PropertyServerDialog(IServerSPtr server, QWidget* parent)
        : QDialog(parent), server_(server)
    {
        using namespace translations;
        CHECK(server_);

        setWindowIcon(GuiFactory::instance().icon(server->type()));

        PropertyTableModel* mod = new PropertyTableModel(this);
        propertyes_table_ = new QTableView;
        VERIFY(connect(mod, &PropertyTableModel::changedProperty, this, &PropertyServerDialog::changedProperty));
        propertyes_table_->setModel(mod);

        QHBoxLayout *mainL = new QHBoxLayout;
        mainL->addWidget(propertyes_table_);
        setLayout(mainL);

        glassWidget_ = new fasto::qt::gui::GlassWidget(GuiFactory::instance().pathToLoadingGif(), trLoading, 0.5, QColor(111, 111, 100), this);

        VERIFY(connect(server.get(), &IServer::startedLoadServerProperty, this, &PropertyServerDialog::startServerProperty));
        VERIFY(connect(server.get(), &IServer::finishedLoadServerProperty, this, &PropertyServerDialog::finishServerProperty));
        VERIFY(connect(server.get(), &IServer::startedChangeServerProperty, this, &PropertyServerDialog::startServerChangeProperty));
        VERIFY(connect(server.get(), &IServer::finishedChangeServerProperty, this, &PropertyServerDialog::finishServerChangeProperty));
        retranslateUi();
    }

    void PropertyServerDialog::startServerProperty(const EventsInfo::ServerPropertyInfoRequest& req)
    {
        glassWidget_->start();
    }

    void PropertyServerDialog::finishServerProperty(const EventsInfo::ServerPropertyInfoResponce& res)
    {
        glassWidget_->stop();
        common::Error er = res.errorInfo();
        if(er && er->isError()){
            return;
        }

        if(server_->type() == REDIS){
            ServerPropertyInfo inf = res.info_;
            PropertyTableModel *model = qobject_cast<PropertyTableModel*>(propertyes_table_->model());
            for(int i = 0; i < inf.propertyes_.size(); ++i)
            {
                PropertyType it = inf.propertyes_[i];
                model->insertItem(new PropertyTableItem(common::convertFromString<QString>(it.first), common::convertFromString<QString>(it.second)));
            }
        }
    }

    void PropertyServerDialog::startServerChangeProperty(const EventsInfo::ChangeServerPropertyInfoRequest& req)
    {

    }

    void PropertyServerDialog::finishServerChangeProperty(const EventsInfo::ChangeServerPropertyInfoResponce& res)
    {
        common::Error er = res.errorInfo();
        if(er && er->isError()){
            return;
        }

        if(server_->type() == REDIS){
            PropertyType pr = res.newItem_;
            if(res.isChange_){
                PropertyTableModel *model = qobject_cast<PropertyTableModel*>(propertyes_table_->model());
                model->changeProperty(pr);
            }
        }
    }

    void PropertyServerDialog::changedProperty(const PropertyType& prop)
    {
        EventsInfo::ChangeServerPropertyInfoRequest req(this, prop);
        server_->changeProperty(req);
    }

    void PropertyServerDialog::changeEvent(QEvent* e)
    {
        if(e->type() == QEvent::LanguageChange){
            retranslateUi();
        }
        QDialog::changeEvent(e);
    }

    void PropertyServerDialog::showEvent(QShowEvent* e)
    {
        QDialog::showEvent(e);
        emit showed();

        EventsInfo::ServerPropertyInfoRequest req(this);
        server_->serverProperty(req);
    }

    void PropertyServerDialog::retranslateUi()
    {
        using namespace translations;
        setWindowTitle(tr("%1 properties").arg(server_->name()));
    }
}
