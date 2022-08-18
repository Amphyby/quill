#ifndef WIFIDIALOG_H
#define WIFIDIALOG_H

#include <QDialog>
#include <QModelIndex>

#include "generaldialog.h"
#include "wifilogger.h"

namespace Ui {
class wifiDialog;
}

class wifiDialog : public QDialog
{
    Q_OBJECT

public:
    QString className = this->metaObject()->className();
    explicit wifiDialog(QWidget *parent = nullptr);
    ~wifiDialog();
    global::wifi::wifiNetworkData connectedNetworkDataParent;
    wifilogger* wifiLoggerDialog = new wifilogger;


private:
    Ui::wifiDialog *ui;
    bool wifiButtonEnabled = false;
    bool scannedAtLeastOnce = false;

    // variables for refreshWait() and network refresh in general
    int elapsedSeconds = 0;
    QFile fullList =  QFile("/external_root/run/wifi_list_full");
    QFile formattedList = QFile("/external_root/run/wifi_list_format");

public slots:
    void launchRefresh();
    void refreshNetworksList();
    void showToastSlot(QString message);

signals:
    void refreshScreen();
    void updateWifiIconSig(int mode);
    void showToast(QString messageToDisplay);

    void killNetworkWidgets();

private slots:
    void on_refreshBtn_clicked();
    void on_Wificheckbox_stateChanged(int arg1);
    void turnOnWifi();
    void turnOffWifi();
    void on_logBtn_clicked();
    // This function is a more clever sleep(1), non blocking
    void refreshWait();
};

#endif // WIFIDIALOG_H
