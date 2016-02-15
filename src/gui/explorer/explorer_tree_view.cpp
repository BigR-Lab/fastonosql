/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/explorer/explorer_tree_view.h"

#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>

#include "gui/explorer/explorer_tree_model.h"

#include "gui/dialogs/info_server_dialog.h"
#include "gui/dialogs/property_server_dialog.h"
#include "gui/dialogs/history_server_dialog.h"
#include "gui/dialogs/load_contentdb_dialog.h"
#include "gui/dialogs/create_dbkey_dialog.h"
#include "gui/dialogs/view_keys_dialog.h"
#include "gui/dialogs/change_password_server_dialog.h"

#include "common/qt/convert_string.h"
#include "common/logger.h"

#include "core/settings_manager.h"
#include "core/icluster.h"

#include "translations/global.h"

namespace fastonosql {

ExplorerTreeView::ExplorerTreeView(QWidget* parent)
  : QTreeView(parent) {
  setModel(new ExplorerTreeModel(this));

  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setContextMenuPolicy(Qt::CustomContextMenu);
  VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
                 this, SLOT(showContextMenu(const QPoint&))));

  connectAction_ = new QAction(this);
  VERIFY(connect(connectAction_, &QAction::triggered,
                 this, &ExplorerTreeView::connectDisconnectToServer));
  openConsoleAction_ = new QAction(this);
  VERIFY(connect(openConsoleAction_, &QAction::triggered, this, &ExplorerTreeView::openConsole));
  loadDatabaseAction_ = new QAction(this);
  VERIFY(connect(loadDatabaseAction_, &QAction::triggered, this, &ExplorerTreeView::loadDatabases));

  infoServerAction_ = new QAction(this);
  VERIFY(connect(infoServerAction_, &QAction::triggered,
                 this, &ExplorerTreeView::openInfoServerDialog));

  propertyServerAction_ = new QAction(this);
  VERIFY(connect(propertyServerAction_, &QAction::triggered,
                 this, &ExplorerTreeView::openPropertyServerDialog));

  setServerPassword_ = new QAction(this);
  VERIFY(connect(setServerPassword_, &QAction::triggered,
                 this, &ExplorerTreeView::openSetPasswordServerDialog));

  setMaxClientConnection_ = new QAction(this);
  VERIFY(connect(setMaxClientConnection_, &QAction::triggered,
                 this, &ExplorerTreeView::openMaxClientSetDialog));

  historyServerAction_ = new QAction(this);
  VERIFY(connect(historyServerAction_, &QAction::triggered,
                 this, &ExplorerTreeView::openHistoryServerDialog));

  clearHistoryServerAction_ = new QAction(this);
  VERIFY(connect(clearHistoryServerAction_, &QAction::triggered,
                 this, &ExplorerTreeView::clearHistory));

  closeServerAction_ = new QAction(this);
  VERIFY(connect(closeServerAction_, &QAction::triggered,
                 this, &ExplorerTreeView::closeServerConnection));

  closeClusterAction_ = new QAction(this);
  VERIFY(connect(closeClusterAction_, &QAction::triggered,
                 this, &ExplorerTreeView::closeClusterConnection));

  importAction_ = new QAction(this);
  VERIFY(connect(importAction_, &QAction::triggered, this, &ExplorerTreeView::importServer));

  backupAction_ = new QAction(this);
  VERIFY(connect(backupAction_, &QAction::triggered, this, &ExplorerTreeView::backupServer));

  shutdownAction_ = new QAction(this);
  VERIFY(connect(shutdownAction_, &QAction::triggered, this, &ExplorerTreeView::shutdownServer));

  loadContentAction_ = new QAction(this);
  VERIFY(connect(loadContentAction_, &QAction::triggered, this, &ExplorerTreeView::loadContentDb));

  setDefaultDbAction_ = new QAction(this);
  VERIFY(connect(setDefaultDbAction_, &QAction::triggered, this, &ExplorerTreeView::setDefaultDb));

  createKeyAction_ = new QAction(this);
  VERIFY(connect(createKeyAction_, &QAction::triggered, this, &ExplorerTreeView::createKey));

  viewKeysAction_ = new QAction(this);
  VERIFY(connect(viewKeysAction_, &QAction::triggered, this, &ExplorerTreeView::viewKeys));

  getValueAction_ = new QAction(this);
  VERIFY(connect(getValueAction_, &QAction::triggered, this, &ExplorerTreeView::getValue));

  deleteKeyAction_ = new QAction(this);
  VERIFY(connect(deleteKeyAction_, &QAction::triggered, this, &ExplorerTreeView::deleteKey));

  retranslateUi();
}

