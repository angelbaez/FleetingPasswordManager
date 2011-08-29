// This file is part of Fleeting Password Manager (Fleetingpm).
// Copyright (C) 2011 Jussi Lind <jussi.lind@iki.fi>
//
// Fleetingpm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Fleetingpm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fleetingpm. If not, see <http://www.gnu.org/licenses/>.
//

#include "aboutdlg.h"
#include "config.h"
#include "engine.h"
#include "instructionsdlg.h"
#include "loginio.h"
#include "mainwindow.h"
#include "settingsdlg.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimeLine>

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_defaultDelay(5)
, m_defaultLength(8)
, m_removeText(tr("&Remove URL && User"))
, m_rememberText(tr("&Remember URL && User"))
, m_rememberToolTip(tr("Remember current URL & User. Passwords are not saved."))
, m_removeToolTip(tr("Don't remember current URL & User anymore."))
, m_masterPasswordRedText(tr("<b><font color=#aa0000>Master password:</font></b>"))
, m_masterPasswordGreenText(tr("<b><font color=#00aa00>Master password:</font></b>"))
, m_delay(m_defaultDelay)
, m_length(m_defaultLength)
, m_autoCopy(false)
, m_autoClear(false)
, m_masterEdit(new QLineEdit(this))
, m_masterLabel(new QLabel(this))
, m_userEdit(new QLineEdit(this))
, m_passwdEdit(new QLineEdit(this))
, m_urlCombo(new QComboBox(this))
, m_genButton(new QPushButton(tr("&Show password:"), this))
, m_rmbButton(new QPushButton(m_rememberText, this))
, m_lengthSpinBox(new QSpinBox(this))
, m_timeLine(new QTimeLine())
, m_settingsDlg(new SettingsDlg(this))
{
    setWindowTitle("Fleeting Password Manager");
    setWindowIcon(QIcon(":/fleetingpm.png"));
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    // Make the size fixed
    resize(QSize(Config::MAINWINDOW_WIDTH, Config::MAINWINDOW_HEIGHT));
    setMaximumSize(size());
    setMinimumSize(size());

    initWidgets();
    initMenu();
    initBackground();
    loadSettings();

    // Initialize the timer used when fading out the
    // generated password
    m_timeLine->setDuration(m_delay * 1000);
    connect(m_timeLine, SIGNAL(frameChanged(int)), this, SLOT(decreasePasswordAlpha(int)));
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(invalidate()));
    m_timeLine->setFrameRange(0, 255);

    // Load previous location or center the window.
    centerOrRestoreLocation();
}

void MainWindow::centerOrRestoreLocation()
{
    // Calculate center coordinates
    QRect geom(QApplication::desktop()->availableGeometry());
    int centerX = geom.width()  / 2 - frameGeometry().width() / 2;
    int centerY = geom.height() / 2 -frameGeometry().height() / 2;

    // Try to load previous location and use the
    // calculated center as the fallback
    QSettings s(Config::COMPANY, Config::SOFTWARE);
    int x = s.value("x", centerX).toInt();
    int y = s.value("y", centerY).toInt();
    move(x, y);
}

void MainWindow::initBackground()
{
     QPalette palette = QPalette();
     palette.setBrush(QPalette::Window, QPixmap(":back.png"));
     setPalette(palette);
     setAutoFillBackground(true);
}

