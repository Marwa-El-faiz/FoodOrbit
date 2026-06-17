#include "ordervalidator.h"

// ── Réutilise la même clé Groq que AIAssistant ───────────────────────────────
// Si AIAssistant.h n'est pas dans le projet, décommentez et renseignez :
// static const QString GROQ_API_KEY = "VOTRE_CLE_GROQ_ICI";
#include "AIAssistant.h"   // pour GROQ_API_KEY et GROQ_URL

// ═══════════════════════════════════════════════════════════════════════════════
//  CONSTRUCTEUR
// ═══════════════════════════════════════════════════════════════════════════════
OrderValidator::OrderValidator(QObject* parent) : QObject(parent)
{
    m_network = new QNetworkAccessManager(this);
    connect(m_network, &QNetworkAccessManager::finished,
            this, &OrderValidator::onReponseRecue);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  POINT D'ENTRÉE
// ═══════════════════════════════════════════════════════════════════════════════
void OrderValidator::valider(int commandeId, ValidationCallback callback)
{
    m_commandeId = commandeId;
    m_callback   = callback;

    ContexteCommande ctx = collecterContexte(commandeId);

    // Si pas de clé Groq configurée → fallback local
    if (GROQ_API_KEY == "VOTRE_CLE_GROQ_ICI" || GROQ_API_KEY.isEmpty()) {
        qDebug() << "OrderValidator: pas de clé Groq, fallback local";
        ValidationResult r = fallbackLocal(ctx);
        mettreAJourStatutDB(commandeId, r.statut);
        if (m_callback) m_callback(r);
        return;
    }

    // ── Appel Groq ──
    QUrl url(GROQ_URL);                          // ← étape 1 : construire QUrl séparément
    QNetworkRequest req;                          // ← étape 2 : construire sans argument
    req.setUrl(url);                              // ← étape 3 : setUrl évite le Most Vexing Parse
    req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));
    req.setRawHeader(QByteArray("Authorization"),
                     ("Bearer " + GROQ_API_KEY).toUtf8());

    QJsonArray messages;

    QJsonObject sys;
    sys["role"]    = "system";
    sys["content"] = construirePrompt(ctx);
    messages.append(sys);

    QJsonObject usr;
    usr["role"]    = "user";
    usr["content"] = QString(
                         "Valide la commande #%1 selon le contexte fourni. "
                         "Réponds UNIQUEMENT en JSON valide, sans texte autour."
                         ).arg(commandeId);
    messages.append(usr);

    QJsonObject body;
    body["model"]       = GROQ_MODEL;
    body["messages"]    = messages;
    body["max_tokens"]  = 200;
    body["temperature"] = 0.2;   // basse pour des réponses cohérentes
    body["stream"]      = false;

    QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    m_network->post(req, payload);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  COLLECTER LE CONTEXTE DEPUIS LA DB
