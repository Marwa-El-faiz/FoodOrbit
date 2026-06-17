#include "database.h"
#include <QCryptographicHash>

static QString hasherMDP(const QString& mdp)
{
    return QString(QCryptographicHash::hash(
                       mdp.toUtf8(), QCryptographicHash::Sha256).toHex());
}

Database& Database::instance()
{
    static Database instance;
    return instance;
}

bool Database::openDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("foodorbit.db");   // ← renommé FoodOrbit
    if (!db.open()) {
        qDebug() << "Erreur ouverture DB :" << db.lastError().text();
        return false;
    }
    createTables();
    migrateSchema();
    seedAdmin();
    seedLivreur();
    seedTestData();
    return true;
}

QSqlDatabase& Database::getDB()  { return db; }
bool Database::isConnected()     { return db.isOpen(); }
void Database::close()           { if (db.isOpen()) db.close(); }

void Database::createTables()
{
    QSqlQuery q;
    q.exec("PRAGMA foreign_keys = ON");

    q.exec("CREATE TABLE IF NOT EXISTS utilisateurs ("
           "  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  nom            TEXT    NOT NULL,"
           "  prenom         TEXT,"
           "  email          TEXT    UNIQUE NOT NULL,"
           "  mot_de_passe   TEXT    NOT NULL,"
           "  telephone      TEXT,"
           "  role           TEXT    NOT NULL"
           "    CHECK(role IN ('client','livreur','admin')),"
           "  reset_token      TEXT,"
           "  reset_expiration TEXT"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS clients ("
           "  id      INTEGER PRIMARY KEY,"
           "  adresse TEXT,"
           "  FOREIGN KEY(id) REFERENCES utilisateurs(id) ON DELETE CASCADE"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS livreurs ("
           "  id         INTEGER PRIMARY KEY,"
           "  vehicule   TEXT,"
           "  disponible INTEGER DEFAULT 1,"
           "  position   TEXT,"
           "  FOREIGN KEY(id) REFERENCES utilisateurs(id) ON DELETE CASCADE"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS categories ("
           "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  nom        TEXT NOT NULL,"
           "  image_path TEXT"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS plats ("
           "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  categorie_id INTEGER NOT NULL,"
           "  nom          TEXT    NOT NULL,"
           "  description  TEXT,"
           "  prix         REAL    NOT NULL,"
           "  disponible   INTEGER DEFAULT 1,"
           "  image_path   TEXT,"
           "  FOREIGN KEY(categorie_id) REFERENCES categories(id)"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS commandes ("
           "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  client_id  INTEGER NOT NULL,"
           "  livreur_id INTEGER,"
           "  statut     TEXT    DEFAULT 'en_attente'"
           "    CHECK(statut IN ('en_attente','confirmee','en_preparation','en_livraison','livree','annulee')),"
           "  date_heure TEXT    DEFAULT (datetime('now')),"
           "  total      REAL    DEFAULT 0.0,"
           "  FOREIGN KEY(client_id)  REFERENCES clients(id),"
           "  FOREIGN KEY(livreur_id) REFERENCES livreurs(id)"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS lignes_commande ("
           "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  commande_id   INTEGER NOT NULL,"
           "  plat_id       INTEGER NOT NULL,"
           "  quantite      INTEGER DEFAULT 1,"
           "  prix_unitaire REAL    NOT NULL,"
           "  FOREIGN KEY(commande_id) REFERENCES commandes(id) ON DELETE CASCADE,"
           "  FOREIGN KEY(plat_id)     REFERENCES plats(id)"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS paiements ("
           "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  commande_id INTEGER NOT NULL UNIQUE,"
           "  montant     REAL    NOT NULL,"
           "  methode     TEXT    CHECK(methode IN ('carte','especes','paypal')),"
           "  statut      TEXT    DEFAULT 'en_attente'"
           "    CHECK(statut IN ('en_attente','confirme','echoue')),"
           "  date_heure  TEXT    DEFAULT (datetime('now')),"
           "  FOREIGN KEY(commande_id) REFERENCES commandes(id)"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS avis ("
           "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  client_id   INTEGER NOT NULL,"
           "  note        INTEGER NOT NULL CHECK(note BETWEEN 1 AND 5),"
           "  commentaire TEXT    NOT NULL,"
           "  date_heure  TEXT    DEFAULT (datetime('now')),"
           "  FOREIGN KEY(client_id) REFERENCES utilisateurs(id) ON DELETE CASCADE"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS messages ("
           "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  client_id   INTEGER,"
           "  nom         TEXT    NOT NULL,"
           "  email       TEXT    NOT NULL,"
           "  sujet       TEXT    DEFAULT 'Contact général',"
           "  message     TEXT    NOT NULL,"
           "  date_heure  TEXT    DEFAULT (datetime('now')),"
           "  lu          INTEGER DEFAULT 0,"
           "  FOREIGN KEY(client_id) REFERENCES utilisateurs(id) ON DELETE SET NULL"
           ")");

    // ── Table horaires ──────────────────────────────────────────────────────
    // Un enregistrement par jour de la semaine (0=Lundi … 6=Dimanche)
    // service : 'petit_dej' | 'dejeuner' | 'diner'  (une ligne par service)
    q.exec("CREATE TABLE IF NOT EXISTS horaires ("
           "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
           "  jour         INTEGER NOT NULL CHECK(jour BETWEEN 0 AND 6),"
           "  service      TEXT    NOT NULL"
           "    CHECK(service IN ('petit_dej','dejeuner','diner')),"
           "  ouvert       INTEGER DEFAULT 1,"
           "  heure_debut  TEXT    NOT NULL DEFAULT '08:00',"
           "  heure_fin    TEXT    NOT NULL DEFAULT '22:00',"
           "  UNIQUE(jour, service)"
           ")");

    qDebug() << "[FoodOrbit] Tables vérifiées/créées.";
}

void Database::seedAdmin()
{
    QSqlQuery query;
    query.prepare(
        "INSERT OR IGNORE INTO utilisateurs "
        "(nom, prenom, email, mot_de_passe, telephone, role) "
        "VALUES (:nom, :prenom, :email, :mdp, :tel, 'admin')");
    query.bindValue(":nom",    "Admin");
    query.bindValue(":prenom", "FoodOrbit");
    query.bindValue(":email",  "admin@foodorbit.ma");
    query.bindValue(":mdp",    hasherMDP("admin2005"));
    query.bindValue(":tel",    "0600000000");
    query.exec();

    if (query.numRowsAffected() > 0)
        qDebug() << "[FoodOrbit] Admin créé : admin@foodorbit.ma / admin2005";
    else
        qDebug() << "[FoodOrbit] Admin déjà présent.";
}

void Database::seedLivreur()
{
    QSqlQuery q;
    q.prepare(
        "INSERT OR IGNORE INTO utilisateurs "
        "(nom, prenom, email, mot_de_passe, telephone, role) "
        "VALUES (:nom, :prenom, :email, :mdp, :tel, 'livreur')");
    q.bindValue(":nom",    "Mohamed");
    q.bindValue(":prenom", "Livreur");
    q.bindValue(":email",  "mohamed@foodorbit.ma");
    q.bindValue(":mdp",    hasherMDP("livreur123"));
    q.bindValue(":tel",    "0612345678");
    q.exec();

    if (q.numRowsAffected() > 0)
        qDebug() << "[FoodOrbit] Livreur créé : mohamed@foodorbit.ma / livreur123";

    QSqlQuery qLiv;
    qLiv.exec(
        "INSERT OR IGNORE INTO livreurs (id, vehicule, disponible) "
        "VALUES ("
        "  (SELECT id FROM utilisateurs WHERE email='mohamed@foodorbit.ma'),"
        "  'Moto', 1"
        ")"
        );
}

void Database::migrateSchema()
{
    QSqlQuery q;

    // ── Colonnes image_path (anciennes BD) ──
    q.exec("ALTER TABLE categories ADD COLUMN image_path TEXT");
    q.exec("ALTER TABLE plats      ADD COLUMN image_path TEXT");

    // ── Colonnes reset mot de passe ──────────────────────────────
    // CORRECTION : ces colonnes étaient manquantes → resetwindow plantait
    q.exec("ALTER TABLE utilisateurs ADD COLUMN reset_token      TEXT");
    q.exec("ALTER TABLE utilisateurs ADD COLUMN reset_expiration TEXT");

    // ── Migration MDP admin en clair → SHA-256 ──
    QSqlQuery fixAdmin;
    fixAdmin.prepare(
        "UPDATE utilisateurs SET mot_de_passe = :h "
        "WHERE email = 'admin@foodrush.ma' AND mot_de_passe = 'admin2005'");
    fixAdmin.bindValue(":h", hasherMDP("admin2005"));
    if (fixAdmin.exec() && fixAdmin.numRowsAffected() > 0)
        qDebug() << "[FoodOrbit] MDP admin migré vers SHA-256.";

    // ── Migration MDP livreur en clair → SHA-256 ──
    QSqlQuery fixLiv;
    fixLiv.prepare(
        "UPDATE utilisateurs SET mot_de_passe = :h "
        "WHERE email = 'mohamed@gmail.com' AND mot_de_passe = 'livreur123'");
    fixLiv.bindValue(":h", hasherMDP("livreur123"));
    if (fixLiv.exec() && fixLiv.numRowsAffected() > 0)
        qDebug() << "[FoodOrbit] MDP livreur migré vers SHA-256.";

    qDebug() << "[FoodOrbit] Migration schéma terminée.";

    // ── Horaires : mode TEST 24h/24 tous les jours ─────────────────────────
    // Vider et recréer pour couvrir 00:00-23:59 chaque jour.
    // Pour passer en production : remplacer ces lignes par les vrais horaires.
    QSqlQuery qDel;
    qDel.exec("DELETE FROM horaires");

    QSqlQuery ins;
    ins.prepare("INSERT INTO horaires (jour, service, ouvert, heure_debut, heure_fin) "
                "VALUES (:j, 'service_continu', 1, '00:00', '23:59')");
    for (int jour = 0; jour <= 6; jour++) {
        ins.bindValue(":j", jour);
        ins.exec();
    }
    qDebug() << "[FoodOrbit] Horaires 24h/24 activés (mode test).";
}
// ════════════════════════════════════════════════════════════════
//  SEED TEST DATA — Menu permanent + comptes de test
//  Appelé automatiquement si les catégories sont vides
// ════════════════════════════════════════════════════════════════
void Database::seedTestData()
{
    QSqlQuery check;
    check.exec("SELECT COUNT(*) FROM categories");
    if (check.next() && check.value(0).toInt() > 0) {
        qDebug() << "[FoodOrbit] Menu déjà présent, seed ignoré.";
        return;
    }

    qDebug() << "[FoodOrbit] Insertion du menu et des données de test...";

    // ── 1. CATÉGORIES ────────────────────────────────────────────
    struct Cat { const char* nom; };
    static const Cat cats[] = {
        {"Marocaine"},
        {"Italienne"},
        {"Asiatique"},
        {"Fast Food"},
        {"Vegetarienne"},
        {"Desserts et Patisseries"},
        {"Boissons"}
    };
    QSqlQuery qCat;
    qCat.prepare("INSERT OR IGNORE INTO categories (nom) VALUES (:nom)");
    for (const auto& c : cats) {
        qCat.bindValue(":nom", c.nom);
        qCat.exec();
    }

    // ── 2. PLATS ─────────────────────────────────────────────────
    struct Plat { const char* categorie; const char* nom; const char* desc; double prix; };
    static const Plat plats[] = {
                                 // Marocaine
                                 {"Marocaine", "Tajine Poulet Citron Confit",
                                  "Tajine traditionnel au poulet, citron confit et olives vertes", 65.0},
                                 {"Marocaine", "Couscous Royal",
                                  "Couscous aux 7 legumes, agneau, poulet et merguez", 85.0},
                                 {"Marocaine", "Pastilla au Poulet",
                                  "Feuillete croustillant farci au poulet, amandes et cannelle", 55.0},
                                 {"Marocaine", "Harira",
                                  "Soupe marocaine traditionnelle aux lentilles et pois chiches", 20.0},
                                 {"Marocaine", "Tajine Kefta aux Oeufs",
                                  "Boulettes de viande epicees en sauce tomate avec oeufs poches", 58.0},
                                 {"Marocaine", "Mechoui Agneau",
                                  "Epaule d'agneau rotie lentement au four, epices douces", 95.0},
                                 // Italienne
                                 {"Italienne", "Pizza Margherita",
                                  "Tomate, mozzarella, basilic frais, huile d'olive", 55.0},
                                 {"Italienne", "Pizza Quattro Formaggi",
                                  "Mozzarella, gorgonzola, parmesan, chevre", 70.0},
                                 {"Italienne", "Spaghetti Carbonara",
                                  "Pates al dente, guanciale, oeuf, pecorino romano", 60.0},
                                 {"Italienne", "Penne Arrabbiata",
                                  "Pates en sauce tomate epicee, ail, piment rouge", 52.0},
                                 {"Italienne", "Risotto aux Champignons",
                                  "Riz cremeux, champignons porcini, parmesan, truffe noire", 75.0},
                                 {"Italienne", "Lasagne Bolognaise",
                                  "Couches de pates, boeuf hache, bechamel, fromage gratiné", 65.0},
                                 // Asiatique
                                 {"Asiatique", "Sushi Mix 12 pieces",
                                  "Assortiment saumon, thon, avocat, concombre", 90.0},
                                 {"Asiatique", "Ramen Tonkotsu",
                                  "Bouillon de porc, nouilles, chashu, oeuf mollet, nori", 72.0},
                                 {"Asiatique", "Pad Thai",
                                  "Nouilles de riz sautees, crevettes, cacahuetes, citron vert", 68.0},
                                 {"Asiatique", "Riz Cantonais",
                                  "Riz saute aux legumes, oeufs, sauce soja, poulet", 50.0},
                                 {"Asiatique", "Dim Sum 8 pieces",
                                  "Raviolis vapeur porc et crevettes, sauce dipping", 58.0},
                                 {"Asiatique", "Curry Thai Vert",
                                  "Poulet, lait de coco, basilic thai, legumes croquants", 70.0},
                                 // Fast Food
                                 {"Fast Food", "Burger Classic",
                                  "Steak hache 150g, cheddar, salade, tomate, sauce maison", 48.0},
                                 {"Fast Food", "Burger Crispy Chicken",
                                  "Filet de poulet pane, pickles, coleslaw, sauce buffalo", 52.0},
                                 {"Fast Food", "Double Smash Burger",
                                  "Double galette smashee, double cheddar, oignons caramelises", 68.0},
                                 {"Fast Food", "Hot Dog New Yorkais",
                                  "Saucisse grilee, moutarde, ketchup, oignons frits", 35.0},
                                 {"Fast Food", "Frites Maison Grand",
                                  "Frites croustillantes, sel de mer, sauce au choix", 22.0},
                                 {"Fast Food", "Nuggets de Poulet 10 pieces",
                                  "Nuggets dores, sauces BBQ, miel moutarde, ketchup", 40.0},
                                 // Végétarienne
                                 {"Vegetarienne", "Bowl Buddha",
                                  "Quinoa, avocat, pois chiches rotis, legumes grilles, tahini", 62.0},
                                 {"Vegetarienne", "Salade Cesar Vege",
                                  "Romaine, croutons, parmesan, sauce Cesar vegetarienne", 48.0},
                                 {"Vegetarienne", "Wrap Falafel",
                                  "Falafels croustillants, houmous, roquette, sauce yaourt", 45.0},
                                 {"Vegetarienne", "Tagine de Legumes",
                                  "Carottes, courgettes, pois chiches, epices douces", 52.0},
                                 {"Vegetarienne", "Soupe de Lentilles",
                                  "Lentilles corail, cumin, citron, pain grille", 28.0},
                                 // Desserts
                                 {"Desserts et Patisseries", "Tiramisu Maison",
                                  "Mascarpone, cafe, biscuits savoiardi, cacao en poudre", 38.0},
                                 {"Desserts et Patisseries", "Cornes de Gazelle",
                                  "Patisserie marocaine aux amandes et eau de fleur d'oranger", 25.0},
                                 {"Desserts et Patisseries", "Cheesecake New York",
                                  "Base biscuitee, creme fromageree, coulis fruits rouges", 42.0},
                                 {"Desserts et Patisseries", "Mochi Glace 4 pieces",
                                  "Pate de riz, glace matcha, fraise, vanille, mangue", 45.0},
                                 {"Desserts et Patisseries", "Chebakia",
                                  "Patisserie marocaine au miel et sesame, fleur d'oranger", 20.0},
                                 // Boissons
                                 {"Boissons", "Jus d'Orange Frais",
                                  "Oranges pressees a la minute, sans sucre ajoute", 18.0},
                                 {"Boissons", "Smoothie Mangue Passion",
                                  "Mangue fraiche, fruit de la passion, lait de coco", 25.0},
                                 {"Boissons", "The a la Menthe",
                                  "The gunpowder, menthe fraiche, sucre traditionnel", 15.0},
                                 {"Boissons", "Limonade Maison",
                                  "Citron, menthe, eau petillante, sirop agave", 20.0},
                                 {"Boissons", "Cafe Marocain Nous-Nous",
                                  "Moitie espresso, moitie lait chaud, cardamome", 16.0},
                                 };

    QSqlQuery qPlat;
    qPlat.prepare(
        "INSERT OR IGNORE INTO plats (categorie_id, nom, description, prix, disponible) "
        "VALUES ((SELECT id FROM categories WHERE nom=:cat), :nom, :desc, :prix, 1)"
        );
    for (const auto& p : plats) {
        qPlat.bindValue(":cat",  p.categorie);
        qPlat.bindValue(":nom",  p.nom);
        qPlat.bindValue(":desc", p.desc);
        qPlat.bindValue(":prix", p.prix);
        qPlat.exec();
    }

    // ── 3. COMPTES CLIENT DE TEST ────────────────────────────────
    // Mot de passe "test1234" → SHA-256
    QString hash = QString(QCryptographicHash::hash(
                               "test1234", QCryptographicHash::Sha256).toHex());

    struct UserTest { const char* nom; const char* prenom; const char* email; const char* tel; const char* role; const char* adresse; };
    static const UserTest users[] = {
                                     {"Alami",   "Youssef", "client1@test.ma",  "0612111111", "client",  "Av. Mohammed V, Casablanca"},
                                     {"Bennani", "Fatima",  "client2@test.ma",  "0623222222", "client",  "Rue Ibn Batouta, Rabat"},
                                     {"Chraibi", "Amine",   "client3@test.ma",  "0634333333", "client",  "Bd Allal El Fassi, Fes"},
                                     {"Rami",    "Khalid",  "livreur2@test.ma", "0645444444", "livreur", ""},
                                     };

    QSqlQuery qUser, qClient, qLiv;
    qUser.prepare(
        "INSERT OR IGNORE INTO utilisateurs (nom,prenom,email,mot_de_passe,telephone,role) "
        "VALUES (:nom,:prenom,:email,:mdp,:tel,:role)"
        );
    qClient.prepare(
        "INSERT OR IGNORE INTO clients (id, adresse) "
        "SELECT id, :adresse FROM utilisateurs WHERE email=:email"
        );
    qLiv.prepare(
        "INSERT OR IGNORE INTO livreurs (id, vehicule, disponible) "
        "SELECT id, 'Scooter', 1 FROM utilisateurs WHERE email=:email"
        );

    for (const auto& u : users) {
        qUser.bindValue(":nom",    u.nom);
        qUser.bindValue(":prenom", u.prenom);
        qUser.bindValue(":email",  u.email);
        qUser.bindValue(":mdp",    hash);
        qUser.bindValue(":tel",    u.tel);
        qUser.bindValue(":role",   u.role);
        qUser.exec();

        QString role = u.role;
        if (role == "client") {
            qClient.bindValue(":adresse", u.adresse);
            qClient.bindValue(":email",   u.email);
            qClient.exec();
        } else if (role == "livreur") {
            qLiv.bindValue(":email", u.email);
            qLiv.exec();
        }
    }

    qDebug() << "[FoodOrbit] Menu et donnees de test inseres avec succes.";
}