void MainWindow::initWidgets()
{
    // Create main layout as a grid layout
    // No need to store as a member.
    QGridLayout * layout = new QGridLayout();

    // Create a horizontal line between the user name field
    // and the password field.
    // No need to store as a member.
    QFrame * frame = new QFrame(this);
    frame->setFrameShape(QFrame::HLine);

    // Create a star image / logo as a label
    // No need to store as a member.
    QLabel * starsLabel = new QLabel();
    starsLabel->setPixmap(QPixmap(":/stars.png"));

    // Create and connect the quit-button
    // No need to store as a member.
    QPushButton * quitButton = new QPushButton(tr("&Quit"));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    // Set tooltip for the master password field
    m_masterEdit->setToolTip(tr("Enter the master password common to all of your logins.\n"
                                "Note that the master password is not validated: different\n"
                                "master passwords just result in different "
                                "generated passwords."));

    // Set password-styled echo mode
    m_masterEdit->setEchoMode(QLineEdit::Password);

    // Make the URL combo box editable
    m_urlCombo->setEditable(true);

    // Set tooltip for the URL combo box
    m_urlCombo->setToolTip(tr("Enter or select a saved URL/ID.\n"
                              "For example \"facebook\", \"google\","
                              "\"gmail.com\", \"myserver\".."));

    // Set tooltip for the username combo box
    m_userEdit->setToolTip(tr("Enter your user name corresponding to the selected URL/ID."));

    // Set range from 8 to 32 for the password length spin box
    m_lengthSpinBox->setRange(8, 32);

    // Set tooltip for the password length spin box
    m_lengthSpinBox->setToolTip(tr("The length of the generated password."));

    // Set tooltip for the password field
    m_passwdEdit->setToolTip(tr("This is the generated password,\n"
                                "which is always the same with the same master password,\n"
                                "URL/ID and user name."));

    // Make the password line edit read-only and disabled by default
    m_passwdEdit->setReadOnly(true);
    m_passwdEdit->setEnabled(false);

    // Set the generate-button disabled by default
    m_genButton->setEnabled(false);

    // Set tooltip for the generate-button
    m_genButton->setToolTip(tr("Generate and show the password"));

    // Set the remember-button disabled by default
    m_rmbButton->setEnabled(false);

    // Set tooltip for the remember-button
    m_rmbButton->setToolTip(m_rememberToolTip);

    // Add the widgets to the grid layout
    const int COLS = 5;
    layout->addWidget(m_masterEdit,    0, 1, 1, COLS);
    layout->addWidget(m_urlCombo,      1, 1, 1, COLS);
    layout->addWidget(m_userEdit,      2, 1, 1, COLS - 1);
    layout->addWidget(m_lengthSpinBox, 2, COLS);
    layout->addWidget(frame,           3, 1, 1, COLS);
    layout->addWidget(m_genButton,     4, 0);
    layout->addWidget(m_passwdEdit,    4, 1, 1, COLS);
    layout->addWidget(starsLabel,      5, 0);
    layout->addWidget(m_rmbButton,     5, 1, 1, COLS - 1);
    layout->addWidget(quitButton,      5, COLS);

    // Add the "master password:"-label to the layout
    m_masterLabel->setText(m_masterPasswordRedText);
    layout->addWidget(m_masterLabel, 0, 0);

    // Create and add the "URL/ID:"-label to the layout
    // No need to store as a member.
    layout->addWidget(new QLabel(tr("<b>URL/ID:</b>")), 1, 0);

    // Create and add the "User name:"-label to the layout
    // No need to store as a member.
    layout->addWidget(new QLabel(tr("<b>User name:</b>")), 2, 0);

    // Create the central widget and set the layout to it.
    // No need to store as a member.
    QWidget * dummy = new QWidget();
    dummy->setLayout(layout);
    setCentralWidget(dummy);

    // Connect the rest of the signals emitted by the widgets
    connectSignalsFromWidgets();
}

void MainWindow::connectSignalsFromWidgets()
{
    // Connect signal to update user name field when a URL gets
    // selected in the combo box
    connect(m_urlCombo, SIGNAL(activated(const QString &)),
            this, SLOT(updateUser(const QString &)));

    // Decide the text of the remember/remove-button if URL-field is changed
    connect(m_urlCombo, SIGNAL(editTextChanged(const QString &)),
            this, SLOT(setRmbButtonText(const QString &)));

    // Remember of remove a saved login when remember/remove-button is clicked
    connect(m_rmbButton, SIGNAL(clicked()), this, SLOT(rememberOrRemoveLogin()));

    // Generate the password when generate-button is clicked
    connect(m_genButton, SIGNAL(clicked()), this, SLOT(doGenerate()));

    // Invalidate generated password if one of the inputs gets changed
    connect(m_masterEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(invalidate()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)),
            this, SLOT(invalidate()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(invalidate()));

    // Enable generate-button if all inputs are valid
    connect(m_masterEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableGenButton()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableGenButton()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableGenButton()));

    // Enable remember-button if the URL not already saved
    connect(m_masterEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableRmbButton()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableRmbButton()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(enableRmbButton()));

    // Set "master password"-label color
    connect(m_masterEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(setMasterPasswordLabelColor()));
}

