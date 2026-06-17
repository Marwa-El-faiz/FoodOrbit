#include "loginwindow.h"
#include "database.h"
#include "style.h"
#include "languagemanager.h"
#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ── Nom & icône de l'application ──
    a.setApplicationName("FoodOrbit");
    a.setApplicationDisplayName("FoodOrbit");
    a.setApplicationVersion("1.0.0");

    // ── Définir le français comme langue par défaut (premier lancement uniquement) ──
    QSettings settings("FoodOrbit", "FoodOrbit");
    if (!settings.contains("language")) {
        settings.setValue("language", "fr");
    }

    // ── Initialiser le gestionnaire de langue (charge la langue sauvegardée) ──
    LanguageManager::instance();

    // ── Style global unifié (QMessageBox, QLineEdit, tables…) ──
    a.setStyleSheet(FO::styleApp());

    // ── Ouvrir la base de données ──
    if (!Database::instance().openDatabase()) {
        qDebug() << "Impossible d'ouvrir la base de données !";
        return -1;
    }

    // ── Fenêtre de Login ──
    LoginWindow loginWindow;
    loginWindow.show();

    return a.exec();
}