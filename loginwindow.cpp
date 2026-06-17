#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QDebug>
#include <QPixmap>
#include <QResizeEvent>
#include "resetwindow.h"
#include "newpasswordwindow.h"
#include "clientwindow.h"
#include "Interface_Restaurant.h"
#include "Interface_Livreur.h"
#include "languagemanager.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    // ── Fenetre redimensionnable desktop ──
    setMinimumSize(820, 560);
    resize(1000, 640);

    // ── Chargement du logo ──
    // ── Echo password ──
    ui->lineEditMDP_3->setEchoMode(QLineEdit::Password);
    ui->lineEditMDPS->setEchoMode(QLineEdit::Password);

    // ── Langue ──
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,
            this, &LoginWindow::retranslateUi);
    retranslateUi();

    // Effacer les messages d'erreur dès que l'utilisateur retape
    connect(ui->lineEditEmail_3, &QLineEdit::textChanged,
            this, [this](){ ui->lblErreurLogin->setText(""); });
    connect(ui->lineEditMDP_3, &QLineEdit::textChanged,
            this, [this](){ ui->lblErreurLogin->setText(""); });
    connect(ui->lineEditNom, &QLineEdit::textChanged,
            this, [this](){ ui->lblErreurSignup->setText(""); ui->lineEditNom->setStyleSheet(""); });
    connect(ui->lineEditPrenom, &QLineEdit::textChanged,
            this, [this](){ ui->lineEditPrenom->setStyleSheet(""); });
    connect(ui->lineEditEmailS, &QLineEdit::textChanged,
            this, [this](){ ui->lblErreurSignup->setText(""); ui->lineEditEmailS->setStyleSheet(""); });
    connect(ui->lineEditMDPS, &QLineEdit::textChanged,
            this, [this](){ ui->lineEditMDPS->setStyleSheet(""); });
    connect(ui->lineEditTel, &QLineEdit::textChanged,
            this, [this](){ ui->lineEditTel->setStyleSheet(""); });
    connect(ui->lineEditAdresse, &QLineEdit::textChanged,
            this, [this](){ ui->lineEditAdresse->setStyleSheet(""); });

    showLoginTab();
}

LoginWindow::~LoginWindow() { delete ui; }

void LoginWindow::chargerLogo()
{
    QStringList chemins = {
        "logo.png",
        ":/logo.png",
        "../logo.png",
        "../../logo.png",
        "../../../logo.png",
        QCoreApplication::applicationDirPath() + "/logo.png",
        QCoreApplication::applicationDirPath() + "/../logo.png",
        QCoreApplication::applicationDirPath() + "/../../logo.png"
    };
    for (const QString& chemin : chemins) {
        QPixmap px(chemin);
        if (!px.isNull()) {
            ui->lblLogoImage->setPixmap(
                px.scaled(ui->lblLogoImage->maximumWidth(),
                          ui->lblLogoImage->maximumHeight(),
                          Qt::KeepAspectRatio,
                          Qt::SmoothTransformation));
            ui->lblLogoImage->setText("");
            qDebug() << "[Logo] Chargé depuis :" << chemin;
            return;
        }
    }
    qDebug() << "[Logo] Introuvable — place logo.png dans le dossier build ou source";
    // Fallback : afficher le nom stylisé
    ui->lblLogoImage->setText("FoodOrbit");
    ui->lblLogoImage->setStyleSheet(
        "color:white;font-size:28px;font-weight:bold;"
        "font-family:\'Segoe UI\';background:transparent;");
}

void LoginWindow::retranslateUi()
{
    setWindowTitle(tr("FoodOrbit - Connexion"));
    ui->btnTabLogin->setText(tr("Connexion"));
    ui->btnTabSignup->setText(tr("Inscription"));
    ui->lineEditEmail_3->setPlaceholderText(tr("votre@email.com"));
    ui->lineEditMDP_3->setPlaceholderText(tr("Mot de passe"));
    ui->btnLogin_3->setText(tr("Se connecter"));
    ui->btnForgot_3->setText(tr("Mot de passe oublie ?"));
    ui->btnToggleMDP_3->setText(mdpLoginVisible ? tr("Masquer") : tr("Afficher"));
    ui->btnToggleMDPS->setText(mdpSignupVisible ? tr("Masquer") : tr("Afficher"));
    ui->lineEditNom->setPlaceholderText(tr("Votre nom"));
    ui->lineEditPrenom->setPlaceholderText(tr("Votre prenom"));
    ui->lineEditEmailS->setPlaceholderText(tr("votre@email.com"));
    ui->lineEditMDPS->setPlaceholderText(tr("Minimum 6 caracteres"));
    ui->lineEditTel->setPlaceholderText(tr("06XXXXXXXX"));
    ui->lineEditAdresse->setPlaceholderText(tr("Votre adresse"));
    ui->pushButton->setText(tr("Creer mon compte"));
    ui->lblSlogan->setText(tr("Le Gout a Portee d'Orbite"));
}

