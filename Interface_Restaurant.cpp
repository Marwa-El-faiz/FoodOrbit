#include "Interface_Restaurant.h"
#include "ui_Interface_Restaurant.h"
#include <QTimer>
#include <QDate>
#include <QRegularExpression>
#include <algorithm>

Interface_Restaurant::Interface_Restaurant(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Interface_Restaurant)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    // Style global QMessageBox — thème Food Rush
    qApp->setStyleSheet(qApp->styleSheet() +
                        "QMessageBox {"
                        "  background-color: #F4F6F0;"
                        "  font-family: 'Segoe UI', sans-serif;"
                        "}"
                        "QMessageBox QLabel {"
                        "  color: #1B4332;"
                        "  font-size: 14px;"
                        "  font-weight: bold;"
                        "  min-width: 250px;"
                        "}"
                        "QMessageBox QPushButton {"
                        "  background-color: #2D6A4F;"
                        "  color: white;"
                        "  border: none;"
                        "  border-radius: 8px;"
                        "  padding: 8px 24px;"
                        "  font-size: 13px;"
                        "  font-weight: bold;"
                        "  min-width: 80px;"
                        "}"
                        "QMessageBox QPushButton:hover {"
                        "  background-color: #1B4332;"
                        "}"
                        "QMessageBox QDialogButtonBox {"
                        "  button-layout: 0;"
                        "}"
                        );
    setWindowTitle("🍽️  Dashboard Restaurant");
    setMinimumSize(1100, 680);

    // ── Fix sidebar background (QWidget ignore stylesheet sans autoFillBackground) ──
    ui->sidebar->setAutoFillBackground(true);
    QPalette pal = ui->sidebar->palette();
    pal.setColor(QPalette::Window, QColor("#2D6A4F"));
    ui->sidebar->setPalette(pal);

    // Navigation
    connect(ui->btnNavMenu,        &QPushButton::clicked, this, &Interface_Restaurant::afficherPageMenu);
    connect(ui->btnNavPlats,       &QPushButton::clicked, this, &Interface_Restaurant::afficherPagePlats);
    connect(ui->btnNavCategories,  &QPushButton::clicked, this, &Interface_Restaurant::afficherPageCategories);
    connect(ui->btnNavCommandes,   &QPushButton::clicked, this, &Interface_Restaurant::afficherPageCommandes);
    connect(ui->btnNavStatistiques,&QPushButton::clicked, this, &Interface_Restaurant::afficherPageStatistiques);
    connect(ui->btnNavPlanning,    &QPushButton::clicked, this, &Interface_Restaurant::afficherPagePlanning);

    // Tableaux
    connect(ui->tableauPlats,      &QTableWidget::itemSelectionChanged,
            this, &Interface_Restaurant::on_tableauPlats_itemSelectionChanged);
    connect(ui->tableauCategories, &QTableWidget::itemSelectionChanged,
            this, &Interface_Restaurant::on_tableauCategories_itemSelectionChanged);

    // Plats
    connect(ui->btnAjouterPlat,      &QPushButton::clicked, this, &Interface_Restaurant::on_btnAjouterPlat_clicked);
    connect(ui->btnModifierPlat,     &QPushButton::clicked, this, &Interface_Restaurant::on_btnModifierPlat_clicked);
    connect(ui->btnSupprimerPlat,    &QPushButton::clicked, this, &Interface_Restaurant::on_btnSupprimerPlat_clicked);
    connect(ui->btnToggleDisponible, &QPushButton::clicked, this, &Interface_Restaurant::on_btnToggleDisponible_clicked);
    connect(ui->btnChoisirImagePlat, &QPushButton::clicked, this, &Interface_Restaurant::on_btnChoisirImagePlat_clicked);

    // Catégories
    connect(ui->btnAjouterCategorie,     &QPushButton::clicked, this, &Interface_Restaurant::on_btnAjouterCategorie_clicked);
    connect(ui->btnModifierCategorie,    &QPushButton::clicked, this, &Interface_Restaurant::on_btnModifierCategorie_clicked);
    connect(ui->btnSupprimerCategorie,   &QPushButton::clicked, this, &Interface_Restaurant::on_btnSupprimerCategorie_clicked);
    connect(ui->btnChoisirImageCategorie,&QPushButton::clicked, this, &Interface_Restaurant::on_btnChoisirImageCategorie_clicked);

    // Commandes
    connect(ui->btnChangerStatut, &QPushButton::clicked, this, &Interface_Restaurant::on_btnChangerStatut_clicked);

    // Planning
    connect(ui->btnSauvegarderHoraires, &QPushButton::clicked, this, &Interface_Restaurant::sauvegarderHoraires);

    // Déconnexion
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, &Interface_Restaurant::on_btnDeconnexion_clicked);

    // Init
    chargerCategoriesDansCombo();
    chargerPlats();
    chargerCategories();
    chargerCommandes();
    afficherPageMenu(); // page par défaut = Menu

    // ── Timer : avancement automatique des statuts ────────────────────────
    // Avance chaque commande active vers le statut suivant toutes les 30s.
    // En production : augmenter l'intervalle (ex: 5 * 60 * 1000 = 5 min).
    m_timerAuto = new QTimer(this);
    connect(m_timerAuto, &QTimer::timeout,
            this, &Interface_Restaurant::avancerStatutsAuto);
    m_timerAuto->start(30 * 1000); // 30 secondes
    qDebug() << "[FoodOrbit] Timer auto-statuts démarré (30s)";
}

// ════════════════════════════════════════════════════════════════
//  AVANCEMENT AUTOMATIQUE DES STATUTS (appelé toutes les 30s)
//  Avance toutes les commandes actives vers le statut suivant.
//  Statuts ignorés : "livree", "annulee" (terminaux).
// ════════════════════════════════════════════════════════════════
void Interface_Restaurant::avancerStatutsAuto()
{
    // Récupérer toutes les commandes non terminées
    QSqlQuery q;
    q.exec(
        "SELECT id, statut FROM commandes "
        "WHERE statut NOT IN ('livree', 'annulee') "
        "ORDER BY id ASC"
        );

    QList<QPair<int,QString>> aAvancer;
    while (q.next())
        aAvancer.append({q.value(0).toInt(), q.value(1).toString()});

    if (aAvancer.isEmpty()) return;

    int nbAvances = 0;
    for (const auto& cmd : qAsConst(aAvancer)) {
        int     id      = cmd.first;
        QString statut  = cmd.second;
        QString suivant = prochainStatut(statut);

        if (suivant.isEmpty()) continue; // déjà au statut final

        QSqlQuery upd;
        upd.prepare("UPDATE commandes SET statut = :s WHERE id = :id");
        upd.bindValue(":s",  suivant);
        upd.bindValue(":id", id);
        if (upd.exec()) {
            qDebug() << "[FoodOrbit] Auto-avancement commande #" << id
                     << ":" << statut << "→" << suivant;
            nbAvances++;
        }
    }

    // Rafraîchir le tableau si on est sur la page Commandes
    if (nbAvances > 0)
        chargerCommandes();
}

Interface_Restaurant::~Interface_Restaurant()
{
    delete ui;
}

void Interface_Restaurant::afficherPageMenu()
{
    ui->stackedWidget->setCurrentIndex(0);
    chargerBoutonsCategoriesMenu();
    afficherCartesPlats(-1); // toutes les catégories
}

void Interface_Restaurant::afficherPagePlats()
{
    ui->stackedWidget->setCurrentIndex(1);
    chargerPlats();
}

void Interface_Restaurant::afficherPageCategories()
{
    ui->stackedWidget->setCurrentIndex(2);
    chargerCategories();
}

void Interface_Restaurant::afficherPageCommandes()
{
    ui->stackedWidget->setCurrentIndex(3);
    chargerCommandes();
}

void Interface_Restaurant::afficherPageStatistiques()
{
    ui->stackedWidget->setCurrentIndex(4);
    chargerStatistiques();
}

