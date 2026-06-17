#include "clientwindow.h"
#include <algorithm>
#include "ui_clientwindow.h"
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "languagemanager.h"   // ← AJOUT

ClientWindow::ClientWindow(const client& c, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ClientWindow), currentClient(c)
{
    ui->setupUi(this);

    // Style global QMessageBox
    qApp->setStyleSheet(qApp->styleSheet() +
                        "QMessageBox { background-color: #F4F6F0; font-family: 'Segoe UI', sans-serif; }"
                        "QMessageBox QLabel { color: #1B4332; font-size: 14px; font-weight: bold; min-width: 250px; }"
                        "QMessageBox QPushButton { background-color: #2D6A4F; color: white; border: none;"
                        "  border-radius: 8px; padding: 8px 24px; font-size: 13px; font-weight: bold; min-width: 80px; }"
                        "QMessageBox QPushButton:hover { background-color: #1B4332; }"
                        "QMessageBox QDialogButtonBox { button-layout: 0; }"
                        );

    setMinimumSize(1100, 700);

    // Fix sidebar background
    ui->sidebar->setAutoFillBackground(true);
    QPalette pal = ui->sidebar->palette();
    pal.setColor(QPalette::Window, QColor("#2D6A4F"));
    ui->sidebar->setPalette(pal);

    ui->lblNomClient->setText(c.getPrenom() + " " + c.getNom());

    connect(ui->btnMenu,      &QPushButton::clicked, this, &ClientWindow::afficherPageMenu);
    connect(ui->btnPanier,    &QPushButton::clicked, this, &ClientWindow::afficherPagePanier);
    connect(ui->btnCommandes, &QPushButton::clicked, this, &ClientWindow::afficherPageCommandes);
    connect(ui->btnSuivi,     &QPushButton::clicked, this, &ClientWindow::afficherPageSuivi);
    connect(ui->btnAvis,      &QPushButton::clicked, this, &ClientWindow::afficherPageAvis);
    connect(ui->btnProfil,    &QPushButton::clicked, this, &ClientWindow::afficherPageProfil);
    connect(ui->btnLogout,    &QPushButton::clicked, this, &ClientWindow::onLogout);

    connect(ui->btnPasserCommande,     &QPushButton::clicked, this, &ClientWindow::onPasserCommande);
    connect(ui->btnPanierHeader,       &QPushButton::clicked, this, &ClientWindow::afficherPagePanier);
    connect(ui->lineEditRecherche,     &QLineEdit::textChanged, this, &ClientWindow::onRechercher);

    connect(ui->btnSoumettreAvis,       &QPushButton::clicked, this, &ClientWindow::soumettreAvis);
    connect(ui->btnAvisRecents,         &QPushButton::clicked, this, [this](){ chargerAvis("recent"); });
    connect(ui->btnAvisMieuxNotes,      &QPushButton::clicked, this, [this](){ chargerAvis("note"); });
    connect(ui->btnContacterRestaurant, &QPushButton::clicked, this, &ClientWindow::ouvrirDialogContact);

    connect(ui->btnSauvegarderProfil, &QPushButton::clicked, this, &ClientWindow::onSauvegarderProfil);
    connect(ui->btnChangerMdp,        &QPushButton::clicked, this, &ClientWindow::onChangerMotDePasse);

    connect(ui->btnChoisirPhoto, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, tr("Choisir une photo de profil"), "",
            tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (path.isEmpty()) return;
        photoProfilPath = path;
        QPixmap px(path);
        QPixmap rounded(80, 80);
        rounded.fill(Qt::transparent);
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path2;
        path2.addEllipse(0, 0, 80, 80);
        painter.setClipPath(path2);
        painter.drawPixmap(0, 0, px.scaled(80, 80, Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation));
        painter.end();
        ui->lblAvatarProfil->setPixmap(rounded);
        ui->lblAvatarProfil->setStyleSheet(
            "QLabel#lblAvatarProfil{border-radius:40px;border:3px solid #B7E4C7;background:transparent;}");
        ui->btnChoisirPhoto->setText(tr("📷  Changer la photo"));
    });

    boutonEtoiles = { ui->btnEtoile1, ui->btnEtoile2,
                     ui->btnEtoile3, ui->btnEtoile4, ui->btnEtoile5 };
    for (int i = 0; i < boutonEtoiles.size(); i++) {
        int n = i + 1;
        connect(boutonEtoiles[i], &QPushButton::clicked, this,
                [this, n](){ setNoteSelectionnee(n); });
    }

    // ── Bouton IA ──
    setupBoutonIA();

    // ── Timer : rafraîchissement suivi toutes les 30s ──────────────────────
    m_timerSuivi = new QTimer(this);
    connect(m_timerSuivi, &QTimer::timeout, this, &ClientWindow::rafraichirSuivi);
    m_timerSuivi->start(30 * 1000);

    // ── Network pour estimation IA ──────────────────────────────────────────
    m_networkIA = new QNetworkAccessManager(this);


    // ── Bouton Langue dans la sidebar ──
    setupBoutonLangue();   // ← AJOUT

    // ── Connexion au signal de langue ──
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged,
            this, &ClientWindow::retranslateUi);

    // ── Textes initiaux ──
    retranslateUi();

    afficherPageMenu();
}

ClientWindow::~ClientWindow() { delete ui; }

// ── Bouton langue dans la sidebar ────────────────────────────────────────────
void ClientWindow::setupBoutonLangue()
{
    m_btnLang = new QPushButton("🌐  " + tr("Langue"), ui->sidebar);
    m_btnLang->setCursor(Qt::PointingHandCursor);
    m_btnLang->setStyleSheet(
        "QPushButton{background:transparent;color:rgba(255,255,255,0.75);"
        "border-radius:10px;font-size:13px;text-align:left;padding:10px 20px;border:none;}"
        "QPushButton:hover{background:rgba(255,255,255,0.1);color:white;}");

    QMenu* menu = new QMenu(m_btnLang);
    menu->setStyleSheet(
        "QMenu{background:#1B4332;border-radius:8px;border:1px solid #40916C;padding:4px;}"
        "QMenu::item{color:white;padding:8px 20px;font-size:13px;border-radius:6px;}"
        "QMenu::item:selected{background:#40916C;}");

    menu->addAction("🇫🇷  Français",  []{ LanguageManager::instance().loadLanguage("fr"); });
    menu->addAction("🇸🇦  العربية",   []{ LanguageManager::instance().loadLanguage("ar"); });
    menu->addAction("🇬🇧  English",   []{ LanguageManager::instance().loadLanguage("en"); });

    m_btnLang->setMenu(menu);

    // Insérer dans le layout sidebar — avant le bouton Déconnexion
    QVBoxLayout* sidebarLayout = qobject_cast<QVBoxLayout*>(ui->sidebar->layout());
    if (sidebarLayout) {
        // Chercher l'index du btnLogout et insérer avant
        int logoutIndex = sidebarLayout->indexOf(ui->btnLogout);
        if (logoutIndex >= 0)
            sidebarLayout->insertWidget(logoutIndex, m_btnLang);
        else
            sidebarLayout->addWidget(m_btnLang);
    }
}

