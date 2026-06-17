#ifndef STYLE_H
#define STYLE_H

// ═══════════════════════════════════════════════════════════════
//  style.h — FoodOrbit Design System
//  Palette et styles QSS centralisés, réutilisables partout.
//  Usage : #include "style.h"  puis  FO::styleApp()
// ═══════════════════════════════════════════════════════════════

namespace FO {

// ── Couleurs ────────────────────────────────────────────────────
static const char* C_VERT_FONCE  = "#2D6A4F";
static const char* C_VERT_MOYEN  = "#40916C";
static const char* C_VERT_CLAIR  = "#52B788";
static const char* C_VERT_PALE   = "#B7E4C7";
static const char* C_VERT_BG     = "#D8F3DC";
static const char* C_FOND        = "#F4F6F0";
static const char* C_TEXTE_DARK  = "#1B4332";
static const char* C_TEXTE_MID   = "#2D6A4F";
static const char* C_ROUGE       = "#E63946";
static const char* C_ORANGE      = "#F4A261";
static const char* C_BLANC       = "#FFFFFF";

// ── Fenêtre principale ─────────────────────────────────────────
inline QString styleDialog()
{
    return QString(R"(
QDialog, QMainWindow {
    background-color: #F4F6F0;
    font-family: 'Segoe UI', sans-serif;
}
)");
}

// ── Sidebar ────────────────────────────────────────────────────
inline QString styleSidebar()
{
    return QString(R"(
QWidget#sidebar {
    background-color: #2D6A4F;
}
)");
}

inline QString styleBtnNavActif()
{
    return QString(R"(
QPushButton {
    background: #40916C;
    color: white;
    border-radius: 10px;
    font-size: 14px;
    font-weight: bold;
    text-align: left;
    padding: 12px 20px;
    border: none;
}
)");
}

inline QString styleBtnNavInactif()
{
    return QString(R"(
QPushButton {
    background: transparent;
    color: rgba(255,255,255,0.80);
    border-radius: 10px;
    font-size: 14px;
    text-align: left;
    padding: 12px 20px;
    border: none;
}
QPushButton:hover {
    background: rgba(255,255,255,0.12);
    color: white;
}
)");
}

// ── Champs de saisie ──────────────────────────────────────────
inline QString styleLineEdit()
{
    return QString(R"(
QLineEdit {
    background: white;
    color: #1B4332;
    border: 1.5px solid #B7E4C7;
    border-radius: 10px;
    padding: 10px 14px;
    font-size: 13px;
}
QLineEdit:focus {
    border-color: #2D6A4F;
    background: #FAFFFE;
}
QLineEdit:disabled {
    background: #F0F0F0;
    color: #999;
}
)");
}

// ── Boutons principaux ────────────────────────────────────────
inline QString styleBtnPrimaire()
{
    return QString(R"(
QPushButton {
    background-color: #2D6A4F;
    color: white;
    border: none;
    border-radius: 10px;
    padding: 10px 20px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton:hover   { background-color: #40916C; }
QPushButton:pressed { background-color: #1B4332; }
QPushButton:disabled { background-color: #B7E4C7; color: #fff; }
)");
}

inline QString styleBtnSecondaire()
{
    return QString(R"(
QPushButton {
    background-color: #D8F3DC;
    color: #2D6A4F;
    border: 1.5px solid #74C69D;
    border-radius: 10px;
    padding: 10px 20px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton:hover   { background-color: #B7E4C7; }
QPushButton:pressed { background-color: #95D5B2; }
)");
}

inline QString styleBtnDanger()
{
    return QString(R"(
QPushButton {
    background-color: #E63946;
    color: white;
    border: none;
    border-radius: 10px;
    padding: 10px 20px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton:hover   { background-color: #C1121F; }
QPushButton:pressed { background-color: #9D0208; }
)");
}

// ── Bouton toggle mot de passe ────────────────────────────────
inline QString styleBtnToggleMDP()
{
    return QString(R"(
QPushButton {
    background: transparent;
    border: none;
    color: #888;
    font-size: 13px;
    padding: 0px 6px;
    font-weight: normal;
}
QPushButton:hover { color: #2D6A4F; }
)");
}

// ── Tables ────────────────────────────────────────────────────
inline QString styleTable()
{
    return QString(R"(
QTableWidget {
    background-color: white;
    border: 1px solid #D8EDDF;
    border-radius: 8px;
    gridline-color: #E9F5EC;
    font-size: 13px;
    color: #1B4332;
}
QTableWidget::item         { padding: 6px; color: #1B4332; }
QTableWidget::item:selected { background-color: #B7E4C7; color: #1B4332; }
QHeaderView::section {
    background-color: #40916C;
    color: white;
    font-weight: bold;
    padding: 8px;
    border: none;
    font-size: 13px;
}
)");
}

// ── ComboBox ──────────────────────────────────────────────────
inline QString styleCombo()
{
    return QString(R"(
QComboBox {
    border: 1.5px solid #B7E4C7;
    border-radius: 8px;
    padding: 7px 10px;
    background: #F8FFF9;
    font-size: 13px;
    color: #1B4332;
}
QComboBox:focus { border-color: #2D6A4F; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background: white;
    color: #1B4332;
    selection-background-color: #B7E4C7;
    selection-color: #1B4332;
    border: 1px solid #B7E4C7;
    font-size: 13px;
}
)");
}

// ── QMessageBox uniformisé ────────────────────────────────────
inline QString styleMessageBox()
{
    return QString(R"(
QMessageBox {
    background-color: #F4F6F0;
    font-family: 'Segoe UI', sans-serif;
}
QMessageBox QLabel {
    color: #1B4332;
    font-size: 14px;
    min-width: 260px;
}
QMessageBox QPushButton {
    background-color: #2D6A4F;
    color: white;
    border: none;
    border-radius: 8px;
    padding: 8px 24px;
    font-size: 13px;
    font-weight: bold;
    min-width: 80px;
}
QMessageBox QPushButton:hover   { background-color: #40916C; }
QMessageBox QPushButton:pressed { background-color: #1B4332; }
)");
}

// ── Application globale (appeler dans main.cpp) ───────────────
inline QString styleApp()
{
    return styleDialog()
    + styleSidebar()
        + styleLineEdit()
        + styleTable()
        + styleCombo()
        + styleMessageBox();
}

} // namespace FO

#endif // STYLE_H