ExplorerTreeView::~ExplorerTreeView(){

}

void ExplorerTreeView::addServer(IServerSPtr server) {
  DCHECK(server);
  if (!server) {
    return;
  }

  ExplorerTreeModel *mod = static_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  syncWithServer(server.get());

  mod->addServer(server);
}

void ExplorerTreeView::removeServer(IServerSPtr server) {
  DCHECK(server);
  if (!server) {
    return;
  }

  ExplorerTreeModel *mod = static_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  unsyncWithServer(server.get());

  mod->removeServer(server);
  emit closeServer(server);
}

void ExplorerTreeView::addCluster(IClusterSPtr cluster) {
  ExplorerTreeModel *mod = static_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  ICluster::nodes_type nodes = cluster->nodes();
  for (size_t i = 0; i < nodes.size(); ++i) {
    syncWithServer(nodes[i].get());
  }

  mod->addCluster(cluster);
}

void ExplorerTreeView::removeCluster(IClusterSPtr cluster) {
  DCHECK(cluster);
  if (!cluster) {
    return;
  }

  ExplorerTreeModel *mod = static_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  ICluster::nodes_type nodes = cluster->nodes();
  for (int i = 0; i < nodes.size(); ++i) {
    unsyncWithServer(nodes[i].get());
  }

  mod->removeCluster(cluster);
  emit closeCluster(cluster);
}

void ExplorerTreeView::showContextMenu(const QPoint& point) {
  QPoint menuPoint = mapToGlobal(point);
  menuPoint.setY(menuPoint.y() + header()->height());

  QModelIndex sel = selectedIndex();
  if (sel.isValid()) {
    IExplorerTreeItem *node = common::utils_qt::item<IExplorerTreeItem*>(sel);
    DCHECK(node);
    if (!node) {
      return;
    }

    if (node->type() == IExplorerTreeItem::eCluster) {
      QMenu menu(this);
      closeClusterAction_->setEnabled(true);
      menu.addAction(closeClusterAction_);
      menu.exec(menuPoint);
    } else if (node->type() == IExplorerTreeItem::eServer) {
      QMenu menu(this);
      menu.addAction(connectAction_);
      menu.addAction(openConsoleAction_);

      IServerSPtr server = node->server();
      bool isCon = server->isConnected();
      bool isAuth = server->isAuthenticated();

      bool isClusterMember = dynamic_cast<ExplorerClusterItem*>(node->parent()) != NULL;

      loadDatabaseAction_->setEnabled(isAuth);
      menu.addAction(loadDatabaseAction_);
      infoServerAction_->setEnabled(isAuth);
      menu.addAction(infoServerAction_);
      propertyServerAction_->setEnabled(isAuth);
      menu.addAction(propertyServerAction_);

      setServerPassword_->setEnabled(isAuth);
      menu.addAction(setServerPassword_);
\
      setMaxClientConnection_->setEnabled(isAuth);
      menu.addAction(setMaxClientConnection_);

      menu.addAction(historyServerAction_);
      menu.addAction(clearHistoryServerAction_);
      closeServerAction_->setEnabled(!isClusterMember);
      menu.addAction(closeServerAction_);

      bool isCanRemote = server->isCanRemote();
      bool isLocal = true;
      if (isCanRemote) {
        common::net::hostAndPort host = server->address();
        isLocal = host.isLocalHost();
      }

      importAction_->setEnabled(!isCon && isLocal);
      menu.addAction(importAction_);
      backupAction_->setEnabled(isCon && isLocal);
      menu.addAction(backupAction_);
      shutdownAction_->setEnabled(isAuth);
      menu.addAction(shutdownAction_);

      menu.exec(menuPoint);
    } else if (node->type() == IExplorerTreeItem::eDatabase) {
      ExplorerDatabaseItem *db = dynamic_cast<ExplorerDatabaseItem*>(node);
      QMenu menu(this);
      menu.addAction(loadContentAction_);
      bool isDefault = db && db->isDefault();
      loadContentAction_->setEnabled(isDefault);

      menu.addAction(createKeyAction_);
      createKeyAction_->setEnabled(isDefault);

      if (isDefault) {
        menu.addAction(viewKeysAction_);
      }

      menu.addAction(setDefaultDbAction_);
      setDefaultDbAction_->setEnabled(!isDefault);
      menu.exec(menuPoint);
    } else if (node->type() == IExplorerTreeItem::eKey) {
      QMenu menu(this);
      menu.addAction(getValueAction_);
      menu.addAction(deleteKeyAction_);
      menu.exec(menuPoint);
    }
  }
}