// ── Mise à jour de tous les textes traduisibles ───────────────────────────────
void ClientWindow::retranslateUi()
{
    setWindowTitle(tr("FoodOrbit — Espace Client"));

    // Sidebar — navigation
    ui->btnMenu->setText("  " + tr("Menu"));
    ui->btnPanier->setText("  " + tr("Panier"));
    ui->btnCommandes->setText("  " + tr("Commandes"));
    ui->btnSuivi->setText("  " + tr("Suivi"));
    ui->btnAvis->setText("  " + tr("Avis"));
    ui->btnProfil->setText("  " + tr("Mon Profil"));
    ui->btnLogout->setText("  " + tr("Déconnexion"));
    if (m_btnLang)
        m_btnLang->setText("🌐  " + tr("Langue"));

    // Header pages
    ui->lblTitreMenu->setText(tr("Menu du Restaurant"));
    ui->lblSousTitreMenu->setText(tr("Choisissez vos plats préférés"));
    ui->lineEditRecherche->setPlaceholderText(tr("   Rechercher un plat..."));

    ui->lblTitrePanier->setText(tr("Mon Panier"));
    ui->lblTitreCommandes->setText(tr("Mes Commandes"));
    ui->lblSousTitreCommandes->setText(tr("Historique de vos commandes"));
    ui->lblTitreSuivi->setText(tr("Suivi de commande"));
    ui->lblTitreAvis->setText(tr("Recommandations & Avis"));
    ui->lblSousTitreAvis->setText(tr("L'expérience de nos clients"));
    ui->lblTitreProfil->setText(tr("Mon Profil"));
    ui->lblSousTitreProfil->setText(tr("Gérez vos informations personnelles"));

    // Panier — récapitulatif
    ui->lblLabelSousTotal->setText(tr("Sous-total"));
    ui->lblLabelLivraison->setText(tr("Frais de livraison"));
    ui->lblLabelTotal->setText(tr("Total"));
    ui->btnPasserCommande->setText(tr("Passer la commande  →"));

    // Suivi — étapes
    ui->lblTitreEtapes->setText(tr("État de la commande"));
    ui->lblTitreDetails->setText(tr("Détails de la commande"));
    ui->lblTempsLabel->setText(tr("Temps estimé"));

    // Avis
    ui->btnAvisRecents->setText(tr("Les plus récents"));
    ui->btnAvisMieuxNotes->setText(tr("Les mieux notés"));
    ui->lblNoteChoisie->setText(tr("Cliquez sur une étoile"));
    ui->btnSoumettreAvis->setText(tr("Publier mon avis"));
    ui->btnContacterRestaurant->setText(tr("Contacter le restaurant"));
    ui->textEditCommentaire->setPlaceholderText(
        tr("Partagez votre expérience avec les autres clients..."));

    // Profil
    ui->btnChoisirPhoto->setText(tr("  Choisir une photo"));
    ui->btnSauvegarderProfil->setText(tr("Enregistrer les modifications"));
    ui->btnChangerMdp->setText(tr("Changer le mot de passe"));

    // Mettre à jour le badge panier (contient un texte)
    updateBadgePanier();
}

// ─────────────────────────────────────────────────────────────────────────────

void ClientWindow::afficherPageMenu()      { ui->stackedWidget->setCurrentIndex(0); updateSidebarStyle(0); chargerCategories(); chargerPlats(); }
void ClientWindow::afficherPagePanier()    { ui->stackedWidget->setCurrentIndex(1); updateSidebarStyle(1); afficherPanier(); }
void ClientWindow::afficherPageCommandes() { ui->stackedWidget->setCurrentIndex(2); updateSidebarStyle(2); chargerCommandes(); }
void ClientWindow::afficherPageSuivi()     { ui->stackedWidget->setCurrentIndex(3); updateSidebarStyle(3); if (commandeSuivieId >= 0) afficherSuivi(commandeSuivieId); }
void ClientWindow::afficherPageAvis()      { ui->stackedWidget->setCurrentIndex(4); updateSidebarStyle(4); chargerAvis(); }
void ClientWindow::afficherPageProfil()    { ui->stackedWidget->setCurrentIndex(5); updateSidebarStyle(5); chargerProfil(); }

void ClientWindow::onLogout()
{
    LoginWindow* lw = new LoginWindow();
    lw->show();
    this->close();
}

void ClientWindow::updateSidebarStyle(int pageIndex)
{
    QString actif = R"(QPushButton{background:#40916C;color:white;border-radius:10px;
        font-size:14px;font-weight:bold;text-align:left;padding:12px 20px;border:none;})";
    QString inactif = R"(QPushButton{background:transparent;color:rgba(255,255,255,0.75);
        border-radius:10px;font-size:14px;text-align:left;padding:12px 20px;border:none;}
        QPushButton:hover{background:rgba(255,255,255,0.1);color:white;})";

    ui->btnMenu->setStyleSheet(pageIndex      == 0 ? actif : inactif);
    ui->btnPanier->setStyleSheet(pageIndex    == 1 ? actif : inactif);
    ui->btnCommandes->setStyleSheet(pageIndex == 2 ? actif : inactif);
    ui->btnSuivi->setStyleSheet(pageIndex     == 3 ? actif : inactif);
    ui->btnAvis->setStyleSheet(pageIndex      == 4 ? actif : inactif);
    ui->btnProfil->setStyleSheet(pageIndex    == 5 ? actif : inactif);
}

void ClientWindow::updateBadgePanier()
{
    int total = 0;
    for (const auto& a : std::as_const(panier)) total += a.quantite;
    ui->btnPanier->setText(total > 0
                               ? QString("  %1 (%2)").arg(tr("Panier")).arg(total)
                               : "  " + tr("Panier"));
}

double ClientWindow::calculerTotal() const
{
    double total = 0;
    for (const auto& a : std::as_const(panier)) total += a.prix * a.quantite;
    return total;
}

void ClientWindow::viderLayout(QLayout* layout)
{
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void ClientWindow::setNoteSelectionnee(int note)
{
    noteSelectionnee = note;
    for (int i = 0; i < boutonEtoiles.size(); i++) {
        boutonEtoiles[i]->setStyleSheet(
            i < note
                ? "font-size:22px;border:none;background:transparent;color:#F4A100;"
                : "font-size:22px;border:none;background:transparent;color:#D0D0D0;");
    }
    if (ui->lblNoteChoisie)
        ui->lblNoteChoisie->setText(QString("%1 / 5").arg(note));
}

QString ClientWindow::genererInitiales(const QString& nom) const
{
    QString ini;
    for (const QString& p : nom.split(' ', Qt::SkipEmptyParts))
        if (!p.isEmpty()) ini += p[0].toUpper();
    return ini.left(2);
}

QString ClientWindow::couleurPourId(int id) const
{
    static const QStringList c = {"#2D6A4F","#40916C","#52B788","#74C69D",
                                  "#1B4332","#081C15","#52796F","#354F52"};
    return c[id % c.size()];
}

// ═══════════════════════════════════════════
//  PAGE MENU
// ═══════════════════════════════════════════

void ClientWindow::chargerCategories()
{
    viderLayout(ui->layoutCategories);

    QPushButton* btnAll = new QPushButton(tr("Tous"));
    btnAll->setStyleSheet("QPushButton{background:#2D6A4F;color:white;border-radius:50px;"
                          "padding:8px 20px;font-size:13px;font-weight:bold;border:none;}");
    btnAll->setCursor(Qt::PointingHandCursor);
    connect(btnAll, &QPushButton::clicked, this, [this]() { chargerPlats(-1); });
    ui->layoutCategories->addWidget(btnAll);

    QSqlQuery query;
    query.exec("SELECT id, nom FROM categories ORDER BY nom");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString nom = query.value(1).toString();
        QPushButton* btn = new QPushButton(nom);
        btn->setStyleSheet(
            "QPushButton{background:white;color:#333;border-radius:20px;padding:8px 20px;"
            "font-size:13px;border:1px solid #ddd;}"
            "QPushButton:hover{background:#e8f5e9;border-color:#2e7d32;color:#2e7d32;}");
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, id]() { onCategorieClicked(id); });
        ui->layoutCategories->addWidget(btn);
    }
    ui->layoutCategories->addStretch();
}

void ClientWindow::onCategorieClicked(int categorieId) { chargerPlats(categorieId); }

void ClientWindow::chargerPlats(int categorieId)
{
    viderLayout(ui->layoutPlats);
    QSqlQuery query;
    if (categorieId == -1) {
        query.exec(
            "SELECT p.id, p.nom, p.description, p.prix, p.image_path, c.nom "
            "FROM plats p LEFT JOIN categories c ON p.categorie_id = c.id "
            "WHERE p.disponible = 1 ORDER BY c.nom, p.nom");
    } else {
        query.prepare(
            "SELECT p.id, p.nom, p.description, p.prix, p.image_path, c.nom "
            "FROM plats p LEFT JOIN categories c ON p.categorie_id = c.id "
            "WHERE p.disponible = 1 AND p.categorie_id = :cat ORDER BY p.nom");
        query.bindValue(":cat", categorieId);
        query.exec();
    }
    bool hasPlats = false;
    while (query.next()) {
        hasPlats = true;
        QMap<QString, QVariant> plat;
        plat["id"]          = query.value(0).toInt();
        plat["nom"]         = query.value(1).toString();
        plat["description"] = query.value(2).toString();
        plat["prix"]        = query.value(3).toDouble();
        plat["imagePath"]   = query.value(4).toString();
        plat["categorie"]   = query.value(5).toString();
        ui->layoutPlats->addWidget(creerCartePlat(plat));
    }
    if (!hasPlats) {
        QLabel* lbl = new QLabel(tr("Aucun plat disponible dans cette catégorie."));
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color:#999;font-size:14px;padding:40px;");
        ui->layoutPlats->addWidget(lbl);
    }
    ui->layoutPlats->addStretch();
}

