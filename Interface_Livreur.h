#ifndef INTERFACE_LIVREUR_H
#define INTERFACE_LIVREUR_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QProgressBar>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QPixmap>

namespace Ui {
class Interface_Livreur;
}

class Interface_Livreur : public QDialog
{
    Q_OBJECT

public:
    explicit Interface_Livreur(int livreurId, QWidget *parent = nullptr);
    ~Interface_Livreur();

private slots:
    void afficherPageDisponibles();
    void afficherPageMesLivraisons();
    void afficherPageHistorique();
    void afficherPageStatistiques();
    void onLogout();

    void prendreEnCharge(int commandeId);
    void marquerLivree(int commandeId);
    void marquerEchouee(int commandeId);

    void chargerCommandesDisponibles();
    void chargerMesLivraisons();
    void chargerHistorique();
    void chargerStatistiques();

private:
    Ui::Interface_Livreur *ui;
    int m_livreurId;

    void viderLayout(QLayout* layout);
    void updateSidebarStyle(int pageIndex);
    QList<QPair<QString,int>> chargerArticles(int commandeId);

    QWidget* creerCarteCommande(int commandeId,
                                const QString& clientNom,
                                const QString& adresse,
                                const QString& dateHeure,
                                double total,
                                const QString& statut,
                                const QList<QPair<QString,int>>& articles,
                                bool disponible,
                                bool enCours);

    QFrame* creerCarteKPI(const QString& titre, const QString& valeur,
                          const QString& couleurBg, const QString& couleurTexte);
};

#endif // INTERFACE_LIVREUR_H