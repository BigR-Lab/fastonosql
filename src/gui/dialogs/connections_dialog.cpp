#include "gui/dialogs/connections_dialog.h"

#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QEvent>
#include <QToolBar>
#include <QAction>

#include "common/qt/convert_string.h"

#include "core/settings_manager.h"
#include "core/connection_settings.h"

#include "gui/gui_factory.h"
#include "gui/dialogs/connection_listwidget_items.h"
#include "gui/dialogs/connection_dialog.h"
#include "gui/dialogs/cluster_dialog.h"

#include "translations/global.h"

namespace fastoredis
{
    /**
     * @brief Creates dialog
     */
    ConnectionsDialog::ConnectionsDialog(QWidget* parent)
        : QDialog(parent)
    {
        using namespace translations;

        setWindowIcon(GuiFactory::instance().connectIcon());

        // Remove help button (?)
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        listWidget_ = new QTreeWidget;
        listWidget_->setIndentation(5);

        QStringList colums;
        colums << trName << trAddress;
        listWidget_->setHeaderLabels(colums);

        //listWidget_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        //listWidget_->header()->setSectionResizeMode(1, QHeaderView::Stretch);

        //listWidget_->setViewMode(QListView::ListMode);
        listWidget_->setContextMenuPolicy(Qt::ActionsContextMenu);
        listWidget_->setIndentation(15);
        listWidget_->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
        listWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);

        //listWidget_->setDragEnabled(true);
        //listWidget_->setDragDropMode(QAbstractItemView::InternalMove);
        setMinimumSize(QSize(min_width, min_height));
        VERIFY(connect(listWidget_, &QTreeWidget::itemDoubleClicked, this, &ConnectionsDialog::accept));
        VERIFY(connect(listWidget_, &QTreeWidget::itemSelectionChanged, this, &ConnectionsDialog::connectionSelectChange));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        buttonBox->button(QDialogButtonBox::Ok)->setIcon(GuiFactory::instance().serverIcon());
        acButton_ = buttonBox->button(QDialogButtonBox::Ok);
        acButton_->setEnabled(false);