void ExplorerTreeView::connectDisconnectToServer() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  if (server->isConnected()) {
    events_info::DisConnectInfoRequest req(this);
    server->disconnect(req);
  } else {
    events_info::ConnectInfoRequest req(this);
    server->connect(req);
  }
}

void ExplorerTreeView::openConsole() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (node) {
    emit openedConsole(node->server(), QString());
  }
}

void ExplorerTreeView::loadDatabases() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (node) {
    node->loadDatabases();
  }
}

void ExplorerTreeView::openInfoServerDialog() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  InfoServerDialog infDialog(server, this);
  infDialog.exec();
}

void ExplorerTreeView::openPropertyServerDialog() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  PropertyServerDialog infDialog(server, this);
  infDialog.exec();
}

void ExplorerTreeView::openSetPasswordServerDialog() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  ChangePasswordServerDialog pass(QString("Change password for %1 server").arg(node->name()),
                                  server, this);
  pass.exec();
}

void ExplorerTreeView::openMaxClientSetDialog() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  bool ok;
  int maxcl = QInputDialog::getInt(this, tr("Set max connection on %1 server").arg(server->name()),
                                         tr("Maximum connection:"), 10000, 1, INT32_MAX, 100, &ok);
  if (ok) {
    events_info::ChangeMaxConnectionRequest req(this, maxcl);
    server->setMaxConnection(req);
  }
}

void ExplorerTreeView::openHistoryServerDialog() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  ServerHistoryDialog histDialog(server, this);
  histDialog.exec();
}

void ExplorerTreeView::clearHistory() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (!server) {
    return;
  }

  events_info::ClearServerHistoryRequest req(this);
  server->clearHistory(req);
}

void ExplorerTreeView::closeServerConnection() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem* snode = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (snode) {
    IServerSPtr server = snode->server();
    if (server) {
      removeServer(server);
    }
    return;
  }

  ExplorerClusterItem* cnode = common::utils_qt::item<ExplorerClusterItem*>(sel);
  if (cnode && cnode->type() == IExplorerTreeItem::eCluster) {
    IClusterSPtr server = cnode->cluster();
    if (server) {
      removeCluster(server);
    }
    return;
  }
}

void ExplorerTreeView::closeClusterConnection() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerClusterItem* cnode = common::utils_qt::item<ExplorerClusterItem*>(sel);
  if (cnode) {
    IClusterSPtr server = cnode->cluster();
    if (server) {
      removeCluster(server);
    }
    return;
  }
}

void ExplorerTreeView::backupServer() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  QString filepath = QFileDialog::getOpenFileName(this, translations::trBackup,
                                                  QString(), translations::trfilterForRdb);
  if (!filepath.isEmpty() && server) {
    events_info::BackupInfoRequest req(this, common::convertToString(filepath));
    server->backupToPath(req);
  }
}

void ExplorerTreeView::importServer() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  QString filepath = QFileDialog::getOpenFileName(this, translations::trImport,
                                                  QString(), translations::trfilterForRdb);
  if (filepath.isEmpty() && server) {
    events_info::ExportInfoRequest req(this, common::convertToString(filepath));
    server->exportFromPath(req);
  }
}

void ExplorerTreeView::shutdownServer() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerServerItem *node = common::utils_qt::item<ExplorerServerItem*>(sel);
  if (!node) {
    return;
  }

  IServerSPtr server = node->server();
  if (server && server->isConnected()) {
    // Ask user
    int answer = QMessageBox::question(this, "Shutdown",
                                       QString("Really shutdown \"%1\" server?").arg(server->name()),
                                       QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes) {
      return;
    }

    events_info::ShutDownInfoRequest req(this);
    server->shutDown(req);
  }
}

