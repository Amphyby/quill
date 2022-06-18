/*
    InkBox: Open-source Qt-based eBook reader
    Copyright (C) 2021-2022 Nicolas Mailloux <nicolecrivain@gmail.com>
    SPDX-License-Identifier: GPL-3.0-only

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "alert.h"
#include "generaldialog.h"
#include "functions.h"
#include "reader.h"
#include "encryptionmanager.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QRect>
#include <QScreen>
#include <QDirIterator>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

void createMainJson()
{
    QDirIterator appsDir("/mnt/onboard/onboard/.apps", QDirIterator::NoIteratorFlags);
    QFile newJsonFile = QFile{"/mnt/onboard/onboard/.apps/apps.json"};
    QJsonDocument newJsonDocument;
    QJsonArray array;

    while (appsDir.hasNext())
    {
        QDir dir(appsDir.next());
        if(dir.exists() == true) {
            if(dir.path().split("/").last().contains(".") == false) {
                QFile jsonSmall = QFile{dir.path() + "/app.json"};
                if(jsonSmall.exists() == true) {
                    jsonSmall.open(QIODevice::ReadOnly | QIODevice::Text);
                    QString jsonString = jsonSmall.readAll();
                    jsonSmall.close();

                    QJsonDocument jsonSmallDoc = QJsonDocument::fromJson(jsonString.toUtf8());
                    if(jsonSmallDoc["app"].isObject() == true) {
                        QJsonObject jsonSmallMainObj = jsonSmallDoc["app"].toObject();
                        array.append(jsonSmallMainObj);

                    }
                    else {
                        log("Small JSON user application file is missing main object 'app'", "main");
                    }

                }
                else {
                    QString message = "User applications folder does not contain any 'app.json' file: ";
                    message.append(jsonSmall.fileName());
                    log(message, "main");
                }
            }
        }
    }
    // https://forum.qt.io/topic/104791/how-i-can-create-json-format-in-qt/5
    QJsonObject root;
    root["list"] = array;
    newJsonDocument.setObject(root);

    newJsonFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
    newJsonFile.write(newJsonDocument.toJson());
    newJsonFile.flush();
    newJsonFile.close();
}

void createSmallJsonFiles()
{
    QFile jsonFile = QFile{"/mnt/onboard/onboard/.apps/apps.json"};

    jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString fileRead = jsonFile.readAll();
    jsonFile.close();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(fileRead.toUtf8());

    if(jsonDocument["list"].isArray() == true) {
        QJsonArray jsonArray = jsonDocument["list"].toArray();
        for(QJsonValueRef refJsonObject: jsonArray) {
            QJsonObject jsonMainObject = refJsonObject.toObject();
            QString appName = jsonMainObject["Name"].toString();

            // This needs to be here and not at the beggining of this function because it is an iterator
            QDirIterator appsDir("/mnt/onboard/onboard/.apps", QDirIterator::NoIteratorFlags);
            while (appsDir.hasNext()) {
                QDir dir(appsDir.next());
                if(dir.exists() == true) {
                    if(dir.path().split("/").last().toLower().contains(appName.toLower()) == true) {
                        QJsonObject root;
                        root["app"] = refJsonObject.toObject();;
                        QJsonDocument newJsonDocument;
                        newJsonDocument.setObject(root);

                        QFile newSmallJson = QFile{dir.path() + "/" + "app.json"};

                        newSmallJson.open(QIODevice::ReadWrite);
                        QTextStream stream(&newSmallJson);
                        stream << newJsonDocument.toJson() << Qt::endl;
                        newSmallJson.flush();
                        newSmallJson.close();
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if(char * debug = std::getenv("DEBUG")) {
        if(std::atoi(debug) == 1) {
            global::logger::status = true;
        }
    }
    global::deviceID = readFile("/opt/inkbox_device");
    log("Running on device " + global::deviceID, "main", true);

    setDefaultWorkDir();
    if(checkconfig("/run/wifi_able") == true) {
        log("Device has Wi-Fi capabilities", "main");
        global::device::isWifiAble = true;
    }
    else {
        log("Device does not have Wi-Fi capabilities", "main");
        global::device::isWifiAble = false;
    }
    if(checkconfig(".config/18-encrypted_storage/status") == true and checkconfig("/external_root/run/encfs_mounted") == false) {
        // Open Encryption Manager to unlock encrypted storage
        QApplication a(argc, argv);
        encryptionManager w;
        const QScreen * screen = qApp->primaryScreen();
        w.setGeometry(QRect(QPoint(0,0), screen->geometry().size()));
        w.show();
        return a.exec();
    }
    else if(checkconfig("/external_root/run/encfs_mounted") == true and checkconfig("/external_root/run/encfs_repack") == true) {
        log("Launching encryptionManager", "main");
        QApplication a(argc, argv);
        encryptionManager w;
        const QScreen * screen = qApp->primaryScreen();
        w.setGeometry(QRect(QPoint(0,0), screen->geometry().size()));
        w.show();
        return a.exec();
    }
    else {
        // Tell scripts that we're currently running
        string_writeconfig("/tmp/inkbox_running", "true");

        // Variables
        global::reader::startBatteryWatchdog = false;
        global::reader::startUsbmsPrompt = false;

        // Checking if battery level is critical; if true (and if it is not charging), then display a "Please charge your eReader" splash and power off.
        if(global::deviceID != "emu\n") {
            if(isBatteryCritical() == true) {
                if(!isUsbPluggedIn()) {
                    global::battery::showCriticalBatteryAlert = true;
                    QApplication a(argc, argv);
                    alert w;

                    const QScreen* screen = qApp->primaryScreen();
                    w.setGeometry(QRect(QPoint(0,0), screen->geometry().size()));
                    w.show();
                    return a.exec();
                }
            }
        }

        // Checking if there has been an ALERT flag set up, and if there is, show a big warning
        if(checkconfig("/external_root/boot/flags/ALERT") == true) {
            QApplication a(argc, argv);
            alert w;

            const QScreen * screen = qApp->primaryScreen();
            w.setGeometry(QRect(QPoint(0,0), screen->geometry().size()));
            w.show();
            return a.exec();
        }
        // If we're waking from sleep and we have the lockscreen enabled, we'll "resume" the book from scratch
        else if(checkconfig("/tmp/suspendBook") == true && checkconfig("/inkbox/bookIsEpub") == false) {
            // Start the low/critical battery alert timer from the Reader framework since MainWindow is not going to be shown
            global::runningInstanceIsReaderOnly = true;
            global::reader::startBatteryWatchdog = true;
            global::reader::startUsbmsPrompt = true;
            global::reader::skipOpenDialog = true;

            string_writeconfig("/inkbox/skip_opendialog", "true");
            if(global::deviceID == "n705\n") {
                global::isN705 = true;
            }
            else if(global::deviceID == "n905\n") {
                global::isN905C = true;
            }
            else if(global::deviceID == "n613\n") {
                global::isN613 = true;
            }
            else if(global::deviceID == "n873\n") {
                global::isN873 = true;
            }
            else if(global::deviceID == "n236\n") {
                global::isN236 = true;
            }
            else if(global::deviceID == "n437\n") {
                global::isN437 = true;
            }
            else if(global::deviceID == "n306\n") {
                global::isN306 = true;
            }
            else if(global::deviceID == "kt\n") {
                global::isKT = true;
            }
            else {
                ;
            }

            QApplication a(argc, argv);
            reader w;

            const QScreen* screen = qApp->primaryScreen();
            w.setGeometry(QRect(QPoint(0,0), screen->geometry().size()));
            w.show();
            return a.exec();

        }
        else {
            QProcess::execute("remount_tmpfs_launch.sh", QStringList());

            QApplication a(argc, argv);
            MainWindow w;

            QApplication::setStyle("windows");
            QFile stylesheetFile("/mnt/onboard/.adds/inkbox/eink.qss");
            stylesheetFile.open(QFile::ReadOnly);
            w.setStyleSheet(stylesheetFile.readAll());
            stylesheetFile.close();

            if(global::deviceID == "n705\n") {
                global::isN705 = true;
            }
            else if(global::deviceID == "n905\n") {
                global::isN905C = true;
            }
            else if(global::deviceID == "n613\n") {
                global::isN613 = true;
            }
            else if(global::deviceID == "n873\n") {
                global::isN873 = true;
            }
            else if(global::deviceID == "n236\n") {
                global::isN236 = true;
            }
            else if(global::deviceID == "n437\n") {
                global::isN437 = true;
            }
            else if(global::deviceID == "n306\n") {
                global::isN306 = true;
            }
            else if(global::deviceID == "kt\n") {
                global::isKT = true;
            }
            else {
                ;
            }

            const QScreen * screen = qApp->primaryScreen();
            w.setGeometry(QRect(QPoint(0,0), screen->geometry ().size()));
            w.show();
            return a.exec();
        }
    }
}
