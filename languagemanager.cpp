#include "languagemanager.h"
#include <QApplication>
#include <QSettings>

LanguageManager& LanguageManager::instance()
{
    static LanguageManager inst;
    return inst;
}

LanguageManager::LanguageManager(QObject* parent)
    : QObject(parent)
{
    // Charger la langue sauvegardée, sinon français par défaut
    QSettings settings("FoodOrbit", "FoodOrbit");
    QString saved = settings.value("language", "fr").toString();
    loadLanguage(saved);
}

void LanguageManager::loadLanguage(const QString& langCode)
{
    if (m_currentLang == langCode)
        return;

    m_currentLang = langCode;

    // Retirer l'ancien translator
    qApp->removeTranslator(&m_translator);

    // Charger le nouveau fichier .qm embarqué dans les ressources
    const QString path = QString(":/translations/app_%1.qm").arg(langCode);
    if (m_translator.load(path)) {
        qApp->installTranslator(&m_translator);
    }

    // Arabe → mise en page droite-à-gauche
    if (langCode == "ar")
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);

    // Sauvegarder le choix
    QSettings settings("FoodOrbit", "FoodOrbit");
    settings.setValue("language", langCode);

    emit languageChanged();
}