// ═══════════════════════════════════════════════════════════════════════════════
OrderValidator::ContexteCommande OrderValidator::collecterContexte(int commandeId) const
{
    ContexteCommande ctx;
    ctx.commandeId = commandeId;

    // ── Infos commande ──
    QSqlQuery qCmd;
    qCmd.prepare(
        "SELECT client_id, total FROM commandes WHERE id = :id");
    qCmd.bindValue(":id", commandeId);
    qCmd.exec();
    if (qCmd.next()) {
        ctx.clientId = qCmd.value(0).toInt();
        ctx.total    = qCmd.value(1).toDouble();
    }

    // ── Liste des plats commandés ──
    QSqlQuery qItems;
    qItems.prepare(
        "SELECT p.nom, lc.quantite "
        "FROM lignes_commande lc JOIN plats p ON lc.plat_id = p.id "
        "WHERE lc.commande_id = :id");
    qItems.bindValue(":id", commandeId);
    qItems.exec();

    QStringList items;
    int nbPlats = 0;
    while (qItems.next()) {
        int qty = qItems.value(1).toInt();
        items << QString("%1x %2").arg(qty).arg(qItems.value(0).toString());
        nbPlats += qty;
    }
    ctx.nbPlats    = nbPlats;
    ctx.listeItems = items.join(", ");

    // ── Commandes actuellement en cuisine (en_attente + confirmee + en_preparation) ──
    QSqlQuery qActives;
    qActives.exec(
        "SELECT COUNT(*) FROM commandes "
        "WHERE statut IN ('en_attente','confirmee','en_preparation')");
    ctx.commandesActives = qActives.next() ? qActives.value(0).toInt() : 0;

    // ── Heure et jour ──
    QDateTime now = QDateTime::currentDateTime();
    ctx.heureActuelle = now.toString("HH:mm");
    static const QStringList jours = {
                                      "Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi","Dimanche"};
    ctx.jourSemaine = jours[now.date().dayOfWeek() - 1];

    int h = now.time().hour();
    ctx.heurePointe = (h >= 12 && h <= 14) || (h >= 19 && h <= 21);

    return ctx;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  CONSTRUIRE LE PROMPT SYSTÈME POUR GROQ
// ═══════════════════════════════════════════════════════════════════════════════
QString OrderValidator::construirePrompt(const ContexteCommande& ctx) const
{
    return QString(
               "Tu es un système de validation de commandes pour un restaurant de livraison.\n"
               "Ton rôle : analyser le contexte et décider si la commande peut être confirmée.\n\n"

               "CONTEXTE DE LA COMMANDE :\n"
               "  • Commande #%1\n"
               "  • Plats : %2\n"
               "  • Nombre d'articles : %3\n"
               "  • Total : %4 DH\n"
               "  • Heure actuelle : %5 (%6)\n"
               "  • Commandes actuellement en cuisine : %7\n"
               "  • Heure de pointe : %8\n\n"

               "RÈGLES DE DÉCISION :\n"
               "  - Si charge > 15 commandes simultanées → 'en_attente' avec délai majoré\n"
               "  - Si charge > 25 commandes → 'annulee' (cuisine saturée)\n"
               "  - Si heure de pointe ET charge > 10 → délai +15 min\n"
               "  - Délai de base : 25 min + 2 min par article commandé\n"
               "  - Délai heure de pointe : base × 1.4\n"
               "  - Commande > 300 DH → toujours confirmer, priorité VIP\n\n"

               "RÉPONDS UNIQUEMENT avec ce JSON (sans markdown, sans texte autour) :\n"
               "{\n"
               "  \"statut\": \"confirmee\" | \"en_attente\" | \"annulee\",\n"
               "  \"delai_estime\": <minutes entier>,\n"
               "  \"raison\": \"<explication courte en français pour le client>\"\n"
               "}"
               )
        .arg(ctx.commandeId)
        .arg(ctx.listeItems)
        .arg(ctx.nbPlats)
        .arg(ctx.total, 0, 'f', 2)
        .arg(ctx.heureActuelle)
        .arg(ctx.jourSemaine)
        .arg(ctx.commandesActives)
        .arg(ctx.heurePointe ? "Oui" : "Non");
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TRAITEMENT DE LA RÉPONSE GROQ
// ═══════════════════════════════════════════════════════════════════════════════
void OrderValidator::onReponseRecue(QNetworkReply* reply)
{
    ContexteCommande ctx = collecterContexte(m_commandeId);
    ValidationResult result;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "OrderValidator: erreur réseau →" << reply->errorString();
        result = fallbackLocal(ctx);
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonArray choices = doc.object()["choices"].toArray();
            if (!choices.isEmpty()) {
                QString content = choices[0].toObject()["message"]
                                      .toObject()["content"].toString().trimmed();
                qDebug() << "OrderValidator réponse IA :" << content;
                result = parseReponseIA(content);
            } else {
                result = fallbackLocal(ctx);
            }
        } else {
            result = fallbackLocal(ctx);
        }
    }

    reply->deleteLater();

    // Mettre à jour le statut en DB
    mettreAJourStatutDB(m_commandeId, result.statut);

    // Appeler le callback
    if (m_callback) m_callback(result);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  PARSER LA RÉPONSE JSON DE L'IA
// ═══════════════════════════════════════════════════════════════════════════════
ValidationResult OrderValidator::parseReponseIA(const QString& jsonStr) const
{
    ValidationResult result;

    // Nettoyer les éventuels blocs markdown ```json ... ```
    QString clean = jsonStr;
    clean.remove("```json").remove("```").remove("`");
    clean = clean.trimmed();

    QJsonDocument doc = QJsonDocument::fromJson(clean.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        result.statut      = obj["statut"].toString("confirmee");
        result.delaiEstime = obj["delai_estime"].toInt(30);
        result.raison      = obj["raison"].toString("Commande acceptée.");

        // Validation des valeurs
        if (result.statut != "confirmee" &&
            result.statut != "en_attente" &&
            result.statut != "annulee") {
            result.statut = "confirmee";
        }
        if (result.delaiEstime < 10) result.delaiEstime = 25;
        if (result.delaiEstime > 120) result.delaiEstime = 60;

    } else {
        // JSON invalide → fallback
        qDebug() << "OrderValidator: JSON invalide, fallback";
        result = fallbackLocal(collecterContexte(m_commandeId));
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  FALLBACK LOCAL (sans IA) — logique simple
// ═══════════════════════════════════════════════════════════════════════════════
ValidationResult OrderValidator::fallbackLocal(const ContexteCommande& ctx) const
{
    ValidationResult result;

    int delaiBase = 25 + ctx.nbPlats * 2;
    if (ctx.heurePointe) delaiBase = qRound(delaiBase * 1.4);

    if (ctx.commandesActives > 25) {
        result.statut      = "annulee";
        result.delaiEstime = 0;
        result.raison      = "La cuisine est saturée en ce moment. "
                        "Réessayez dans 30 minutes.";

    } else if (ctx.commandesActives > 15 || (ctx.heurePointe && ctx.commandesActives > 10)) {
        result.statut      = "en_attente";
        result.delaiEstime = delaiBase + 15;
        result.raison      = QString("Forte affluence en ce moment (%1 commandes en cours). "
                                "Votre commande est en attente, délai estimé : %2 min.")
                            .arg(ctx.commandesActives).arg(result.delaiEstime);

    } else {
        result.statut      = "confirmee";
        result.delaiEstime = delaiBase;
        result.raison      = ctx.heurePointe
                            ? QString("Commande confirmée ! Heure de pointe — délai estimé : %1 min.")
                                  .arg(delaiBase)
                            : QString("Commande confirmée ! Délai estimé : %1 min.")
                                  .arg(delaiBase);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  METTRE À JOUR LE STATUT EN DB
// ═══════════════════════════════════════════════════════════════════════════════
void OrderValidator::mettreAJourStatutDB(int commandeId, const QString& statut) const
{
    QSqlQuery q;
    q.prepare("UPDATE commandes SET statut = :s WHERE id = :id");
    q.bindValue(":s",  statut);
    q.bindValue(":id", commandeId);
    if (!q.exec())
        qDebug() << "OrderValidator: erreur UPDATE statut →" << q.lastError().text();
    else
        qDebug() << "OrderValidator: commande" << commandeId << "→" << statut;
}