void ExplorerTreeView::loadContentDb() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerDatabaseItem *node = common::utils_qt::item<ExplorerDatabaseItem*>(sel);
  if (node) {
    LoadContentDbDialog loadDb(QString("Load %1 content").arg(node->name()),
                               node->server()->type(), this);
    int result = loadDb.exec();
    if (result == QDialog::Accepted) {
      node->loadContent(common::convertToString(loadDb.pattern()), loadDb.count());
    }
  }
}

void ExplorerTreeView::setDefaultDb() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerDatabaseItem *node = common::utils_qt::item<ExplorerDatabaseItem*>(sel);
  if (node) {
    node->setDefault();
  }
}

void ExplorerTreeView::createKey() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerDatabaseItem *node = common::utils_qt::item<ExplorerDatabaseItem*>(sel);
  if (node) {
    CreateDbKeyDialog loadDb(QString("Create key for %1 database").arg(node->name()),
                             node->server()->type(), this);
    int result = loadDb.exec();
    if (result == QDialog::Accepted) {
      NDbKValue key = loadDb.key();
      node->createKey(key);
    }
  }
}

void ExplorerTreeView::viewKeys() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerDatabaseItem* node = common::utils_qt::item<ExplorerDatabaseItem*>(sel);
  if (node) {
    ViewKeysDialog diag(QString("View key in %1 database").arg(node->name()), node->db(), this);
    diag.exec();
  }
}

void ExplorerTreeView::getValue() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerKeyItem *node = common::utils_qt::item<ExplorerKeyItem*>(sel);
  if (node) {
    node->loadValueFromDb();
  }
}

void ExplorerTreeView::deleteKey() {
  QModelIndex sel = selectedIndex();
  if (!sel.isValid()) {
    return;
  }

  ExplorerKeyItem *node = common::utils_qt::item<ExplorerKeyItem*>(sel);
  if (node) {
    node->removeFromDb();
  }
}

void ExplorerTreeView::startLoadDatabases(const events_info::LoadDatabasesInfoRequest& req) {
}

void ExplorerTreeView::finishLoadDatabases(const events_info::LoadDatabasesInfoResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  IServer *serv = qobject_cast<IServer *>(sender());
  DCHECK(serv);
  if (!serv) {
    return;
  }

  ExplorerTreeModel *mod = qobject_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  events_info::LoadDatabasesInfoResponce::database_info_cont_type dbs = res.databases;

  for (int i = 0; i < dbs.size(); ++i) {
    IDataBaseInfoSPtr db = dbs[i];
    mod->addDatabase(serv, db);
  }
}

void ExplorerTreeView::startSetDefaultDatabase(const events_info::SetDefaultDatabaseRequest& req) {
}

void ExplorerTreeView::finishSetDefaultDatabase(const events_info::SetDefaultDatabaseResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  IServer *serv = qobject_cast<IServer *>(sender());
  DCHECK(serv);
  if (!serv) {
    return;
  }

  IDataBaseInfoSPtr db = res.inf;
  ExplorerTreeModel *mod = qobject_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  mod->setDefaultDb(serv, db);
}

void ExplorerTreeView::startLoadDatabaseContent(const events_info::LoadDatabaseContentRequest& req) {
}

void ExplorerTreeView::finishLoadDatabaseContent(const events_info::LoadDatabaseContentResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  IServer *serv = qobject_cast<IServer *>(sender());
  DCHECK(serv);
  if (!serv) {
    return;
  }

  ExplorerTreeModel *mod = qobject_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  events_info::LoadDatabaseContentResponce::keys_cont_type keys = res.keys;

  for (int i = 0; i < keys.size(); ++i) {
    NDbKValue key = keys[i];
    mod->addKey(serv, res.inf, key);
  }
}

void ExplorerTreeView::startExecuteCommand(const events_info::CommandRequest& req) {
}

void ExplorerTreeView::finishExecuteCommand(const events_info::CommandResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  IServer* serv = qobject_cast<IServer *>(sender());
  DCHECK(serv);
  if (!serv) {
    return;
  }

  ExplorerTreeModel* mod = qobject_cast<ExplorerTreeModel*>(model());
  DCHECK(mod);
  if (!mod) {
    return;
  }

  CommandKeySPtr key = res.cmd;
  NDbKValue dbv = key->key();
  if (key->type() == CommandKey::C_DELETE) {
    mod->removeKey(serv, res.inf, dbv);
  } else if (key->type() == CommandKey::C_CREATE) {
    mod->addKey(serv, res.inf, dbv);
  }
}