void MainWindow::initMenu()
{
    // Add file menu
    QMenu * fileMenu = menuBar()->addMenu(tr("&File"));

    // Add action for importing logins
    QAction * importAct = new QAction(tr("&Import logins.."), fileMenu);
    connect(importAct, SIGNAL(triggered()), this, SLOT(importLogins()));
    fileMenu->addAction(importAct);

    // Add action for exporting logins
    QAction * exportAct = new QAction(tr("&Export logins.."), fileMenu);
    connect(exportAct, SIGNAL(triggered()), this, SLOT(exportLogins()));
    fileMenu->addAction(exportAct);

    // Add action for settings
    QAction * setAct = new QAction(tr("&Settings.."), fileMenu);
    connect(setAct, SIGNAL(triggered()), this, SLOT(showSettingsDlg()));
    fileMenu->addAction(setAct);

    // Add action for quit
    QAction * quitAct = new QAction(tr("&Quit"), fileMenu);
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(quitAct);

    // Add help menu
    QMenu * helpMenu = menuBar()->addMenu(tr("&Help"));

    // Add action for instructions
    QAction * instructionsAct = new QAction(tr("&Instructions.."), helpMenu);
    connect(instructionsAct, SIGNAL(triggered()), this, SLOT(showInstructionsDlg()));
    helpMenu->addAction(instructionsAct);

    // Add action for about
    QAction * aboutAct = new QAction(tr("&About ") + windowTitle() + "..", helpMenu);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(showAboutDlg()));
    helpMenu->addAction(aboutAct);

    // Add action for about Qt
    QAction * aboutQtAct = new QAction(tr("About &Qt.."), helpMenu);
    connect(aboutQtAct, SIGNAL(triggered()), this, SLOT(showAboutQtDlg()));
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::showSettingsDlg()
{
    m_settingsDlg->exec();
    m_settingsDlg->getSettings(m_delay, m_length, m_autoCopy, m_autoClear);
    m_timeLine->setDuration(m_delay * 1000);
    saveSettings();
}

void MainWindow::importLogins()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import logins"),
                                                    QDir::homePath(),
                                                    tr("Fleeting Password Manager files (*.fpm)"));
    if (fileName.length() > 0)
    {
        int newLogins = 0;
        int updated   = 0;

        LoginIO::LoginList logins;
        if (LoginIO::importLogins(logins, fileName))
        {
            for (int i = 0; i < logins.count(); i++)
            {
                QString url  = logins.at(i).first;
                QString user = logins.at(i).second;

                int index = m_urlCombo->findText(url);
                if (index != -1)
                {
                    m_urlCombo->setItemData(index, user);
                    updated++;
                }
                else
                {
                    m_urlCombo->addItem(url, user);
                    newLogins++;
                }
            }

            m_urlCombo->model()->sort(0);
            saveSettings();

            QString message(tr("Successfully imported logins from '") +
                            fileName + tr("': %1 new, %2 updated."));
            message = message.arg(newLogins).arg(updated);
            QMessageBox::information(this, tr("Importing logins succeeded"), message);
        }
        else
        {
            QMessageBox::warning(this, tr("Exporting logins failed"),
                                 tr("Failed to import logins from '") + fileName + "'");
        }
    }
}

void MainWindow::exportLogins()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export logins"),
                                                    QDir::homePath(),
                                                    tr("Fleeting Password Manager files (*.fpm)"));
    if (fileName.length() > 0)
    {
        if (!fileName.endsWith(".fpm"))
            fileName.append(".fpm");

        LoginIO::LoginList logins;
        for (int i = 0; i < m_urlCombo->count(); i++)
            logins << QPair<QString, QString>(m_urlCombo->itemText(i),
                                              m_urlCombo->itemData(i).toString());

        if (LoginIO::exportLogins(logins, fileName))
        {
            QMessageBox::information(this, tr("Exporting logins succeeded"),
                                     tr("Successfully exported logins to '") + fileName + "'");
        }
        else
        {
            QMessageBox::warning(this, tr("Exporting logins failed"),
                                 tr("Failed to export logins to '") + fileName + "'");
        }
    }
}

void MainWindow::showInstructionsDlg()
{
    InstructionsDlg instructionsDlg(this);
    instructionsDlg.exec();
}

void MainWindow::showAboutDlg()
{
    AboutDlg aboutDlg(this);
    aboutDlg.exec();
}

void MainWindow::showAboutQtDlg()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::loadSettings()
{
    QSettings s(Config::COMPANY, Config::SOFTWARE);
    m_delay     = s.value("delay", m_defaultDelay).toInt();
    m_length    = s.value("length", m_defaultLength).toInt();
    m_autoCopy  = s.value("autoCopy", false).toBool();
    m_autoClear = s.value("autoClear", false).toBool();

    m_settingsDlg->setSettings(m_delay, m_length, m_autoCopy, m_autoClear);
    m_timeLine->setDuration(m_delay * 1000);

    // Read login data
    m_urlCombo->clear();
    m_loginHash.clear();
    int size = s.beginReadArray("logins");
    for (int i = 0; i < size; i++)
    {
        s.setArrayIndex(i);

        const QString url    = s.value("url").toString();
        const QString user   = s.value("user").toString();
        const int     length = s.value("length", m_length).toInt();

        m_urlCombo->addItem(url);
        m_loginHash[url] = LoginData(url, user, length);
    }
    s.endArray();

    m_urlCombo->model()->sort(0);
    updateUser(m_urlCombo->currentText());
}

