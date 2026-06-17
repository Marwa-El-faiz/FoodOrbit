# FoodOrbit — Application de livraison de repas

> Application desktop de gestion de livraison de repas développée en **C++ / Qt**, avec intelligence artificielle intégrée pour l'assistance client et la validation des commandes.

---

## Fonctionnalités

### Espace Client
- Authentification sécurisée (SHA-256) avec réinitialisation par e-mail
- Parcours de plats par catégories avec images
- Panier interactif et passage de commande
- Suivi en temps réel du statut de livraison
- Génération de reçu PDF imprimable
- **Assistant IA YumRun** : chatbot propulsé par Groq AI (LLaMA 3.3 70B) pour recommander des plats selon les préférences

### Espace Restaurant (Admin)
- Gestion complète du menu (ajout, modification, suppression de plats et catégories)
- Tableau de bord avec graphe mensuel d'évolution des ventes
- Gestion des commandes entrantes (accepter / préparer / prêt)
- Attribution des livraisons aux livreurs disponibles
- Statistiques de ventes et revenus

### Espace Livreur
- Tableau des commandes disponibles à prendre en charge
- Suivi des livraisons en cours
- Historique des livraisons passées
- Statistiques personnelles (livraisons, revenus)
- Mise à jour du statut en temps réel

### Intelligence Artificielle
- **OrderValidator** : validation asynchrone des commandes via Groq AI (prise en compte des horaires, charge cuisine, total commande)
- **AIAssistant** : assistant conversationnel intégré au panier client, avec suggestions rapides (Pizza, Végétarien, < 50 DH, Épicé)

### Multi-langue
- Interface disponible en **Français**, **Anglais** et **Arabe**
- Changement de langue dynamique via Qt Linguist (`.ts` / `.qm`)

---

## Architecture

```
FoodOrbit/
├── main.cpp                     # Point d'entrée, init DB & langue
├── database.h/cpp               # Singleton SQLite (schéma, migration, seed)
├── utilisateur.h/cpp            # Classe de base : auth, reset password
├── client.h/cpp                 # Héritage utilisateur : logique client
├── loginwindow.*                # Fenêtre de connexion / inscription
├── resetwindow.*                # Réinitialisation de mot de passe
├── newpasswordwindow.*          # Saisie du nouveau mot de passe
├── clientwindow.*               # Interface principale client
├── Interface_Restaurant.*       # Dashboard restaurant/admin
├── Interface_Livreur.*          # Dashboard livreur
├── AIAssistant.h/cpp            # Chatbot IA (Groq / LLaMA 3.3)
├── ordervalidator.h/cpp         # Validation IA des commandes
├── languagemanager.h/cpp        # Gestion dynamique de la langue
├── style.h                      # Design System FoodOrbit (palette QSS)
├── translations/
│   ├── app_fr.ts                # Traductions françaises
│   ├── app_en.ts                # Traductions anglaises
│   └── app_ar.ts                # Traductions arabes
└── CMakeLists.txt               # Configuration build CMake
```

### Base de données (SQLite)

| Table | Description |
|---|---|
| `utilisateurs` | Comptes (client / livreur / admin), SHA-256 |
| `clients` | Adresse de livraison |
| `livreurs` | Véhicule, disponibilité, position |
| `categories` | Catégories de plats avec images |
| `plats` | Menu : prix, description, disponibilité |
| `commandes` | Commandes avec statuts (en attente → livrée) |
| `details_commande` | Lignes de commande (plat × quantité × prix) |

---

## Installation

### Prérequis

| Outil | Version minimale |
|---|---|
| Qt | 5.15 ou Qt 6.x |
| CMake | 3.16+ |
| Compilateur C++ | C++17 |
| Modules Qt | Widgets, Sql, Network, PrintSupport, LinguistTools |

### Build

```bash
# Cloner le projet
git clone https://github.com/Marwa-El-faiz/FoodOrbit.git
cd FoodOrbit

# Configurer CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compiler
cmake --build build --parallel

# Lancer
./build/Food_Delivery
```

> **Note :** La base de données SQLite (`foodorbit.db`) est créée automatiquement au premier lancement avec des données de démonstration (admin, livreur de test, plats).

### Comptes par défaut (seed)

| Rôle | Email | Mot de passe |
|---|---|---|
| Admin | admin@foodorbit.com | admin123 |
| Livreur | livreur@foodorbit.com | livreur123 |

---

## Configuration IA

L'assistant IA et le validateur de commandes utilisent l'API **Groq** avec le modèle `llama-3.3-70b-versatile`.

La clé API est lue depuis une variable d'environnement :

```bash
# Linux / macOS
export GROQ_API_KEY="gsk_xxxxxxxxxxxx"

# Windows
set GROQ_API_KEY=gsk_xxxxxxxxxxxx
```

> **Sécurité :** Ne commitez jamais votre clé API directement dans le code. Ajoutez tout fichier `.env` à votre `.gitignore`.

---

## Design System

La palette et les styles QSS sont centralisés dans `style.h` (namespace `FO`) :

| Variable | Couleur | Usage |
|---|---|---|
| `C_VERT_FONCE` | `#2D6A4F` | Primaire (boutons, titres) |
| `C_VERT_MOYEN` | `#40916C` | Hover / accents |
| `C_VERT_CLAIR` | `#52B788` | Tags, badges |
| `C_VERT_PALE` | `#B7E4C7` | Bordures légères |
| `C_FOND` | `#F4F6F0` | Fond d'écran |
| `C_ROUGE` | `#E63946` | Erreurs, suppression |
| `C_ORANGE` | `#F4A261` | Avertissements |

---

## Technologies utilisées

| Technologie | Rôle |
|---|---|
| C++ 17 | Langage principal |
| Qt 5/6 Widgets | Interface graphique native |
| Qt SQL (SQLite) | Base de données embarquée |
| Qt Network | Appels API REST (Groq, reset e-mail) |
| Qt PrintSupport | Génération de reçus PDF |
| Qt Linguist | Internationalisation (FR/EN/AR) |
| Groq API | IA conversationnelle & validation |
| LLaMA 3.3 70B | Modèle de langage (via Groq) |
| SHA-256 | Hachage des mots de passe |
| CMake | Système de build |

---

## Auteurs

Projet développé dans le cadre d'un projet académique.

---

## Licence

MIT License

Copyright (c) 2025 Marwa El Faiz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
