#include "koboxappsdialog.h"
#include "ui_koboxappsdialog.h"

#include "functions.h"

#include <QStringListModel>
#include <QScreen>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

koboxAppsDialog::koboxAppsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::koboxAppsDialog)
{
    ui->setupUi(this);

    // Preventing outside interaction
    this->setModal(true);

    // Stylesheet, style & misc.
    QFile stylesheetFile(":/resources/eink.qss");
    stylesheetFile.open(QFile::ReadOnly);
    this->setStyleSheet(stylesheetFile.readAll());
    stylesheetFile.close();

    string_checkconfig_ro("/opt/inkbox_device");
    if(checkconfig_str_val == "n705\n") {
        // If we don't do this, the text will clip out of the display.
        ui->definitionLabel->setText("Please select an application.\nClick on 'Launch' to start it.");
    }

    ui->launchBtn->setProperty("type", "borderless");
    ui->cancelBtn->setProperty("type", "borderless");
    ui->launchBtn->setStyleSheet("font-size: 9pt; padding: 10px; font-weight: bold; background: lightGrey");
    ui->cancelBtn->setStyleSheet("font-size: 9pt; padding: 10px; font-weight: bold; background: lightGrey");
    ui->appsList->setStyleSheet("font-size: 9pt");
    ui->headerLabel->setStyleSheet("font-weight: bold");

    // UI fonts
    int id = QFontDatabase::addApplicationFont(":/resources/fonts/CrimsonPro-Regular.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    QFont crimson(family);
    ui->definitionLabel->setFont(QFont(crimson));

    this->adjustSize();

    // Centering dialog
    QRect screenGeometry = QGuiApplication::screens()[0]->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    checkApps();
    QStringListModel* model = new QStringListModel(this);
    QStringList list = apps.split("\n", QString::SkipEmptyParts);

    // Apps that aren't extensions
    list.prepend("Geany");

    if(checkconfig("/external_root/opt/root/rooted") == true) {
        list.prepend("KTerm");
    }

    model->setStringList(list);
    ui->appsList->setModel(model);
    ui->appsList->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void koboxAppsDialog::checkApps() {
    QFile apps_list("/external_root/opt/X11/extensions_list");
    apps_list.open(QIODevice::ReadWrite);
    QTextStream in (&apps_list);
    apps = in.readAll();
    apps_list.close();
}

koboxAppsDialog::~koboxAppsDialog()
{
    delete ui;
}

void koboxAppsDialog::on_cancelBtn_clicked()
{
    koboxAppsDialog::close();
}

void koboxAppsDialog::on_launchBtn_clicked()
{
    index = ui->appsList->currentIndex();
    itemText = index.data(Qt::DisplayRole).toString();
    if(itemText == "") {
        QMessageBox::critical(this, tr("Invalid argument"), tr("Please select an application."));
    }
    else {
        // DPI setting
        string_checkconfig(".config/00-kobox/dpiSetting");
        if(checkconfig_str_val == "") {
            dpiSetting = "125";
        }
        else {
            dpiSetting = checkconfig_str_val.toStdString();
        }

        // Fullscreen or windowed (i3)
        // Mostly windowed except for apps like KTerm which ship their own OSK
        dpModeSetting = "windowed";

        if(itemText == "Netsurf") {
            // Bypass standard shell script launch shenanigans
            string_writeconfig("/external_root/tmp/X_program", "!netsurf");
        }
        else if(itemText == "KTerm") {
            string_writeconfig("/external_root/tmp/X_program", "/usr/local/bin/kterm -l /usr/local/share/kterm/layouts/keyboard-kt.xml -k 1");
            dpModeSetting = "fullscreen";
            dpiSetting = "175";
        }
        else if(itemText == "Geany") {
            string_writeconfig("/external_root/tmp/X_program", "geany");
        }
        else {
            QString itemTextLower = itemText.toLower();
            std::string app = itemTextLower.toStdString();
            string_writeconfig("/external_root/tmp/X_program", app);
        }

        string_writeconfig("/external_root/tmp/X_dpmode", dpModeSetting);
        string_writeconfig("/external_root/tmp/X_dpi", dpiSetting);

        /* Wheeee! */
        global::kobox::showKoboxSplash = true;

        // Re-use USBMS splash window for KoBox splash, since it's pretty much the same layout
        usbmsSplashWindow = new usbms_splash();
        usbmsSplashWindow->setAttribute(Qt::WA_DeleteOnClose);
        usbmsSplashWindow->setGeometry(QRect(QPoint(0,0), screen()->geometry ().size()));
        usbmsSplashWindow->show();
        QApplication::processEvents();

        // Write to FIFO to start X11
        string_writeconfig("/opt/ibxd", "x_start_gui\n");
    }
}