QWidget* ClientWindow::creerCartePlat(const QMap<QString, QVariant>& plat)
{
    int     id        = plat["id"].toInt();
    QString nom       = plat["nom"].toString();
    double  prix      = plat["prix"].toDouble();
    QString imagePath = plat["imagePath"].toString();
    QString categorie = plat["categorie"].toString();

    QFrame* carte = new QFrame();
    carte->setStyleSheet(
        "QFrame{background:white;border-radius:12px;border:1px solid #f0f0f0;}"
        "QFrame:hover{border-color:#c8e6c9;}");
    carte->setFixedHeight(100);

    QHBoxLayout* layout = new QHBoxLayout(carte);
    layout->setContentsMargins(15,10,15,10);
    layout->setSpacing(15);

    QLabel* imgLabel = new QLabel();
    imgLabel->setFixedSize(75,75);
    imgLabel->setAlignment(Qt::AlignCenter);
    if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
        QPixmap px(imagePath);
        imgLabel->setPixmap(px.scaled(75,75,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        imgLabel->setStyleSheet("border-radius:10px;");
    } else {
        imgLabel->setText("🍽");
        imgLabel->setStyleSheet("font-size:30px;background:#f5f5f5;border-radius:10px;");
    }
    layout->addWidget(imgLabel);

    QVBoxLayout* info = new QVBoxLayout();
    info->setSpacing(4);
    QLabel* lblNom = new QLabel(nom);
    lblNom->setStyleSheet("font-size:15px;font-weight:bold;color:#1a1a1a;");
    QLabel* lblCat = new QLabel(categorie);
    lblCat->setStyleSheet("QLabel{background:#e8f5e9;color:#2e7d32;border-radius:8px;"
                          "padding:2px 10px;font-size:11px;font-weight:bold;}");
    lblCat->setFixedHeight(22);
    lblCat->setMaximumWidth(120);
    QLabel* lblPrix = new QLabel(QString("%1 DH").arg(prix,0,'f',2));
    lblPrix->setStyleSheet("font-size:14px;font-weight:bold;color:#1a1a1a;");
    info->addWidget(lblNom);
    info->addWidget(lblCat);
    info->addWidget(lblPrix);
    layout->addLayout(info);
    layout->addStretch();

    QPushButton* btnAdd = new QPushButton(tr("+ Ajouter"));
    btnAdd->setFixedSize(110,38);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet(
        "QPushButton{background:#2e7d32;color:white;border-radius:19px;"
        "font-size:13px;font-weight:bold;border:none;}"
        "QPushButton:hover{background:#1b5e20;}");
    connect(btnAdd, &QPushButton::clicked, this, [this,id,nom,prix,imagePath]() {
        onAjouterAuPanier(id,nom,prix,imagePath);
    });
    layout->addWidget(btnAdd);
    return carte;
}

void ClientWindow::onAjouterAuPanier(int platId, const QString& nom,
                                     double prix, const QString& imagePath)
{
    for (auto& a : panier) {
        if (a.platId == platId) {
            a.quantite++;
            updateBadgePanier();
            QMessageBox::information(this, tr("Panier"),
                                     tr("Quantité de \"%1\" mise à jour !").arg(nom));
            return;
        }
    }
    ArticlePanierUI article;
    article.platId = platId; article.nom = nom;
    article.prix = prix; article.imagePath = imagePath; article.quantite = 1;
    panier.append(article);
    updateBadgePanier();
    QMessageBox::information(this, tr("Panier"), tr("\"%1\" ajouté au panier !").arg(nom));
}

void ClientWindow::onRechercher(const QString& texte)
{
    viderLayout(ui->layoutPlats);
    QSqlQuery query;
    query.prepare(
        "SELECT p.id, p.nom, p.description, p.prix, p.image_path, c.nom "
        "FROM plats p LEFT JOIN categories c ON p.categorie_id = c.id "
        "WHERE p.disponible = 1 AND p.nom LIKE :search ORDER BY p.nom");
    query.bindValue(":search", "%" + texte + "%");
    query.exec();
    while (query.next()) {
        QMap<QString,QVariant> plat;
        plat["id"]          = query.value(0).toInt();
        plat["nom"]         = query.value(1).toString();
        plat["description"] = query.value(2).toString();
        plat["prix"]        = query.value(3).toDouble();
        plat["imagePath"]   = query.value(4).toString();
        plat["categorie"]   = query.value(5).toString();
        ui->layoutPlats->addWidget(creerCartePlat(plat));
    }
    ui->layoutPlats->addStretch();
}

// ═══════════════════════════════════════════
//  PAGE PANIER
// ═══════════════════════════════════════════

void ClientWindow::afficherPanier()
{
    viderLayout(ui->layoutArticlesPanier);
    if (panier.isEmpty()) {
        QLabel* lbl = new QLabel(tr("Votre panier est vide"));
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color:#999;font-size:16px;padding:40px;");
        ui->layoutArticlesPanier->addWidget(lbl);
        ui->lblNbArticles->setText(tr("0 article"));
        ui->lblSousTotal->setText("0.00 DH");
        ui->lblFraisLivraison->setText("15.00 DH");
        ui->lblTotal->setText("15.00 DH");
        return;
    }
    for (const auto& a : std::as_const(panier))
        ui->layoutArticlesPanier->addWidget(creerArticlePanier(a));
    ui->layoutArticlesPanier->addStretch();

    double sousTotal = calculerTotal();
    double livraison = 15.0;
    int nbArticles = 0;
    for (const auto& a : std::as_const(panier)) nbArticles += a.quantite;
    ui->lblNbArticles->setText(QString("%1 %2%3")
                                   .arg(nbArticles)
                                   .arg(tr("article"))
                                   .arg(nbArticles>1?"s":""));
    ui->lblSousTotal->setText(QString("%1 DH").arg(sousTotal,0,'f',2));
    ui->lblFraisLivraison->setText(QString("%1 DH").arg(livraison,0,'f',2));
    ui->lblTotal->setText(QString("%1 DH").arg(sousTotal+livraison,0,'f',2));
}

QWidget* ClientWindow::creerArticlePanier(const ArticlePanierUI& article)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet("QFrame{background:white;border-radius:12px;border:1px solid #f0f0f0;}");
    carte->setFixedHeight(90);
    QHBoxLayout* layout = new QHBoxLayout(carte);
    layout->setContentsMargins(15,10,15,10);
    layout->setSpacing(15);

    QLabel* imgLabel = new QLabel();
    imgLabel->setFixedSize(65,65);
    imgLabel->setAlignment(Qt::AlignCenter);
    if (!article.imagePath.isEmpty() && QFile::exists(article.imagePath)) {
        QPixmap px(article.imagePath);
        imgLabel->setPixmap(px.scaled(65,65,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        imgLabel->setStyleSheet("border-radius:8px;");
    } else {
        imgLabel->setText("🍽");
        imgLabel->setStyleSheet("font-size:28px;background:#f5f5f5;border-radius:8px;");
    }
    layout->addWidget(imgLabel);

    QVBoxLayout* info = new QVBoxLayout();
    QLabel* lblNom  = new QLabel(article.nom);
    lblNom->setStyleSheet("font-size:14px;font-weight:bold;color:#1a1a1a;");
    QLabel* lblPrix = new QLabel(QString("%1 DH").arg(article.prix*article.quantite,0,'f',2));
    lblPrix->setStyleSheet("font-size:14px;font-weight:bold;color:#2e7d32;");
    info->addWidget(lblNom); info->addWidget(lblPrix);
    layout->addLayout(info);
    layout->addStretch();

    QString btnStyle =
        "QPushButton{background:#e8f5e9;color:#2e7d32;border-radius:16px;"
        "font-size:16px;font-weight:bold;border:none;}"
        "QPushButton:hover{background:#c8e6c9;}";

    QPushButton* btnMoins = new QPushButton("-");
    btnMoins->setFixedSize(32,32);
    btnMoins->setStyleSheet(btnStyle);

    QLabel* lblQty = new QLabel(QString::number(article.quantite));
    lblQty->setAlignment(Qt::AlignCenter);
    lblQty->setFixedWidth(30);
    lblQty->setStyleSheet("font-size:14px;font-weight:bold;color:#1a1a1a;");

    QPushButton* btnPlus = new QPushButton("+");
    btnPlus->setFixedSize(32,32);
    btnPlus->setStyleSheet(btnStyle);

    QPushButton* btnDel = new QPushButton("X");
    btnDel->setFixedSize(32,32);
    btnDel->setStyleSheet(
        "QPushButton{background:transparent;color:#e53935;border:none;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#ffebee;border-radius:16px;}");

    int platId = article.platId;
    connect(btnMoins, &QPushButton::clicked, this, [this,platId](){ onDiminuerQuantite(platId); });
    connect(btnPlus,  &QPushButton::clicked, this, [this,platId](){ onAugmenterQuantite(platId); });
    connect(btnDel,   &QPushButton::clicked, this, [this,platId](){ onSupprimerDuPanier(platId); });

    layout->addWidget(btnMoins);
    layout->addWidget(lblQty);
    layout->addWidget(btnPlus);
    layout->addSpacing(10);
    layout->addWidget(btnDel);
    return carte;
}

void ClientWindow::onAugmenterQuantite(int platId)
{
    for (auto& a : panier) if (a.platId == platId) { a.quantite++; break; }
    afficherPanier();
}

void ClientWindow::onDiminuerQuantite(int platId)
{
    for (int i = 0; i < panier.size(); i++) {
        if (panier[i].platId == platId) {
            panier[i].quantite--;
            if (panier[i].quantite <= 0) panier.removeAt(i);
            break;
        }
    }
    updateBadgePanier();
    afficherPanier();
}

void ClientWindow::onSupprimerDuPanier(int platId)
{
    for (int i = 0; i < panier.size(); i++)
        if (panier[i].platId == platId) { panier.removeAt(i); break; }
    updateBadgePanier();
    afficherPanier();
}

void ClientWindow::onPasserCommande()
{
    if (panier.isEmpty()) {
        QMessageBox::warning(this, tr("Panier vide"), tr("Ajoutez des plats avant de commander !"));
        return;
    }

    // ── Horaires : FoodOrbit est ouvert 24h/24 (mode test) ──────────────────
    // Pour activer les restrictions d'horaires en production,
    // remplacer ce bloc par la vérification de la table horaires.
    double total = calculerTotal() + 15.0;
    QSqlQuery query;
    query.prepare(
        "INSERT INTO commandes (client_id, statut, date_heure, total) "
        "VALUES (:cid, 'en_attente', datetime('now'), :total)");
    query.bindValue(":cid",   currentClient.getId());
    query.bindValue(":total", total);
    if (!query.exec()) { QMessageBox::critical(this, tr("Erreur"), query.lastError().text()); return; }

    int commandeId = query.lastInsertId().toInt();
    for (const auto& article : std::as_const(panier)) {
        QSqlQuery lq;
        lq.prepare("INSERT INTO lignes_commande (commande_id,plat_id,quantite,prix_unitaire) "
                   "VALUES (:cmd,:plat,:qty,:prix)");
        lq.bindValue(":cmd",  commandeId);
        lq.bindValue(":plat", article.platId);
        lq.bindValue(":qty",  article.quantite);
        lq.bindValue(":prix", article.prix);
        lq.exec();
    }
    panier.clear();
    updateBadgePanier();
    commandeSuivieId = commandeId;
    QMessageBox::information(this, tr("Commande passée !"),
                             tr("Commande #%1 passée avec succès !\nTotal : %2 DH")
                                 .arg(commandeId).arg(total,0,'f',2));
    afficherPageSuivi();
}

// ═══════════════════════════════════════════
//  PAGE COMMANDES
// ═══════════════════════════════════════════

void ClientWindow::chargerCommandes()
{
    viderLayout(ui->layoutCommandes);
    QSqlQuery query;
    query.prepare(
        "SELECT id, statut, date_heure, total, "
        "  (SELECT COUNT(*) FROM lignes_commande WHERE commande_id = commandes.id) as nb "
        "FROM commandes WHERE client_id = :cid ORDER BY date_heure DESC");
    query.bindValue(":cid", currentClient.getId());
    query.exec();
    bool has = false;
    while (query.next()) {
        has = true;
        ui->layoutCommandes->addWidget(creerCarteCommande(
            query.value(0).toInt(), query.value(2).toString().left(10),
            query.value(1).toString(), query.value(4).toInt(),
            query.value(3).toDouble()));
    }
    if (!has) {
        QLabel* lbl = new QLabel(tr("Vous n'avez pas encore de commandes."));
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color:#999;font-size:14px;padding:40px;");
        ui->layoutCommandes->addWidget(lbl);
    }
    ui->layoutCommandes->addStretch();
}

QWidget* ClientWindow::creerCarteCommande(int id, const QString& date,
                                          const QString& statut,
                                          int nbArticles, double total)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet("QFrame{background:white;border-radius:14px;border:1px solid #f0f0f0;}");
    QVBoxLayout* vl = new QVBoxLayout(carte);
    vl->setContentsMargins(20,15,20,15); vl->setSpacing(10);

    QHBoxLayout* topL = new QHBoxLayout();
    QLabel* lblIcon = new QLabel("📦");
    lblIcon->setFixedSize(45,45); lblIcon->setAlignment(Qt::AlignCenter);
    lblIcon->setStyleSheet("background:#e8f5e9;border-radius:22px;font-size:20px;");

    QVBoxLayout* titleL = new QVBoxLayout();
    QLabel* lblId   = new QLabel(QString("#%1").arg(id));
    lblId->setStyleSheet("font-size:16px;font-weight:bold;color:#1a1a1a;");
    QLabel* lblDate = new QLabel(date);
    lblDate->setStyleSheet("font-size:12px;color:#999;");
    titleL->addWidget(lblId); titleL->addWidget(lblDate);

    QString cb, tb;
    if      (statut=="livree")         { cb="background:#e8f5e9;color:#2e7d32;"; tb=tr("Livrée"); }
    else if (statut=="en_attente")     { cb="background:#fff8e1;color:#f57f17;"; tb=tr("En attente"); }
    else if (statut=="confirmee")      { cb="background:#e3f2fd;color:#1565c0;"; tb=tr("Confirmée"); }
    else if (statut=="en_preparation") { cb="background:#e3f2fd;color:#1565c0;"; tb=tr("En préparation"); }
    else if (statut=="en_livraison")   { cb="background:#fff8e1;color:#e65100;"; tb=tr("En livraison"); }
    else                               { cb="background:#f5f5f5;color:#666;";    tb=statut; }

    QLabel* lblStatut = new QLabel(tb);
    lblStatut->setStyleSheet(QString("QLabel{%1 border-radius:12px;padding:5px 14px;"
                                     "font-size:12px;font-weight:bold;}").arg(cb));
    topL->addWidget(lblIcon); topL->addSpacing(10);
    topL->addLayout(titleL); topL->addStretch(); topL->addWidget(lblStatut);

    QHBoxLayout* botL = new QHBoxLayout();
    QLabel* lblNb = new QLabel(QString("%1 %2%3")
                                   .arg(nbArticles).arg(tr("article")).arg(nbArticles>1?"s":""));
    lblNb->setStyleSheet("font-size:13px;color:#666;");
    QLabel* lblTot = new QLabel(QString("%1 DH").arg(total,0,'f',2));
    lblTot->setStyleSheet("font-size:15px;font-weight:bold;color:#1a1a1a;");
    botL->addWidget(lblNb); botL->addStretch(); botL->addWidget(lblTot);

    vl->addLayout(topL); vl->addLayout(botL);

    if (statut!="livree" && statut!="annulee") {
        QPushButton* btnSuivre = new QPushButton(tr("Suivre la commande"));
        btnSuivre->setFixedHeight(42); btnSuivre->setCursor(Qt::PointingHandCursor);
        btnSuivre->setStyleSheet(
            "QPushButton{background:#2e7d32;color:white;border-radius:10px;"
            "font-size:14px;font-weight:bold;border:none;}"
            "QPushButton:hover{background:#1b5e20;}");
        connect(btnSuivre, &QPushButton::clicked, this, [this,id](){ onSuivreCommande(id); });
        vl->addWidget(btnSuivre);
    }

    QPushButton* btnPdf = new QPushButton(tr("Imprimer le ticket PDF"));
    btnPdf->setFixedHeight(38); btnPdf->setCursor(Qt::PointingHandCursor);
    btnPdf->setStyleSheet(
        "QPushButton{background:#f1f8e9;color:#33691e;border-radius:10px;"
        "font-size:13px;font-weight:bold;border:1px solid #aed581;}"
        "QPushButton:hover{background:#dcedc8;border-color:#8bc34a;}");
    connect(btnPdf, &QPushButton::clicked, this, [this,id](){ imprimerTicketCommande(id); });
    vl->addWidget(btnPdf);
    return carte;
}

void ClientWindow::onSuivreCommande(int commandeId)
{
    commandeSuivieId = commandeId;
    afficherPageSuivi();
}

// ═══════════════════════════════════════════
//  PAGE SUIVI
// ═══════════════════════════════════════════

void ClientWindow::afficherSuivi(int commandeId)
{
    QSqlQuery query;
    query.prepare("SELECT statut, date_heure, total FROM commandes WHERE id = :id");
    query.bindValue(":id", commandeId);
    query.exec();
    if (!query.next()) return;

    QString statut = query.value(0).toString();
    double  total  = query.value(2).toDouble();
    ui->lblTitreCommande->setText(QString(tr("Commande #%1")).arg(commandeId));
    ui->lblTotalSuivi->setText(QString("%1 DH").arg(total,0,'f',2));

    // ── Étapes visuelles ──
    QList<QPair<QString,QString>> etapes = {
        {"en_attente",     tr("Reçue")},
        {"confirmee",      tr("Confirmée")},
        {"en_preparation", tr("En cuisine")},
        {"en_livraison",   tr("En route")},
        {"livree",         tr("Livrée ✓")}
    };
    int indexActuel = 0;
    for (int i = 0; i < etapes.size(); i++)
        if (etapes[i].first == statut) { indexActuel = i; break; }

    QList<QLabel*> labels = {
        ui->lblEtape1, ui->lblEtape2, ui->lblEtape3, ui->lblEtape4, ui->lblEtape5
    };
    for (int i = 0; i < etapes.size() && i < labels.size(); i++) {
        bool fait    = (i < indexActuel);
        bool actuel  = (i == indexActuel);
        if (actuel)
            labels[i]->setStyleSheet(
                "background:#2D6A4F;color:white;border-radius:20px;"
                "font-size:13px;font-weight:bold;padding:8px;"
                "border:3px solid #1B4332;");
        else if (fait)
            labels[i]->setStyleSheet(
                "background:#95D5B2;color:#1B4332;border-radius:20px;"
                "font-size:13px;font-weight:bold;padding:8px;");
        else
            labels[i]->setStyleSheet(
                "background:#e0e0e0;color:#999;border-radius:20px;"
                "font-size:13px;padding:8px;");
        labels[i]->setText(etapes[i].second);
    }

    // ── Temps estimé local (base) puis IA async ──
    int commandesEnCours = 0;
    QSqlQuery qCharge;
    qCharge.exec("SELECT COUNT(*) FROM commandes WHERE statut IN ('confirmee','en_preparation','en_livraison')");
    if (qCharge.next()) commandesEnCours = qCharge.value(0).toInt();

    int nbPlats = 0;
    QSqlQuery qNb;
    qNb.prepare("SELECT SUM(quantite) FROM lignes_commande WHERE commande_id=:id");
    qNb.bindValue(":id", commandeId);
    if (qNb.exec() && qNb.next()) nbPlats = qNb.value(0).toInt();

    // Estimation locale rapide
    int delaiLocal = 25 + nbPlats * 2 + commandesEnCours * 2;
    if (statut == "en_livraison") delaiLocal = qMax(5, delaiLocal / 3);
    if (statut == "livree")       delaiLocal = 0;

    if (statut == "livree") {
        ui->lblTempsLabel->setText(tr("Commande livrée !"));
        ui->lblTempsValeur->setText("✓");
        ui->lblTempsValeur->setStyleSheet("color:#2D6A4F;font-size:28px;font-weight:bold;");
    } else {
        ui->lblTempsLabel->setText(tr("Temps estimé"));
        ui->lblTempsValeur->setText(QString("%1 min").arg(delaiLocal));
        ui->lblTempsValeur->setStyleSheet("color:#1B4332;font-size:20px;font-weight:bold;");
        // Lancer estimation IA en arrière-plan
        demanderEstimationIA();
    }

    // ── Détails des plats ──
    viderLayout(ui->layoutDetailsSuivi);
    QSqlQuery dq;
    dq.prepare("SELECT p.nom, lc.quantite, lc.prix_unitaire "
               "FROM lignes_commande lc JOIN plats p ON lc.plat_id = p.id "
               "WHERE lc.commande_id = :id");
    dq.bindValue(":id", commandeId);
    dq.exec();
    while (dq.next()) {
        QString nom  = dq.value(0).toString();
        int     qty  = dq.value(1).toInt();
        double  prix = dq.value(2).toDouble();
        QHBoxLayout* ligne = new QHBoxLayout();
        QLabel* lN = new QLabel(QString("%1x %2").arg(qty).arg(nom));
        QLabel* lP = new QLabel(QString("%1 DH").arg(prix*qty,0,'f',2));
        lN->setStyleSheet("font-size:13px;color:#333;");
        lP->setStyleSheet("font-size:13px;color:#2D6A4F;font-weight:bold;");
        ligne->addWidget(lN); ligne->addStretch(); ligne->addWidget(lP);
        QWidget* w = new QWidget(); w->setLayout(ligne);
        ui->layoutDetailsSuivi->addWidget(w);
    }
    ui->layoutDetailsSuivi->addStretch();
}

// ════════════════════════════════════════════════════════════════
//  RAFRAÎCHISSEMENT AUTOMATIQUE DU SUIVI (toutes les 30s)
// ════════════════════════════════════════════════════════════════
void ClientWindow::rafraichirSuivi()
{
    if (commandeSuivieId < 0) return;

    // Lire le statut actuel depuis la DB
    QSqlQuery q;
    q.prepare("SELECT statut FROM commandes WHERE id = :id");
    q.bindValue(":id", commandeSuivieId);
    if (!q.exec() || !q.next()) return;

    QString statut = q.value(0).toString();

    // Rafraîchir la page suivi si on est dessus
    if (ui->stackedWidget->currentIndex() == 3)
        afficherSuivi(commandeSuivieId);

    // Rafraîchir aussi la liste des commandes
    chargerCommandes();

    // Arrêter le timer si la commande est terminée
    if (statut == "livree" || statut == "annulee") {
        m_timerSuivi->stop();
        qDebug() << "[FoodOrbit] Suivi terminé pour commande #" << commandeSuivieId;
    }
}

// ════════════════════════════════════════════════════════════════
//  ESTIMATION IA DU TEMPS DE LIVRAISON (Groq/LLaMA)
// ════════════════════════════════════════════════════════════════
void ClientWindow::demanderEstimationIA()
{
    if (commandeSuivieId < 0) return;
    if (GROQ_API_KEY.isEmpty() || GROQ_API_KEY == "VOTRE_CLE_GROQ_ICI") return;

    // Collecter contexte
    QSqlQuery q;
    q.prepare("SELECT statut, total FROM commandes WHERE id=:id");
    q.bindValue(":id", commandeSuivieId);
    if (!q.exec() || !q.next()) return;

    QString statut = q.value(0).toString();
    double  total  = q.value(1).toDouble();
    if (statut == "livree" || statut == "annulee") return;

    int commandesActives = 0;
    QSqlQuery qC;
    qC.exec("SELECT COUNT(*) FROM commandes WHERE statut IN ('confirmee','en_preparation','en_livraison')");
    if (qC.next()) commandesActives = qC.value(0).toInt();

    int nbPlats = 0;
    QSqlQuery qN;
    qN.prepare("SELECT SUM(quantite) FROM lignes_commande WHERE commande_id=:id");
    qN.bindValue(":id", commandeSuivieId);
    if (qN.exec() && qN.next()) nbPlats = qN.value(0).toInt();

    int heure = QTime::currentTime().hour();
    bool heurePointe = (heure >= 12 && heure <= 14) || (heure >= 19 && heure <= 21);

    QString prompt =
        QString("statut=%1 articles=%2 commandes_actives=%3 heure_pointe=%4 total=%5")
            .arg(statut).arg(nbPlats).arg(commandesActives)
            .arg(heurePointe ? "oui" : "non").arg(total, 0, 'f', 0);
    prompt = "Estime le delai de livraison restant en minutes pour FoodOrbit. "
             "Reponds UNIQUEMENT en JSON: {\"delai\":25,\"message\":\"message court\"}. "
             "Contexte: " + prompt;

    QJsonObject sys; sys["role"]="system";
    sys["content"]="Tu estimes les délais de livraison. Réponds uniquement en JSON valide sans backticks.";
    QJsonObject usr; usr["role"]="user"; usr["content"]=prompt;
    QJsonArray msgs; msgs.append(sys); msgs.append(usr);

    QJsonObject body;
    body["model"]      = GROQ_MODEL;
    body["messages"]   = msgs;
    body["max_tokens"] = 80;
    body["temperature"]= 0.3;
    body["stream"]     = false;

    // ── Fix Most Vexing Parse : construire QUrl puis QNetworkRequest séparément ──
    QUrl    groqUrl(GROQ_URL);
    QNetworkRequest req;
    req.setUrl(groqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QVariant(QString("application/json")));
    req.setRawHeader(QByteArray("Authorization"),
                     QString("Bearer " + GROQ_API_KEY).toUtf8());

    QNetworkReply* reply = m_networkIA->post(req, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;

        QString content = doc.object()["choices"].toArray()[0]
                              .toObject()["message"].toObject()["content"]
                              .toString().trimmed();
        content.remove("```json").remove("```").trimmed();

        QJsonDocument r = QJsonDocument::fromJson(content.toUtf8());
        if (!r.isObject()) return;

        int     delai   = r.object()["delai"].toInt(0);
        QString message = r.object()["message"].toString();

        if (delai > 0 && delai <= 120) {
            ui->lblTempsValeur->setText(QString("%1 min").arg(delai));
            ui->lblTempsValeur->setStyleSheet(
                "color:#2D6A4F;font-size:20px;font-weight:bold;");
            if (!message.isEmpty())
                ui->lblTempsLabel->setText("🤖 " + message);
            qDebug() << "[FoodOrbit] Estimation IA:" << delai << "min —" << message;
        }
    });
}

