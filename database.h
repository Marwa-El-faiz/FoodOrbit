// ============================================================
//  database.h
//  Déclaration du Singleton Database.
//  Une seule instance gère toute la connexion SQLite.
// ============================================================

#ifndef DATABASE_H
#define DATABASE_H

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>
#include <QString>

class Database
{
public:
    // ── Singleton ──
    // Retourne toujours la même instance (thread-safe en C++11)
    static Database& instance();

    // Ouvrir la BD (à appeler une seule fois au démarrage dans main.cpp)
    bool openDatabase();

    // Accès direct à la connexion (pour QSqlQuery dans les autres classes)
    QSqlDatabase& getDB();

    // Vérifier si la connexion est active
    bool isConnected();

    // Fermer proprement à la fin du programme
    void close();

private:
    // Constructeur privé → empêche l'instanciation directe
    Database() = default;

    // Copie et affectation interdites (règles du Singleton)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    QSqlDatabase db; // la connexion SQLite

    // Appelées automatiquement par openDatabase()
    void createTables();   // crée les tables si absentes
    void migrateSchema();  // ajoute les colonnes manquantes (image_path)
    void seedAdmin();      // insère le compte admin par défaut si absent
    void seedLivreur();    // insère le compte livreur de test si absent
    void seedTestData();   // insère le menu et comptes de test si vides
};

#endif // DATABASE_H