#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QPixmap>
#include <QFile>
#include <QDateTime>
#include <QPrinter>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QCryptographicHash>
#include <QResizeEvent>
#include <QMenu>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "loginwindow.h"
#include "client.h"
#include "AIAssistant.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ClientWindow; }
QT_END_NAMESPACE

struct ArticlePanierUI {
    int     platId;
    QString nom;
    QString imagePath;
    double  prix;
    int     quantite;
};

class ClientWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClientWindow(const client& c, QWidget *parent = nullptr);
    ~ClientWindow();

private slots:
    void afficherPageMenu();
    void afficherPagePanier();
    void afficherPageCommandes();
    void afficherPageSuivi();
    void afficherPageAvis();
    void afficherPageProfil();
    void onLogout();

    void chargerCategories();
    void chargerPlats(int categorieId = -1);
    void onCategorieClicked(int categorieId);
    void onAjouterAuPanier(int platId, const QString& nom,
                           double prix, const QString& imagePath);
    void onRechercher(const QString& texte);

    void afficherPanier();
    void onAugmenterQuantite(int platId);
    void onDiminuerQuantite(int platId);
    void onSupprimerDuPanier(int platId);
    void onPasserCommande();

    void chargerCommandes();
    void onSuivreCommande(int commandeId);
    void afficherSuivi(int commandeId);

    void chargerAvis(const QString& tri = "recent");
    void soumettreAvis();
    void ouvrirDialogContact();

    void chargerProfil();
    void onSauvegarderProfil();
    void onChangerMotDePasse();

    void imprimerTicketCommande(int commandeId);

    void ouvrirAssistantIA();
    void rafraichirSuivi();         // ← timer refresh
    void demanderEstimationIA();    // ← appel Groq

    // ── Langue ──
    void retranslateUi();   // appelé par LanguageManager::languageChanged()

private:
    Ui::ClientWindow *ui;
    client  currentClient;
    QList<ArticlePanierUI> panier;
    int     commandeSuivieId = -1;
    int     noteSelectionnee = 0;
    QString photoProfilPath;
    QList<QPushButton*> boutonEtoiles;

    BoutonIA*      m_btnIA     = nullptr;
    AIAssistant*   m_assistant = nullptr;

    // ── Suivi temps réel ──
    QTimer*               m_timerSuivi  = nullptr;
    QNetworkAccessManager* m_networkIA  = nullptr;

    // ── Bouton langue dans la sidebar ──
    QPushButton* m_btnLang = nullptr;   // ← AJOUT

    void    updateSidebarStyle(int pageIndex);
    void    updateBadgePanier();
    double  calculerTotal() const;
    void    viderLayout(QLayout* layout);
    void    setNoteSelectionnee(int note);

    QWidget* creerCartePlat(const QMap<QString, QVariant>& plat);
    QWidget* creerArticlePanier(const ArticlePanierUI& article);
    QWidget* creerCarteCommande(int id, const QString& date,
                                const QString& statut,
                                int nbArticles, double total);
    QWidget* creerCarteAvis(const QString& nomClient, int note,
                            const QString& commentaire,
                            const QString& dateStr, int clientId);
    QWidget* creerCartePlatTop(const QString& nom, const QString& imagePath,
                               int nbCommandes, int rang);

    QString genererInitiales(const QString& nom) const;
    QString couleurPourId(int id) const;

    void setupBoutonIA();
    void setupBoutonLangue();   // ← AJOUT

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // CLIENTWINDOW_H