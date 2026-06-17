#ifndef INTERFACE_RESTAURANT_H
#define INTERFACE_RESTAURANT_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QFileDialog>
#include <QPixmap>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QColor>
#include <QDebug>
#include <QString>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QProgressBar>
#include <QGridLayout>
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <QFont>
#include <QPen>
#include <QTimer>

// ── Widget graphe en courbes : évolution mensuelle ────────────────────────────
class GrapheMensuel : public QWidget
{
    Q_OBJECT
public:
    struct Serie {
        QString     label;
        QColor      couleur;
        QVector<double> valeurs; // une valeur par mois (index 0 = Jan)
    };

    explicit GrapheMensuel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet("background: transparent;");
    }

    void ajouterSerie(const Serie& s) { m_series.append(s); update(); }
    void setMois(const QStringList& mois) { m_mois = mois; update(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (m_series.isEmpty() || m_mois.isEmpty()) return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int marginL = 50, marginR = 20, marginT = 20, marginB = 50;
        QRect zone(marginL, marginT, width() - marginL - marginR, height() - marginT - marginB);

        // Fond blanc arrondi
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#FFFFFF"));
        p.drawRoundedRect(rect(), 12, 12);

        // Calcul max
        double maxVal = 1.0;
        for (const Serie& s : m_series)
            for (double v : s.valeurs)
                if (v > maxVal) maxVal = v;

        int nbMois = m_mois.size();
        if (nbMois < 2) return;

        // Grille horizontale
        p.setPen(QPen(QColor("#E8F5E9"), 1, Qt::DashLine));
        int nbLignes = 4;
        for (int i = 0; i <= nbLignes; ++i) {
            int y = zone.bottom() - i * zone.height() / nbLignes;
            p.drawLine(zone.left(), y, zone.right(), y);

            // Étiquette axe Y
            p.setPen(QColor("#74C69D"));
            p.setFont(QFont("Segoe UI", 8));
            p.drawText(0, y - 6, marginL - 4, 14,
                       Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(qRound(maxVal * i / nbLignes)));
            p.setPen(QPen(QColor("#E8F5E9"), 1, Qt::DashLine));
        }

        // Étiquettes axe X
        p.setPen(QColor("#1B4332"));
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        double stepX = (double)zone.width() / (nbMois - 1);
        for (int i = 0; i < nbMois; ++i) {
            int x = zone.left() + qRound(i * stepX);
            p.drawText(x - 18, zone.bottom() + 8, 36, 18,
                       Qt::AlignCenter, m_mois[i]);
        }

        // Courbes
        for (const Serie& s : m_series) {
            if (s.valeurs.size() < nbMois) continue;

            // Aire remplie
            QPainterPath aire;
            aire.moveTo(zone.left(), zone.bottom());
            for (int i = 0; i < nbMois; ++i) {
                int x = zone.left() + qRound(i * stepX);
                int y = zone.bottom() - qRound(s.valeurs[i] / maxVal * zone.height());
                if (i == 0) aire.lineTo(x, y); else aire.lineTo(x, y);
            }
            aire.lineTo(zone.left() + qRound((nbMois-1) * stepX), zone.bottom());
            aire.closeSubpath();
            QColor fill = s.couleur; fill.setAlpha(30);
            p.fillPath(aire, fill);

            // Ligne
            QPainterPath ligne;
            for (int i = 0; i < nbMois; ++i) {
                int x = zone.left() + qRound(i * stepX);
                int y = zone.bottom() - qRound(s.valeurs[i] / maxVal * zone.height());
                if (i == 0) ligne.moveTo(x, y); else ligne.lineTo(x, y);
            }
            p.setPen(QPen(s.couleur, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPath(ligne);

            // Points + valeurs
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            for (int i = 0; i < nbMois; ++i) {
                int x = zone.left() + qRound(i * stepX);
                int y = zone.bottom() - qRound(s.valeurs[i] / maxVal * zone.height());
                p.setBrush(s.couleur);
                p.setPen(QPen(Qt::white, 1.5));
                p.drawEllipse(QPoint(x, y), 5, 5);

                // Valeur au-dessus
                p.setPen(s.couleur);
                p.drawText(x - 20, y - 18, 40, 14,
                           Qt::AlignCenter,
                           QString::number(qRound(s.valeurs[i])));
            }
        }

        // Légende en bas
        int lx = marginL;
        int ly = height() - 16;
        p.setFont(QFont("Segoe UI", 9, QFont::Bold));
        for (const Serie& s : m_series) {
            p.setBrush(s.couleur);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(lx, ly - 7, 14, 10, 3, 3);
            p.setPen(QColor("#1B4332"));
            p.drawText(lx + 18, ly - 8, 120, 14, Qt::AlignLeft | Qt::AlignVCenter, s.label);
            lx += 140;
        }
    }

private:
    QVector<Serie> m_series;
    QStringList    m_mois;
};
// ─────────────────────────────────────────────────────────────────────────────

namespace Ui {
class Interface_Restaurant;
}

class Interface_Restaurant : public QDialog
{
    Q_OBJECT

public:
    explicit Interface_Restaurant(QWidget *parent = nullptr);
    ~Interface_Restaurant();

private slots:
    // Navigation Sidebar
    void afficherPageMenu();
    void afficherPagePlats();
    void afficherPageCategories();
    void afficherPageCommandes();

    // Filtrage menu par catégorie
    void filtrerParCategorie(int categorieId);

    // Plats (Page 1)
    void chargerPlats();
    void on_btnAjouterPlat_clicked();
    void on_btnModifierPlat_clicked();
    void on_btnSupprimerPlat_clicked();
    void on_btnToggleDisponible_clicked();
    void on_btnChoisirImagePlat_clicked();
    void on_tableauPlats_itemSelectionChanged();

    // Catégories (Page 2)
    void chargerCategories();
    void on_btnAjouterCategorie_clicked();
    void on_btnModifierCategorie_clicked();
    void on_btnSupprimerCategorie_clicked();
    void on_btnChoisirImageCategorie_clicked();
    void on_tableauCategories_itemSelectionChanged();

    // Commandes (Page 3)
    void chargerCommandes();
    void on_btnChangerStatut_clicked();

    // Statistiques (Page 4)
    void afficherPageStatistiques();
    void chargerStatistiques();

    // Planning & Horaires (Page 5)
    void afficherPagePlanning();
    void chargerHoraires();
    void sauvegarderHoraires();

    // Déconnexion
    void on_btnDeconnexion_clicked();

    // ── Avancement automatique des statuts ──
    void avancerStatutsAuto();

private:
    Ui::Interface_Restaurant *ui;

    QString m_cheminImagePlatSelectionne;
    QString m_cheminImageCategorieSelectionnee;
    int     m_categorieSelectionneeId = -1; // -1 = toutes les catégories

    // Planning — widgets générés dynamiquement (1 ligne par jour×service)
    struct LigneHoraire {
        int        id;          // id en base (-1 si nouveau)
        int        jour;
        QString    service;
        QCheckBox* chkOuvert;
        QLineEdit* leDebut;
        QLineEdit* leFin;
    };
    QList<LigneHoraire> m_lignesHoraires;

    // ── Timer auto-avancement statuts ──
    QTimer* m_timerAuto = nullptr;
    int     m_intervalleSecondes = 30; // avancer toutes les 30s en test

    // Helpers
    QString copierImage(const QString& cheminSource, const QString& sousRepertoire);
    void    afficherImageDansLabel(const QString& chemin, QLabel* label,
                                int largeur = 120, int hauteur = 100);
    void    viderFormulairePlat();
    void    viderFormulaireCategorie();
    void    chargerCategoriesDansCombo();
    void    chargerBoutonsCategoriesMenu();
    void    afficherCartesPlats(int categorieId = -1);
    QString prochainStatut(const QString& statutActuel);
};

#endif // INTERFACE_RESTAURANT_H