void Interface_Restaurant::chargerBoutonsCategoriesMenu()
{
    // Vider les anciens boutons
    QLayout* layout = ui->scrollWidgetCatBtns->layout();
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Style commun pour les boutons catégorie
    QString styleInactif =
        "QPushButton {"
        "  border: 2px solid #2D6A4F;"
        "  border-radius: 20px;"
        "  padding: 8px 20px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  color: #2D6A4F;"
        "  background: #FFFFFF;"
        "}"
        "QPushButton:hover { background: #D8F3DC; }";

    QString styleActif =
        "QPushButton {"
        "  border: 2px solid #2D6A4F;"
        "  border-radius: 20px;"
        "  padding: 8px 20px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  color: #FFFFFF;"
        "  background: #2D6A4F;"
        "}";

    // Bouton "Tous"
    QPushButton* btnTous = new QPushButton("Tous");
    btnTous->setStyleSheet(m_categorieSelectionneeId == -1 ? styleActif : styleInactif);
    connect(btnTous, &QPushButton::clicked, this, [this]() {
        m_categorieSelectionneeId = -1;
        chargerBoutonsCategoriesMenu();
        afficherCartesPlats(-1);
    });
    layout->addWidget(btnTous);

    // Boutons par catégorie depuis la BD
    QSqlQuery query;
    query.exec("SELECT id, nom FROM categories ORDER BY nom");
    while (query.next()) {
        int     id  = query.value(0).toInt();
        QString nom = query.value(1).toString();

        QPushButton* btn = new QPushButton(nom);
        btn->setStyleSheet(m_categorieSelectionneeId == id ? styleActif : styleInactif);

        connect(btn, &QPushButton::clicked, this, [this, id]() {
            m_categorieSelectionneeId = id;
            chargerBoutonsCategoriesMenu();
            afficherCartesPlats(id);
        });
        layout->addWidget(btn);
    }

    // Spacer à droite
    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(spacer);
}


