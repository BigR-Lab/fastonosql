#include "gui/dialogs/connection_dialog.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>

#include "gui/dialogs/connection_diagnostic_dialog.h"

#include "common/qt/convert_string.h"

#include "gui/gui_factory.h"

#include "translations/global.h"

namespace
{
    QString stableCommandLine(QString input)
    {
        return input.replace('\n', "\\n");
    }

    QString toRawCommandLine(QString input)
    {
        return input.replace("\\n", "\n");
    }

    const QString defaultNameConnection = "New Connection";
}

namespace fastonosql
{
    ConnectionDialog::ConnectionDialog(QWidget* parent, IConnectionSettingsBase* connection)
        : QDialog(parent), connection_(connection)
    {
        using namespace translations;

        setWindowIcon(GuiFactory::instance().serverIcon());
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)

        connectionName_ = new QLineEdit;
        QString conName = defaultNameConnection;
        if(connection_){
            conName = common::convertFromString<QString>(connection_->connectionName());
        }
        connectionName_->setText(conName);
        typeConnection_ = new QComboBox;

        for(int i = 0; i < SIZEOFMASS(connnectionType); ++i){
            connectionTypes ct = static_cast<connectionTypes>(i);
            std::string str = common::convertToString(ct);
            typeConnection_->addItem(GuiFactory::instance().icon(ct), common::convertFromString<QString>(str), i);
        }

        if(connection_){
            typeConnection_->setCurrentIndex(connection_->connectionType());
        }

        typedef void (QComboBox::*qind)(int);
        VERIFY(connect(typeConnection_, static_cast<qind>(&QComboBox::currentIndexChanged), this, &ConnectionDialog::typeConnectionChange));

        logging_ = new QCheckBox;
        if(connection_){
            logging_->setChecked(connection_->loggingEnabled());
        }
        else{
            logging_->setChecked(false);
        }

        commandLine_ = new QLineEdit;
        commandLine_->setMinimumWidth(240);
        if(connection_){
            commandLine_->setText(stableCommandLine(common::convertFromString<QString>(connection_->commandLine())));
        }

        QVBoxLayout *inputLayout = new QVBoxLayout;
        inputLayout->addWidget(connectionName_);
        inputLayout->addWidget(typeConnection_);
        inputLayout->addWidget(logging_);
        inputLayout->addWidget(commandLine_);

        SSHInfo info;
        if(connection_){
            info = connection_->sshInfo();
        }
        useSsh_ = new QCheckBox;
        useSsh_->setChecked(info.isValid());

        sshHostName_ = new QLineEdit;
        sshHostName_->setText(common::convertFromString<QString>(info.hostName_));

        userName_ = new QLineEdit;
        userName_->setText(common::convertFromString<QString>(info.userName_));

        sshPort_ = new QLineEdit;
        sshPort_->setFixedWidth(80);
        QRegExp rx("\\d+");//(0-65554)
        sshPort_->setValidator(new QRegExpValidator(rx, this));
        sshPort_->setText(QString::number(info.port_));

        passwordLabel_ = new QLabel;
        sshPrivateKeyLabel_ = new QLabel;
        sshPassphraseLabel_ = new QLabel;
        sshAddressLabel_ = new QLabel;
        sshUserNameLabel_ = new QLabel;
        sshAuthMethodLabel_ = new QLabel;

        security_ = new QComboBox;
        security_->addItems(QStringList() << trPassword << trPrivateKey);
        if (info.authMethod() == SSHInfo::PUBLICKEY) {
            security_->setCurrentText(trPrivateKey);
        }
        else {
            security_->setCurrentText(trPassword);
        }

        typedef void (QComboBox::*ind)(const QString&);
        VERIFY(connect(security_, static_cast<ind>(&QComboBox::currentIndexChanged), this, &ConnectionDialog::securityChange));

        passwordBox_ = new QLineEdit;
        passwordBox_->setText(common::convertFromString<QString>(info.password_));
        passwordBox_->setEchoMode(QLineEdit::Password);
        passwordEchoModeButton_ = new QPushButton(trShow);
        VERIFY(connect(passwordEchoModeButton_, &QPushButton::clicked, this, &ConnectionDialog::togglePasswordEchoMode));

        privateKeyBox_ = new QLineEdit;
        privateKeyBox_->setText(common::convertFromString<QString>(info.privateKey_));

        passphraseBox_ = new QLineEdit;
        passphraseBox_->setText(common::convertFromString<QString>(info.passphrase_));
        passphraseBox_->setEchoMode(QLineEdit::Password);
        passphraseEchoModeButton_ = new QPushButton(trShow);
        VERIFY(connect(passphraseEchoModeButton_, &QPushButton::clicked, this, &ConnectionDialog::togglePassphraseEchoMode));

        useSshWidget_ = new QWidget;

        QHBoxLayout *hostAndPasswordLayout = new QHBoxLayout;
        hostAndPasswordLayout->addWidget(sshHostName_);
        hostAndPasswordLayout->addWidget(new QLabel(":"));
        hostAndPasswordLayout->addWidget(sshPort_);

