#ifndef FLASHEXAM_H
#define FLASHEXAM_H

#include <QWidget>

namespace Ui {
class flashExam;
}

class flashExam : public QWidget
{
    Q_OBJECT

public:
    QString className = this->metaObject()->className();
    explicit flashExam(QWidget *parent = nullptr);
    ~flashExam();

private:
    Ui::flashExam *ui;
    int cardsTotal;
    int currentCardNumber;
    bool randomize;
    bool answerShown = false;
    QStringList cardsStringList;
    QStringList answersStringList;
    void setupCardsList();
    void initCardsList(QString cardsList, QString answersList);
    void displayCard(bool existingCardNumber);

signals:
    void showToast(QString messageToDisplay);
private slots:
    void on_closeBtn_clicked();
    void on_startBtn_clicked();
    void on_backBtn_clicked();
    void on_revealBtn_clicked();
    void on_nextBtn_clicked();
};

#endif // FLASHEXAM_H
