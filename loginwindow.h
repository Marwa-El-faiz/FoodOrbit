#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QPixmap>
#include <QCoreApplication>

#include "client.h"
#include "utilisateur.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void on_btnTabLogin_clicked();
    void on_btnTabSignup_clicked();
    void on_btnLogin_3_clicked();
    void on_btnForgot_3_clicked();
    void on_pushButton_clicked();
    void on_btnToggleMDP_3_clicked();
    void on_btnToggleMDPS_clicked();
    void retranslateUi();

private:
    Ui::LoginWindow *ui;
    client currentClient;
    bool mdpLoginVisible  = false;
    bool mdpSignupVisible = false;

    void chargerLogo();
    void showLoginTab();
    void showSignupTab();
    void clearFields();
    void ouvrirFenetreSelonRole(const QString& role);
};

#endif // LOGINWINDOW_H