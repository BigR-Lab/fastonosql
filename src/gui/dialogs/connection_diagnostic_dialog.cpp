#include "gui/dialogs/connection_diagnostic_dialog.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QThread>
#include <QLabel>

#include "common/time.h"

#ifdef BUILD_WITH_REDIS
#include "core/redis/redis_driver.h"
#endif

#ifdef BUILD_WITH_MEMCACHED
#include "core/memcached/memcached_driver.h"
#endif

#ifdef BUILD_WITH_SSDB
#include "core/ssdb/ssdb_driver.h"
#endif

#ifdef BUILD_WITH_LEVELDB
#include "core/leveldb/leveldb_driver.h"
#endif

#ifdef BUILD_WITH_ROCKSDB
#include "core/rocksdb/rocksdb_driver.h"
#endif

#include "gui/gui_factory.h"
#include "fasto/qt/gui/glass_widget.h"

#include "translations/global.h"

namespace
{
    const QString timeTemplate = "Time execute msec: %1";
    const QString connectionStatusTemplate = "Connection state: %1";
    const QSize stateIconSize = QSize(64, 64);

    common::ErrorValueSPtr testConnectionTMethod(fastonosql::IConnectionSettingsBaseSPtr connection)
    {
        using namespace fastonosql;
        connectionTypes type = connection->connectionType();
#ifdef BUILD_WITH_REDIS
        if(type == REDIS){
            return testConnection(dynamic_cast<RedisConnectionSettings*>(connection.get()));
        }
#endif
#ifdef BUILD_WITH_MEMCACHED
        if(type == MEMCACHED){
            return testConnection(dynamic_cast<MemcachedConnectionSettings*>(connection.get()));
        }
#endif
#ifdef BUILD_WITH_SSDB
        if(type == SSDB){
            return testConnection(dynamic_cast<SsdbConnectionSettings*>(connection.get()));
        }
#endif
#ifdef BUILD_WITH_LEVELDB
        if(type == LEVELDB){
            return testConnection(dynamic_cast<LeveldbConnectionSettings*>(connection.get()));
        }
#endif
#ifdef BUILD_WITH_ROCKSDB
        if(type == ROCKSDB){
            return testConnection(dynamic_cast<RocksdbConnectionSettings*>(connection.get()));
        }
#endif
        return common::make_error_value("Invalid setting type", common::ErrorValue::E_ERROR);
    }
}

namespace fastonosql
{
    TestConnection::TestConnection(IConnectionSettingsBaseSPtr conn, QObject* parent)
        : QObject(parent), connection_(conn), startTime_(common::time::current_mstime())
    {

    }

    void TestConnection::routine()
    {
        if(!connection_){
            emit connectionResult(false, common::time::current_mstime() - startTime_, "Invalid connection settings");
            return;
        }

        common::ErrorValueSPtr er = testConnectionTMethod(connection_);

        if(er){
            emit connectionResult(false, common::time::current_mstime() - startTime_, common::convertFromString<QString>(er->description()));
        }
        else{
            emit connectionResult(true, common::time::current_mstime() - startTime_, "Success");
        }
    }

    ConnectionDiagnosticDialog::ConnectionDiagnosticDialog(QWidget* parent, IConnectionSettingsBaseSPtr connection)
        : QDialog(parent)
    {
        using namespace translations;

        setWindowTitle(trConnectionDiagnostic);
        setWindowIcon(GuiFactory::instance().icon(connection->connectionType()));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)

        QVBoxLayout* mainLayout = new QVBoxLayout;

        executeTimeLabel_ = new QLabel;
        executeTimeLabel_->setText(connectionStatusTemplate.arg("execute..."));
        mainLayout->addWidget(executeTimeLabel_);

        statusLabel_ = new QLabel(timeTemplate.arg("calculate..."));
        statusLabel_->setWordWrap(true);
        iconLabel_ = new QLabel;
        QIcon icon = GuiFactory::instance().failIcon();
        const QPixmap pm = icon.pixmap(stateIconSize);
        iconLabel_->setPixmap(pm);

        mainLayout->addWidget(statusLabel_);
        mainLayout->addWidget(iconLabel_, 1, Qt::AlignCenter);

        QDialogButtonBox* buttonBox = new QDialogButtonBox;
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        VERIFY(connect(buttonBox, &QDialogButtonBox::accepted, this, &ConnectionDiagnosticDialog::accept));

        mainLayout->addWidget(buttonBox);
        setFixedSize(QSize(fix_width, fix_height));
        setLayout(mainLayout);

        glassWidget_ = new fasto::qt::gui::GlassWidget(GuiFactory::instance().pathToLoadingGif(), trTryToConnect, 0.5, QColor(111, 111, 100), this);
        testConnection(connection);
    }

    void ConnectionDiagnosticDialog::connectionResult(bool suc, qint64 mstimeExecute, const QString& resultText)
    {
        glassWidget_->stop();

        executeTimeLabel_->setText(timeTemplate.arg(mstimeExecute));
        if(suc){
            QIcon icon = GuiFactory::instance().successIcon();
            const QPixmap pm = icon.pixmap(stateIconSize);
            iconLabel_->setPixmap(pm);
        }
        statusLabel_->setText(connectionStatusTemplate.arg(resultText));
    }

    void ConnectionDiagnosticDialog::showEvent(QShowEvent* e)
    {
        QDialog::showEvent(e);
        glassWidget_->start();
    }

    void ConnectionDiagnosticDialog::testConnection(IConnectionSettingsBaseSPtr connection)
    {
        QThread* th = new QThread;
        TestConnection* cheker = new TestConnection(connection);
        cheker->moveToThread(th);
        VERIFY(connect(th, &QThread::started, cheker, &TestConnection::routine));
        VERIFY(connect(cheker, &TestConnection::connectionResult, this, &ConnectionDiagnosticDialog::connectionResult));
        VERIFY(connect(cheker, &TestConnection::connectionResult, th, &QThread::quit));
        VERIFY(connect(th, &QThread::finished, cheker, &TestConnection::deleteLater));
        VERIFY(connect(th, &QThread::finished, th, &QThread::deleteLater));
        th->start();
    }
}
