#ifndef RESETWINDOW_H
#define RESETWINDOW_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class ResetWindow; }
QT_END_NAMESPACE

class ResetWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ResetWindow(const QString& email, QWidget *parent = nullptr);
    ~ResetWindow();

private slots:
    void on_btnVerifier_clicked();

private:
    Ui::ResetWindow *ui;
    QString m_email;
};

#endif // RESETWINDOW_H