#include "Interface_Livreur.h"
#include "ui_Interface_Livreur.h"
#include "loginwindow.h"
#include <QDate>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
// Constructeur
// ─────────────────────────────────────────────────────────────
Interface_Livreur::Interface_Livreur(int livreurId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Interface_Livreur)
    , m_livreurId(livreurId)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    qApp->setStyleSheet(qApp->styleSheet() +
                        "QMessageBox { background-color: #F4F6F0; }"
                        "QMessageBox QLabel { color: #1B4332; font-size: 14px;"
                        "  font-weight: bold; min-width: 250px; }"
                        "QMessageBox QPushButton { background-color: #2D6A4F; color: white;"
                        "  border: none; border-radius: 8px; padding: 8px 24px;"
                        "  font-size: 13px; font-weight: bold; min-width: 80px; }"
                        "QMessageBox QPushButton:hover { background-color: #1B4332; }"
                        );

    setWindowTitle(" Dashboard Livreur — Food Rush");
    setMinimumSize(1000, 660);

    // Nom du livreur dans la sidebar
    QSqlQuery q;
    q.prepare("SELECT prenom, nom FROM utilisateurs WHERE id = :id");
    q.bindValue(":id", m_livreurId);
    q.exec();
    if (q.next())
        ui->lblNomLivreur->setText(q.value(0).toString() + " " + q.value(1).toString());

    // ── Connexions navigation ──
    connect(ui->btnDisponibles,    &QPushButton::clicked, this, &Interface_Livreur::afficherPageDisponibles);
    connect(ui->btnMesLivraisons,  &QPushButton::clicked, this, &Interface_Livreur::afficherPageMesLivraisons);
    connect(ui->btnHistorique,     &QPushButton::clicked, this, &Interface_Livreur::afficherPageHistorique);
    connect(ui->btnStatistiques,   &QPushButton::clicked, this, &Interface_Livreur::afficherPageStatistiques);
    connect(ui->btnLogout,         &QPushButton::clicked, this, &Interface_Livreur::onLogout);

    afficherPageDisponibles();
}

Interface_Livreur::~Interface_Livreur()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────
// Navigation + style sidebar
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::afficherPageDisponibles()
{
    ui->stackedWidget->setCurrentIndex(0);
    updateSidebarStyle(0);
    chargerCommandesDisponibles();
}

void Interface_Livreur::afficherPageMesLivraisons()
{
    ui->stackedWidget->setCurrentIndex(1);
    updateSidebarStyle(1);
    chargerMesLivraisons();
}

void Interface_Livreur::afficherPageHistorique()
{
    ui->stackedWidget->setCurrentIndex(2);
    updateSidebarStyle(2);
    chargerHistorique();
}

void Interface_Livreur::afficherPageStatistiques()
{
    ui->stackedWidget->setCurrentIndex(3);
    updateSidebarStyle(3);
    chargerStatistiques();
}

void Interface_Livreur::onLogout()
{
    LoginWindow* w = new LoginWindow();
    w->show();
    this->close();
}

void Interface_Livreur::updateSidebarStyle(int pageIndex)
{
    QString actif = R"(QPushButton {
        background: #40916C; color: white; border-radius: 10px;
        font-size: 14px; font-weight: bold; text-align: left;
        padding: 12px 20px; border: none; })";

    QString inactif = R"(QPushButton {
        background: transparent; color: rgba(255,255,255,0.75);
        border-radius: 10px; font-size: 14px; text-align: left;
        padding: 12px 20px; border: none; }
        QPushButton:hover { background: rgba(255,255,255,0.1); color: white; })";

    ui->btnDisponibles->setStyleSheet(pageIndex == 0 ? actif : inactif);
    ui->btnMesLivraisons->setStyleSheet(pageIndex == 1 ? actif : inactif);
    ui->btnHistorique->setStyleSheet(pageIndex == 2 ? actif : inactif);
    ui->btnStatistiques->setStyleSheet(pageIndex == 3 ? actif : inactif);
}

