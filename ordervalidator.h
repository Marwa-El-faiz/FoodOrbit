#ifndef ORDERVALIDATOR_H
#define ORDERVALIDATOR_H

#include <QObject>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <functional>

// ── Résultat de la validation IA ─────────────────────────────────────────────
struct ValidationResult {
    QString statut;       // "confirmee" | "en_attente" | "annulee"
    int     delaiEstime;  // minutes estimées avant livraison
    QString raison;       // explication textuelle pour le client
};

// ── Callback appelé quand la validation est terminée ─────────────────────────
using ValidationCallback = std::function<void(const ValidationResult&)>;

// ═══════════════════════════════════════════════════════════════════════════════
//  ORDERVALIDATOR
//  Valide une commande via Groq AI en tenant compte de :
//    - L'heure actuelle (horaires d'ouverture)
//    - Le nombre de commandes en cours (charge cuisine)
//    - Le total de la commande
//    - Les plats commandés
//  Retourne une ValidationResult via callback asynchrone.
// ═══════════════════════════════════════════════════════════════════════════════
class OrderValidator : public QObject
{
    Q_OBJECT

public:
    explicit OrderValidator(QObject* parent = nullptr);

    // Lance la validation — le callback est appelé quand l'IA répond
    void valider(int commandeId, ValidationCallback callback);

private slots:
    void onReponseRecue(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_network;
    ValidationCallback     m_callback;
    int                    m_commandeId = -1;

    // ── Collecte du contexte depuis la DB ──
    struct ContexteCommande {
        int     commandeId;
        int     clientId;
        double  total;
        int     nbPlats;
        QString listeItems;      // "2x Pizza, 1x Burger"
        int     commandesActives;// nombre de commandes en cours en cuisine
        QString heureActuelle;   // "14:32"
        QString jourSemaine;     // "Lundi"
        bool    heurePointe;     // true si 12h-14h ou 19h-21h
    };

    ContexteCommande collecterContexte(int commandeId) const;
    QString          construirePrompt(const ContexteCommande& ctx) const;
    ValidationResult parseReponseIA(const QString& json) const;
    ValidationResult fallbackLocal(const ContexteCommande& ctx) const;
    void             mettreAJourStatutDB(int commandeId, const QString& statut) const;
};

#endif // ORDERVALIDATOR_H