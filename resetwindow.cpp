#include "resetwindow.h"
#include "ui_resetwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QPixmap>
#include <QSqlQuery>
#include <QSqlError>

ResetWindow::ResetWindow(const QString& email, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::ResetWindow),
    m_email(email)
{
    ui->setupUi(this);
    setWindowTitle("Réinitialisation du mot de passe - FoodOrbit");

    // ── Fenêtre redimensionnable desktop ──
    setMinimumSize(700, 480);
    resize(860, 540);

    // ── Logo : image si disponible, sinon emoji ──
    QPixmap logo("logo.png");
    if (!logo.isNull()) {
        ui->lblEmoji->setPixmap(
            logo.scaled(ui->lblEmoji->width(),
                        ui->lblEmoji->height(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation));
        ui->lblEmoji->setText("");
    }

    ui->lineEditCode->setMaxLength(6);
    ui->lineEditCode->setAlignment(Qt::AlignCenter);
    ui->lineEditCode->setPlaceholderText("1  2  3  4  5  6");

    connect(ui->btnVerifier, &QPushButton::clicked,
            this, &ResetWindow::on_btnVerifier_clicked);
}

ResetWindow::~ResetWindow()
{
    delete ui;
}

void ResetWindow::on_btnVerifier_clicked()
{
    QString code = ui->lineEditCode->text().trimmed();

    if (code.isEmpty() || code.length() != 6) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer le code à 6 chiffres !");
        return;
    }

    qDebug() << "[Reset] Email:"  << m_email;
    qDebug() << "[Reset] Code saisi:" << code;

    QSqlQuery debug;
    debug.prepare("SELECT reset_token, reset_expiration FROM utilisateurs WHERE email = :email");
    debug.bindValue(":email", m_email);
    if (debug.exec() && debug.next()) {
        qDebug() << "[Reset] reset_token en DB    :" << debug.value(0).toString();
        qDebug() << "[Reset] reset_expiration en DB:" << debug.value(1).toString();
    } else {
        qDebug() << "[Reset] Utilisateur introuvable pour cet email !";
    }

    QSqlQuery query;
    query.prepare(
        "SELECT id FROM utilisateurs "
        "WHERE email = :email "
        "AND reset_token = :code "
        "AND reset_expiration > datetime('now')"
        );
    query.bindValue(":email", m_email);
    query.bindValue(":code",  code);

    if (!query.exec()) {
        qDebug() << "[Reset] Erreur SQL:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Erreur lors de la vérification.");
        return;
    }

    if (query.next()) {
        QSqlQuery clear;
        clear.prepare("UPDATE utilisateurs SET reset_token = NULL, reset_expiration = NULL WHERE email = :email");
        clear.bindValue(":email", m_email);
        clear.exec();

        QMessageBox::information(this, "Succès",
                                 "Code valide !\nVous pouvez maintenant choisir un nouveau mot de passe.");
        accept();
    } else {
        QMessageBox::critical(this, "Code invalide",
                              "Code incorrect ou expiré.\nVeuillez recommencer.");
        ui->lineEditCode->clear();
        ui->lineEditCode->setFocus();
    }
}