// ═══════════════════════════════════════════
//  PAGE AVIS
// ═══════════════════════════════════════════

void ClientWindow::chargerAvis(const QString& tri)
{
    QSqlQuery qMoy;
    qMoy.exec("SELECT AVG(note), COUNT(*) FROM avis");
    if (qMoy.next()) {
        double moy = qMoy.value(0).toDouble();
        int nb     = qMoy.value(1).toInt();
        if (nb > 0) {
            ui->lblNoteMoyenne->setText(QString::number(moy,'f',1));
            QString etoiles;
            for (int i=1;i<=5;i++) etoiles += (i<=qRound(moy))?"*":"-";
            ui->lblEtoilesMoyenne->setText(etoiles);
            ui->lblNbAvis->setText(QString("%1 %2").arg(nb).arg(tr("avis")));
        } else {
            ui->lblNoteMoyenne->setText("-");
            ui->lblEtoilesMoyenne->setText("-----");
            ui->lblNbAvis->setText(tr("Aucun avis pour l'instant"));
        }
    }

    viderLayout(ui->layoutPlatsTop);
    QSqlQuery qTop;
    qTop.exec(
        "SELECT p.nom, p.image_path, COUNT(lc.plat_id) AS nb "
        "FROM lignes_commande lc JOIN plats p ON lc.plat_id = p.id "
        "JOIN commandes c ON lc.commande_id = c.id WHERE c.statut = 'livree' "
        "GROUP BY lc.plat_id ORDER BY nb DESC LIMIT 3");
    bool aucunTop = true;
    int rang = 1;
    while (qTop.next()) {
        aucunTop = false;
        ui->layoutPlatsTop->addWidget(creerCartePlatTop(
            qTop.value(0).toString(), qTop.value(1).toString(),
            qTop.value(2).toInt(), rang++));
    }
    if (aucunTop) {
        QLabel* l = new QLabel(tr("Aucune commande encore passée."));
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet("color:#74C69D;font-size:13px;");
        ui->layoutPlatsTop->addWidget(l);
    }

    viderLayout(ui->layoutListeAvis);
    QString ordre = (tri=="note") ? "note DESC" : "date_heure DESC";
    QSqlQuery qA;
    qA.exec("SELECT u.prenom || ' ' || u.nom, a.note, a.commentaire, "
            "a.date_heure, a.client_id FROM avis a "
            "JOIN utilisateurs u ON a.client_id = u.id ORDER BY " + ordre);
    bool aucunAvis = true;
    while (qA.next()) {
        aucunAvis = false;
        ui->layoutListeAvis->addWidget(creerCarteAvis(
            qA.value(0).toString(), qA.value(1).toInt(),
            qA.value(2).toString(), qA.value(3).toString().left(10),
            qA.value(4).toInt()));
    }
    if (aucunAvis) {
        QLabel* l = new QLabel(tr("Aucun avis pour l'instant.\nSoyez le premier à donner votre avis !"));
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet("color:#74C69D;font-size:13px;");
        ui->layoutListeAvis->addWidget(l);
    }

    setNoteSelectionnee(noteSelectionnee > 0 ? noteSelectionnee : 5);
}

