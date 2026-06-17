#include "client.h"
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
// Constructeur vide
client::client()
    : utilisateur(), adresse("")
{}

// Constructeur avec paramètres

client::client(const QString& nom,
               const QString& prenom,
               const QString& email,
               const QString& motDePasse,
               const QString& telephone,
               const QString& adresse)
    : utilisateur(nom, prenom, email, motDePasse, telephone, "client"),
    adresse(adresse)
{}
// Getter / Setter adresse
QString client::getAdresse() const { return adresse; }
void    client::setAdresse(const QString& a) { adresse = a; }

// Ajouter un plat au panier
void client::ajouterAuPanier(int platId,int quantite){
    // Vérifier si le plat est déjà dans le panier
    for (auto& article : panier) {
        if (article.first == platId) {
            article.second += quantite; // augmenter la quantité
            qDebug() << "Quantite mise a jour pour plat ID:" << platId;
            return;
        }
    }
    // Sinon ajouter le plat
    panier.append(qMakePair(platId, quantite));
    qDebug() << "Plat" << platId << "ajoute au panier (x" << quantite << ")";
}
// Supprimer un plat du panier
void client::supprimerDuPanier(int platId){
    for (int i=0 ;i<panier.size();i++){
        if (panier[i].first==platId){
            panier.removeAt(i);
            qDebug()<<"Plat"<<platId<<"supprime du panier";
            return;
        }
    }
    qDebug() << "Plat" << platId << "introuvable dans le panier";
}
// Vider le panier
void client::viderPanier()
{
    panier.clear();
    qDebug() << "Panier vide !";
}
// Obtenir le panier
QList<ArticlePanier> client::getPanier() const
{
    return panier;
}
// Calculer le total du panier
double client::calculerTotalPanier() const
{
    double total = 0.0;

    for (const auto& article : panier) {
        int platId    = article.first;
        int quantite  = article.second;

        // Récupérer le prix du plat depuis la DB
        QSqlQuery query;
        query.prepare("SELECT prix FROM plats WHERE id = :id");
        query.bindValue(":id", platId);

        if (query.exec() && query.next()) {
            double prix = query.value("prix").toDouble();
            total += prix * quantite;
        }
    }

    qDebug() << "Total panier :" << total << "DH";
    return total;
}

// Passer une commande

bool client::passerCommande()
{
    // Vérifier que le panier n'est pas vide
    if (panier.isEmpty()) {
        qDebug() << "Panier vide - impossible de passer commande !";
        return false;
    }

    // Calculer le total
    double total = calculerTotalPanier();

    // ── 1. Insérer la commande ──
    QSqlQuery query;
    query.prepare(
        "INSERT INTO commandes (client_id, statut, date_heure, total) "
        "VALUES (:client_id, 'en_attente', datetime('now'), :total)"
        );
    query.bindValue(":client_id", id);
    query.bindValue(":total",     total);

    if (!query.exec()) {
        qDebug() << "Erreur commande :" << query.lastError().text();
        return false;
    }

    // Récupérer l'id de la commande créée
    int commandeId = query.lastInsertId().toInt();
    qDebug() << "Commande creee ID :" << commandeId;

    // ── 2. Insérer les lignes de commande ──
    for (const auto& article : panier) {
        int platId   = article.first;
        int quantite = article.second;

        // Récupérer le prix du plat
        QSqlQuery prixQuery;
        prixQuery.prepare("SELECT prix FROM plats WHERE id = :id");
        prixQuery.bindValue(":id", platId);
        prixQuery.exec();
        prixQuery.next();
        double prix = prixQuery.value("prix").toDouble();

        // Insérer la ligne
        QSqlQuery ligneQuery;
        ligneQuery.prepare(
            "INSERT INTO lignes_commande (commande_id, plat_id, quantite, prix_unitaire) "
            "VALUES (:commande_id, :plat_id, :quantite, :prix)"
            );
        ligneQuery.bindValue(":commande_id", commandeId);
        ligneQuery.bindValue(":plat_id",     platId);
        ligneQuery.bindValue(":quantite",    quantite);
        ligneQuery.bindValue(":prix",        prix);

        if (!ligneQuery.exec()) {
            qDebug() << "Erreur ligne commande :" << ligneQuery.lastError().text();
            return false;
        }
    }

    qDebug() << "Commande passee avec succes ! Total :" << total << "DH";

    // Vider le panier après commande
    viderPanier();
    return true;
}
// Suivre une commande

bool client::suivreCommande(int commandeId) const
{
    QSqlQuery query;
    query.prepare(
        "SELECT statut, date_heure, total "
        "FROM commandes "
        "WHERE id = :id AND client_id = :client_id"
        );
    query.bindValue(":id",        commandeId);
    query.bindValue(":client_id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suivi :" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        QString statut     = query.value("statut").toString();
        QString dateHeure  = query.value("date_heure").toString();
        double  total      = query.value("total").toDouble();

        qDebug() << "Commande #" << commandeId;
        qDebug() << "Statut    :" << statut;
        qDebug() << "Date      :" << dateHeure;
        qDebug() << "Total     :" << total << "DH";
        return true;
    }

    qDebug() << "Commande introuvable !";
    return false;
}

// Voir historique des commandes

bool client::voirHistorique() const
{
    QSqlQuery query;
    query.prepare(
        "SELECT id, statut, date_heure, total "
        "FROM commandes "
        "WHERE client_id = :client_id "
        "ORDER BY date_heure DESC"
        );
    query.bindValue(":client_id", id);

    if (!query.exec()) {
        qDebug() << "Erreur historique :" << query.lastError().text();
        return false;
    }

    qDebug() << "── Historique des commandes ──";
    bool hasCommandes = false;

    while (query.next()) {
        hasCommandes = true;
        int     cmdId     = query.value("id").toInt();
        QString statut    = query.value("statut").toString();
        QString dateHeure = query.value("date_heure").toString();
        double  total     = query.value("total").toDouble();

        qDebug() << "Commande #" << cmdId
                 << "| Statut:" << statut
                 << "| Date:"   << dateHeure
                 << "| Total:"  << total << "DH";
    }

    if (!hasCommandes) {
        qDebug() << "Aucune commande trouvee.";
    }

    return true;
}
// Inscrire le client (utilisateurs + clients)

bool client::sInscrireClient()
{
    // ── 1. Insérer dans utilisateurs (classe mère) ──
    if (!sInscrire()) {
        return false; // email déjà utilisé ou erreur
    }

    // ── 2. Insérer dans clients ──
    QSqlQuery query;
    query.prepare(
        "INSERT INTO clients (id, adresse) "
        "VALUES (:id, :adresse)"
        );
    query.bindValue(":id",      id);
    query.bindValue(":adresse", adresse);

    if (!query.exec()) {
        qDebug() << "Erreur inscription client :" << query.lastError().text();
        return false;
    }

    qDebug() << "Client inscrit avec succes ! ID :" << id;
    return true;
}