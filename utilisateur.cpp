#include "utilisateur.h"
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

// ─────────────────────────────────────────
// Constructeurs
// ─────────────────────────────────────────
utilisateur::utilisateur()
    : id(0), nom(""), prenom(""), email(""),
    motDePasse(""), telephone(""), role("")
{}

utilisateur::utilisateur(const QString& nom,
                         const QString& prenom,
                         const QString& email,
                         const QString& motDePasse,
                         const QString& telephone,
                         const QString& role)
    : id(0), nom(nom), prenom(prenom), email(email),
    motDePasse(motDePasse), telephone(telephone), role(role)
{}

// ─────────────────────────────────────────
// Getters
// ─────────────────────────────────────────
int     utilisateur::getId()        const { return id; }
QString utilisateur::getNom()       const { return nom; }
QString utilisateur::getPrenom()    const { return prenom; }
QString utilisateur::getEmail()     const { return email; }
QString utilisateur::getTelephone() const { return telephone; }
QString utilisateur::getRole()      const { return role; }

// ─────────────────────────────────────────
// Setters
// ─────────────────────────────────────────
void utilisateur::setNom(const QString& n)       { nom = n; }
void utilisateur::setPrenom(const QString& p)    { prenom = p; }
void utilisateur::setTelephone(const QString& t) { telephone = t; }
void utilisateur::setMotDePasse(const QString& m){ motDePasse = m; }

// ─────────────────────────────────────────
// Hash SHA-256
// ─────────────────────────────────────────
static QString hasherMDP(const QString& mdp)
{
    return QString(QCryptographicHash::hash(
                       mdp.toUtf8(),
                       QCryptographicHash::Sha256
                       ).toHex());
}

// ─────────────────────────────────────────
// Se connecter
// ─────────────────────────────────────────
bool utilisateur::seConnecter(const QString& emailSaisi, const QString& mdpSaisi)
{
    // Sécurité : rejeter les champs vides immédiatement
    if (emailSaisi.trimmed().isEmpty() || mdpSaisi.isEmpty()) {
        qDebug() << "Email ou mot de passe vide !";
        return false;
    }

    QSqlQuery query;
    query.prepare("SELECT id, nom, prenom, telephone, role "
                  "FROM utilisateurs "
                  "WHERE LOWER(email) = LOWER(:email) AND mot_de_passe = :mdp");
    query.bindValue(":email", emailSaisi.trimmed());
    query.bindValue(":mdp",   hasherMDP(mdpSaisi));

    if (!query.exec()) {
        qDebug() << "Erreur connexion :" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        id        = query.value("id").toInt();
        nom       = query.value("nom").toString();
        prenom    = query.value("prenom").toString();
        email     = emailSaisi.trimmed().toLower();
        telephone = query.value("telephone").toString();
        role      = query.value("role").toString();
        qDebug() << "Connexion reussie :" << nom << " | Role :" << role;
        return true;
    }

    qDebug() << "Email ou mot de passe incorrect";
    return false;
}