void ExplorerTreeView::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }

  QTreeView::changeEvent(e);
}

void ExplorerTreeView::mouseDoubleClickEvent(QMouseEvent* e) {
  if (SettingsManager::instance().fastViewKeys()) {
    getValue();
  }

  QTreeView::mouseDoubleClickEvent(e);
}

void ExplorerTreeView::syncWithServer(IServer* server) {
  if (!server) {
    return;
  }

  VERIFY(connect(server, &IServer::startedLoadDatabases,
                 this, &ExplorerTreeView::startLoadDatabases));
  VERIFY(connect(server, &IServer::finishedLoadDatabases,
                 this, &ExplorerTreeView::finishLoadDatabases));
  VERIFY(connect(server, &IServer::startedSetDefaultDatabase,
                 this, &ExplorerTreeView::startSetDefaultDatabase));
  VERIFY(connect(server, &IServer::finishedSetDefaultDatabase,
                 this, &ExplorerTreeView::finishSetDefaultDatabase));
  VERIFY(connect(server, &IServer::startedLoadDataBaseContent,
                 this, &ExplorerTreeView::startLoadDatabaseContent));
  VERIFY(connect(server, &IServer::finishedLoadDatabaseContent,
                 this, &ExplorerTreeView::finishLoadDatabaseContent));
  VERIFY(connect(server, &IServer::startedExecuteCommand,
                 this, &ExplorerTreeView::startExecuteCommand));
  VERIFY(connect(server, &IServer::finishedExecuteCommand,
                 this, &ExplorerTreeView::finishExecuteCommand));
}

void ExplorerTreeView::unsyncWithServer(IServer* server) {
  if (!server) {
    return;
  }

  VERIFY(disconnect(server, &IServer::startedLoadDatabases,
                    this, &ExplorerTreeView::startLoadDatabases));
  VERIFY(disconnect(server, &IServer::finishedLoadDatabases,
                    this, &ExplorerTreeView::finishLoadDatabases));
  VERIFY(disconnect(server, &IServer::startedSetDefaultDatabase,
                    this, &ExplorerTreeView::startSetDefaultDatabase));
  VERIFY(disconnect(server, &IServer::finishedSetDefaultDatabase,
                    this, &ExplorerTreeView::finishSetDefaultDatabase));
  VERIFY(disconnect(server, &IServer::startedLoadDataBaseContent,
                    this, &ExplorerTreeView::startLoadDatabaseContent));
  VERIFY(disconnect(server, &IServer::finishedLoadDatabaseContent,
                    this, &ExplorerTreeView::finishLoadDatabaseContent));
  VERIFY(disconnect(server, &IServer::startedExecuteCommand,
                    this, &ExplorerTreeView::startExecuteCommand));
  VERIFY(disconnect(server, &IServer::finishedExecuteCommand,
                    this, &ExplorerTreeView::finishExecuteCommand));
}

void ExplorerTreeView::retranslateUi() {
  connectAction_->setText(tr("Connect/Disconnect"));
  openConsoleAction_->setText(translations::trOpenConsole);
  loadDatabaseAction_->setText(translations::trLoadDataBases);
  infoServerAction_->setText(translations::trInfo);
  propertyServerAction_->setText(translations::trProperty);
  setServerPassword_->setText(translations::trSetPassword);
  setMaxClientConnection_->setText(translations::trSetMaxNumberOfClients);
  historyServerAction_->setText(translations::trHistory);
  clearHistoryServerAction_->setText(translations::trClearHistory);
  closeServerAction_->setText(translations::trClose);
  closeClusterAction_->setText(translations::trClose);
  backupAction_->setText(translations::trBackup);
  importAction_->setText(translations::trImport);
  shutdownAction_->setText(translations::trShutdown);

  loadContentAction_->setText(translations::trLoadContOfDataBases);
  createKeyAction_->setText(translations::trCreateKey);
  viewKeysAction_->setText(translations::trViewKeysDialog);
  setDefaultDbAction_->setText(translations::trSetDefault);
  getValueAction_->setText(translations::trValue);
  deleteKeyAction_->setText(translations::trDelete);
}

QModelIndex ExplorerTreeView::selectedIndex() const {
  QModelIndexList indexses = selectionModel()->selectedRows();

  if (indexses.count() != 1) {
    return QModelIndex();
  }

  return indexses[0];
}

}  // namespace fastonosql