void LoginWindow::showLoginTab()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->btnTabLogin->setStyleSheet(
        "QPushButton{background:#2D6A4F;color:white;border-radius:7px;"
        "font-weight:bold;font-size:13px;font-family:\"Segoe UI\";border:none;}"
        "QPushButton:hover{background:#1B4332;}");
    ui->btnTabSignup->setStyleSheet(
        "QPushButton{background:transparent;color:#52B788;border-radius:7px;"
        "font-size:13px;font-family:\"Segoe UI\";border:none;}"
        "QPushButton:hover{color:#2D6A4F;background:rgba(255,255,255,0.5);}");
}

void LoginWindow::showSignupTab()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->btnTabSignup->setStyleSheet(
        "QPushButton{background:#2D6A4F;color:white;border-radius:7px;"
        "font-weight:bold;font-size:13px;font-family:\"Segoe UI\";border:none;}"
        "QPushButton:hover{background:#1B4332;}");
    ui->btnTabLogin->setStyleSheet(
        "QPushButton{background:transparent;color:#52B788;border-radius:7px;"
        "font-size:13px;font-family:\"Segoe UI\";border:none;}"
        "QPushButton:hover{color:#2D6A4F;background:rgba(255,255,255,0.5);}");
}

void LoginWindow::on_btnTabLogin_clicked()  { showLoginTab(); }
void LoginWindow::on_btnTabSignup_clicked() { showSignupTab(); }

void LoginWindow::on_btnToggleMDP_3_clicked()
{
    mdpLoginVisible = !mdpLoginVisible;
    ui->lineEditMDP_3->setEchoMode(mdpLoginVisible ? QLineEdit::Normal : QLineEdit::Password);
    ui->btnToggleMDP_3->setText(mdpLoginVisible ? tr("Masquer") : tr("Afficher"));
    ui->lineEditMDP_3->setFocus();
}

void LoginWindow::on_btnToggleMDPS_clicked()
{
    mdpSignupVisible = !mdpSignupVisible;
    ui->lineEditMDPS->setEchoMode(mdpSignupVisible ? QLineEdit::Normal : QLineEdit::Password);
    ui->btnToggleMDPS->setText(mdpSignupVisible ? tr("Masquer") : tr("Afficher"));
    ui->lineEditMDPS->setFocus();
}

void LoginWindow::ouvrirFenetreSelonRole(const QString& role)
{
    if (role == "client") {
        ClientWindow* w = new ClientWindow(currentClient, this);
        w->show();
        this->hide();
    } else if (role == "admin") {
        Interface_Restaurant* w = new Interface_Restaurant(this);
        w->show();
        this->hide();
    } else if (role == "livreur") {
        QSqlQuery q;
        q.prepare("SELECT id FROM utilisateurs WHERE LOWER(email) = LOWER(:email)");
        q.bindValue(":email", ui->lineEditEmail_3->text().trimmed());
        q.exec();
        if (q.next()) {
            Interface_Livreur* w = new Interface_Livreur(q.value(0).toInt(), this);
            w->show();
            this->hide();
        } else {
            QMessageBox::critical(this, tr("Erreur"),
                                  tr("Impossible de charger le profil livreur."));
        }
    } else {
        QMessageBox::warning(this, tr("Role inconnu"),
                             QString(tr("Role '%1' non reconnu !")).arg(role));
    }
}