// ─────────────────────────────────────────────────────────────
// Charger les articles d'une commande
// ─────────────────────────────────────────────────────────────
QList<QPair<QString,int>> Interface_Livreur::chargerArticles(int commandeId)
{
    QList<QPair<QString,int>> articles;
    QSqlQuery q;
    q.prepare(
        "SELECT p.nom, lc.quantite "
        "FROM lignes_commande lc JOIN plats p ON lc.plat_id = p.id "
        "WHERE lc.commande_id = :id"
        );
    q.bindValue(":id", commandeId);
    q.exec();
    while (q.next())
        articles.append({q.value(0).toString(), q.value(1).toInt()});
    return articles;
}

// ─────────────────────────────────────────────────────────────
// PAGE 0 — Commandes disponibles (confirmées, sans livreur)
// Lien avec Restaurant : le restaurant passe 'en_attente' → 'confirmee'
// Le livreur prend ensuite en charge.
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::chargerCommandesDisponibles()
{
    viderLayout(ui->layoutDisponibles);

    QSqlQuery query;
    query.exec(
        "SELECT c.id, u.prenom || ' ' || u.nom, "
        "       COALESCE(cl.adresse,'Non renseignée'), c.date_heure, c.total "
        "FROM commandes c "
        "JOIN clients cl         ON c.client_id = cl.id "
        "JOIN utilisateurs u     ON cl.id = u.id "
        "WHERE c.statut = 'confirmee' AND c.livreur_id IS NULL "
        "ORDER BY c.date_heure ASC"
        );

    bool aucune = true;
    while (query.next()) {
        aucune = false;
        int     id      = query.value(0).toInt();
        QString client  = query.value(1).toString();
        QString adresse = query.value(2).toString();
        QString date    = query.value(3).toString().left(16);
        double  total   = query.value(4).toDouble();

        ui->layoutDisponibles->addWidget(
            creerCarteCommande(id, client, adresse, date, total,
                               "confirmee", chargerArticles(id), true, false));
    }

    if (aucune) {
        QLabel* lbl = new QLabel("Aucune commande disponible pour le moment.");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #74C69D; font-size: 15px; padding: 40px;");
        ui->layoutDisponibles->addWidget(lbl);
    }
    ui->layoutDisponibles->addStretch();
}

// ─────────────────────────────────────────────────────────────
// PAGE 1 — Mes livraisons en cours
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::chargerMesLivraisons()
{
    viderLayout(ui->layoutMesLivraisons);

    QSqlQuery query;
    query.prepare(
        "SELECT c.id, u.prenom || ' ' || u.nom, "
        "       COALESCE(cl.adresse,'Non renseignée'), c.date_heure, c.total "
        "FROM commandes c "
        "JOIN clients cl         ON c.client_id = cl.id "
        "JOIN utilisateurs u     ON cl.id = u.id "
        "WHERE c.statut = 'en_livraison' AND c.livreur_id = :lid "
        "ORDER BY c.date_heure ASC"
        );
    query.bindValue(":lid", m_livreurId);
    query.exec();

    bool aucune = true;
    while (query.next()) {
        aucune = false;
        int     id      = query.value(0).toInt();
        QString client  = query.value(1).toString();
        QString adresse = query.value(2).toString();
        QString date    = query.value(3).toString().left(16);
        double  total   = query.value(4).toDouble();

        ui->layoutMesLivraisons->addWidget(
            creerCarteCommande(id, client, adresse, date, total,
                               "en_livraison", chargerArticles(id), false, true));
    }

    if (aucune) {
        QLabel* lbl = new QLabel("  Aucune livraison en cours.");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #74C69D; font-size: 15px; padding: 40px;");
        ui->layoutMesLivraisons->addWidget(lbl);
    }
    ui->layoutMesLivraisons->addStretch();
}