        QGridLayout *sshWidgetLayout = new QGridLayout;
        sshWidgetLayout->setAlignment(Qt::AlignTop);
        sshWidgetLayout->setColumnStretch(1, 1);
        sshWidgetLayout->addWidget(sshAddressLabel_ , 1, 0);
        sshWidgetLayout->addLayout(hostAndPasswordLayout, 1, 1, 1, 2);
        sshWidgetLayout->addWidget(sshUserNameLabel_, 2, 0);
        sshWidgetLayout->addWidget(userName_, 2, 1, 1, 2);
        sshWidgetLayout->addWidget(sshAuthMethodLabel_, 4, 0);
        sshWidgetLayout->addWidget(security_, 4, 1, 1, 2);
        sshWidgetLayout->addWidget(passwordLabel_, 5, 0);
        sshWidgetLayout->addWidget(passwordBox_, 5, 1);
        sshWidgetLayout->addWidget(passwordEchoModeButton_, 5, 2);
        sshWidgetLayout->addWidget(sshPrivateKeyLabel_, 7, 0);
        sshWidgetLayout->addWidget(privateKeyBox_, 7, 1);
        selectPrivateFileButton_ = new QPushButton("...");
        selectPrivateFileButton_->setFixedSize(20, 20);
        sshWidgetLayout->addWidget(selectPrivateFileButton_, 7, 2);
        sshWidgetLayout->addWidget(sshPassphraseLabel_, 8, 0);
        sshWidgetLayout->addWidget(passphraseBox_, 8, 1);
        sshWidgetLayout->addWidget(passphraseEchoModeButton_, 8, 2);
        useSshWidget_->setLayout(sshWidgetLayout);

        inputLayout->addWidget(useSsh_);

        VERIFY(connect(selectPrivateFileButton_, &QPushButton::clicked, this, &ConnectionDialog::setPrivateFile));
        VERIFY(connect(useSsh_, &QCheckBox::stateChanged, this, &ConnectionDialog::sshSupportStateChange));