void MainWindow::saveSettings()
{
    QSettings s(Config::COMPANY, Config::SOFTWARE);
    s.setValue("delay",     m_delay);
    s.setValue("length",    m_length);
    s.setValue("autoCopy",  m_autoCopy);
    s.setValue("autoClear", m_autoClear);

    // Write login data that user wants to be saved
    s.beginWriteArray("logins");
    QList<LoginData> values = m_loginHash.values();
    for (int i = 0; i < values.count(); i++)
    {
        s.setArrayIndex(i);
        s.setValue("url",  values.at(i).url());
        s.setValue("user", values.at(i).userName());

        if (values.at(i).passwordLength() != m_length)
        {
            s.setValue("length", values.at(i).passwordLength());
        }
    }
    s.endArray();
}

void MainWindow::doGenerate()
{
    QString passwd = Engine::generate(m_masterEdit->text(),
                                      m_urlCombo->currentText(),
                                      m_userEdit->text(),
                                      m_length);

    // Enable the text field and  show the generated passwd
    m_passwdEdit->setEnabled(true);
    m_passwdEdit->setText(passwd);

    // Copy to clipboard if wanted
    if (m_autoCopy)
    {
        m_passwdEdit->selectAll();
        m_passwdEdit->copy();
    }

    // Start timer to slowly fade out the text
    m_timeLine->stop();
    m_timeLine->start();
}

void MainWindow::decreasePasswordAlpha(int frame)
{
    QColor color = QColor();
    color.setAlpha(255 - frame);
    QPalette palette = QPalette(m_passwdEdit->palette());
    palette.setColor(QPalette::Text, color);
    m_passwdEdit->setPalette(palette);
}

void MainWindow::invalidate()
{
    // Clear the password edit
    m_passwdEdit->setText("");

    // Clear the clipboard
    if (m_autoClear)
    {
        QApplication::clipboard()->clear();
    }

    m_passwdEdit->setEnabled(false);
}

void MainWindow::enableGenButton()
{
    m_genButton->setEnabled(m_masterEdit->text().length()      > 0 &&
                            m_urlCombo->currentText().length() > 0 &&
                            m_userEdit->text().length()        > 0);
}

void MainWindow::enableRmbButton()
{
    m_rmbButton->setEnabled(m_urlCombo->currentText().length() > 0 &&
                            m_userEdit->text().length()        > 0);
}

void MainWindow::setMasterPasswordLabelColor()
{
    if (m_masterEdit->text().length() > 0)
    {
        m_masterLabel->setText(m_masterPasswordGreenText);
    }
    else
    {
        m_masterLabel->setText(m_masterPasswordRedText);
    }
}

void MainWindow::rememberOrRemoveLogin()
{
    QString user = m_userEdit->text();
    QString url  = m_urlCombo->currentText();

    if (m_rmbButton->text() == m_rememberText)
    {
        // Remember url and user
        int index = m_urlCombo->findText(url);
        if (index == -1)
        {
            m_urlCombo->addItem(url);
            m_urlCombo->model()->sort(0);
        }

        m_loginHash[url] = LoginData(url, user, m_lengthSpinBox->value());

        saveSettings();

        m_rmbButton->setText(m_removeText);
    }
    else
    {
        // Remove url and user
        int index = m_urlCombo->findText(url);
        if (index != -1)
        {
            m_urlCombo->removeItem(index);
        }

        m_loginHash.remove(url);

        saveSettings();

        m_rmbButton->setText(m_rememberText);
    }
}

void MainWindow::updateUser(const QString & url)
{
    int index = m_urlCombo->findText(url);
    if (index != -1)
    {
        m_userEdit->setText(m_loginHash.value(m_urlCombo->itemText(index)).userName());
        m_rmbButton->setText(m_removeText);
    }
}

void MainWindow::setRmbButtonText(const QString & url)
{
    int index = m_urlCombo->findText(url);
    if (index != -1)
    {
        m_userEdit->setText(m_loginHash.value(m_urlCombo->itemText(index)).userName());
        m_rmbButton->setText(m_removeText);
        m_rmbButton->setToolTip(m_removeToolTip);
    }
    else
    {
        m_rmbButton->setText(m_rememberText);
        m_rmbButton->setToolTip(m_rememberToolTip);
    }
}

void MainWindow::closeEvent(QCloseEvent * event)
{
    QSettings s(Config::COMPANY, Config::SOFTWARE);
    s.setValue("x", x());
    s.setValue("y", y());

    QApplication::clipboard()->clear();
    event->accept();
}

MainWindow::~MainWindow()
{
    delete m_timeLine;
}