// ─────────────────────────────────────────────────────────────
// PAGE 2 — Historique (livrees + annulees par moi)
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::chargerHistorique()
{
    viderLayout(ui->layoutHistorique);

    QSqlQuery query;
    query.prepare(
        "SELECT c.id, u.prenom || ' ' || u.nom, "
        "       COALESCE(cl.adresse,'Non renseignée'), c.date_heure, c.total, c.statut "
        "FROM commandes c "
        "JOIN clients cl         ON c.client_id = cl.id "
        "JOIN utilisateurs u     ON cl.id = u.id "
        "WHERE c.livreur_id = :lid AND c.statut IN ('livree','annulee') "
        "ORDER BY c.date_heure DESC"
        );
    query.bindValue(":lid", m_livreurId);
    query.exec();

    bool aucune = true;
    while (query.next()) {
        aucune = false;
        int     id      = query.value(0).toInt();
        QString client  = query.value(1).toString();
        QString adresse = query.value(2).toString();
        QString date    = query.value(3).toString().left(16);
        double  total   = query.value(4).toDouble();
        QString statut  = query.value(5).toString();

        ui->layoutHistorique->addWidget(
            creerCarteCommande(id, client, adresse, date, total,
                               statut, chargerArticles(id), false, false));
    }

    if (aucune) {
        QLabel* lbl = new QLabel(" Aucun historique.");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #74C69D; font-size: 15px; padding: 40px;");
        ui->layoutHistorique->addWidget(lbl);
    }
    ui->layoutHistorique->addStretch();
}

