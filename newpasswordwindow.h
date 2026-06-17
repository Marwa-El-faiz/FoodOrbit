#ifndef NEWPASSWORDWINDOW_H
#define NEWPASSWORDWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class NewPasswordWindow : public QDialog
{
    Q_OBJECT
public:
    explicit NewPasswordWindow(const QString& email, QWidget* parent = nullptr);

private slots:
    void onValider();
    void toggleMdp1();
    void toggleMdp2();

private:
    QString      m_email;
    QLineEdit*   m_mdp1;
    QLineEdit*   m_mdp2;
    QPushButton* m_toggle1;
    QPushButton* m_toggle2;
    bool         m_vis1 = false;
    bool         m_vis2 = false;

    static QString hasher(const QString& mdp);
};

#endif // NEWPASSWORDWINDOW_H