void Interface_Restaurant::afficherCartesPlats(int categorieId)
{
    // Vider les anciennes cartes
    QLayout* layout = ui->scrollAreaWidgetContents->layout();
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Requête SQL filtrée ou complète
    QSqlQuery query;
    if (categorieId == -1) {
        query.exec(
            "SELECT p.id, p.image_path, p.nom, p.description, p.prix, p.disponible "
            "FROM plats p ORDER BY p.nom"
            );
    } else {
        query.prepare(
            "SELECT p.id, p.image_path, p.nom, p.description, p.prix, p.disponible "
            "FROM plats p WHERE p.categorie_id = :cat ORDER BY p.nom"
            );
        query.bindValue(":cat", categorieId);
        query.exec();
    }

    bool aucunPlat = true;

    while (query.next()) {
        aucunPlat = false;

        QString imagePath   = query.value(1).toString();
        QString nom         = query.value(2).toString();
        QString description = query.value(3).toString();
        double  prix        = query.value(4).toDouble();
        bool    disponible  = query.value(5).toBool();

        // ── Carte ──
        QFrame* carte = new QFrame();
        carte->setStyleSheet(
            "QFrame {"
            "  background: #FFFFFF;"
            "  border-radius: 12px;"
            "  border: 1px solid #D8EDDF;"
            "}"
            );
        carte->setFixedHeight(100);

        QHBoxLayout* carteLay = new QHBoxLayout(carte);
        carteLay->setContentsMargins(12, 10, 16, 10);
        carteLay->setSpacing(16);

        // Image du plat
        QLabel* lblImg = new QLabel();
        lblImg->setFixedSize(75, 75);
        lblImg->setAlignment(Qt::AlignCenter);
        lblImg->setStyleSheet("border-radius: 8px; background: #F0FAF3; border: none;");
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            QPixmap pm(imagePath);
            lblImg->setPixmap(pm.scaled(75, 75, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            lblImg->setText("🍽️");
            lblImg->setStyleSheet("font-size: 28px; border: none; background: #F0FAF3; border-radius: 8px;");
        }
        carteLay->addWidget(lblImg);

        // Infos texte
        QVBoxLayout* infoLay = new QVBoxLayout();
        infoLay->setSpacing(3);

        QLabel* lblNom = new QLabel(nom);
        lblNom->setStyleSheet("font-size: 15px; font-weight: bold; color: #1B4332; border: none; background: transparent;");

        QLabel* lblDesc = new QLabel(description.isEmpty() ? " " : description);
        lblDesc->setStyleSheet("font-size: 12px; color: #74C69D; border: none; background: transparent;");

        QLabel* lblPrix = new QLabel(QString("%1 DH").arg(QString::number(prix, 'f', 2)));
        lblPrix->setStyleSheet("font-size: 14px; font-weight: bold; color: #2D6A4F; border: none; background: transparent;");

        infoLay->addWidget(lblNom);
        infoLay->addWidget(lblDesc);
        infoLay->addWidget(lblPrix);
        carteLay->addLayout(infoLay);
        carteLay->addStretch();

        // Badge disponibilité
        QLabel* lblDispo = new QLabel(disponible ? " Disponible" : " Indisponible");
        lblDispo->setStyleSheet(
            disponible
                ? "color: #2D6A4F; font-size: 12px; font-weight: bold; border: none; background: transparent;"
                : "color: #E63946; font-size: 12px; font-weight: bold; border: none; background: transparent;"
            );
        carteLay->addWidget(lblDispo);

        layout->addWidget(carte);
    }

    if (aucunPlat) {
        QLabel* lblVide = new QLabel("Aucun plat dans cette catégorie.");
        lblVide->setAlignment(Qt::AlignCenter);
        lblVide->setStyleSheet("color: #74C69D; font-size: 14px;");
        layout->addWidget(lblVide);
    }

    // Spacer en bas
    QSpacerItem* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    layout->addItem(spacer);
}

void Interface_Restaurant::filtrerParCategorie(int categorieId)
{
    afficherCartesPlats(categorieId);
}

QString Interface_Restaurant::copierImage(const QString& cheminSource, const QString& sousRepertoire)
{
    if (cheminSource.isEmpty()) return "";
    QString dossier = QString("images/%1").arg(sousRepertoire);
    QDir().mkpath(dossier);
    QString nomFichier  = QFileInfo(cheminSource).fileName();
    QString destination = QString("%1/%2").arg(dossier, nomFichier);
    if (!QFile::exists(destination)) {
        if (!QFile::copy(cheminSource, destination)) return "";
    }
    return destination;
}

void Interface_Restaurant::afficherImageDansLabel(const QString& chemin, QLabel* label,
                                                  int largeur, int hauteur)
{
    if (chemin.isEmpty() || !QFile::exists(chemin)) {
        label->setText("Aucune image"); label->setAlignment(Qt::AlignCenter); return;
    }
    QPixmap image(chemin);
    if (image.isNull()) { label->setText("Image invalide"); return; }
    label->setPixmap(image.scaled(largeur, hauteur, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void Interface_Restaurant::viderFormulairePlat()
{
    ui->lineEditNomPlat->clear();
    ui->lineEditPrixPlat->clear();
    ui->lineEditDescriptionPlat->clear();
    ui->checkBoxDisponible->setChecked(true);
    ui->labelImagePlat->setText("Aucune image");
    ui->labelImagePlat->setAlignment(Qt::AlignCenter);
    m_cheminImagePlatSelectionne = "";
    ui->tableauPlats->clearSelection();
}

void Interface_Restaurant::viderFormulaireCategorie()
{
    ui->lineEditNomCategorie->clear();
    ui->labelImageCategorie->setText("Aucune image");
    ui->labelImageCategorie->setAlignment(Qt::AlignCenter);
    m_cheminImageCategorieSelectionnee = "";
    ui->tableauCategories->clearSelection();
}

void Interface_Restaurant::chargerCategoriesDansCombo()
{
    ui->comboCategorie->clear();
    QSqlQuery query;
    query.exec("SELECT id, nom FROM categories ORDER BY nom");
    while (query.next()) {
        ui->comboCategorie->addItem(query.value(1).toString(), query.value(0).toInt());
    }
}

QString Interface_Restaurant::prochainStatut(const QString& statutActuel)
{
    if (statutActuel == "en_attente")     return "confirmee";
    if (statutActuel == "confirmee")      return "en_preparation";
    if (statutActuel == "en_preparation") return "en_livraison";
    if (statutActuel == "en_livraison")   return "livree";
    return "";
}

void Interface_Restaurant::chargerPlats()
{
    ui->tableauPlats->setColumnCount(7);
    ui->tableauPlats->setHorizontalHeaderLabels(
        {"ID", "Image", "Nom", "Catégorie", "Prix (DH)", "Description", "Disponible"});
    ui->tableauPlats->setColumnHidden(0, true);
    // Colonnes avec largeurs adaptées au contenu
    QHeaderView* hv = ui->tableauPlats->horizontalHeader();
    hv->setSectionResizeMode(QHeaderView::Interactive);
    hv->setStretchLastSection(false);
    ui->tableauPlats->setColumnWidth(1, 90);   // Image
    ui->tableauPlats->setColumnWidth(2, 160);  // Nom
    ui->tableauPlats->setColumnWidth(3, 120);  // Catégorie
    ui->tableauPlats->setColumnWidth(4, 90);   // Prix
    ui->tableauPlats->setColumnWidth(5, 200);  // Description — s'étire
    ui->tableauPlats->setColumnWidth(6, 90);   // Disponible
    hv->setSectionResizeMode(5, QHeaderView::Stretch); // Description prend le reste
    ui->tableauPlats->verticalHeader()->setDefaultSectionSize(85);
    ui->tableauPlats->verticalHeader()->setVisible(false);
    ui->tableauPlats->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableauPlats->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QSqlQuery query;
    query.exec(
        "SELECT p.id, p.image_path, p.nom, c.nom, p.prix, p.description, p.disponible "
        "FROM plats p LEFT JOIN categories c ON p.categorie_id = c.id ORDER BY c.nom, p.nom");

    ui->tableauPlats->setRowCount(0);
    while (query.next()) {
        int ligne = ui->tableauPlats->rowCount();
        ui->tableauPlats->insertRow(ligne);

        ui->tableauPlats->setItem(ligne, 0, new QTableWidgetItem(QString::number(query.value(0).toInt())));

        QLabel* labelImg = new QLabel();
        labelImg->setAlignment(Qt::AlignCenter);
        QString imagePath = query.value(1).toString();
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            QPixmap pm(imagePath);
            labelImg->setPixmap(pm.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            labelImg->setText("📷");
        }
        ui->tableauPlats->setCellWidget(ligne, 1, labelImg);

        ui->tableauPlats->setItem(ligne, 2, new QTableWidgetItem(query.value(2).toString()));
        ui->tableauPlats->setItem(ligne, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->tableauPlats->setItem(ligne, 4, new QTableWidgetItem(QString::number(query.value(4).toDouble(), 'f', 2)));
        ui->tableauPlats->setItem(ligne, 5, new QTableWidgetItem(query.value(5).toString()));

        bool dispo = query.value(6).toBool();
        QTableWidgetItem* itemDispo = new QTableWidgetItem(dispo ? "✔ Oui" : "✘ Non");
        itemDispo->setTextAlignment(Qt::AlignCenter);
        itemDispo->setForeground(dispo ? QColor("#2D6A4F") : QColor("#E63946"));
        ui->tableauPlats->setItem(ligne, 6, itemDispo);
    }
}

void Interface_Restaurant::on_btnChoisirImagePlat_clicked()
{
    QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir une image pour le plat", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!chemin.isEmpty()) {
        m_cheminImagePlatSelectionne = chemin;
        afficherImageDansLabel(chemin, ui->labelImagePlat);
    }
}

void Interface_Restaurant::on_btnAjouterPlat_clicked()
{
    QString nom         = ui->lineEditNomPlat->text().trimmed();
    QString description = ui->lineEditDescriptionPlat->text().trimmed();
    QString prixStr     = ui->lineEditPrixPlat->text().trimmed();
    int     categorieId = ui->comboCategorie->currentData().toInt();
    bool    disponible  = ui->checkBoxDisponible->isChecked();

    if (nom.isEmpty() || prixStr.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Le nom et le prix sont obligatoires."); return;
    }
    bool ok; double prix = prixStr.toDouble(&ok);
    if (!ok || prix <= 0) {
        QMessageBox::warning(this, "Prix invalide", "Entrez un prix valide supérieur à 0."); return;
    }
    if (categorieId == 0) {
        QMessageBox::warning(this, "Catégorie manquante", "Créez d'abord une catégorie."); return;
    }

    QString cheminBD = copierImage(m_cheminImagePlatSelectionne, "plats");
    QSqlQuery query;
    query.prepare("INSERT INTO plats (categorie_id, nom, description, prix, disponible, image_path) "
                  "VALUES (:cat, :nom, :desc, :prix, :dispo, :img)");
    query.bindValue(":cat",   categorieId);
    query.bindValue(":nom",   nom);
    query.bindValue(":desc",  description);
    query.bindValue(":prix",  prix);
    query.bindValue(":dispo", disponible ? 1 : 0);
    query.bindValue(":img",   cheminBD);

    if (query.exec()) {
        QMessageBox::information(this, "Succès", QString("Plat \"%1\" ajouté.").arg(nom));
        viderFormulairePlat(); chargerPlats();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::on_tableauPlats_itemSelectionChanged()
{
    int ligne = ui->tableauPlats->currentRow();
    if (ligne < 0) return;

    ui->lineEditNomPlat->setText(ui->tableauPlats->item(ligne, 2)->text());
    int indexCombo = ui->comboCategorie->findText(ui->tableauPlats->item(ligne, 3)->text());
    if (indexCombo >= 0) ui->comboCategorie->setCurrentIndex(indexCombo);
    ui->lineEditPrixPlat->setText(ui->tableauPlats->item(ligne, 4)->text());
    ui->lineEditDescriptionPlat->setText(ui->tableauPlats->item(ligne, 5)->text());
    ui->checkBoxDisponible->setChecked(ui->tableauPlats->item(ligne, 6)->text().contains("Oui"));

    int id = ui->tableauPlats->item(ligne, 0)->text().toInt();
    QSqlQuery q;
    q.prepare("SELECT image_path FROM plats WHERE id = :id");
    q.bindValue(":id", id); q.exec();
    if (q.next()) {
        m_cheminImagePlatSelectionne = q.value(0).toString();
        afficherImageDansLabel(m_cheminImagePlatSelectionne, ui->labelImagePlat);
    }
}

void Interface_Restaurant::on_btnModifierPlat_clicked()
{
    int ligne = ui->tableauPlats->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez un plat."); return; }

    int id = ui->tableauPlats->item(ligne, 0)->text().toInt();
    QString nom = ui->lineEditNomPlat->text().trimmed();
    if (nom.isEmpty()) { QMessageBox::warning(this, "Champ manquant", "Le nom est obligatoire."); return; }

    bool ok; double prix = ui->lineEditPrixPlat->text().toDouble(&ok);
    if (!ok || prix <= 0) { QMessageBox::warning(this, "Prix invalide", "Entrez un prix valide."); return; }

    QString cheminBD = m_cheminImagePlatSelectionne;
    if (!cheminBD.isEmpty() && !cheminBD.startsWith("images/"))
        cheminBD = copierImage(m_cheminImagePlatSelectionne, "plats");

    QSqlQuery query;
    query.prepare("UPDATE plats SET nom=:nom, description=:desc, prix=:prix, "
                  "disponible=:dispo, categorie_id=:cat, image_path=:img WHERE id=:id");
    query.bindValue(":nom",   nom);
    query.bindValue(":desc",  ui->lineEditDescriptionPlat->text().trimmed());
    query.bindValue(":prix",  prix);
    query.bindValue(":dispo", ui->checkBoxDisponible->isChecked() ? 1 : 0);
    query.bindValue(":cat",   ui->comboCategorie->currentData().toInt());
    query.bindValue(":img",   cheminBD);
    query.bindValue(":id",    id);

    if (query.exec()) {
        QMessageBox::information(this, "Succès", "Plat modifié.");
        viderFormulairePlat(); chargerPlats();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::on_btnSupprimerPlat_clicked()
{
    int ligne = ui->tableauPlats->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez un plat."); return; }

    QString nomPlat = ui->tableauPlats->item(ligne, 2)->text();
    int id = ui->tableauPlats->item(ligne, 0)->text().toInt();

    auto reponse = QMessageBox::question(this, "Confirmation",
                                         QString("Supprimer \"%1\" ?").arg(nomPlat), QMessageBox::Yes | QMessageBox::No);
    if (reponse != QMessageBox::Yes) return;

    QSqlQuery query;
    query.prepare("DELETE FROM plats WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec()) {
        QMessageBox::information(this, "Supprimé", QString("\"%1\" supprimé.").arg(nomPlat));
        viderFormulairePlat(); chargerPlats();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::on_btnToggleDisponible_clicked()
{
    int ligne = ui->tableauPlats->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez un plat."); return; }

    int id = ui->tableauPlats->item(ligne, 0)->text().toInt();
    QSqlQuery sel;
    sel.prepare("SELECT disponible FROM plats WHERE id = :id");
    sel.bindValue(":id", id); sel.exec(); sel.next();
    int nouvelEtat = 1 - sel.value(0).toInt();

    QSqlQuery upd;
    upd.prepare("UPDATE plats SET disponible = :dispo WHERE id = :id");
    upd.bindValue(":dispo", nouvelEtat); upd.bindValue(":id", id);
    if (upd.exec()) {
        QMessageBox::information(this, "Mise à jour", nouvelEtat ? "Plat activé ✅" : "Plat désactivé ❌");
        chargerPlats();
    } else {
        QMessageBox::critical(this, "Erreur BD", upd.lastError().text());
    }
}


void Interface_Restaurant::chargerCategories()
{
    ui->tableauCategories->setColumnCount(3);
    ui->tableauCategories->setHorizontalHeaderLabels({"ID", "Image", "Nom"});
    ui->tableauCategories->setColumnHidden(0, true);
    ui->tableauCategories->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableauCategories->verticalHeader()->setDefaultSectionSize(80);
    ui->tableauCategories->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableauCategories->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QSqlQuery query;
    query.exec("SELECT id, image_path, nom FROM categories ORDER BY nom");
    ui->tableauCategories->setRowCount(0);

    while (query.next()) {
        int ligne = ui->tableauCategories->rowCount();
        ui->tableauCategories->insertRow(ligne);
        ui->tableauCategories->setItem(ligne, 0, new QTableWidgetItem(QString::number(query.value(0).toInt())));

        QLabel* labelImg = new QLabel();
        labelImg->setAlignment(Qt::AlignCenter);
        QString imgPath = query.value(1).toString();
        if (!imgPath.isEmpty() && QFile::exists(imgPath)) {
            QPixmap pm(imgPath);
            labelImg->setPixmap(pm.scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            labelImg->setText("📁");
        }
        ui->tableauCategories->setCellWidget(ligne, 1, labelImg);
        ui->tableauCategories->setItem(ligne, 2, new QTableWidgetItem(query.value(2).toString()));
    }
}

void Interface_Restaurant::on_btnChoisirImageCategorie_clicked()
{
    QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir une image", "", "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!chemin.isEmpty()) {
        m_cheminImageCategorieSelectionnee = chemin;
        afficherImageDansLabel(chemin, ui->labelImageCategorie);
    }
}

void Interface_Restaurant::on_btnAjouterCategorie_clicked()
{
    QString nom = ui->lineEditNomCategorie->text().trimmed();
    if (nom.isEmpty()) { QMessageBox::warning(this, "Champ manquant", "Entrez le nom."); return; }

    QString cheminBD = copierImage(m_cheminImageCategorieSelectionnee, "categories");
    QSqlQuery query;
    query.prepare("INSERT INTO categories (nom, image_path) VALUES (:nom, :img)");
    query.bindValue(":nom", nom); query.bindValue(":img", cheminBD);
    if (query.exec()) {
        QMessageBox::information(this, "Succès", QString("Catégorie \"%1\" ajoutée.").arg(nom));
        viderFormulaireCategorie(); chargerCategories(); chargerCategoriesDansCombo();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::on_tableauCategories_itemSelectionChanged()
{
    int ligne = ui->tableauCategories->currentRow();
    if (ligne < 0) return;
    ui->lineEditNomCategorie->setText(ui->tableauCategories->item(ligne, 2)->text());
    int id = ui->tableauCategories->item(ligne, 0)->text().toInt();
    QSqlQuery q;
    q.prepare("SELECT image_path FROM categories WHERE id = :id");
    q.bindValue(":id", id); q.exec();
    if (q.next()) {
        m_cheminImageCategorieSelectionnee = q.value(0).toString();
        afficherImageDansLabel(m_cheminImageCategorieSelectionnee, ui->labelImageCategorie);
    }
}

void Interface_Restaurant::on_btnModifierCategorie_clicked()
{
    int ligne = ui->tableauCategories->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez une catégorie."); return; }

    int id = ui->tableauCategories->item(ligne, 0)->text().toInt();
    QString nom = ui->lineEditNomCategorie->text().trimmed();
    if (nom.isEmpty()) { QMessageBox::warning(this, "Champ vide", "Entrez un nom."); return; }

    QString cheminBD = m_cheminImageCategorieSelectionnee;
    if (!cheminBD.isEmpty() && !cheminBD.startsWith("images/"))
        cheminBD = copierImage(m_cheminImageCategorieSelectionnee, "categories");

    QSqlQuery query;
    query.prepare("UPDATE categories SET nom=:nom, image_path=:img WHERE id=:id");
    query.bindValue(":nom", nom); query.bindValue(":img", cheminBD); query.bindValue(":id", id);
    if (query.exec()) {
        QMessageBox::information(this, "Succès", "Catégorie modifiée.");
        viderFormulaireCategorie(); chargerCategories(); chargerCategoriesDansCombo();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::on_btnSupprimerCategorie_clicked()
{
    int ligne = ui->tableauCategories->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez une catégorie."); return; }

    QString nom = ui->tableauCategories->item(ligne, 2)->text();
    int id = ui->tableauCategories->item(ligne, 0)->text().toInt();

    QSqlQuery verif;
    verif.prepare("SELECT COUNT(*) FROM plats WHERE categorie_id = :id");
    verif.bindValue(":id", id); verif.exec(); verif.next();
    if (verif.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Impossible", "Cette catégorie contient des plats. Supprimez-les d'abord."); return;
    }

    auto rep = QMessageBox::question(this, "Confirmation",
                                     QString("Supprimer \"%1\" ?").arg(nom), QMessageBox::Yes | QMessageBox::No);
    if (rep != QMessageBox::Yes) return;

    QSqlQuery query;
    query.prepare("DELETE FROM categories WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec()) {
        QMessageBox::information(this, "Supprimé", "Catégorie supprimée.");
        viderFormulaireCategorie(); chargerCategories(); chargerCategoriesDansCombo();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

void Interface_Restaurant::chargerCommandes()
{
    ui->tableauCommandes->setColumnCount(7);
    ui->tableauCommandes->setHorizontalHeaderLabels(
        {"ID", "Client", "Livreur", "Statut", "Date", "Total (DH)", "Prochain statut"});
    ui->tableauCommandes->setColumnHidden(0, true);
    ui->tableauCommandes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableauCommandes->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableauCommandes->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QSqlQuery query;
    query.exec(
        "SELECT c.id, uc.prenom || ' ' || uc.nom, "
        "COALESCE(ul.prenom || ' ' || ul.nom, 'Non assigné'), "
        "c.statut, c.date_heure, c.total "
        "FROM commandes c "
        "JOIN utilisateurs uc ON c.client_id = uc.id "
        "LEFT JOIN livreurs l ON c.livreur_id = l.id "
        "LEFT JOIN utilisateurs ul ON l.id = ul.id "
        "ORDER BY c.date_heure DESC");

    QMap<QString, QColor> couleurs = {
        {"en_attente",     QColor("#FFF3CD")},
        {"confirmee",      QColor("#D1ECF1")},
        {"en_preparation", QColor("#CCE5FF")},
        {"en_livraison",   QColor("#D4EDDA")},
        {"livree",         QColor("#C3E6CB")},
        {"annulee",        QColor("#F8D7DA")}
    };

    ui->tableauCommandes->setRowCount(0);
    while (query.next()) {
        int ligne = ui->tableauCommandes->rowCount();
        ui->tableauCommandes->insertRow(ligne);
        QString statut = query.value(3).toString();
        QColor couleur = couleurs.value(statut, Qt::white);

        auto setItem = [&](int col, const QString& texte) {
            QTableWidgetItem* item = new QTableWidgetItem(texte);
            item->setBackground(couleur);
            ui->tableauCommandes->setItem(ligne, col, item);
        };
        setItem(0, QString::number(query.value(0).toInt()));
        setItem(1, query.value(1).toString());
        setItem(2, query.value(2).toString());
        setItem(3, statut);
        setItem(4, query.value(4).toString());
        setItem(5, QString::number(query.value(5).toDouble(), 'f', 2));
        QString suivant = prochainStatut(statut);
        setItem(6, suivant.isEmpty() ? "—" : "→ " + suivant);
    }
}

void Interface_Restaurant::on_btnChangerStatut_clicked()
{
    int ligne = ui->tableauCommandes->currentRow();
    if (ligne < 0) { QMessageBox::warning(this, "Aucune sélection", "Sélectionnez une commande."); return; }

    int id = ui->tableauCommandes->item(ligne, 0)->text().toInt();
    QString statutActuel = ui->tableauCommandes->item(ligne, 3)->text();
    QString nouveauStatut = prochainStatut(statutActuel);

    if (nouveauStatut.isEmpty()) {
        QMessageBox::information(this, "Statut final", "Commande déjà au statut final."); return;
    }

    auto rep = QMessageBox::question(this, "Changer le statut",
                                     QString("Passer la commande #%1 de \"%2\" -> \"%3\" ?")
                                         .arg(id).arg(statutActuel).arg(nouveauStatut),
                                     QMessageBox::Yes | QMessageBox::No);
    if (rep != QMessageBox::Yes) return;

    QSqlQuery query;
    query.prepare("UPDATE commandes SET statut = :statut WHERE id = :id");
    query.bindValue(":statut", nouveauStatut); query.bindValue(":id", id);
    if (query.exec()) {
        QMessageBox::information(this, "Mis a jour", QString("Statut -> %1").arg(nouveauStatut));
        chargerCommandes();
    } else {
        QMessageBox::critical(this, "Erreur BD", query.lastError().text());
    }
}

// Helper interne : crée une carte de KPI stylisée
static QFrame* creerCarteKPI(const QString& titre, const QString& valeur,
                             const QString& couleurBg, const QString& couleurTexte,
                             const QString& couleurTitre = "#FFFFFF")
{
    QFrame* carte = new QFrame();
    carte->setStyleSheet(QString(
                             "QFrame { background: %1; border-radius: 12px; border: none; }"
                             ).arg(couleurBg));
    carte->setMinimumHeight(90);

    QVBoxLayout* lay = new QVBoxLayout(carte);
    lay->setContentsMargins(18, 12, 18, 12);
    lay->setSpacing(4);

    QLabel* lblTitre = new QLabel(titre);
    lblTitre->setStyleSheet(QString(
                                "font-size: 12px; font-weight: bold; color: %1; background: transparent; border: none;"
                                ).arg(couleurTitre));
    lblTitre->setWordWrap(true);

    QLabel* lblVal = new QLabel(valeur);
    lblVal->setStyleSheet(QString(
                              "font-size: 22px; font-weight: bold; color: %1; background: transparent; border: none;"
                              ).arg(couleurTexte));

    lay->addWidget(lblTitre);
    lay->addWidget(lblVal);
    return carte;
}

// Helper interne : crée une ligne de tableau de classement
static QFrame* creerLigneClassement(int rang, const QString& nom, const QString& valeur)
{
    QFrame* ligne = new QFrame();
    ligne->setStyleSheet(
        "QFrame { background: #FFFFFF; border-radius: 8px; border: 1px solid #D8EDDF; }"
        );
    ligne->setFixedHeight(44);

    QHBoxLayout* lay = new QHBoxLayout(ligne);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(10);

    QLabel* lblRang = new QLabel(QString("#%1").arg(rang));
    lblRang->setFixedWidth(28);
    lblRang->setStyleSheet("font-size: 13px; font-weight: bold; color: #40916C; background: transparent; border: none;");

    QLabel* lblNom = new QLabel(nom);
    lblNom->setStyleSheet("font-size: 13px; color: #1B4332; background: transparent; border: none;");

    QLabel* lblVal = new QLabel(valeur);
    lblVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblVal->setStyleSheet("font-size: 13px; font-weight: bold; color: #2D6A4F; background: transparent; border: none;");

    lay->addWidget(lblRang);
    lay->addWidget(lblNom, 1);
    lay->addWidget(lblVal);
    return ligne;
}

void Interface_Restaurant::chargerStatistiques()
{
    // Vider le layout existant
    QLayout* rootLayout = ui->scrollWidgetStats->layout();
    QLayoutItem* item;
    while ((item = rootLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // ── Requêtes SQL ──────────────────────────────────────────

    // Chiffre d'affaires total (commandes livrees uniquement)
    QSqlQuery qCA;
    qCA.exec("SELECT COALESCE(SUM(total), 0) FROM commandes WHERE statut = 'livree'");
    double caTotalLivrees = qCA.next() ? qCA.value(0).toDouble() : 0.0;

    // Chiffre d'affaires toutes commandes non annulees
    QSqlQuery qCAAll;
    qCAAll.exec("SELECT COALESCE(SUM(total), 0) FROM commandes WHERE statut != 'annulee'");
    double caTotal = qCAAll.next() ? qCAAll.value(0).toDouble() : 0.0;

    // Nombre total de commandes
    QSqlQuery qNbCmd;
    qNbCmd.exec("SELECT COUNT(*) FROM commandes");
    int nbCommandes = qNbCmd.next() ? qNbCmd.value(0).toInt() : 0;

    // Commandes livrees
    QSqlQuery qLivrees;
    qLivrees.exec("SELECT COUNT(*) FROM commandes WHERE statut = 'livree'");
    int nbLivrees = qLivrees.next() ? qLivrees.value(0).toInt() : 0;

    // Commandes annulees
    QSqlQuery qAnnulees;
    qAnnulees.exec("SELECT COUNT(*) FROM commandes WHERE statut = 'annulee'");
    int nbAnnulees = qAnnulees.next() ? qAnnulees.value(0).toInt() : 0;

    // Commandes en cours (ni livrees ni annulees)
    QSqlQuery qEnCours;
    qEnCours.exec("SELECT COUNT(*) FROM commandes WHERE statut NOT IN ('livree','annulee')");
    int nbEnCours = qEnCours.next() ? qEnCours.value(0).toInt() : 0;

    // Nombre de plats actifs
    QSqlQuery qPlats;
    qPlats.exec("SELECT COUNT(*) FROM plats WHERE disponible = 1");
    int nbPlatsActifs = qPlats.next() ? qPlats.value(0).toInt() : 0;

    // Nombre de categories
    QSqlQuery qCats;
    qCats.exec("SELECT COUNT(*) FROM categories");
    int nbCategories = qCats.next() ? qCats.value(0).toInt() : 0;

    // Ticket moyen (commandes livrees)
    double ticketMoyen = (nbLivrees > 0) ? (caTotalLivrees / nbLivrees) : 0.0;

    // Taux de livraison
    int tauxLivraison = (nbCommandes > 0) ? qRound((double)nbLivrees / nbCommandes * 100.0) : 0;

    // ── Section : Indicateurs clés ────────────────────────────

    QLabel* lblKPI = new QLabel("Indicateurs cles");
    lblKPI->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F;");
    rootLayout->addWidget(lblKPI);

    QGridLayout* gridKPI = new QGridLayout();
    gridKPI->setSpacing(10);

    gridKPI->addWidget(creerCarteKPI("Chiffre d'affaires (livrees)",
                                     QString("%1 DH").arg(QString::number(caTotalLivrees, 'f', 2)),
                                     "#2D6A4F", "#FFFFFF"), 0, 0);

    gridKPI->addWidget(creerCarteKPI("Chiffre d'affaires (hors annulees)",
                                     QString("%1 DH").arg(QString::number(caTotal, 'f', 2)),
                                     "#40916C", "#FFFFFF"), 0, 1);

    gridKPI->addWidget(creerCarteKPI("Ticket moyen",
                                     QString("%1 DH").arg(QString::number(ticketMoyen, 'f', 2)),
                                     "#52B788", "#FFFFFF"), 0, 2);

    gridKPI->addWidget(creerCarteKPI("Commandes totales",
                                     QString::number(nbCommandes),
                                     "#1B4332", "#FFFFFF"), 1, 0);

    gridKPI->addWidget(creerCarteKPI("Commandes livrees",
                                     QString::number(nbLivrees),
                                     "#74C69D", "#1B4332"), 1, 1);

    gridKPI->addWidget(creerCarteKPI("Commandes en cours",
                                     QString::number(nbEnCours),
                                     "#B7E4C7", "#1B4332"), 1, 2);

    gridKPI->addWidget(creerCarteKPI("Commandes annulees",
                                     QString::number(nbAnnulees),
                                     "#E63946", "#FFFFFF"), 2, 0);

    gridKPI->addWidget(creerCarteKPI("Taux de livraison",
                                     QString("%1 %").arg(tauxLivraison),
                                     "#D8F3DC", "#2D6A4F", "#1B4332"), 2, 1);

    gridKPI->addWidget(creerCarteKPI("Plats disponibles / Categories",
                                     QString("%1 / %2").arg(nbPlatsActifs).arg(nbCategories),
                                     "#F0FAF3", "#2D6A4F", "#1B4332"), 2, 2);

    QWidget* wrapKPI = new QWidget();
    wrapKPI->setStyleSheet("background: transparent;");
    wrapKPI->setLayout(gridKPI);
    rootLayout->addWidget(wrapKPI);

    QLabel* lblTaux = new QLabel(QString("Taux de livraison : %1 %").arg(tauxLivraison));
    lblTaux->setStyleSheet("font-size: 13px; color: #1B4332;");
    rootLayout->addWidget(lblTaux);

    QProgressBar* progressLivraison = new QProgressBar();
    progressLivraison->setRange(0, 100);
    progressLivraison->setValue(tauxLivraison);
    progressLivraison->setFixedHeight(18);
    progressLivraison->setStyleSheet(
        "QProgressBar { border-radius: 9px; background: #D8EDDF; border: none; text-align: center; font-size: 12px; color: #1B4332; }"
        "QProgressBar::chunk { border-radius: 9px; background: #2D6A4F; }"
        );
    rootLayout->addWidget(progressLivraison);

    // ── Section : Repartition par statut ─────────────────────

    QLabel* lblStatuts = new QLabel("Repartition des commandes par statut");
    lblStatuts->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F; margin-top: 6px;");
    rootLayout->addWidget(lblStatuts);

    QSqlQuery qStatuts;
    qStatuts.exec("SELECT statut, COUNT(*) as nb FROM commandes GROUP BY statut ORDER BY nb DESC");

    QMap<QString, QColor> couleurs = {
        {"en_attente",     QColor("#FFF3CD")},
        {"confirmee",      QColor("#D1ECF1")},
        {"en_preparation", QColor("#CCE5FF")},
        {"en_livraison",   QColor("#D4EDDA")},
        {"livree",         QColor("#C3E6CB")},
        {"annulee",        QColor("#F8D7DA")}
    };

    while (qStatuts.next()) {
        QString statut = qStatuts.value(0).toString();
        int     nb     = qStatuts.value(1).toInt();
        int     pct    = (nbCommandes > 0) ? qRound((double)nb / nbCommandes * 100.0) : 0;

        QFrame* rowStatut = new QFrame();
        rowStatut->setStyleSheet(
            QString("QFrame { background: %1; border-radius: 8px; border: none; }").arg(couleurs.value(statut, QColor("#F4F6F0")).name())
            );
        rowStatut->setFixedHeight(38);

        QHBoxLayout* rowLay = new QHBoxLayout(rowStatut);
        rowLay->setContentsMargins(12, 4, 12, 4);
        rowLay->setSpacing(10);

        QLabel* lblStatut = new QLabel(statut.replace("_", " "));
        lblStatut->setFixedWidth(150);
        lblStatut->setStyleSheet("font-size: 13px; font-weight: bold; color: #1B4332; background: transparent; border: none;");

        QProgressBar* bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setValue(pct);
        bar->setFixedHeight(14);
        bar->setStyleSheet(
            "QProgressBar { border-radius: 7px; background: rgba(255,255,255,0.5); border: none; }"
            "QProgressBar::chunk { border-radius: 7px; background: #2D6A4F; }"
            );

        QLabel* lblNb = new QLabel(QString("%1 (%2 %)").arg(nb).arg(pct));
        lblNb->setFixedWidth(80);
        lblNb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblNb->setStyleSheet("font-size: 12px; font-weight: bold; color: #2D6A4F; background: transparent; border: none;");

        rowLay->addWidget(lblStatut);
        rowLay->addWidget(bar, 1);
        rowLay->addWidget(lblNb);
        rootLayout->addWidget(rowStatut);
    }

    // ── Section : Top 5 plats les plus commandes ─────────────

    QLabel* lblTop = new QLabel("Top 5 des plats commandés");
    lblTop->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F; margin-top: 6px;");
    rootLayout->addWidget(lblTop);

    QSqlQuery qTop;
    qTop.exec(
        "SELECT p.nom, COUNT(dc.plat_id) AS nb_fois, SUM(dc.prix_unitaire * dc.quantite) AS gain "
        "FROM details_commande dc "
        "JOIN plats p ON dc.plat_id = p.id "
        "JOIN commandes c ON dc.commande_id = c.id "
        "WHERE c.statut = 'livree' "
        "GROUP BY dc.plat_id, p.nom "
        "ORDER BY nb_fois DESC "
        "LIMIT 5"
        );

    int rang = 1;
    bool aucun = true;
    while (qTop.next()) {
        aucun = false;
        QString nomPlat = qTop.value(0).toString();
        int     nbFois  = qTop.value(1).toInt();
        double  gain    = qTop.value(2).toDouble();
        rootLayout->addWidget(creerLigneClassement(
            rang++,
            nomPlat,
            QString("%1x — %2 DH").arg(nbFois).arg(QString::number(gain, 'f', 2))
            ));
    }
    if (aucun) {
        QLabel* lblAucun = new QLabel("Aucune commande livrée enregistrée.");
        lblAucun->setAlignment(Qt::AlignCenter);
        lblAucun->setStyleSheet("color: #74C69D; font-size: 13px;");
        rootLayout->addWidget(lblAucun);
    }

    // ── Section : Top 5 clients ───────────────────────────────

    QLabel* lblTopClients = new QLabel("Top 5 des meilleurs clients");
    lblTopClients->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F; margin-top: 6px;");
    rootLayout->addWidget(lblTopClients);

    QSqlQuery qClients;
    qClients.exec(
        "SELECT u.prenom || ' ' || u.nom, COUNT(c.id) AS nb, COALESCE(SUM(c.total), 0) AS total "
        "FROM commandes c "
        "JOIN utilisateurs u ON c.client_id = u.id "
        "WHERE c.statut = 'livree' "
        "GROUP BY c.client_id "
        "ORDER BY total DESC "
        "LIMIT 5"
        );

    rang = 1;
    aucun = true;
    while (qClients.next()) {
        aucun = false;
        rootLayout->addWidget(creerLigneClassement(
            rang++,
            qClients.value(0).toString(),
            QString("%1 cmd — %2 DH").arg(qClients.value(1).toInt()).arg(QString::number(qClients.value(2).toDouble(), 'f', 2))
            ));
    }
    if (aucun) {
        QLabel* lblAucun = new QLabel("Aucun client avec des commandes livrées.");
        lblAucun->setAlignment(Qt::AlignCenter);
        lblAucun->setStyleSheet("color: #74C69D; font-size: 13px;");
        rootLayout->addWidget(lblAucun);
    }

    // ── Section : Gains par categorie ────────────────────────

    QLabel* lblCatGains = new QLabel("Gains par categorie de plat");
    lblCatGains->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D6A4F; margin-top: 6px;");
    rootLayout->addWidget(lblCatGains);

    QSqlQuery qCatGains;
    qCatGains.exec(
        "SELECT cat.nom, COALESCE(SUM(dc.prix_unitaire * dc.quantite), 0) AS gain "
        "FROM categories cat "
        "LEFT JOIN plats p ON p.categorie_id = cat.id "
        "LEFT JOIN details_commande dc ON dc.plat_id = p.id "
        "LEFT JOIN commandes c ON dc.commande_id = c.id AND c.statut = 'livree' "
        "GROUP BY cat.id, cat.nom "
        "ORDER BY gain DESC"
        );

    rang = 1;
    while (qCatGains.next()) {
        rootLayout->addWidget(creerLigneClassement(
            rang++,
            qCatGains.value(0).toString(),
            QString("%1 DH").arg(QString::number(qCatGains.value(1).toDouble(), 'f', 2))
            ));
    }

    // ── Section : Graphe évolution mensuelle (mois en cours) ─────────────────

    // Déterminer l'année courante
    QSqlQuery qAnnee;
    qAnnee.exec("SELECT strftime('%Y', date_commande) AS an "
                "FROM commandes ORDER BY date_commande DESC LIMIT 1");
    QString annee = qAnnee.next() ? qAnnee.value(0).toString()
                                  : QString::number(QDate::currentDate().year());

    QLabel* lblGraphe = new QLabel(
        QString("Evolution mensuelle des commandes — %1").arg(annee));
    lblGraphe->setStyleSheet(
        "font-size: 15px; font-weight: bold; color: #2D6A4F; margin-top: 10px;");
    rootLayout->addWidget(lblGraphe);

    // Récupérer les données mois par mois pour l'année choisie
    // Série 1 : nb commandes totales  |  Série 2 : nb commandes livrées  |  Série 3 : CA livré
    QVector<double> vbCommandes(12, 0), vbLivrees(12, 0), vbCA(12, 0);

    QSqlQuery qMois;
    qMois.prepare(
        "SELECT strftime('%m', date_commande) AS mois, "
        "       COUNT(*) AS nb_total, "
        "       SUM(CASE WHEN statut = 'livree' THEN 1 ELSE 0 END) AS nb_livrees, "
        "       SUM(CASE WHEN statut = 'livree' THEN total ELSE 0 END) AS ca "
        "FROM commandes "
        "WHERE strftime('%Y', date_commande) = :an "
        "GROUP BY mois "
        "ORDER BY mois");
    qMois.bindValue(":an", annee);
    qMois.exec();
    while (qMois.next()) {
        int idx = qMois.value(0).toInt() - 1; // 0 = janvier
        if (idx >= 0 && idx < 12) {
            vbCommandes[idx] = qMois.value(1).toDouble();
            vbLivrees[idx]   = qMois.value(2).toDouble();
            vbCA[idx]        = qMois.value(3).toDouble();
        }
    }

    QStringList moisLabels = {"Jan","Fev","Mar","Avr","Mai","Jun",
                              "Jul","Aou","Sep","Oct","Nov","Dec"};

    GrapheMensuel* graphe = new GrapheMensuel();
    graphe->setFixedHeight(240);
    graphe->setMois(moisLabels);

    GrapheMensuel::Serie sTotal;
    sTotal.label   = "Commandes totales";
    sTotal.couleur = QColor("#2D6A4F");
    sTotal.valeurs = vbCommandes;
    graphe->ajouterSerie(sTotal);

    GrapheMensuel::Serie sLivrees;
    sLivrees.label   = "Livrees";
    sLivrees.couleur = QColor("#52B788");
    sLivrees.valeurs = vbLivrees;
    graphe->ajouterSerie(sLivrees);

    // ── Séparateur + conteneur blanc arrondi ─────────────────────────────────
    QFrame* conteneurGraphe = new QFrame();
    conteneurGraphe->setStyleSheet(
        "QFrame { background: #FFFFFF; border-radius: 12px; border: 1px solid #D8EDDF; }");
    QVBoxLayout* conteneurLay = new QVBoxLayout(conteneurGraphe);
    conteneurLay->setContentsMargins(12, 12, 12, 12);
    conteneurLay->addWidget(graphe);

    // Graphe CA mensuel (barres simulées via ProgressBar)
    QLabel* lblCAMois = new QLabel("Chiffre d'affaires mensuel (commandes livrees, DH)");
    lblCAMois->setStyleSheet(
        "font-size: 13px; font-weight: bold; color: #2D6A4F; margin-top: 8px; background: transparent; border: none;");
    conteneurLay->addWidget(lblCAMois);

    double maxCA = *std::max_element(vbCA.begin(), vbCA.end());
    if (maxCA < 1.0) maxCA = 1.0;
    for (int i = 0; i < 12; ++i) {
        QFrame* rowCA = new QFrame();
        rowCA->setStyleSheet("QFrame { background: transparent; border: none; }");
        rowCA->setFixedHeight(26);
        QHBoxLayout* rowLay = new QHBoxLayout(rowCA);
        rowLay->setContentsMargins(0, 2, 0, 2);
        rowLay->setSpacing(8);

        QLabel* lblM = new QLabel(moisLabels[i]);
        lblM->setFixedWidth(30);
        lblM->setStyleSheet("font-size: 11px; font-weight: bold; color: #1B4332; background: transparent; border: none;");

        QProgressBar* barCA = new QProgressBar();
        barCA->setRange(0, 1000);
        barCA->setValue(qRound(vbCA[i] / maxCA * 1000.0));
        barCA->setFixedHeight(12);
        barCA->setStyleSheet(
            "QProgressBar { border-radius: 6px; background: #D8F3DC; border: none; }"
            "QProgressBar::chunk { border-radius: 6px; background: #40916C; }");

        QLabel* lblV = new QLabel(vbCA[i] > 0
                                      ? QString("%1").arg(QString::number(vbCA[i], 'f', 0))
                                      : "-");
        lblV->setFixedWidth(72);
        lblV->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lblV->setStyleSheet("font-size: 11px; color: #2D6A4F; font-weight: bold; background: transparent; border: none;");

        rowLay->addWidget(lblM);
        rowLay->addWidget(barCA, 1);
        rowLay->addWidget(lblV);
        conteneurLay->addWidget(rowCA);
    }

    rootLayout->addWidget(conteneurGraphe);

    // Spacer final
    QSpacerItem* spacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    rootLayout->addItem(spacer);
}
// ─────────────────────────────────────────────────────────────────────────────
//  PLANNING & HORAIRES
// ─────────────────────────────────────────────────────────────────────────────

void Interface_Restaurant::afficherPagePlanning()
{
    ui->stackedWidget->setCurrentIndex(5);
    chargerHoraires();
}

void Interface_Restaurant::chargerHoraires()
{
    // Vider le layout et la liste interne
    QLayout* layout = ui->planningContentLayout;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_lignesHoraires.clear();

    // Noms localisés
    static const char* JOURS[]    = {"Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi","Dimanche"};
    static const char* SERVICES[] = {"petit_dej","dejeuner","diner"};
    static const char* LABELS[]   = {" Petit-déjeuner","  Déjeuner","  Dîner"};
    static const QColor COULEURS[] = {QColor("#FFF3CD"), QColor("#D4EDDA"), QColor("#CCE5FF")};

    // Charger tous les horaires en mémoire
    QMap<QString, QVariantMap> db_map; // clé = "jour_service"
    QSqlQuery q;
    q.exec("SELECT id, jour, service, ouvert, heure_debut, heure_fin FROM horaires ORDER BY jour, service");
    while (q.next()) {
        QString key = QString("%1_%2").arg(q.value(1).toInt()).arg(q.value(2).toString());
        QVariantMap m;
        m["id"]     = q.value(0);
        m["ouvert"] = q.value(3);
        m["debut"]  = q.value(4);
        m["fin"]    = q.value(5);
        db_map[key] = m;
    }

    for (int jour = 0; jour < 7; ++jour) {
        // Carte du jour
        QFrame* carteJour = new QFrame();
        carteJour->setStyleSheet(
            "QFrame{background:#FFFFFF;border-radius:12px;border:1px solid #D8EDDF;}"
            "QLabel{border:none;background:transparent;}"
            "QCheckBox{border:none;background:transparent;}");
        QVBoxLayout* vJour = new QVBoxLayout(carteJour);
        vJour->setContentsMargins(16, 12, 16, 12);
        vJour->setSpacing(8);

        // Titre du jour
        QLabel* lblJour = new QLabel(QString("  %1").arg(JOURS[jour]));
        lblJour->setStyleSheet(
            "font-size:15px;font-weight:bold;color:#1B4332;padding-bottom:4px;border:none;background:transparent;");
        vJour->addWidget(lblJour);

        // Séparateur
        QFrame* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("background:#D8EDDF;max-height:1px;border:none;");
        vJour->addWidget(sep);

        for (int s = 0; s < 3; ++s) {
            QString key = QString("%1_%2").arg(jour).arg(SERVICES[s]);
            QVariantMap data = db_map.value(key);

            // Ligne de service
            QFrame* ligne = new QFrame();
            ligne->setStyleSheet(
                QString("QFrame{background:%1;border-radius:8px;border:none;}"
                        "QLabel{border:none;background:transparent;}"
                        "QCheckBox{border:none;background:transparent;}"
                        "QLineEdit{border:1.5px solid #B7E4C7;border-radius:5px;"
                        "         background:white;padding:4px 8px;font-size:12px;}"
                        ).arg(COULEURS[s].name()));
            QHBoxLayout* hLine = new QHBoxLayout(ligne);
            hLine->setContentsMargins(12, 8, 12, 8);
            hLine->setSpacing(12);

            QCheckBox* chk = new QCheckBox(LABELS[s]);
            chk->setStyleSheet("font-size:13px;font-weight:bold;color:#1B4332;");
            chk->setChecked(data.value("ouvert", true).toBool());
            chk->setFixedWidth(190);

            QLabel* lblDe = new QLabel("De");
            lblDe->setStyleSheet("font-size:12px;color:#555;");

            QLineEdit* leDebut = new QLineEdit(data.value("debut", "08:00").toString());
            leDebut->setFixedWidth(70);
            leDebut->setPlaceholderText("HH:MM");

            QLabel* lblA = new QLabel("à");
            lblA->setStyleSheet("font-size:12px;color:#555;");

            QLineEdit* leFin = new QLineEdit(data.value("fin", "22:00").toString());
            leFin->setFixedWidth(70);
            leFin->setPlaceholderText("HH:MM");

            // Activer/désactiver les champs selon l'état
            auto toggleChamps = [leDebut, leFin, lblDe, lblA](bool ouvert) {
                leDebut->setEnabled(ouvert);
                leFin->setEnabled(ouvert);
                lblDe->setEnabled(ouvert);
                lblA->setEnabled(ouvert);
                QString alpha = ouvert ? "1.0" : "0.4";
                leDebut->setStyleSheet(
                    QString("border:1.5px solid #B7E4C7;border-radius:5px;"
                            "background:white;padding:4px 8px;font-size:12px;opacity:%1;").arg(alpha));
            };
            toggleChamps(chk->isChecked());
            connect(chk, &QCheckBox::toggled, this, [toggleChamps](bool v){ toggleChamps(v); });

            hLine->addWidget(chk);
            hLine->addStretch();
            hLine->addWidget(lblDe);
            hLine->addWidget(leDebut);
            hLine->addWidget(lblA);
            hLine->addWidget(leFin);

            vJour->addWidget(ligne);

            LigneHoraire lh;
            lh.id        = data.value("id", -1).toInt();
            lh.jour      = jour;
            lh.service   = SERVICES[s];
            lh.chkOuvert = chk;
            lh.leDebut   = leDebut;
            lh.leFin     = leFin;
            m_lignesHoraires.append(lh);
        }

        layout->addWidget(carteJour);
    }

    // Spacer final
    static_cast<QVBoxLayout*>(layout)->addStretch();
}

void Interface_Restaurant::sauvegarderHoraires()
{
    // Validation simple : format HH:MM
    QRegularExpression reHeure("^([01]\\d|2[0-3]):[0-5]\\d$");
    for (const LigneHoraire& lh : m_lignesHoraires) {
        if (!reHeure.match(lh.leDebut->text()).hasMatch() ||
            !reHeure.match(lh.leFin->text()).hasMatch()) {
            QMessageBox::warning(this, "Format invalide",
                                 QString("Heure invalide pour %1 (utilisez HH:MM).").arg(lh.service));
            return;
        }
        if (lh.leDebut->text() >= lh.leFin->text()) {
            QMessageBox::warning(this, "Horaire invalide",
                                 QString("L'heure de début doit être avant la fin (%1).").arg(lh.service));
            return;
        }
    }

    QSqlQuery q;
    for (const LigneHoraire& lh : m_lignesHoraires) {
        if (lh.id > 0) {
            q.prepare("UPDATE horaires SET ouvert=:o, heure_debut=:d, heure_fin=:f WHERE id=:id");
            q.bindValue(":id", lh.id);
        } else {
            q.prepare("INSERT OR REPLACE INTO horaires (jour,service,ouvert,heure_debut,heure_fin) "
                      "VALUES (:j,:s,:o,:d,:f)");
            q.bindValue(":j", lh.jour);
            q.bindValue(":s", lh.service);
        }
        q.bindValue(":o", lh.chkOuvert->isChecked() ? 1 : 0);
        q.bindValue(":d", lh.leDebut->text());
        q.bindValue(":f", lh.leFin->text());
        if (!q.exec())
            qDebug() << "[Horaires] Erreur save:" << q.lastError().text();
    }

    QMessageBox::information(this, "Succès", "Horaires sauvegardés !");
    chargerHoraires(); // Recharger pour mettre à jour les ids
}

// ─────────────────────────────────────────────────────────────────────────────
//  DÉCONNEXION
// ─────────────────────────────────────────────────────────────────────────────
void Interface_Restaurant::on_btnDeconnexion_clicked()
{
    if (parentWidget()) {
        // Vider les champs login avant de réafficher
        QLineEdit* email = parentWidget()->findChild<QLineEdit*>("lineEditEmail_3");
        QLineEdit* mdp   = parentWidget()->findChild<QLineEdit*>("lineEditMDP_3");
        if (email) email->clear();
        if (mdp)   mdp->clear();

        parentWidget()->show();
    }
    this->close();
}