#ifndef CLIENT_H
#define CLIENT_H

#include "utilisateur.h"
#include <QList>
#include <QPair>
// QPair<int, int> → (plat_id, quantite)
using ArticlePanier = QPair<int, int>;

class client :public utilisateur
{
public:
    client();
    client(const QString& nom,
           const QString& prenom,
           const QString& email,
           const QString& motDePasse,
           const QString& telephone,
           const QString& adresse);


    // ── Getter / Setter adresse ──
    QString getAdresse() const;
    void    setAdresse(const QString& adresse);

    // gestion panier
    void ajouterAuPanier(int platId, int quantite);
    void supprimerDuPanier(int platId);
    void viderPanier();
    QList<ArticlePanier> getPanier() const;
    double calculerTotalPanier() const;
    //commandes

    bool passerCommande();
    bool suivreCommande(int commandeId) const;
    bool voirHistorique() const;


    // ── Enregistrer dans la DB ──
    bool    sInscrireClient();
private:
    QString              adresse;
    QList<ArticlePanier> panier; // liste des plats dans le panier



};

#endif // CLIENT_H