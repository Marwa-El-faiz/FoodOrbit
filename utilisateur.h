#ifndef UTILISATEUR_H
#define UTILISATEUR_H

#include <QString>
#include <QNetworkAccessManager>
#include "database.h"

class utilisateur
{
public:
    // Constructeurs
    utilisateur();
    utilisateur(const QString& nom,
                const QString& prenom,
                const QString& email,
                const QString& motDePasse,
                const QString& telephone,
                const QString& role);

    // Getters
    int     getId()        const;
    QString getNom()       const;
    QString getPrenom()    const;
    QString getEmail()     const;
    QString getTelephone() const;
    QString getRole()      const;

    // Setters
    void setNom(const QString& nom);
    void setPrenom(const QString& prenom);
    void setTelephone(const QString& telephone);
    void setMotDePasse(const QString& motDePasse);

    // Méthodes principales
    bool    seConnecter(const QString& email, const QString& motDePasse);
    bool    sInscrire();
    QString getInfo() const;

    // ── Utilitaire admin ──
    // À appeler UNE SEULE FOIS au démarrage (ex: dans main.cpp)
    // pour créer l'admin s'il n'existe pas encore en DB.
    static void creerAdminSiAbsent(const QString& emailAdmin, const QString& mdpAdmin);

    // Email reset
    bool demanderReset(const QString& email);
    void envoyerEmailAPI(const QString& email, const QString& code);
    bool verifierCode(const QString& email, const QString& code);
    bool resetMotDePasse(const QString& email, const QString& newMDP);

protected:
    int     id;
    QString nom;
    QString prenom;
    QString email;
    QString motDePasse;
    QString telephone;
    QString role;
};

#endif // UTILISATEUR_H