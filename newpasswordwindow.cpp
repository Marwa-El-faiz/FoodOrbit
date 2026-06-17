#include "newpasswordwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>

// ── Fonction de hachage SHA-256 ──
QString NewPasswordWindow::hasher(const QString& mdp)
{
    return QString(QCryptographicHash::hash(
                       mdp.toUtf8(), QCryptographicHash::Sha256).toHex());
}

NewPasswordWindow::NewPasswordWindow(const QString& email, QWidget* parent)
    : QDialog(parent), m_email(email)
{
    setWindowTitle("Nouveau mot de passe");
    setFixedSize(420, 520);
    setStyleSheet(R"(
        QDialog { background-color: #ffffff; }
        QLabel  { color: #2D6A4F; font-size: 13px; }
        QLineEdit {
            background: white; color: #1a1a1a;
            border: 1.5px solid #D8EDDF; border-radius: 10px;
            padding: 12px 14px; font-size: 13px;
        }
        QLineEdit:focus { border-color: #2D6A4F; }
        QPushButton#btnValider {
            background-color: #2D6A4F; color: white;
            border: none; border-radius: 12px;
            font-size: 15px; font-weight: bold; padding: 14px;
        }
        QPushButton#btnValider:hover   { background-color: #40916C; }
        QPushButton#btnValider:pressed { background-color: #1B4332; }
        QPushButton#btnToggle {
            background: transparent; border: none;
            font-size: 12px; color: #888; padding: 0;
        }
        QPushButton#btnToggle:hover { color: #2D6A4F; }
    )");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(30, 30, 30, 30);
    root->setSpacing(18);

    // ── Header ──
    auto* header = new QFrame;
    header->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "stop:0 #2D6A4F,stop:1 #1B4332);border-radius:12px;");
    auto* hLayout = new QVBoxLayout(header);
    hLayout->setContentsMargins(20, 20, 20, 20);
    auto* icon = new QLabel("🔑");
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("font-size:36px;background:transparent;");
    auto* titre = new QLabel("Nouveau mot de passe");
    titre->setAlignment(Qt::AlignCenter);
    titre->setStyleSheet(
        "color:white;font-size:16px;font-weight:bold;background:transparent;");
    hLayout->addWidget(icon);
    hLayout->addWidget(titre);
    root->addWidget(header);

    // ── Info ──
    auto* info = new QLabel("Choisissez un nouveau mot de passe\npour : " + email);
    info->setAlignment(Qt::AlignCenter);
    info->setWordWrap(true);
    info->setStyleSheet("color:#52B788;font-size:12px;");
    root->addWidget(info);

    // ── Champ 1 ──
    root->addWidget(new QLabel("Nouveau mot de passe"));
    auto* row1 = new QHBoxLayout;
    m_mdp1 = new QLineEdit;
    m_mdp1->setPlaceholderText("Minimum 6 caractères");
    m_mdp1->setEchoMode(QLineEdit::Password);
    m_mdp1->setMinimumHeight(44);
    m_toggle1 = new QPushButton("Afficher");
    m_toggle1->setObjectName("btnToggle");
    m_toggle1->setCursor(Qt::PointingHandCursor);
    row1->addWidget(m_mdp1);
    row1->addWidget(m_toggle1);
    root->addLayout(row1);

    // ── Champ 2 ──
    root->addWidget(new QLabel("Confirmer le mot de passe"));
    auto* row2 = new QHBoxLayout;
    m_mdp2 = new QLineEdit;
    m_mdp2->setPlaceholderText("Répétez le mot de passe");
    m_mdp2->setEchoMode(QLineEdit::Password);
    m_mdp2->setMinimumHeight(44);
    m_toggle2 = new QPushButton("Afficher");
    m_toggle2->setObjectName("btnToggle");
    m_toggle2->setCursor(Qt::PointingHandCursor);
    row2->addWidget(m_mdp2);
    row2->addWidget(m_toggle2);
    root->addLayout(row2);

    root->addStretch();

    // ── Bouton valider ──
    auto* btnValider = new QPushButton("Enregistrer le mot de passe");
    btnValider->setObjectName("btnValider");
    btnValider->setMinimumHeight(50);
    btnValider->setCursor(Qt::PointingHandCursor);
    root->addWidget(btnValider);

    connect(btnValider, &QPushButton::clicked, this, &NewPasswordWindow::onValider);
    connect(m_toggle1,  &QPushButton::clicked, this, &NewPasswordWindow::toggleMdp1);
    connect(m_toggle2,  &QPushButton::clicked, this, &NewPasswordWindow::toggleMdp2);
}

void NewPasswordWindow::toggleMdp1()
{
    m_vis1 = !m_vis1;
    m_mdp1->setEchoMode(m_vis1 ? QLineEdit::Normal : QLineEdit::Password);
    m_toggle1->setText(m_vis1 ? "Masquer" : "Afficher");
}

void NewPasswordWindow::toggleMdp2()
{
    m_vis2 = !m_vis2;
    m_mdp2->setEchoMode(m_vis2 ? QLineEdit::Normal : QLineEdit::Password);
    m_toggle2->setText(m_vis2 ? "Masquer" : "Afficher");
}

void NewPasswordWindow::onValider()
{
    QString mdp1 = m_mdp1->text();
    QString mdp2 = m_mdp2->text();

    if (mdp1.isEmpty() || mdp2.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir les deux champs !");
        return;
    }
    if (mdp1.length() < 6) {
        QMessageBox::warning(this, "Trop court",
                             "Le mot de passe doit contenir au moins 6 caractères !");
        return;
    }
    if (mdp1 != mdp2) {
        QMessageBox::warning(this, "Erreur",
                             "Les mots de passe ne correspondent pas !");
        m_mdp2->clear();
        m_mdp2->setFocus();
        return;
    }

    QSqlQuery q;
    q.prepare("UPDATE utilisateurs SET mot_de_passe = :mdp WHERE email = :email");
    q.bindValue(":mdp",   hasher(mdp1));
    q.bindValue(":email", m_email);

    if (!q.exec() || q.numRowsAffected() == 0) {
        qDebug() << "[NewPassword] Erreur SQL:" << q.lastError().text();
        QMessageBox::critical(this, "Erreur",
                              "Impossible de mettre à jour le mot de passe.");
        return;
    }

    QMessageBox::information(this, "Succès",
                             "✅ Mot de passe modifié avec succès !\nVous pouvez maintenant vous connecter.");
    accept();
}