        testButton_ = new QPushButton("&Test");
        testButton_->setIcon(GuiFactory::instance().messageBoxInformationIcon());
        VERIFY(connect(testButton_, &QPushButton::clicked, this, &ConnectionDialog::testConnection));

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(testButton_, 1, Qt::AlignLeft);
        buttonBox_ = new QDialogButtonBox(this);
        buttonBox_->setOrientation(Qt::Horizontal);
        buttonBox_->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox_, &QDialogButtonBox::accepted, this, &ConnectionDialog::accept));
        VERIFY(connect(buttonBox_, &QDialogButtonBox::rejected, this, &ConnectionDialog::reject));
        bottomLayout->addWidget(buttonBox_);


        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(inputLayout);
        mainLayout->addWidget(useSshWidget_);
        mainLayout->addLayout(bottomLayout);
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        setLayout(mainLayout);

        //update controls
        sshSupportStateChange(useSsh_->checkState());
        securityChange(security_->currentText());
        typeConnectionChange(typeConnection_->currentIndex());
        retranslateUi();
    }

    void ConnectionDialog::setConnectionTypeOnly(connectionTypes type)
    {
        typeConnection_->clear();
        std::string str = common::convertToString(type);
        typeConnection_->addItem(GuiFactory::instance().icon(type), common::convertFromString<QString>(str), type);
    }

    IConnectionSettingsBaseSPtr ConnectionDialog::connection() const
    {
        return connection_;
    }

    void ConnectionDialog::accept()
    {
        if(validateAndApply()){
            QDialog::accept();
        }
    }

    void ConnectionDialog::typeConnectionChange(int index)
    {
        QVariant var = typeConnection_->itemData(index);
        connectionTypes currentType = (connectionTypes)qvariant_cast<unsigned char>(var);
        bool isValidType = currentType != DBUNKNOWN;
        connectionName_->setEnabled(isValidType);
        commandLine_->setEnabled(isValidType);
        buttonBox_->button(QDialogButtonBox::Save)->setEnabled(isValidType);

        const char* helpText = useHelpText(currentType);
        if(helpText){
            QString trHelp = tr(helpText);
            commandLine_->setToolTip(trHelp);
        }

        QString text = commandLine_->text();
        if(text.isEmpty()){
            QString deft = stableCommandLine(common::convertFromString<QString>(defaultCommandLine(currentType)));
            commandLine_->setText(deft);
        }

        useSsh_->setEnabled(isValidType);
        updateSshControls(isValidType);
        testButton_->setEnabled(isValidType);
        logging_->setEnabled(isValidType);
    }

    void ConnectionDialog::securityChange(const QString& )
    {
        bool isKey = selectedAuthMethod() == SSHInfo::PUBLICKEY;
        sshPrivateKeyLabel_->setVisible(isKey);
        privateKeyBox_->setVisible(isKey);
        selectPrivateFileButton_->setVisible(isKey);
        sshPassphraseLabel_->setVisible(isKey);
        passphraseBox_->setVisible(isKey);
        passphraseEchoModeButton_->setVisible(isKey);
        passwordBox_->setVisible(!isKey);
        passwordLabel_->setVisible(!isKey);
        passwordEchoModeButton_->setVisible(!isKey);
    }

    void ConnectionDialog::sshSupportStateChange(int value)
    {
        useSshWidget_->setVisible(value);
        updateSshControls(value);
    }

    void ConnectionDialog::togglePasswordEchoMode()
    {
        using namespace translations;

        bool isPassword = passwordBox_->echoMode() == QLineEdit::Password;
        passwordBox_->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        passwordEchoModeButton_->setText(isPassword ? trHide: trShow);
    }

    void ConnectionDialog::togglePassphraseEchoMode()
    {
        using namespace translations;

        bool isPassword = passphraseBox_->echoMode() == QLineEdit::Password;
        passphraseBox_->setEchoMode(isPassword ? QLineEdit::Normal: QLineEdit::Password);
        passphraseEchoModeButton_->setText(isPassword ? trHide: trShow);
    }

    void ConnectionDialog::setPrivateFile()
    {
        QString filepath = QFileDialog::getOpenFileName(this, tr("Select private key file"),
        privateKeyBox_->text(), tr("Private key files (*.*)"));
        if (filepath.isNull())
            return;

        privateKeyBox_->setText(filepath);
    }

    void ConnectionDialog::testConnection()
    {
        if(validateAndApply()){
            ConnectionDiagnosticDialog diag(this, connection_);
            diag.exec();
        }
    }

    void ConnectionDialog::changeEvent(QEvent* e)
    {
        if(e->type() == QEvent::LanguageChange){
            retranslateUi();
        }
        QDialog::changeEvent(e);
    }

    void ConnectionDialog::retranslateUi()
    {
        setWindowTitle(tr("Connection Settings"));
        logging_->setText(tr("Logging enabled"));
        useSsh_->setText(tr("Use SSH tunnel"));
        passwordLabel_->setText(tr("User Password:"));
        sshPrivateKeyLabel_->setText(tr("Private key:"));
        sshPassphraseLabel_->setText(tr("Passphrase:"));
        sshAddressLabel_->setText(tr("SSH Address:"));
        sshUserNameLabel_->setText(tr("SSH User Name:"));
        sshAuthMethodLabel_->setText(tr("SSH Auth Method:"));
    }

    bool ConnectionDialog::validateAndApply()
    {
        connectionTypes currentType = common::convertFromString<connectionTypes>(common::convertToString(typeConnection_->currentText()));
        bool isValidType = currentType != DBUNKNOWN;
        if(isValidType){
            std::string conName = common::convertToString(connectionName_->text());
            IConnectionSettingsBase* newConnection = IConnectionSettingsBase::createFromType(currentType, conName);
            if(newConnection){
                connection_.reset(newConnection);
                connection_->setCommandLine(common::convertToString(toRawCommandLine(commandLine_->text())));
                connection_->setLoggingEnabled(logging_->isChecked());

                SSHInfo info = connection_->sshInfo();
                info.hostName_ = common::convertToString(sshHostName_->text());
                info.userName_ = common::convertToString(userName_->text());
                info.port_ = sshPort_->text().toInt();
                info.password_ = common::convertToString(passwordBox_->text());
                info.publicKey_ = "";
                info.privateKey_ = common::convertToString(privateKeyBox_->text());
                info.passphrase_ = common::convertToString(passphraseBox_->text());
                if (useSsh_->isChecked()){
                    info.currentMethod_ = selectedAuthMethod();
                }
                else{
                    info.currentMethod_ = SSHInfo::UNKNOWN;
                }
                connection_->setSshInfo(info);
            }
            return true;
        }
        else{
            using namespace translations;
            QMessageBox::critical(this, trError, QObject::tr("Invalid database type!"));
            return false;
        }
    }

    SSHInfo::SupportedAuthenticationMetods ConnectionDialog::selectedAuthMethod() const
    {
        using namespace translations;

        if (security_->currentText() == trPrivateKey){
            return SSHInfo::PUBLICKEY;
        }

        return SSHInfo::PASSWORD;
    }

    void ConnectionDialog::updateSshControls(bool isValidType)
    {
        sshHostName_->setEnabled(isValidType);
        userName_->setEnabled(isValidType);
        sshPort_->setEnabled(isValidType);
        security_->setEnabled(isValidType);
        sshPrivateKeyLabel_->setEnabled(isValidType);
        privateKeyBox_->setEnabled(isValidType);
        selectPrivateFileButton_->setEnabled(isValidType);
        sshAddressLabel_->setEnabled(isValidType);
        sshUserNameLabel_->setEnabled(isValidType);
        sshAuthMethodLabel_->setEnabled(isValidType);
        sshPassphraseLabel_->setEnabled(isValidType);
        passphraseBox_->setEnabled(isValidType);
        passwordBox_->setEnabled(isValidType);
        passwordLabel_->setEnabled(isValidType);
    }
}
