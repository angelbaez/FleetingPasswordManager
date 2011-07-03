// This file is part of Fleeting Password Manager (FleetingPM).
// Copyright (C) 2011 Jussi Lind <jussi.lind@iki.fi>
//
// FleetingPM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// FleetingPM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FleetingPM. If not, see <http://www.gnu.org/licenses/>.
//

#include "mainwindow.h"
#include "settingsdlg.h"
#include "engine.h"

#include <QAction>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QTimeLine>

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent)
, m_defaultDelay(5)
, m_defaultLength(8)
, m_removeText(tr("&Remove URL && User"))
, m_rememberText(tr("&Remember URL && User"))
, m_rememberToolTip(tr("Remember current URL & User. Passwords are not saved."))
, m_removeToolTip(tr("Don't remember current URL & User anymore."))
, m_delay(m_defaultDelay)
, m_length(m_defaultLength)
, m_autoCopy(false)
, m_masterEdit(new QLineEdit(this))
, m_userEdit(new QLineEdit(this))
, m_passwdEdit(new QLineEdit(this))
, m_urlCombo(new QComboBox(this))
, m_genButton(new QPushButton(tr("&Show password:"), this))
, m_rmbButton(new QPushButton(m_rememberText, this))
, m_engine(new Engine())
, m_timeLine(new QTimeLine())
{
    setWindowTitle("Fleeting Password Manager");
    setWindowIcon(QIcon(":/fleetingpm.png"));
    resize(QSize(452, 208));
    setMaximumSize(size());
    setMinimumSize(size());

    initWidgets();
    initMenu();
    initBackground();
    loadSettings();

    m_timeLine->setDuration(m_delay * 1000);
    connect(m_timeLine, SIGNAL(frameChanged(int)), this, SLOT(decreasePasswordAlpha(int)));
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(invalidate()));
    m_timeLine->setFrameRange(0, 255);
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
    QGridLayout * layout = new QGridLayout();
    m_masterEdit->setToolTip(tr("Enter the master password common to all of your logins."));
    m_masterEdit->setEchoMode(QLineEdit::Password);

    m_urlCombo->setEditable(true);
    m_urlCombo->setToolTip(tr("Enter or select a saved URL/ID. For example facebook, google, gmail.com, myserver.."));
    connect(m_urlCombo, SIGNAL(activated(const QString &)), this, SLOT(updateUser(const QString &)));
    connect(m_urlCombo, SIGNAL(editTextChanged(const QString &)), this, SLOT(setRmbButtonText(const QString &)));

    m_userEdit->setToolTip(tr("Enter your user name corresponding to the selected URL/ID."));

    m_passwdEdit->setToolTip(tr("This is the generated password, which is always the same with the same master password, URL/ID and user name."));
    m_passwdEdit->setReadOnly(true);
    m_passwdEdit->setEnabled(false);

    layout->addWidget(m_masterEdit, 0, 1, 1, 3);
    layout->addWidget(m_urlCombo,   1, 1, 1, 3);
    layout->addWidget(m_userEdit,   2, 1, 1, 3);
    layout->addWidget(m_passwdEdit, 4, 1, 1, 3);

    QFrame * frame = new QFrame(this);
    frame->setFrameShape(QFrame::HLine);
    layout->addWidget(new QLabel(tr("<b><font color=#aa0000>Master password:</font></b>")), 0, 0);
    layout->addWidget(new QLabel(tr("<b>URL/ID:</b>")), 1, 0);
    layout->addWidget(new QLabel(tr("<b>User name:</b>")), 2, 0);
    layout->addWidget(frame, 3, 1, 1, 3);

    m_genButton->setEnabled(false);
    m_genButton->setToolTip(tr("Generate and show the password"));
    layout->addWidget(m_genButton, 4, 0);

    QLabel * starsLabel = new QLabel();
    starsLabel->setPixmap(QPixmap(":/stars.png"));
    layout->addWidget(starsLabel, 5, 0);

    m_rmbButton->setEnabled(false);
    connect(m_rmbButton, SIGNAL(clicked()), this, SLOT(rememberOrRemoveLogin()));
    m_rmbButton->setToolTip(m_rememberToolTip);
    layout->addWidget(m_rmbButton, 5, 1, 1, 2);

    QPushButton * quitButton = new QPushButton(tr("&Quit"));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    layout->addWidget(quitButton, 5, 3);

    QWidget * dummy = new QWidget();
    dummy->setLayout(layout);
    setCentralWidget(dummy);

    connect(m_genButton, SIGNAL(clicked()), this, SLOT(doGenerate()));

    connect(m_masterEdit, SIGNAL(textChanged(const QString &)), this, SLOT(invalidate()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)), this, SLOT(invalidate()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)), this, SLOT(invalidate()));

    connect(m_masterEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableGenButton()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)), this, SLOT(enableGenButton()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableGenButton()));

    connect(m_masterEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableRmbButton()));
    connect(m_urlCombo, SIGNAL(textChanged(const QString &)), this, SLOT(enableRmbButton()));
    connect(m_userEdit, SIGNAL(textChanged(const QString &)), this, SLOT(enableRmbButton()));
}

void MainWindow::initMenu()
{
    // Add file menu
    QMenu * fileMenu = menuBar()->addMenu(tr("&File"));

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
}

void MainWindow::showAboutDlg()
{
}

void MainWindow::showAboutQtDlg()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::loadSettings()
{
}

void MainWindow::saveSettings()
{
}

void MainWindow::doGenerate()
{
}

void MainWindow::decreasePasswordAlpha(int frame)
{
}

void MainWindow::invalidate()
{
    m_passwdEdit->setText("");
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

void MainWindow::rememberOrRemoveLogin()
{
}

void MainWindow::updateUser(const QString & url)
{
}

void MainWindow::setRmbButtonText(const QString & url)
{
}

MainWindow::~MainWindow()
{
    delete m_timeLine;
    delete m_engine;
}