// ─────────────────────────────────────────
// S'inscrire
// ─────────────────────────────────────────
bool utilisateur::sInscrire()
{
    // Vérifier si l'email existe déjà (comparaison insensible à la casse)
    QSqlQuery check;
    check.prepare("SELECT id FROM utilisateurs WHERE LOWER(email) = LOWER(:email)");
    check.bindValue(":email", email);
    check.exec();

    if (check.next()) {
        qDebug() << "Email deja utilise !";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO utilisateurs (nom, prenom, email, mot_de_passe, telephone, role) "
        "VALUES (:nom, :prenom, :email, :mdp, :tel, :role)"
        );
    query.bindValue(":nom",    nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email",  email.trimmed().toLower()); // ← stocker en minuscules
    query.bindValue(":mdp",    hasherMDP(motDePasse));
    query.bindValue(":tel",    telephone);
    query.bindValue(":role",   role);

    if (!query.exec()) {
        qDebug() << "Erreur inscription :" << query.lastError().text();
        return false;
    }

    id = query.lastInsertId().toInt();
    qDebug() << "Inscription reussie ! ID :" << id;
    return true;
}

// ─────────────────────────────────────────
// Créer un admin (utilitaire — appeler 1 seule fois au démarrage)
// ─────────────────────────────────────────
// UTILISATION : appelez cette méthode depuis main.cpp ou mainwindow après
// l'ouverture de la DB pour créer l'admin si il n'existe pas encore :
//
//   utilisateur::creerAdminSiAbsent("admin@foodrush.com", "motDePasse123");
//
void utilisateur::creerAdminSiAbsent(const QString& emailAdmin, const QString& mdpAdmin)
{
    QSqlQuery check;
    check.prepare("SELECT id FROM utilisateurs WHERE LOWER(email) = LOWER(:email)");
    check.bindValue(":email", emailAdmin);
    check.exec();

    if (check.next()) {
        qDebug() << "Admin deja present en DB.";
        return;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO utilisateurs (nom, prenom, email, mot_de_passe, telephone, role) "
        "VALUES ('Admin', 'Super', :email, :mdp, '0000000000', 'admin')"
        );
    query.bindValue(":email", emailAdmin.trimmed().toLower());
    query.bindValue(":mdp",   hasherMDP(mdpAdmin));

    if (query.exec()) {
        qDebug() << "Admin cree avec succes ! Email :" << emailAdmin;
    } else {
        qDebug() << "Erreur creation admin :" << query.lastError().text();
    }
}

// ─────────────────────────────────────────
// Afficher les infos
// ─────────────────────────────────────────
QString utilisateur::getInfo() const
{
    return QString("ID: %1 | Nom: %2 %3 | Email: %4 | Rôle: %5")
        .arg(id)
        .arg(prenom)
        .arg(nom)
        .arg(email)
        .arg(role);
}

// ─────────────────────────────────────────
// Envoyer email via API Node.js
// ─────────────────────────────────────────
void utilisateur::envoyerEmailAPI(const QString& email, const QString& code)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager();

    QUrl url("http://localhost:3000/send-code");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["email"] = email;
    json["code"]  = code;

    QJsonDocument doc(json);
    manager->post(request, doc.toJson());

    qDebug() << "Email envoye a :" << email << "| Code :" << code;
}

// ─────────────────────────────────────────
// Vérifier le code reset
// ─────────────────────────────────────────
bool utilisateur::verifierCode(const QString& emailSaisi, const QString& code)
{
    QSqlQuery query;
    query.prepare(
        "SELECT reset_token, reset_expiration "
        "FROM utilisateurs "
        "WHERE LOWER(email) = LOWER(:email)"
        );
    query.bindValue(":email", emailSaisi);

    if (!query.exec() || !query.next()) {
        qDebug() << "Email introuvable !";
        return false;
    }

    QString tokenDB      = query.value("reset_token").toString();
    QString expirationDB = query.value("reset_expiration").toString();

    if (tokenDB != code) {
        qDebug() << "Code incorrect !";
        return false;
    }

    QDateTime expiration = QDateTime::fromString(expirationDB, Qt::ISODate);
    if (QDateTime::currentDateTime() > expiration) {
        qDebug() << "Code expire !";
        return false;
    }

    qDebug() << "Code correct !";
    return true;
}

// ─────────────────────────────────────────
// Réinitialiser le mot de passe
// ─────────────────────────────────────────
bool utilisateur::resetMotDePasse(const QString& emailSaisi, const QString& newMDP)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE utilisateurs "
        "SET mot_de_passe = :mdp, "
        "    reset_token = NULL, "
        "    reset_expiration = NULL "
        "WHERE LOWER(email) = LOWER(:email)"
        );
    query.bindValue(":mdp",   hasherMDP(newMDP));
    query.bindValue(":email", emailSaisi);

    if (!query.exec()) {
        qDebug() << "Erreur reset :" << query.lastError().text();
        return false;
    }

    qDebug() << "Mot de passe reinitialise avec succes !";
    return true;
}

// ─────────────────────────────────────────
// Demander reset (envoi code par email)
// ─────────────────────────────────────────
bool utilisateur::demanderReset(const QString& emailSaisi)
{
    QSqlQuery query(Database::instance().getDB());
    query.prepare("SELECT id FROM utilisateurs WHERE LOWER(email) = LOWER(:email)");
    query.bindValue(":email", emailSaisi);
    if (!query.exec() || !query.next()) {
        qDebug() << "Email introuvable";
        return false;
    }

    QString code = QString::number(
        QRandomGenerator::global()->bounded(100000, 999999)
        );

    QString expiration = QDateTime::currentDateTime()
                             .addSecs(600)
                             .toString(Qt::ISODate);

    QSqlQuery updateQuery;
    updateQuery.prepare(
        "UPDATE utilisateurs "
        "SET reset_token = :token, "
        "    reset_expiration = :expiration "
        "WHERE LOWER(email) = LOWER(:email)"
        );
    updateQuery.bindValue(":token",      code);
    updateQuery.bindValue(":expiration", expiration);
    updateQuery.bindValue(":email",      emailSaisi);
    updateQuery.exec();

    envoyerEmailAPI(emailSaisi, code);
    return true;
}