void ClientWindow::soumettreAvis()
{
    if (noteSelectionnee == 0) {
        QMessageBox::warning(this, tr("Note manquante"), tr("Veuillez choisir une note."));
        return;
    }
    QString commentaire = ui->textEditCommentaire->toPlainText().trimmed();
    if (commentaire.isEmpty()) {
        QMessageBox::warning(this, tr("Commentaire vide"), tr("Veuillez écrire un commentaire."));
        return;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO avis (client_id, note, commentaire) VALUES (:cid, :note, :com)");
    q.bindValue(":cid",  currentClient.getId());
    q.bindValue(":note", noteSelectionnee);
    q.bindValue(":com",  commentaire);
    if (q.exec()) {
        QMessageBox::information(this, tr("Merci !"), tr("Votre avis a été publié !"));
        ui->textEditCommentaire->clear();
        noteSelectionnee = 0;
        setNoteSelectionnee(5);
        chargerAvis();
    } else {
        QMessageBox::critical(this, tr("Erreur"), q.lastError().text());
    }
}

void ClientWindow::ouvrirDialogContact()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Contacter le restaurant"));
    dlg->setMinimumWidth(420);
    QVBoxLayout* lay = new QVBoxLayout(dlg);
    lay->setSpacing(12); lay->setContentsMargins(24,20,24,20);

    auto addChamp = [&](const QString& label, QWidget* champ) {
        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:12px;color:#888;font-weight:bold;");
        lay->addWidget(lbl); lay->addWidget(champ);
    };

    QLineEdit* leNom   = new QLineEdit();
    QLineEdit* leEmail = new QLineEdit();
    QLineEdit* leSujet = new QLineEdit();
    QTextEdit* teMsg   = new QTextEdit();
    teMsg->setMinimumHeight(100);
    leNom->setText(currentClient.getPrenom() + " " + currentClient.getNom());
    leEmail->setText(currentClient.getEmail());

    addChamp(tr("NOM"), leNom);
    addChamp(tr("EMAIL"), leEmail);
    addChamp(tr("SUJET"), leSujet);
    addChamp(tr("MESSAGE"), teMsg);

    QPushButton* btnEnvoyer = new QPushButton(tr("Envoyer le message"));
    btnEnvoyer->setStyleSheet(
        "QPushButton{background:#2D6A4F;color:white;border-radius:10px;"
        "font-size:14px;font-weight:bold;border:none;padding:12px;}"
        "QPushButton:hover{background:#1B4332;}");
    lay->addWidget(btnEnvoyer);

    connect(btnEnvoyer, &QPushButton::clicked, dlg, [=]() {
        if (leNom->text().isEmpty() || leEmail->text().isEmpty() || teMsg->toPlainText().isEmpty()) {
            QMessageBox::warning(dlg, tr("Champs requis"), tr("Nom, email et message sont requis."));
            return;
        }
        QSqlQuery q;
        q.prepare("INSERT INTO messages (client_id,nom,email,sujet,message) "
                  "VALUES (:cid,:nom,:email,:sujet,:msg)");
        q.bindValue(":cid",   currentClient.getId());
        q.bindValue(":nom",   leNom->text().trimmed());
        q.bindValue(":email", leEmail->text().trimmed());
        q.bindValue(":sujet", leSujet->text().trimmed().isEmpty()
                                  ? tr("Contact général") : leSujet->text().trimmed());
        q.bindValue(":msg",   teMsg->toPlainText().trimmed());
        if (q.exec()) {
            QMessageBox::information(dlg, tr("Message envoyé"), tr("Votre message a été transmis !"));
            dlg->accept();
        } else {
            QMessageBox::critical(dlg, tr("Erreur"), q.lastError().text());
        }
    });
    dlg->exec();
}