        VERIFY(connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectionsDialog::accept));
        VERIFY(connect(buttonBox, &QDialogButtonBox::rejected, this, &ConnectionsDialog::reject));

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(buttonBox);

        QToolBar *savebar = new QToolBar;

        QAction *addB = new QAction(GuiFactory::instance().loadIcon(), trAddConnection, savebar);
        typedef void(QAction::*trig)(bool);
        VERIFY(connect(addB, static_cast<trig>(&QAction::triggered), this, &ConnectionsDialog::add));
        savebar->addAction(addB);

        QAction *addc = new QAction(GuiFactory::instance().clusterIcon(), trAddClusterConnection, savebar);
        typedef void(QAction::*trig)(bool);
        VERIFY(connect(addc, static_cast<trig>(&QAction::triggered), this, &ConnectionsDialog::addCls));
        savebar->addAction(addc);

        QAction *rmB = new QAction(GuiFactory::instance().removeIcon(), trRemoveConnection, savebar);
        VERIFY(connect(rmB, static_cast<trig>(&QAction::triggered), this, &ConnectionsDialog::remove));
        savebar->addAction(rmB);

        QAction *editB = new QAction(GuiFactory::instance().editIcon(), trEditConnection, savebar);
        VERIFY(connect(editB, static_cast<trig>(&QAction::triggered), this, &ConnectionsDialog::edit));
        savebar->addAction(editB);

        QVBoxLayout *firstColumnLayout = new QVBoxLayout;
        firstColumnLayout->addWidget(savebar);
        firstColumnLayout->addWidget(listWidget_);
        firstColumnLayout->addLayout(bottomLayout);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->addLayout(firstColumnLayout, 1);

        // Populate list with connections
        SettingsManager::ConnectionSettingsContainerType connections = SettingsManager::instance().connections();
        for (SettingsManager::ConnectionSettingsContainerType::const_iterator it = connections.begin(); it != connections.end(); ++it) {
            IConnectionSettingsBaseSPtr connectionModel = (*it);
            addConnection(connectionModel);
        }

        SettingsManager::ClusterSettingsContainerType clusters = SettingsManager::instance().clusters();
        for (SettingsManager::ClusterSettingsContainerType::const_iterator it = clusters.begin(); it != clusters.end(); ++it) {
            IClusterSettingsBaseSPtr connectionModel = (*it);
            addCluster(connectionModel);
        }

        // Highlight first item
        if (listWidget_->topLevelItemCount() > 0)
            listWidget_->setCurrentItem(listWidget_->topLevelItem(0));
        retranslateUi();
    }

    IConnectionSettingsBaseSPtr ConnectionsDialog::selectedConnection() const
    {
        ConnectionListWidgetItem *currentItem = dynamic_cast<ConnectionListWidgetItem *>(listWidget_->currentItem());
        if (currentItem){
            return currentItem->connection();
        }

        return IConnectionSettingsBaseSPtr();
    }

    IClusterSettingsBaseSPtr ConnectionsDialog::selectedCluster() const
    {
        ClusterConnectionListWidgetItem *currentItem = dynamic_cast<ClusterConnectionListWidgetItem *>(listWidget_->currentItem());
        if (currentItem){
            return currentItem->connection();
        }

        return IClusterSettingsBaseSPtr();
    }

    void ConnectionsDialog::add()
    {
        ConnectionDialog dlg(this);
        int result = dlg.exec();
        IConnectionSettingsBaseSPtr p = dlg.connection();
        if(result == QDialog::Accepted && p){
            SettingsManager::instance().addConnection(p);
            addConnection(p);
        }
    }

    void ConnectionsDialog::addCls()
    {
        ClusterDialog dlg(this);
        int result = dlg.exec();
        IClusterSettingsBaseSPtr p = dlg.connection();
        if(result == QDialog::Accepted && p){
            SettingsManager::instance().addCluster(p);
            addCluster(p);
        }
    }

    void ConnectionsDialog::remove()
    {
        QTreeWidgetItem* qitem = listWidget_->currentItem();
        if(!qitem){
            return;
        }

        ConnectionListWidgetItem* currentItem = dynamic_cast<ConnectionListWidgetItem *>(qitem);
        if(currentItem){

            QTreeWidgetItem* qpitem = qitem->parent();
            if(!qpitem){
                // Ask user
                int answer = QMessageBox::question(this, "Connections", QString("Really delete \"%1\" connection?").arg(currentItem->text(0)),
                                                   QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

                if (answer != QMessageBox::Yes)
                    return;

                IConnectionSettingsBaseSPtr connection = currentItem->connection();
                delete currentItem;
                SettingsManager::instance().removeConnection(connection);
                return;
            }
            else{
                qitem = qpitem;
            }
        }

        ClusterConnectionListWidgetItem* clCurrentItem = dynamic_cast<ClusterConnectionListWidgetItem *>(qitem);
        if(clCurrentItem){

            // Ask user
            int answer = QMessageBox::question(this, "Connections", QString("Really delete \"%1\" cluster?").arg(clCurrentItem->text(0)),
                                               QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

            if (answer != QMessageBox::Yes)
                return;

            IClusterSettingsBaseSPtr connection = clCurrentItem->connection();
            delete clCurrentItem;
            SettingsManager::instance().removeCluster(connection);
        }
    }

    void ConnectionsDialog::edit()
    {
        QTreeWidgetItem* qitem = listWidget_->currentItem();
        if(!qitem){
            return;
        }

        ConnectionListWidgetItem* currentItem = dynamic_cast<ConnectionListWidgetItem *>(qitem);
        if(currentItem){
            QTreeWidgetItem* qpitem = qitem->parent();
            if(!qpitem){
                IConnectionSettingsBaseSPtr con = currentItem->connection();
                ConnectionDialog dlg(this, dynamic_cast<IConnectionSettingsBase*>(con->clone()));
                int result = dlg.exec();
                IConnectionSettingsBaseSPtr newConnection = dlg.connection();
                if(result == QDialog::Accepted && newConnection){
                    currentItem->setConnection(newConnection);
                    SettingsManager::instance().removeConnection(con);
                    SettingsManager::instance().addConnection(newConnection);
                }
                return;
            }
            else{
                qitem = qpitem;
            }
        }


        ClusterConnectionListWidgetItem* clCurrentItem = dynamic_cast<ClusterConnectionListWidgetItem *>(qitem);
        if(clCurrentItem){
            IClusterSettingsBaseSPtr con = clCurrentItem->connection();
            ClusterDialog dlg(this, dynamic_cast<IClusterSettingsBase*>(con->clone()));
            int result = dlg.exec();
            IClusterSettingsBaseSPtr newConnection = dlg.connection();
            if(result == QDialog::Accepted && newConnection){
                clCurrentItem->setConnection(newConnection);
                SettingsManager::instance().removeCluster(con);
                SettingsManager::instance().addCluster(newConnection);
            }
        }
    }

    void ConnectionsDialog::connectionSelectChange()
    {
        bool isEnable = selectedConnection() != NULL || selectedCluster() != NULL;
        acButton_->setEnabled(isEnable);
    }

    /**
     * @brief This function is called when user clicks on "Connect" button.
     */
    void ConnectionsDialog::accept()
    {
        QDialog::accept();
    }

    void ConnectionsDialog::changeEvent(QEvent* e)
    {
        if(e->type() == QEvent::LanguageChange){
            retranslateUi();
        }
        QDialog::changeEvent(e);
    }

    void ConnectionsDialog::retranslateUi()
    {
        using namespace translations;

        setWindowTitle(trConnections);
        acButton_->setText(trOpen);
    }

    void ConnectionsDialog::addConnection(IConnectionSettingsBaseSPtr con)
    {
        ConnectionListWidgetItem* item = new ConnectionListWidgetItem(con);
        listWidget_->addTopLevelItem(item);
    }

    void ConnectionsDialog::addCluster(IClusterSettingsBaseSPtr con)
    {
        ClusterConnectionListWidgetItem* item = new ClusterConnectionListWidgetItem(con);
        listWidget_->addTopLevelItem(item);
    }
}
