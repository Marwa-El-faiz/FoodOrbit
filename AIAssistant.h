#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlQuery>
#include <QTimer>
#include <QApplication>
#include <QScrollBar>
#include <functional>

extern const QString GROQ_API_KEY;
extern const QString GROQ_MODEL;
extern const QString GROQ_URL;

struct MessageChat {
    QString role;
    QString contenu;
};

using AjouterPlatFn = std::function<void(int platId, const QString& nom, double prix)>;

// ═══════════════════════════════════════════════════════════════
//  AIASSISTANT — Dialog chat
// ═══════════════════════════════════════════════════════════════
class AIAssistant : public QDialog
{
    Q_OBJECT

public:
    explicit AIAssistant(int clientId,
                         const QString& prenomClient,
                         AjouterPlatFn  callback = nullptr,
                         QWidget*       parent   = nullptr);
    ~AIAssistant();

private slots:
    void envoyerMessage();
    void onReponseRecue(QNetworkReply* reply);

private:
    QScrollArea*  m_scroll;
    QWidget*      m_chatWidget;
    QVBoxLayout*  m_chatLayout;
    QLineEdit*    m_input;
    QPushButton*  m_btnSend;
    QLabel*       m_lblTyping;

    QList<MessageChat>     m_historique;
    QNetworkAccessManager* m_network;
    AjouterPlatFn          m_callback;
    int                    m_clientId;
    QString                m_prenom;
    bool                   m_busy = false;

    void    setupUI();
    void    envoyerBienvenue();
    QString construireSystemPrompt() const;
    QString chargerMenuDB()          const;
    void    appelGroq();
    void    ajouterBubble(const QString& texte, bool estUser);
    void    afficherBoutonAjout(int platId, const QString& nom, double prix);
    void    analyserPourPlats(const QString& reponse);
    void    scrollBas();
};

// ═══════════════════════════════════════════════════════════════
//  BOUTON FLOTTANT
// ═══════════════════════════════════════════════════════════════
class BoutonIA : public QPushButton
{
    Q_OBJECT

public:
    explicit BoutonIA(QWidget* parent = nullptr);

private slots:
    void animer();

private:
    QTimer* m_timer;
    bool    m_etat = false;
};

#endif // AIASSISTANT_H