QWidget* ClientWindow::creerCarteAvis(const QString& nomClient, int note,
                                      const QString& commentaire,
                                      const QString& dateStr, int clientId)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet("QFrame{background:white;border-radius:12px;border:1px solid #f0f0f0;}");
    QVBoxLayout* vl = new QVBoxLayout(carte);
    vl->setContentsMargins(16,14,16,14); vl->setSpacing(8);

    QHBoxLayout* hTop = new QHBoxLayout();
    QLabel* lblAv = new QLabel(genererInitiales(nomClient));
    lblAv->setFixedSize(40,40); lblAv->setAlignment(Qt::AlignCenter);
    lblAv->setStyleSheet(QString("background:%1;color:white;border-radius:20px;"
                                 "font-size:14px;font-weight:bold;").arg(couleurPourId(clientId)));

    QVBoxLayout* vi = new QVBoxLayout(); vi->setSpacing(2);
    QLabel* lblNom  = new QLabel(nomClient);
    lblNom->setStyleSheet("font-size:13px;font-weight:bold;color:#1a1a1a;");
    QString etoiles;
    for (int i=1;i<=5;i++) etoiles += (i<=note)?"*":"-";
    QLabel* lblNote = new QLabel(etoiles);
    lblNote->setStyleSheet("font-size:14px;color:#F4A100;");
    vi->addWidget(lblNom); vi->addWidget(lblNote);

    QLabel* lblDate = new QLabel(dateStr);
    lblDate->setStyleSheet("font-size:11px;color:#aaa;");
    hTop->addWidget(lblAv); hTop->addSpacing(10);
    hTop->addLayout(vi); hTop->addStretch(); hTop->addWidget(lblDate);

    QLabel* lblCom = new QLabel(commentaire);
    lblCom->setWordWrap(true);
    lblCom->setStyleSheet("font-size:13px;color:#555;");

    vl->addLayout(hTop); vl->addWidget(lblCom);
    return carte;
}