// ─────────────────────────────────────────────────────────────
// PAGE 3 — Statistiques personnelles du livreur
// Combinées avec les données du restaurant (commandes globales)
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::chargerStatistiques()
{
    viderLayout(ui->layoutStats);

    // ── KPI personnels ──────────────────────────────────────
    QSqlQuery qLivrees;
    qLivrees.prepare("SELECT COUNT(*), COALESCE(SUM(total),0) FROM commandes "
                     "WHERE livreur_id=:lid AND statut='livree'");
    qLivrees.bindValue(":lid", m_livreurId); qLivrees.exec(); qLivrees.next();
    int    nbLivrees  = qLivrees.value(0).toInt();
    double gainsTotal = qLivrees.value(1).toDouble();

    QSqlQuery qEnCours;
    qEnCours.prepare("SELECT COUNT(*) FROM commandes "
                     "WHERE livreur_id=:lid AND statut='en_livraison'");
    qEnCours.bindValue(":lid", m_livreurId); qEnCours.exec(); qEnCours.next();
    int nbEnCours = qEnCours.value(0).toInt();

    QSqlQuery qAnnulees;
    qAnnulees.prepare("SELECT COUNT(*) FROM commandes "
                      "WHERE livreur_id=:lid AND statut='annulee'");
    qAnnulees.bindValue(":lid", m_livreurId); qAnnulees.exec(); qAnnulees.next();
    int nbAnnulees = qAnnulees.value(0).toInt();

    int    nbTotal      = nbLivrees + nbEnCours + nbAnnulees;
    double tauxReussite = nbTotal > 0 ? qRound((double)nbLivrees / nbTotal * 100.0) : 0;
    double ticketMoyen  = nbLivrees > 0 ? gainsTotal / nbLivrees : 0.0;

    // ── Section titre ──
    QLabel* lblTitre = new QLabel("Mes statistiques de livraison");
    lblTitre->setStyleSheet("font-size: 18px; font-weight: bold; color: #1B4332; margin-bottom: 8px;");
    ui->layoutStats->addWidget(lblTitre);

    // ── Grille KPI ──
    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(12);
    grid->addWidget(creerCarteKPI("Livraisons réussies",
                                  QString::number(nbLivrees), "#2D6A4F", "white"), 0, 0);
    grid->addWidget(creerCarteKPI("En cours",
                                  QString::number(nbEnCours), "#1D4ED8", "white"), 0, 1);
    grid->addWidget(creerCarteKPI("Annulées",
                                  QString::number(nbAnnulees), "#DC2626", "white"), 0, 2);
    grid->addWidget(creerCarteKPI("Taux de réussite",
                                  QString("%1%").arg(tauxReussite), "#059669", "white"), 1, 0);
    grid->addWidget(creerCarteKPI("Total traité",
                                  QString::number(nbTotal), "#6D28D9", "white"), 1, 1);
    grid->addWidget(creerCarteKPI("Ticket moyen",
                                  QString("%1 DH").arg(ticketMoyen, 0, 'f', 2), "#B45309", "white"), 1, 2);

    QWidget* gridW = new QWidget();
    gridW->setLayout(grid);
    ui->layoutStats->addWidget(gridW);

    // ── Barre progression taux de réussite ──
    QLabel* lblBarre = new QLabel("Taux de réussite global");
    lblBarre->setStyleSheet("font-size: 13px; font-weight: bold; color: #2D6A4F; margin-top: 10px;");
    ui->layoutStats->addWidget(lblBarre);

    QProgressBar* barre = new QProgressBar();
    barre->setRange(0, 100);
    barre->setValue(tauxReussite);
    barre->setFixedHeight(18);
    barre->setStyleSheet(
        "QProgressBar { border-radius: 9px; background: #D8F3DC; border: none; }"
        "QProgressBar::chunk { border-radius: 9px; background: #40916C; }");
    ui->layoutStats->addWidget(barre);

    // ── Séparateur ──
    QFrame* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #D1FAE5; margin: 10px 0;");
    ui->layoutStats->addWidget(sep);

    // ── Mes 5 dernières livraisons ──
    QLabel* lblDernieres = new QLabel("  Mes 5 dernières livraisons");
    lblDernieres->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F;");
    ui->layoutStats->addWidget(lblDernieres);

    QSqlQuery qDernieres;
    qDernieres.prepare(
        "SELECT c.id, u.prenom || ' ' || u.nom, c.date_heure, c.total, c.statut "
        "FROM commandes c "
        "JOIN utilisateurs u ON c.client_id = u.id "
        "WHERE c.livreur_id = :lid AND c.statut IN ('livree','annulee') "
        "ORDER BY c.date_heure DESC LIMIT 5"
        );
    qDernieres.bindValue(":lid", m_livreurId);
    qDernieres.exec();

    bool aucune = true;
    while (qDernieres.next()) {
        aucune = false;
        int     cmdId  = qDernieres.value(0).toInt();
        QString client = qDernieres.value(1).toString();
        QString date   = qDernieres.value(2).toString().left(16);
        double  total  = qDernieres.value(3).toDouble();
        QString statut = qDernieres.value(4).toString();

        QFrame* ligne = new QFrame();
        ligne->setFixedHeight(46);
        QString bg = (statut == "livree") ? "#D1FAE5" : "#FEE2E2";
        QString tc = (statut == "livree") ? "#065F46" : "#991B1B";
        ligne->setStyleSheet(QString("QFrame { background:%1; border-radius:10px; border:none; }").arg(bg));

        QHBoxLayout* hb = new QHBoxLayout(ligne);
        hb->setContentsMargins(14, 6, 14, 6);

        auto lbl = [](const QString& t, const QString& style) {
            QLabel* l = new QLabel(t);
            l->setStyleSheet(style + " background:transparent; border:none;");
            return l;
        };

        hb->addWidget(lbl(QString("#%1").arg(cmdId),
                          "font-size:12px; font-weight:bold; color:#1B4332;"));
        hb->addWidget(lbl(client, "font-size:12px; color:#374151;"), 1);
        hb->addWidget(lbl(date, "font-size:11px; color:#6B7280;"));
        hb->addWidget(lbl(QString("%1 DH").arg(total,0,'f',2),
                          "font-size:12px; font-weight:bold; color:#2D6A4F;"));
        hb->addWidget(lbl(statut.toUpper().replace("_"," "),
                          QString("font-size:11px; font-weight:bold; color:%1;").arg(tc)));

        ui->layoutStats->addWidget(ligne);
    }

    if (aucune) {
        QLabel* l = new QLabel("Aucune livraison terminée pour le moment.");
        l->setStyleSheet("color: #74C69D; font-size: 13px;");
        ui->layoutStats->addWidget(l);
    }

    // ── Séparateur ──
    QFrame* sep2 = new QFrame();
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #D1FAE5; margin: 10px 0;");
    ui->layoutStats->addWidget(sep2);

    // ── Stats globales restaurant (vue livreur) ──
    QLabel* lblGlobal = new QLabel("️  Vue globale restaurant");
    lblGlobal->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F;");
    ui->layoutStats->addWidget(lblGlobal);

    QSqlQuery qGlobal;
    qGlobal.exec("SELECT COUNT(*), "
                 "SUM(CASE WHEN statut='livree' THEN 1 ELSE 0 END), "
                 "SUM(CASE WHEN statut='en_attente' THEN 1 ELSE 0 END), "
                 "SUM(CASE WHEN statut='confirmee' THEN 1 ELSE 0 END), "
                 "SUM(CASE WHEN statut='en_livraison' THEN 1 ELSE 0 END) "
                 "FROM commandes");
    qGlobal.next();
    int gTotal      = qGlobal.value(0).toInt();
    int gLivrees    = qGlobal.value(1).toInt();
    int gAttente    = qGlobal.value(2).toInt();
    int gConfirmees = qGlobal.value(3).toInt();
    int gEnLivr     = qGlobal.value(4).toInt();

    QGridLayout* grid2 = new QGridLayout();
    grid2->setSpacing(10);
    grid2->addWidget(creerCarteKPI("Total commandes",
                                   QString::number(gTotal),     "#374151", "white"), 0, 0);
    grid2->addWidget(creerCarteKPI("Livrées",
                                   QString::number(gLivrees),   "#059669", "white"), 0, 1);
    grid2->addWidget(creerCarteKPI("En attente",
                                   QString::number(gAttente),   "#D97706", "white"), 1, 0);
    grid2->addWidget(creerCarteKPI("Confirmées dispo",
                                   QString::number(gConfirmees),"#1D4ED8", "white"), 1, 1);
    grid2->addWidget(creerCarteKPI("En livraison",
                                   QString::number(gEnLivr),    "#7C3AED", "white"), 1, 2);

    QWidget* grid2W = new QWidget();
    grid2W->setLayout(grid2);
    ui->layoutStats->addWidget(grid2W);

    // ── Évolution mensuelle de mes livraisons ──
    QLabel* lblMois = new QLabel(" Mes livraisons par mois (année en cours)");
    lblMois->setStyleSheet("font-size: 14px; font-weight: bold; color: #2D6A4F; margin-top: 10px;");
    ui->layoutStats->addWidget(lblMois);

    QString annee = QString::number(QDate::currentDate().year());
    QVector<int> parMois(12, 0);

    QSqlQuery qMois;
    qMois.prepare(
        "SELECT strftime('%m', date_heure) AS mois, COUNT(*) "
        "FROM commandes "
        "WHERE livreur_id=:lid AND statut='livree' "
        "  AND strftime('%Y', date_heure)=:an "
        "GROUP BY mois"
        );
    qMois.bindValue(":lid", m_livreurId);
    qMois.bindValue(":an",  annee);
    qMois.exec();
    while (qMois.next())
        parMois[qMois.value(0).toInt() - 1] = qMois.value(1).toInt();

    int maxMois = *std::max_element(parMois.begin(), parMois.end());
    if (maxMois < 1) maxMois = 1;

    QStringList moisLabels = {"Jan","Fév","Mar","Avr","Mai","Jun",
                              "Jul","Aoû","Sep","Oct","Nov","Déc"};

    QFrame* conteneurMois = new QFrame();
    conteneurMois->setStyleSheet(
        "QFrame { background:#FFFFFF; border-radius:12px; border:1px solid #D8EDDF; }");
    QVBoxLayout* conteneurLay = new QVBoxLayout(conteneurMois);
    conteneurLay->setContentsMargins(14,12,14,12);
    conteneurLay->setSpacing(6);

    for (int i = 0; i < 12; ++i) {
        QFrame* row = new QFrame();
        row->setStyleSheet("QFrame { background:transparent; border:none; }");
        row->setFixedHeight(24);
        QHBoxLayout* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0,2,0,2); rl->setSpacing(8);

        QLabel* lblM = new QLabel(moisLabels[i]);
        lblM->setFixedWidth(32);
        lblM->setStyleSheet("font-size:11px; font-weight:bold; color:#1B4332; background:transparent; border:none;");

        QProgressBar* bar = new QProgressBar();
        bar->setRange(0, 1000);
        bar->setValue(qRound((double)parMois[i] / maxMois * 1000.0));
        bar->setFixedHeight(10);
        bar->setStyleSheet(
            "QProgressBar { border-radius:5px; background:#D8F3DC; border:none; }"
            "QProgressBar::chunk { border-radius:5px; background:#40916C; }");

        QLabel* lblV = new QLabel(parMois[i] > 0 ? QString::number(parMois[i]) : "-");
        lblV->setFixedWidth(24);
        lblV->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblV->setStyleSheet("font-size:11px; color:#2D6A4F; font-weight:bold; background:transparent; border:none;");

        rl->addWidget(lblM);
        rl->addWidget(bar, 1);
        rl->addWidget(lblV);
        conteneurLay->addWidget(row);
    }

    ui->layoutStats->addWidget(conteneurMois);

    // Spacer final
    ui->layoutStats->addItem(
        new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

// ─────────────────────────────────────────────────────────────
// Action : Prendre en charge (confirmee → en_livraison)
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::prendreEnCharge(int commandeId)
{
    QSqlQuery q;
    q.prepare(
        "UPDATE commandes SET statut='en_livraison', livreur_id=:lid "
        "WHERE id=:id AND statut='confirmee' AND livreur_id IS NULL"
        );
    q.bindValue(":lid", m_livreurId);
    q.bindValue(":id",  commandeId);

    if (!q.exec() || q.numRowsAffected() == 0) {
        QMessageBox::warning(this, "Erreur",
                             "Impossible de prendre cette commande.\n"
                             "Elle a peut-être déjà été assignée à un autre livreur.");
        chargerCommandesDisponibles();
        return;
    }

    QMessageBox::information(this, " Commande prise en charge",
                             QString("Commande #%1 assignée. Bonne livraison !").arg(commandeId));
    afficherPageMesLivraisons();
}

// ─────────────────────────────────────────────────────────────
// Action : Marquer livrée (en_livraison → livree)
// Le client voit ce statut dans son suivi de commande
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::marquerLivree(int commandeId)
{
    QSqlQuery q;
    q.prepare(
        "UPDATE commandes SET statut='livree' "
        "WHERE id=:id AND livreur_id=:lid AND statut='en_livraison'"
        );
    q.bindValue(":id",  commandeId);
    q.bindValue(":lid", m_livreurId);

    if (!q.exec() || q.numRowsAffected() == 0) {
        QMessageBox::warning(this, "Erreur", "Impossible de mettre à jour le statut.");
        return;
    }

    QMessageBox::information(this, " Livraison confirmée",
                             QString("Commande #%1 marquée comme livrée !").arg(commandeId));
    chargerMesLivraisons();
}

// ─────────────────────────────────────────────────────────────
// Action : Marquer échouée (en_livraison → annulee)
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::marquerEchouee(int commandeId)
{
    auto rep = QMessageBox::question(this, "Confirmer l'échec",
                                     QString("Marquer la commande #%1 comme non livrée ?").arg(commandeId),
                                     QMessageBox::Yes | QMessageBox::No);
    if (rep != QMessageBox::Yes) return;

    QSqlQuery q;
    q.prepare(
        "UPDATE commandes SET statut='annulee' "
        "WHERE id=:id AND livreur_id=:lid AND statut='en_livraison'"
        );
    q.bindValue(":id",  commandeId);
    q.bindValue(":lid", m_livreurId);

    if (!q.exec() || q.numRowsAffected() == 0) {
        QMessageBox::warning(this, "Erreur", "Impossible de mettre à jour le statut.");
        return;
    }
    chargerMesLivraisons();
}

// ─────────────────────────────────────────────────────────────
// Carte commande (réutilisable pour les 3 pages)
// ─────────────────────────────────────────────────────────────
QWidget* Interface_Livreur::creerCarteCommande(
    int commandeId, const QString& clientNom, const QString& adresse,
    const QString& dateHeure, double total, const QString& statut,
    const QList<QPair<QString,int>>& articles, bool disponible, bool enCours)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet(R"(
        QFrame { background:white; border-radius:14px; border:1px solid #D8EDDF; }
        QFrame:hover { border-color:#40916C; }
    )");
    carte->setMinimumHeight(130);

    QVBoxLayout* vMain = new QVBoxLayout(carte);
    vMain->setContentsMargins(18,14,18,14);
    vMain->setSpacing(8);

    // ── En-tête ──
    QHBoxLayout* hHead = new QHBoxLayout();

    QLabel* lblId = new QLabel(QString("Commande #%1").arg(commandeId));
    lblId->setStyleSheet("font-size:15px; font-weight:bold; color:#1B4332;");

    // Badge statut
    QMap<QString,QString> bg = {
        {"confirmee","#DBEAFE"},{"en_livraison","#FFF3CD"},
        {"livree","#D1FAE5"},   {"annulee","#FEE2E2"}
    };
    QMap<QString,QString> tc = {
        {"confirmee","#1E40AF"},{"en_livraison","#856404"},
        {"livree","#065F46"},   {"annulee","#991B1B"}
    };
    QLabel* lblStatut = new QLabel(statut.toUpper().replace("_"," "));
    lblStatut->setStyleSheet(QString(
                                 "background:%1; color:%2; border-radius:10px;"
                                 "padding:3px 12px; font-size:11px; font-weight:bold;")
                                 .arg(bg.value(statut,"#E5E7EB"), tc.value(statut,"#374151")));
    lblStatut->setFixedHeight(22);

    QLabel* lblDate = new QLabel(dateHeure);
    lblDate->setStyleSheet("font-size:11px; color:#6B7280;");

    hHead->addWidget(lblId);
    hHead->addStretch();
    hHead->addWidget(lblStatut);
    hHead->addSpacing(10);
    hHead->addWidget(lblDate);
    vMain->addLayout(hHead);

    // ── Infos client + adresse ──
    QHBoxLayout* hInfo = new QHBoxLayout();
    QLabel* lblClient = new QLabel("  " + clientNom);
    lblClient->setStyleSheet("font-size:13px; color:#374151;");
    QLabel* lblAddr   = new QLabel("  " + adresse);
    lblAddr->setStyleSheet("font-size:13px; color:#374151;");
    lblAddr->setWordWrap(true);
    hInfo->addWidget(lblClient);
    hInfo->addSpacing(20);
    hInfo->addWidget(lblAddr, 1);
    vMain->addLayout(hInfo);

    // ── Articles ──
    if (!articles.isEmpty()) {
        QStringList lst;
        for (const auto& a : articles)
            lst << QString("%1 x%2").arg(a.first).arg(a.second);
        QLabel* lblArt = new QLabel("🍽️  " + lst.join("  ·  "));
        lblArt->setStyleSheet(
            "font-size:12px; color:#6B7280; background:#F9FAFB;"
            "border-radius:8px; padding:4px 10px;");
        lblArt->setWordWrap(true);
        vMain->addWidget(lblArt);
    }

    // ── Pied ──
    QHBoxLayout* hFoot = new QHBoxLayout();
    QLabel* lblTotal = new QLabel(QString("Total : %1 DH").arg(total,0,'f',2));
    lblTotal->setStyleSheet("font-size:14px; font-weight:bold; color:#2D6A4F;");
    hFoot->addWidget(lblTotal);
    hFoot->addStretch();

    if (disponible) {
        QPushButton* btn = new QPushButton("  Prendre en charge");
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(QPushButton { background:#2D6A4F; color:white;
            border-radius:10px; font-size:13px; font-weight:bold;
            border:none; padding:0 18px; }
            QPushButton:hover { background:#1B4332; })");
        connect(btn, &QPushButton::clicked, this, [this, commandeId]() {
            prendreEnCharge(commandeId);
        });
        hFoot->addWidget(btn);
    }

    if (enCours) {
        QPushButton* btnEch = new QPushButton(" Échec");
        btnEch->setFixedHeight(36);
        btnEch->setCursor(Qt::PointingHandCursor);
        btnEch->setStyleSheet(R"(QPushButton { background:#DC2626; color:white;
            border-radius:10px; font-size:13px; font-weight:bold;
            border:none; padding:0 16px; }
            QPushButton:hover { background:#991B1B; })");
        connect(btnEch, &QPushButton::clicked, this, [this, commandeId]() {
            marquerEchouee(commandeId);
        });

        QPushButton* btnLiv = new QPushButton("  Livré");
        btnLiv->setFixedHeight(36);
        btnLiv->setCursor(Qt::PointingHandCursor);
        btnLiv->setStyleSheet(R"(QPushButton { background:#40916C; color:white;
            border-radius:10px; font-size:13px; font-weight:bold;
            border:none; padding:0 16px; }
            QPushButton:hover { background:#2D6A4F; })");
        connect(btnLiv, &QPushButton::clicked, this, [this, commandeId]() {
            marquerLivree(commandeId);
        });

        hFoot->addSpacing(8);
        hFoot->addWidget(btnEch);
        hFoot->addSpacing(6);
        hFoot->addWidget(btnLiv);
    }

    vMain->addLayout(hFoot);
    return carte;
}

// ─────────────────────────────────────────────────────────────
// Carte KPI
// ─────────────────────────────────────────────────────────────
QFrame* Interface_Livreur::creerCarteKPI(const QString& titre, const QString& valeur,
                                         const QString& couleurBg, const QString& couleurTexte)
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet(QString(
                             "QFrame { background:%1; border-radius:12px; border:none; }").arg(couleurBg));
    carte->setMinimumHeight(80);

    QVBoxLayout* lay = new QVBoxLayout(carte);
    lay->setContentsMargins(16,10,16,10);
    lay->setSpacing(4);

    QLabel* lblT = new QLabel(titre);
    lblT->setStyleSheet(QString("font-size:11px; font-weight:bold; color:%1;"
                                " background:transparent; border:none;").arg(couleurTexte));
    lblT->setWordWrap(true);

    QLabel* lblV = new QLabel(valeur);
    lblV->setStyleSheet(QString("font-size:22px; font-weight:bold; color:%1;"
                                " background:transparent; border:none;").arg(couleurTexte));

    lay->addWidget(lblT);
    lay->addWidget(lblV);
    return carte;
}

// ─────────────────────────────────────────────────────────────
// Vider un layout
// ─────────────────────────────────────────────────────────────
void Interface_Livreur::viderLayout(QLayout* layout)
{
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}