void LoginWindow::on_btnLogin_3_clicked()
{
    QString email = ui->lineEditEmail_3->text().trimmed();
    QString mdp   = ui->lineEditMDP_3->text();
    if (email.isEmpty() || mdp.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Veuillez remplir tous les champs !"));
        return;
    }
    ui->lblErreurLogin->setText("");
    if (currentClient.seConnecter(email, mdp)) {
        ouvrirFenetreSelonRole(currentClient.getRole());
    } else {
        ui->lblErreurLogin->setText(tr("⚠  Email ou mot de passe incorrect !"));
        ui->lineEditMDP_3->clear();
        ui->lineEditMDP_3->setFocus();
        if (mdpLoginVisible) {
            mdpLoginVisible = false;
            ui->lineEditMDP_3->setEchoMode(QLineEdit::Password);
            ui->btnToggleMDP_3->setText(tr("Afficher"));
        }
    }
}

void LoginWindow::on_pushButton_clicked()
{
    QString nom       = ui->lineEditNom->text().trimmed();
    QString prenom    = ui->lineEditPrenom->text().trimmed();
    QString email     = ui->lineEditEmailS->text().trimmed();
    QString mdp       = ui->lineEditMDPS->text();
    QString telephone = ui->lineEditTel->text().trimmed();
    QString adresse   = ui->lineEditAdresse->text().trimmed();

    ui->lblErreurSignup->setText("");

    // Style bordure rouge pour champ vide
    static auto markField = [](QLineEdit* f, bool vide) {
        f->setStyleSheet(vide
                             ? "background:#FFF5F5;border:2px solid #E63946;border-radius:8px;padding:11px 16px;font-size:13px;"
                             : "");
    };

    bool ok = true;
    markField(ui->lineEditNom,     nom.isEmpty());      if (nom.isEmpty())       ok = false;
    markField(ui->lineEditPrenom,  prenom.isEmpty());   if (prenom.isEmpty())    ok = false;
    markField(ui->lineEditEmailS,  email.isEmpty());    if (email.isEmpty())     ok = false;
    markField(ui->lineEditMDPS,    mdp.isEmpty());      if (mdp.isEmpty())       ok = false;
    markField(ui->lineEditTel,     telephone.isEmpty());if (telephone.isEmpty()) ok = false;
    markField(ui->lineEditAdresse, adresse.isEmpty());  if (adresse.isEmpty())   ok = false;

    if (!ok) {
        ui->lblErreurSignup->setText(tr("⚠  Veuillez remplir tous les champs obligatoires."));
        return;
    }
    if (mdp.length() < 6) {
        markField(ui->lineEditMDPS, true);
        ui->lblErreurSignup->setText(tr("⚠  Le mot de passe doit contenir au moins 6 caracteres."));
        ui->lineEditMDPS->setFocus();
        return;
    }
    client newClient(nom, prenom, email, mdp, telephone, adresse);
    if (newClient.sInscrireClient()) {
        QMessageBox::information(this, tr("Succes"),
                                 tr("Compte cree !\nVous pouvez vous connecter."));
        showLoginTab();
        clearFields();
    } else {
        markField(ui->lineEditEmailS, true);
        ui->lblErreurSignup->setText(tr("⚠  Cet email est deja utilise."));
    }
}

void LoginWindow::on_btnForgot_3_clicked()
{
    QString email = ui->lineEditEmail_3->text().trimmed();
    if (email.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Entrez votre email d'abord !"));
        return;
    }
    utilisateur u;
    if (u.demanderReset(email)) {
        ResetWindow* r = new ResetWindow(email, this);
        if (r->exec() == QDialog::Accepted) {
            NewPasswordWindow* np = new NewPasswordWindow(email, this);
            if (np->exec() == QDialog::Accepted) {
                ui->lineEditEmail_3->setText(email);
                ui->lineEditMDP_3->clear();
                ui->lineEditMDP_3->setFocus();
            }
            delete np;
        }
        delete r;
    } else {
        QMessageBox::critical(this, tr("Erreur"), tr("Email introuvable !"));
    }
}

void LoginWindow::clearFields()
{
    ui->lineEditEmail_3->clear();
    ui->lineEditMDP_3->clear();
    ui->lineEditNom->clear();
    ui->lineEditPrenom->clear();
    ui->lineEditEmailS->clear();
    ui->lineEditMDPS->clear();
    ui->lineEditTel->clear();
    ui->lineEditAdresse->clear();
    mdpLoginVisible = mdpSignupVisible = false;
    ui->lineEditMDP_3->setEchoMode(QLineEdit::Password);
    ui->lineEditMDPS->setEchoMode(QLineEdit::Password);
    ui->btnToggleMDP_3->setText(tr("Afficher"));
    ui->btnToggleMDPS->setText(tr("Afficher"));
}