QWidget* ClientWindow::creerCartePlatTop(const QString& nom, const QString& imagePath,
                                         int nbCommandes, int rang)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet("QFrame{background:white;border-radius:10px;border:1px solid #f0f0f0;}");
    carte->setFixedHeight(60);
    QHBoxLayout* lay = new QHBoxLayout(carte);
    lay->setContentsMargins(10,8,14,8); lay->setSpacing(10);

    static const QStringList medals = {"1st","2nd","3rd"};
    QLabel* lblMed = new QLabel(rang<=3 ? medals[rang-1] : QString::number(rang));
    lblMed->setFixedSize(36,30); lblMed->setAlignment(Qt::AlignCenter);
    lblMed->setStyleSheet("font-size:14px;font-weight:bold;color:#2D6A4F;");

    QLabel* lblImg = new QLabel();
    lblImg->setFixedSize(40,40); lblImg->setAlignment(Qt::AlignCenter);
    if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
        QPixmap px(imagePath);
        lblImg->setPixmap(px.scaled(40,40,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        lblImg->setStyleSheet("border-radius:6px;");
    } else {
        lblImg->setText("🍽"); lblImg->setStyleSheet("font-size:20px;background:#f5f5f5;border-radius:6px;");
    }

    QLabel* lblNom = new QLabel(nom);
    lblNom->setStyleSheet("font-size:13px;font-weight:bold;color:#1a1a1a;");
    QLabel* lblNb = new QLabel(QString("%1 %2").arg(nbCommandes).arg(tr("commandes")));
    lblNb->setStyleSheet("font-size:12px;color:#888;");

    lay->addWidget(lblMed); lay->addWidget(lblImg);
    lay->addWidget(lblNom); lay->addStretch(); lay->addWidget(lblNb);
    return carte;
}

// ═══════════════════════════════════════════
//  PAGE PROFIL
// ═══════════════════════════════════════════

void ClientWindow::chargerProfil()
{
    QSqlQuery q;
    q.prepare("SELECT u.prenom, u.nom, u.email, u.telephone, c.adresse "
              "FROM utilisateurs u JOIN clients c ON u.id = c.id WHERE u.id = :id");
    q.bindValue(":id", currentClient.getId());
    if (!q.exec() || !q.next()) return;

    QString prenom    = q.value(0).toString();
    QString nom       = q.value(1).toString();
    QString email     = q.value(2).toString();
    QString telephone = q.value(3).toString();
    QString adresse   = q.value(4).toString();

    ui->lePrenom->setText(prenom);
    ui->leNom->setText(nom);
    ui->leEmail->setText(email);
    ui->leTelephone->setText(telephone);
    ui->leAdresse->setText(adresse);
    ui->leMdpActuel->clear();
    ui->leMdpNouveau->clear();
    ui->leMdpConfirm->clear();

    ui->lblNomCompletProfil->setText(prenom + " " + nom);
    ui->lblEmailProfil->setText(email);

    if (!photoProfilPath.isEmpty()) {
        QPixmap px(photoProfilPath);
        QPixmap rounded(80, 80);
        rounded.fill(Qt::transparent);
        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath pp;
        pp.addEllipse(0, 0, 80, 80);
        painter.setClipPath(pp);
        painter.drawPixmap(0, 0, px.scaled(80, 80, Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation));
        painter.end();
        ui->lblAvatarProfil->setPixmap(rounded);
        ui->lblAvatarProfil->setStyleSheet(
            "QLabel#lblAvatarProfil{border-radius:40px;border:3px solid #B7E4C7;background:transparent;}");
    } else {
        QString initiales = genererInitiales(prenom + " " + nom);
        ui->lblAvatarProfil->setText(initiales);
        ui->lblAvatarProfil->setStyleSheet(
            "QLabel#lblAvatarProfil{background:#2D6A4F;color:white;border-radius:40px;"
            "font-size:26px;font-weight:bold;border:3px solid #B7E4C7;}");
    }
}

void ClientWindow::onSauvegarderProfil()
{
    QString prenom    = ui->lePrenom->text().trimmed();
    QString nom       = ui->leNom->text().trimmed();
    QString email     = ui->leEmail->text().trimmed();
    QString telephone = ui->leTelephone->text().trimmed();
    QString adresse   = ui->leAdresse->text().trimmed();

    if (prenom.isEmpty() || nom.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, tr("Champs requis"), tr("Le prénom, le nom et l'email sont obligatoires."));
        return;
    }
    QSqlQuery qCheck;
    qCheck.prepare("SELECT id FROM utilisateurs WHERE email = :email AND id != :id");
    qCheck.bindValue(":email", email);
    qCheck.bindValue(":id",    currentClient.getId());
    qCheck.exec();
    if (qCheck.next()) {
        QMessageBox::warning(this, tr("Email déjà utilisé"), tr("Cet email est déjà associé à un autre compte."));
        return;
    }
    QSqlQuery qU;
    qU.prepare("UPDATE utilisateurs SET prenom=:prenom, nom=:nom, "
               "email=:email, telephone=:telephone WHERE id=:id");
    qU.bindValue(":prenom",    prenom);
    qU.bindValue(":nom",       nom);
    qU.bindValue(":email",     email);
    qU.bindValue(":telephone", telephone);
    qU.bindValue(":id",        currentClient.getId());
    if (!qU.exec()) { QMessageBox::critical(this, tr("Erreur"), qU.lastError().text()); return; }

    QSqlQuery qC;
    qC.prepare("UPDATE clients SET adresse=:adresse WHERE id=:id");
    qC.bindValue(":adresse", adresse);
    qC.bindValue(":id",      currentClient.getId());
    qC.exec();

    ui->lblNomClient->setText(prenom + " " + nom);
    ui->lblNomCompletProfil->setText(prenom + " " + nom);
    ui->lblEmailProfil->setText(email);
    QMessageBox::information(this, tr("Profil mis à jour"), tr("Vos informations ont été enregistrées !"));
}

void ClientWindow::onChangerMotDePasse()
{
    QString actuel  = ui->leMdpActuel->text();
    QString nouveau = ui->leMdpNouveau->text();
    QString confirm = ui->leMdpConfirm->text();

    if (actuel.isEmpty() || nouveau.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, tr("Champs requis"), tr("Veuillez remplir tous les champs de mot de passe."));
        return;
    }
    if (nouveau != confirm) {
        QMessageBox::warning(this, tr("Erreur"), tr("Le nouveau mot de passe et la confirmation ne correspondent pas."));
        return;
    }
    if (nouveau.length() < 6) {
        QMessageBox::warning(this, tr("Trop court"), tr("Le mot de passe doit contenir au moins 6 caractères."));
        return;
    }
    auto hash = [](const QString& s) {
        return QString(QCryptographicHash::hash(s.toUtf8(), QCryptographicHash::Sha256).toHex());
    };
    QSqlQuery qVerif;
    qVerif.prepare("SELECT id FROM utilisateurs WHERE id=:id AND mot_de_passe=:mdp");
    qVerif.bindValue(":id",  currentClient.getId());
    qVerif.bindValue(":mdp", hash(actuel));
    qVerif.exec();
    if (!qVerif.next()) {
        QMessageBox::warning(this, tr("Incorrect"), tr("Le mot de passe actuel est incorrect."));
        return;
    }
    QSqlQuery qMaj;
    qMaj.prepare("UPDATE utilisateurs SET mot_de_passe=:mdp WHERE id=:id");
    qMaj.bindValue(":mdp", hash(nouveau));
    qMaj.bindValue(":id",  currentClient.getId());
    if (!qMaj.exec()) { QMessageBox::critical(this, tr("Erreur"), qMaj.lastError().text()); return; }
    ui->leMdpActuel->clear(); ui->leMdpNouveau->clear(); ui->leMdpConfirm->clear();
    QMessageBox::information(this, tr("Mot de passe modifié"), tr("Votre mot de passe a été changé !"));
}

