#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    // Singleton : LanguageManager::instance()
    static LanguageManager& instance();

    // Charge la langue : "fr", "ar", "en"
    void loadLanguage(const QString& langCode);

    // Langue actuellement active
    QString currentLanguage() const { return m_currentLang; }

signals:
    // Émis après chaque changement — tous les widgets s'y connectent
    void languageChanged();

private:
    explicit LanguageManager(QObject* parent = nullptr);
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    QTranslator m_translator;
    QString     m_currentLang;
};

#endif // LANGUAGEMANAGER_H