// ═══════════════════════════════════════════
//  ASSISTANT IA
// ═══════════════════════════════════════════

void ClientWindow::setupBoutonIA()
{
    m_btnIA = new BoutonIA(this);
    m_btnIA->move(this->width() - 76, this->height() - 76);
    m_btnIA->raise();
    m_btnIA->show();
    connect(m_btnIA, &QPushButton::clicked, this, &ClientWindow::ouvrirAssistantIA);
}

void ClientWindow::ouvrirAssistantIA()
{
    if (m_assistant && m_assistant->isVisible()) {
        m_assistant->raise();
        m_assistant->activateWindow();
        return;
    }
    delete m_assistant;
    auto cb = [this](int platId, const QString& nom, double prix) {
        onAjouterAuPanier(platId, nom, prix, "");
    };
    m_assistant = new AIAssistant(
        currentClient.getId(),
        currentClient.getPrenom(),
        cb,
        this
        );
    QPoint pos = this->mapToGlobal(
        QPoint(this->width() - m_assistant->width() - 20,
               this->height() - m_assistant->height() - 20));
    m_assistant->move(pos);
    m_assistant->show();
    m_assistant->raise();
}

void ClientWindow::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);
    if (m_btnIA) {
        m_btnIA->move(this->width() - 76, this->height() - 76);
        m_btnIA->raise();
    }
}

// ═══════════════════════════════════════════
//  IMPRESSION PDF
// ═══════════════════════════════════════════

void ClientWindow::imprimerTicketCommande(int commandeId)
{
    QSqlQuery qCmd;
    qCmd.prepare(
        "SELECT c.statut, c.date_heure, c.total, "
        "       u_client.prenom || ' ' || u_client.nom, "
        "       COALESCE(u_livreur.prenom || ' ' || u_livreur.nom, 'Non assigne') "
        "FROM commandes c "
        "JOIN clients cl              ON c.client_id  = cl.id "
        "JOIN utilisateurs u_client   ON cl.id        = u_client.id "
        "LEFT JOIN livreurs lv        ON c.livreur_id = lv.id "
        "LEFT JOIN utilisateurs u_livreur ON lv.id    = u_livreur.id "
        "WHERE c.id = :id");
    qCmd.bindValue(":id", commandeId);
    if (!qCmd.exec() || !qCmd.next()) {
        QMessageBox::warning(this, tr("Erreur"),
                             QString(tr("Impossible de charger la commande #%1.\n%2"))
                                 .arg(commandeId).arg(qCmd.lastError().text()));
        return;
    }
    QString statut     = qCmd.value(0).toString();
    QString date       = qCmd.value(1).toString().left(10);
    double  total      = qCmd.value(2).toDouble();
    QString clientNom  = qCmd.value(3).toString();
    QString livreurNom = qCmd.value(4).toString();

    struct Ligne { QString nom; int qte; double prixU; };
    QList<Ligne> lignes;
    QSqlQuery qL;
    qL.prepare("SELECT p.nom, lc.quantite, lc.prix_unitaire "
               "FROM lignes_commande lc JOIN plats p ON lc.plat_id = p.id "
               "WHERE lc.commande_id = :id");
    qL.bindValue(":id", commandeId);
    qL.exec();
    while (qL.next())
        lignes.append({qL.value(0).toString(), qL.value(1).toInt(), qL.value(2).toDouble()});

    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Enregistrer le ticket PDF"),
        QDir::homePath() + QString("/ticket_commande_%1.pdf").arg(commandeId),
        "PDF (*.pdf)");
    if (filePath.isEmpty()) return;

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize::A4);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(40,40,40,40), QPageLayout::Point);

    QPainter p;
    if (!p.begin(&printer)) {
        QMessageBox::critical(this, tr("Erreur PDF"), tr("Impossible de créer le fichier."));
        return;
    }

    QRectF page = printer.pageRect(QPrinter::Point);
    double W = page.width(), y = 0;
    double ticketW = qMin(W, 340.0);
    double offsetX = (W - ticketW) / 2.0;

    auto drawLine = [&](const QString& txt, Qt::Alignment align, const QFont& font, double h) {
        p.setFont(font);
        p.drawText(QRectF(offsetX,y,ticketW,h), align|Qt::AlignVCenter, txt);
        y += h;
    };
    auto drawSep = [&](bool thick = false) {
        p.setPen(QPen(Qt::black, thick ? 2.5 : 1.0));
        p.drawLine(QPointF(offsetX, y+5), QPointF(offsetX+ticketW, y+5));
        p.setPen(Qt::black);
        y += thick ? 18 : 14;
    };
    auto drawPair = [&](const QString& cle, const QString& val) {
        QFont fN("Courier New",11), fB("Courier New",11,QFont::Bold);
        p.setFont(fN);
        p.drawText(QRectF(offsetX,y,ticketW*0.42,26), Qt::AlignLeft|Qt::AlignVCenter, cle);
        p.setFont(fB);
        p.drawText(QRectF(offsetX+ticketW*0.42,y,ticketW*0.58,26),
                   Qt::AlignLeft|Qt::AlignVCenter, ": "+val);
        y += 26;
    };

    QFont fT("Courier New",18,QFont::Bold), fST("Courier New",12,QFont::Bold);
    QFont fN("Courier New",11), fB("Courier New",11,QFont::Bold), fSm("Courier New",10);

    y += 10;
    drawLine("FOOD RUSH",            Qt::AlignHCenter, fT,  42);
    drawLine("Restaurant Dashboard", Qt::AlignHCenter, fST, 28);
    drawLine(tr("Ticket de commande"),Qt::AlignHCenter, fST, 28);
    y += 8; drawSep(true);

    drawPair(tr("Commande N"), QString::number(commandeId));
    drawPair(tr("Date"),       date);
    drawPair(tr("Client"),     clientNom);
    drawPair(tr("Livreur"),    livreurNom);
    drawPair(tr("Statut"),     statut.toUpper().replace("_"," "));
    y += 6; drawSep(true);

    p.setFont(fB);
    p.drawText(QRectF(offsetX,           y, ticketW*0.55, 28), Qt::AlignLeft|Qt::AlignVCenter, tr("Plat"));
    p.drawText(QRectF(offsetX+ticketW*0.55, y, ticketW*0.15, 28), Qt::AlignHCenter, tr("Qté"));
    p.drawText(QRectF(offsetX+ticketW*0.70, y, ticketW*0.30, 28), Qt::AlignRight|Qt::AlignVCenter, tr("Prix"));
    y += 28; drawSep(true);

    for (const Ligne& l : std::as_const(lignes)) {
        double montant = l.qte * l.prixU;
        p.setFont(fN);
        p.drawText(QRectF(offsetX,           y, ticketW*0.55, 26), Qt::AlignLeft|Qt::AlignVCenter, l.nom);
        p.drawText(QRectF(offsetX+ticketW*0.55, y, ticketW*0.15, 26), Qt::AlignHCenter, QString::number(l.qte));
        p.drawText(QRectF(offsetX+ticketW*0.70, y, ticketW*0.30, 26), Qt::AlignRight|Qt::AlignVCenter,
                   QString("%1 DH").arg(qRound(montant)));
        y += 26;
    }
    y += 6; drawSep(true);
    p.setFont(fB);
    p.drawText(QRectF(offsetX,y,ticketW,32), Qt::AlignHCenter|Qt::AlignVCenter,
               QString(tr("TOTAL : %1 DH")).arg(total,0,'f',2));
    y += 32; drawSep(true);
    y += 8;
    drawLine(tr("Merci pour votre commande!"), Qt::AlignHCenter, fST, 30);
    drawLine("www.foodorbit.ma",                Qt::AlignHCenter, fSm, 24);
    p.end();
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}