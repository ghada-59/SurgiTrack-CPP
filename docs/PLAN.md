# Plan technique senior : 3 projets biomédicaux

> **À lire en premier (agents et humains)**
>
> Ce fichier est la référence technique des trois projets. Ordre de priorité en cas de contradiction :
> 1. le fichier d'instructions du dossier (`CLAUDE.md`, `AGENTS.md` ou `.cursor/rules/projet.mdc`) ;
> 2. ce fichier, `docs/PLAN.md` ;
> 3. `docs/PLAN_2H.md` (planning minute par minute et prompt de départ).
>
> Répartition des outils : Claude Code → projet 1 (Python), Cursor → projet 2 (C++), Codex → projet 3 (MATLAB). Jamais deux outils dans le même dossier.
>
> Version nettoyée du plan : les sections 9 à 17 existaient en double dans le document d'origine. Seule la version à laquelle renvoient les fichiers d'instructions est gardée (section 11 = choix des graphiques). Les deux exemples de code utiles de l'autre version sont en annexe A.

Pour Ghada · octobre 2026 · projets académiques, non cliniques

## 0. Décisions d'ensemble

- **Le deep learning est utilisé dans un seul projet : Python (U-Net).** C'est là qu'il se justifie. Le C++ (géométrie) et le MATLAB (traitement d'image 3D) restent classiques, volontairement : choisir le bon outil pour chaque problème est exactement ce que les recruteurs Stryker et ZEISS veulent voir.

- **Type de réseau : CNN.** Pas de RNN (il n'y a pas de séquences), pas d'ANN seul (les images ont une structure spatiale). Des couches denses (ANN) n'apparaissent que dans une petite tête auxiliaire.

- Les plans de 2 heures déjà créés (SurgiTrack v2, Python + MATLAB) restent la base du noyau. Ce document ajoute l'architecture, l'entraînement, la validation et le dashboard.

- **Ordre : noyau d'abord, dashboard ensuite.** Un dashboard sur un modèle qui ne marche pas ne vaut rien.

- Chaque README dit : « Projet académique, non clinique. Résultats obtenus sur des données de recherche publiques. »

## 1. Environnement commun

| Outil               | Choix                                                             | Où                              |
|---------------------|-------------------------------------------------------------------|---------------------------------|
| Éditeur             | VS Code + Claude Code (projet 1), Cursor (projet 2), Codex (projet 3) : un outil par dossier | C: (léger)                      |
| Contrôle de version | Git + GitHub, un repo par projet, GitHub Actions pour les tests   | C:                              |
| Python              | 3.11, un venv par projet                                          | D:\Python311, venvs dans D:\dev |
| Node.js             | version LTS, pour React + Vite                                    | D: si proposé                   |
| C++                 | MSYS2 UCRT64 : g++, CMake, Ninja, Eigen, GoogleTest               | D:\msys64                       |
| MATLAB              | R2022b ou plus, avec Image Processing Toolbox                     | D:\MATLAB                       |
| Docker              | Docker Desktop, disque image déplacé                              | D:                              |

Pas de Streamlit, pas de Render. Les dashboards sont de vrais front-ends React.

**Variables d'environnement vers D:** PIP_CACHE_DIR=D:\cache\pip, TORCH_HOME=D:\cache\torch, TEMP et TMP=D:\temp.

**Structure type de chaque repo**

```text
projet/
  README.md  LICENSE  .gitignore
  src/  tests/  data/ (ignoré par git)
  results/ (JSON, CSV, PNG exportés)
  dashboard/ (React + Vite)
  docs/ (figures du README)
```

## 2. Projet 1 : segmentation d'échographies (Python)

### 2.1 Objectif

Segmenter automatiquement les lésions sur des échographies mammaires avec un U-Net, mesurer la qualité honnêtement, puis la servir par une API FastAPI dans Docker avec un dashboard React.

### 2.2 Dataset exact : BUSI (Breast Ultrasound Images)

- 780 images : 437 bénignes, 210 malignes, 133 normales, avec masques de lésion. Disponible sur Kaggle (compte gratuit).

- **Pour la segmentation, utilise les 647 images avec lésion** (les images normales ont un masque vide, ce qui fausse le Dice).

- Certaines images ont plusieurs masques : fusionne-les en un seul masque binaire.

- **Limite à écrire dans le README :** le dataset ne donne pas d'identifiant patient. Tu ne peux donc pas séparer par patient. Vérifie les doublons par hash d'image et dis-le clairement.

- Lis la licence avant de publier. Ne mets pas les images dans le repo, seulement un script de téléchargement.

### 2.3 Quel type de neurones ?

| Type                 | Pour quoi                              | Décision                          |
|----------------------|----------------------------------------|-----------------------------------|
| CNN                  | images, motifs locaux                  | **Oui, base du modèle**           |
| ANN (couches denses) | classer un vecteur de caractéristiques | Seulement dans la tête auxiliaire |
| RNN / LSTM           | séquences, vidéo, signaux              | Non, inutile ici                  |

### 2.4 Architecture exacte : U-Net avec branche auxiliaire

Entrée : image en niveaux de gris 256 × 256 × 1.

- **Encodeur, 4 blocs.** Chaque bloc : Conv 3×3 → BatchNorm → ReLU → Conv 3×3 → BatchNorm → ReLU, puis MaxPool 2×2. Canaux : 32, 64, 128, 256 (base 32, léger, environ 7,8 millions de paramètres ; base 64 donne environ 31 millions).

- **Goulot (1 bloc)** : double convolution à 512 canaux, **Dropout2d p = 0,3**.

- **Décodeur, 4 blocs.** Chaque bloc : ConvTranspose 2×2 (monte en résolution) → concaténation avec la skip connection de l'encodeur → double convolution.

- **Sortie segmentation** : Conv 1×1 → 1 canal → **sigmoid** → masque de probabilités.

- **Branche auxiliaire (classification bénin / malin)** : sortie du goulot → Global Average Pooling → Linear(512→64) → ReLU → **Dropout p = 0,3** → Linear(64→1) → sigmoid.

**À quoi sert la branche auxiliaire ?** Elle force le goulot à encoder si la lésion est bénigne ou maligne, ce qui aide souvent l'encodeur à mieux représenter la forme. C'est de l'apprentissage multitâche. Elle est jetable : on peut l'enlever pour mesurer son effet (voir 2.9).

### 2.5 Ce qui se passe dans un neurone

- **Neurone :** z = w · x + b, puis a = f(z). Les poids w pondèrent les entrées, le **biais b** décale le seuil d'activation, f est la fonction d'activation.

- **Dans un CNN**, un filtre 3×3 est un neurone partagé qui glisse sur l'image. Avec BatchNorm juste après, le biais de la convolution est redondant : on met bias=False. Les couches Linear gardent leur biais.

- **Le va-et-vient** a deux sens. *Forward :* l'image traverse le réseau jusqu'à la prédiction, on calcule la loss. *Backward (rétropropagation) :* on calcule le gradient de la loss par rapport à chaque poids avec la règle de dérivation en chaîne, puis l'optimiseur met à jour les poids.

- **Dropout 30 %.** À chaque pas d'entraînement, 30 % des activations d'une couche sont mises à zéro au hasard. Cela empêche le réseau de dépendre de quelques neurones et réduit le sur-apprentissage. Ce n'est pas fait « à la fin » et ça ne rend pas le modèle plus « humain » : **c'est actif seulement pendant l'entraînement**. En inférence (model.eval()), tous les neurones fonctionnent.

### 2.6 Fonctions d'activation et où elles servent

| Fonction          | Où                                                        | Pourquoi                                          |
|-------------------|-----------------------------------------------------------|---------------------------------------------------|
| ReLU              | toutes les couches cachées                                | simple, rapide, limite la disparition du gradient |
| LeakyReLU (essai) | si des neurones « meurent »                               | laisse passer un petit gradient négatif           |
| Sigmoid           | sortie du masque et de la tête auxiliaire                 | probabilité entre 0 et 1 par pixel                |
| Softmax           | si tu passes à plus de 2 classes (normal / bénin / malin) | probabilités qui somment à 1                      |

Astuce PyTorch : entraîne avec BCEWithLogitsLoss (sigmoid intégré, plus stable) et applique la sigmoid seulement à l'inférence.

### 2.7 Fonction de perte

- **Segmentation :** L_seg = 0,5 × BCE + 0,5 × DiceLoss. La BCE donne un gradient stable pixel par pixel ; le Dice gère le déséquilibre (la lésion occupe peu de pixels).

- **DiceLoss** = 1 − (2·\|P ∩ G\| + ε) / (\|P\| + \|G\| + ε), avec ε = 1e-6.

- **Auxiliaire :** BCEWithLogitsLoss.

- **Totale :** L = L_seg + λ × L_cls avec λ = 0,3 (teste 0,1 / 0,3 / 0,5).

### 2.8 Entraînement

- Optimiseur AdamW, lr = 1e-3, weight decay 1e-4.

- Scheduler ReduceLROnPlateau (facteur 0,5, patience 5) ou CosineAnnealing.

- Batch 8, 60 époques maximum, **early stopping** (patience 10 sur le Dice de validation), gradient clipping à 1,0.

- Augmentations (albumentations) : flip horizontal, rotation ±15°, brightness/contrast, léger bruit gaussien, déformation élastique douce. **Jamais sur validation et test.**

- Graine fixée (42), versions figées dans requirements.txt.

- Sans GPU : base 32, images 256, 30 époques, c'est faisable sur CPU mais lent. Dis-moi si tu as un GPU NVIDIA.

### 2.9 Quoi comparer (ablation)

| Expérience                                         | Ce qu'elle prouve                      |
|----------------------------------------------------|----------------------------------------|
| A. Baseline classique (Otsu + morphologie)         | le deep learning apporte quelque chose |
| B. U-Net, BCE seule                                | point de départ                        |
| C. U-Net, BCE + Dice                               | effet de la loss                       |
| D. C + branche auxiliaire                          | effet du multitâche                    |
| E. D + encodeur pré-entraîné (ResNet34), optionnel | effet du transfert                     |

Un tableau honnête, avec des écarts-types, vaut plus qu'un seul beau chiffre.

### 2.10 Erreurs attendues et corrections

| Problème                        | Signe                               | Remède                                           |
|---------------------------------|-------------------------------------|--------------------------------------------------|
| Sur-apprentissage               | loss train baisse, loss val remonte | plus d'augmentation, dropout, early stopping     |
| Lésions minuscules manquées     | Dice bas sur petites lésions        | Dice/focal loss, crops centrés sur la lésion     |
| Faux positifs (ombres, speckle) | taches isolées                      | garder la plus grande composante connexe         |
| Contours flous                  | IoU bas malgré bon Dice             | seuil réglé sur validation, post-traitement      |
| Fuite de données                | résultats trop beaux                | dédoublonnage par hash, split fixé et sauvegardé |

### 2.11 Validation

- Split 70 / 15 / 15 stratifié par classe (bénin / malin), sauvegardé dans un fichier pour être reproductible.

- **Validation :** réglage des hyperparamètres, early stopping, seuil de binarisation.

- **Test :** utilisé une seule fois, à la fin.

- **Validation croisée 5 plis** pour donner moyenne ± écart-type.

- Métriques segmentation : Dice, IoU, précision, rappel, Hausdorff 95 (optionnel). Métriques auxiliaire : exactitude, F1, AUC. Temps d'inférence par image.

- Analyse par taille de lésion et par classe.

### 2.12 Résultats estimés (à confirmer par tes mesures)

- Baseline Otsu : Dice environ 0,3 à 0,5.

- U-Net depuis zéro : Dice environ 0,70 à 0,80.

- Avec encodeur pré-entraîné : environ 0,78 à 0,85.

- Ce sont des ordres de grandeur issus de la littérature sur BUSI, pas des promesses. Publie ce que tu mesures, même si c'est plus bas.

### 2.13 Déploiement : FastAPI + Docker

- **POST /predict** : image PNG ou JPG → masque en base64, aire de la lésion, probabilité bénin/malin, temps d'inférence.

- **GET /health** et **GET /model-info** (version, date, hyperparamètres).

- Validation des entrées (type, taille, dimensions), erreurs claires en JSON, limite de taille de fichier.

- Chargement du modèle une seule fois au démarrage, mode eval, torch.no_grad().

- Dockerfile : python:3.11-slim, PyTorch version CPU, utilisateur non root, healthcheck. Tests avec pytest et httpx.

### 2.14 Dashboard React (pas de Streamlit)

Stack : **React 18 + Vite, JSX, modules ES6, hooks, CSS avec variables, Recharts** pour les courbes, canvas pour l'image.

- **Page Analyse :** glisser-déposer, image originale, masque prédit, superposition avec curseur d'opacité, curseur avant/après, zoom et déplacement.

- **Page Performance :** courbes de loss, Dice par image, matrice de confusion de la branche auxiliaire.

- **Page Erreurs :** galerie des pires cas, filtre par taille de lésion, vérité terrain en contour sarcelle contre prédiction en corail.

- **Page Modèle :** fiche du modèle, hyperparamètres, latence.

- États de chargement et d'erreur soignés, export PNG et CSV, responsive, bandeau « Projet académique, non clinique ».

- Appel de l'API avec fetch et CORS configuré dans FastAPI.

**Palette (clair, sans défaut IA)**

| Rôle            | Couleur  |
|-----------------|----------|
| Fond            | \#F2F5F3 |
| Surface         | \#FFFFFF |
| Texte           | \#1E2B29 |
| Accent sarcelle | \#0E7C7B |
| Alerte corail   | \#E4572E |

Polices : IBM Plex Sans + IBM Plex Mono pour les chiffres. Pas de dégradés, pas de cartes toutes identiques, pas d'icônes décoratives : une grille asymétrique avec l'image en grand à gauche et les mesures en colonne étroite à droite.

## 3. Projet 2 : SurgiTrack-CPP v2 (C++)

### 3.1 Objectif

Recalage rigide par points (fiduciaux) pour la navigation chirurgicale, avec mesure rigoureuse des erreurs. C'est l'algorithme au cœur des systèmes de navigation, d'où le lien direct avec les offres Stryker.

### 3.2 Deep learning ou ML ?

**Non, et c'est volontaire.** Le recalage par SVD est exact et se démontre mathématiquement. Mettre un réseau de neurones ici serait un mauvais choix technique. Extension ML optionnelle : à partir du CSV Monte Carlo, entraîner en Python un modèle simple (forêt aléatoire) qui prédit le TRE selon la géométrie des fiduciaux.

### 3.3 Données

Synthétiques, générées par le programme : un nuage de 3 à 10 fiduciaux, une rotation et une translation connues, du bruit gaussien. Aucun dataset à télécharger.

### 3.4 Algorithme (Kabsch / Horn avec SVD)

1.  Calculer les centroïdes p̄ et q̄ des deux nuages.

2.  Centrer les points, former H = Σ (pᵢ − p̄)(qᵢ − q̄)ᵀ.

3.  SVD : H = U Σ Vᵀ.

4.  R = V · diag(1, 1, det(V Uᵀ)) · Uᵀ. Le terme det évite une **réflexion** (déterminant −1).

5.  t = q̄ − R p̄.

**Erreurs de mesure**

- **FRE** : erreur résiduelle aux fiduciaux utilisés pour le calcul (RMS).

- **TRE** : erreur à des points cibles **non utilisés** dans le calcul. C'est ce qui compte cliniquement.

- **FLE** : erreur de localisation du fiducial, c'est le bruit injecté.

### 3.5 Simulation Monte Carlo

- Niveaux de bruit FLE (RMS total) : 0,1 / 0,25 / 0,5 / 1,0 mm. Nombre de fiduciaux : 3 à 10. 10 000 essais par configuration, graine fixée.

- Sorties : moyenne, médiane, percentiles 50 / 95 / 99 du TRE, histogramme, courbe erreur contre bruit, courbe erreur contre nombre de fiduciaux.

- **Vérification théorique :** TRE² ≈ (FLE² / N) × (1 + (1/3) Σ dₖ² / fₖ²) (formule de Fitzpatrick). Ton Monte Carlo doit être à environ 5 % de cette formule. C'est ta meilleure preuve de justesse.

### 3.6 Tests (GoogleTest)

- Sans bruit : R et t retrouvés à moins de 1e-9.

- Rotation identité et translation nulle.

- Cas de réflexion (det = −1) géré correctement.

- Fiduciaux alignés ou coplanaires : détectés et signalés.

- Moins de 3 points : erreur claire.

- Mauvais appariement des points : détecté par un FRE anormal.

### 3.7 Outils et fonctions

C++17, CMake + Ninja, Eigen (Matrix3d, Vector3d, JacobiSVD), GoogleTest, nlohmann/json pour exporter, clang-format, GitHub Actions. Sanitizers (address, undefined) : sous Windows, MinGW les supporte mal, utilise WSL2 si disponible.

### 3.8 Erreurs fréquentes

Réflexion ignorée, confusion ligne/colonne dans Eigen, mélange de millimètres et de mètres, appariement des points inversé, comparaison de flottants avec == au lieu d'une tolérance, SVD mal conditionnée sur des points presque alignés.

### 3.8 (suite) Comment corriger chaque erreur

Chaque erreur a un symptôme visible, une correction dans le code et un test qui l'empêche de revenir.

| Erreur                          | Comment la repérer                                  | Correction dans le code                                                                                                                                   | Test GoogleTest                                                                       |
|---------------------------------|-----------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------|
| Réflexion ignorée               | det(R) vaut −1, ou le FRE est élevé même sans bruit | Matrice de correction diag(1, 1, d), avec d = signe de det(V·Uᵀ)                                                                                          | Nuage miroir : vérifier que det(R) = +1 à 1e-9 près                                   |
| Confusion ligne / colonne Eigen | Résultat transposé, rotation inverse                | Une seule convention : un point = une colonne d'une Matrix3Xd (3 × N), écrite en commentaire dans l'en-tête                                               | Rotation connue de 90° autour de z : un point précis doit arriver à un endroit précis |
| Millimètres et mètres mélangés  | TRE absurde, par facteur 1000                       | Suffixe \_mm sur chaque variable, conversion à un seul endroit (la lecture des données), champ units: mm dans le JSON exporté                             | Translation de 10 mm connue : le t retrouvé vaut 10, pas 0,01                         |
| Appariement des points inversé  | FRE élevé sans bruit                                | Vérifier avant le calcul que les distances entre paires de points sont les mêmes dans les deux nuages (elles ne changent pas dans un mouvement rigide)    | Permuter deux points : la fonction doit signaler l'erreur d'appariement               |
| Flottants comparés avec ==      | Test qui passe ou échoue selon le compilateur       | EXPECT_NEAR(a, b, 1e-9) ou isApprox, avec une tolérance relative ; jamais ==                                                                              | Tous les tests numériques utilisent une tolérance                                     |
| SVD mal conditionnée            | Points presque alignés, rotation instable           | Examiner les valeurs singulières de H : si la deuxième est quasi nulle, les points sont alignés et la rotation autour de l'axe est indéterminée → refuser | Trois points alignés : exception claire ; presque alignés : avertissement             |

**Lire les valeurs singulières de H (σ₁ ≥ σ₂ ≥ σ₃)**

- σ₂ proche de 0 : fiduciaux alignés. Refuser le calcul, la rotation autour de la droite n'est pas définie.

- σ₃ proche de 0 : fiduciaux coplanaires. Le calcul reste possible, mais seulement avec la correction du déterminant ci-dessus, et le TRE sera moins bon hors du plan. Afficher un avertissement.

- Seuils de départ : refus si σ₂ \< 1e-6 × σ₁, avertissement si σ₂ \< 1e-2 × σ₁. À calibrer avec tes propres tests, ce ne sont pas des valeurs officielles.

**Noyau corrigé (Eigen, C++17)**

```cpp
// P, Q : Eigen::Matrix3Xd (3 x N), un point par colonne, unités : mm
Eigen::Vector3d cp = P.rowwise().mean();
Eigen::Vector3d cq = Q.rowwise().mean();
Eigen::Matrix3Xd Pc = P.colwise() - cp;
Eigen::Matrix3Xd Qc = Q.colwise() - cq;

Eigen::Matrix3d H = Pc * Qc.transpose();
Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
const auto& s = svd.singularValues();   // s(0) >= s(1) >= s(2)

if (s(1) < 1e-6 * s(0))
    throw std::invalid_argument("fiduciaux alignes : recalage indetermine");

double d = (svd.matrixV() * svd.matrixU().transpose()).determinant() > 0 ? 1.0 : -1.0;
Eigen::Matrix3d D = Eigen::Vector3d(1.0, 1.0, d).asDiagonal();

Eigen::Matrix3d R = svd.matrixV() * D * svd.matrixU().transpose();
Eigen::Vector3d t = cq - R * cp;
```

**Contrôles à ajouter autour de ce noyau**

1.  Avant : même nombre de points dans P et Q, au moins 3, aucune valeur NaN.

2.  Avant : comparer les distances entre paires de points de P et de Q ; un écart au-delà de la tolérance indique un mauvais appariement ou un mélange d'unités.

3.  Après : vérifier que R est bien une rotation (RᵀR = I et det(R) = 1, à 1e-9 près).

4.  Après : calculer le FRE ; s'il est grand alors que le bruit est nul, la cause est l'appariement, les unités ou la réflexion.

**Avant chaque commit :** compiler avec -Wall -Wextra -Werror, lancer ctest, puis les sanitizers (address, undefined) sous WSL2 si disponible.

### 3.9 Résultats estimés

- Sans bruit : erreur de l'ordre de 1e-12 mm.

- Avec FLE de 0,25 mm et 6 fiduciaux bien répartis : TRE d'environ 0,1 à 0,2 mm au centre du volume, plus grand en périphérie. La simulation le confirmera.

- Plus de fiduciaux et une meilleure répartition réduisent le TRE ; des fiduciaux presque alignés le dégradent fortement.

### 3.10 Dashboard (front-end statique)

Le programme C++ exporte results.json et mc.csv ; le dashboard React les lit, sans serveur.

- **Vue d'ensemble :** moyenne du TRE, percentile 95, nombre d'essais, tests passés.

- **Vue Recalage :** nuage de points 3D avant et après (three.js via react-three-fiber), matrice R et t, tableau des résidus par point.

- **Vue Monte Carlo :** histogramme du TRE, courbe erreur contre bruit avec le seuil en brique, comparaison à la formule théorique.

- **Vue Géométrie :** impact du nombre et de la disposition des fiduciaux.

- **Vue Tests et CI :** liste des tests, durée, état du build.

- Contrôles : curseurs de bruit et de nombre de fiduciaux qui changent la vue (à partir des résultats pré-calculés).

**Palette (clair, esprit plan d'ingénieur)**

| Rôle                | Couleur                             |
|---------------------|-------------------------------------|
| Fond                | \#F1F3F5                            |
| Surface             | \#FFFFFF                            |
| Texte               | \#1B2631                            |
| Accent bleu ardoise | \#35566F                            |
| Seuil, alerte       | \#B23A2E (brique), \#D98E04 (ambre) |

Polices : DM Sans + JetBrains Mono. Grille de fond fine, axes lisibles, beaucoup de données mais bien aérées.

## 4. Projet 3 : morphométrie osseuse 3D à partir de CT (MATLAB)

### 4.1 Objectif

À partir d'un volume CT, segmenter l'os, reconstruire sa surface 3D et mesurer volume, surface, axes principaux, avec une **validation chiffrée sur des fantômes de géométrie connue**.

### 4.2 Deep learning ou ML ?

**Non** pour la version principale : seuillage d'Otsu et morphologie suffisent et se valident. Ajouter un réseau ici dupliquerait le projet Python. Si tu ajoutes de l'IA plus tard, ce sera un 3D U-Net à part, dans la roadmap.

### 4.3 Données

1.  **Fantômes synthétiques d'ellipsoïdes** (données principales) : demi-axes a, b, c connus, donc volume exact V = 4/3 · π · a · b · c. Tailles de voxel 0,5 / 1 / 2 mm, bruit ajouté en unités Hounsfield.

2.  **Un volume CT réel, optionnel :** une série d'un jeu de données ouvert de vertèbres de type VerSe, ou une série TCIA. Vérifie la licence et la taille avant de télécharger, et garde-la sur D:.

### 4.4 Pipeline

1.  Lecture DICOM : dicomreadVolume, dicominfo. Conversion en HU avec RescaleSlope et RescaleIntercept.

2.  Espacement des voxels depuis PixelSpacing et SliceThickness (ou SpacingBetweenSlices).

3.  Lissage : imgaussfilt3.

4.  Segmentation : graythresh / multithresh (Otsu) comparé à un seuil fixe en HU.

5.  Morphologie : imopen, imclose, bwareaopen, puis garder la plus grande composante avec bwconncomp.

6.  Surface : isosurface, isonormals, reducepatch ; aire par somme des triangles ; volume du maillage par le théorème de la divergence.

7.  Mesures : regionprops3 (Volume, PrincipalAxisLength, Orientation, Centroid, Solidity) ; sphéricité = π^(1/3) (6V)^(2/3) / A.

8.  Export : JSON des mesures, STL du maillage, PNG, vidéo.

### 4.5 Visualisation et simulation 3D

- Trois coupes orthogonales (axiale, coronale, sagittale) avec window/level.

- Maillage 3D avec patch, camlight, lighting gouraud, os en ivoire et axes principaux dessinés en terre cuite.

- **Vidéo de rotation :** camorbit + VideoWriter, MP4 intégré au dashboard.

- **Simulation :** pour chaque taille de voxel et niveau de bruit, générer le fantôme, mesurer, tracer l'erreur de volume (%) en fonction de la résolution. Test d'invariance : tourner le fantôme et vérifier que les mesures ne changent pas.

- Si tu ajoutes des cartes de couleur (comme dans ton idée), précise toujours ce que la colorbar représente : HU, distance, erreur.

### 4.6 Validation

- Erreur relative du volume voxel et du volume maillage contre le volume analytique, par taille de voxel et par bruit.

- Erreur des axes principaux contre a, b, c.

- Comparaison Otsu contre seuil fixe.

- Tests MATLAB (runtests) sur les fantômes.

### 4.7 Erreurs fréquentes

Oublier le voxel anisotrope (volume faux), oublier RescaleIntercept (HU faux), seuil trop bas qui inclut la table du scanner, effet d'escalier sur la surface (aire surestimée), confondre l'ordre des axes dans meshgrid et ndgrid.

### 4.8 Résultats estimés (à confirmer)

- Erreur de volume sur fantôme : environ 1 à 2 % à 0,5 ou 1 mm, plus élevée à 2 mm, de l'ordre de quelques pourcents.

- Erreur d'aire : plus grande que le volume à cause de l'effet d'escalier, environ 2 à 6 % sans lissage du maillage.

### 4.9 Dashboard (React + three.js)

MATLAB exporte results.json, des PNG et un STL ; le dashboard les affiche.

- **Vue 3D :** maillage chargé avec STLLoader, rotation à la souris, axes principaux activables, coupe de section.

- **Vue Coupes :** les trois plans avec curseur de position et window/level.

- **Vue Mesures :** tableau volume, aire, axes, sphéricité, avec leur erreur contre la référence.

- **Vue Validation :** courbe erreur contre résolution, boîtes à moustaches sur les fantômes.

- **Vidéo :** rotation du maillage en MP4.

**Palette (clair, esprit os et laboratoire)**

| Rôle               | Couleur  |
|--------------------|----------|
| Fond               | \#FAF6EF |
| Surface            | \#FFFFFF |
| Texte              | \#2B2420 |
| Accent terre cuite | \#B8502F |
| Accent pétrole     | \#2F6F73 |

Polices : Source Serif 4 pour les titres, Public Sans pour le texte. Maillage de l'os en ivoire \#E8DCC8.

## 5. Hébergement et déploiement des dashboards

- Projet Python : API en Docker en local ; le dashboard appelle l'API. Pour le publier, il faut un hébergeur pour l'API, à choisir plus tard.

- Projets C++ et MATLAB : dashboards **statiques** (résultats pré-calculés), publiables gratuitement avec GitHub Pages.

## 6. Planning réaliste

| Jour  | Noyau                                             | Dashboard      |
|-------|---------------------------------------------------|----------------|
| 1     | Python : données, U-Net, entraînement, évaluation | API + Docker   |
| 2     | C++ : recalage, tests, Monte Carlo                | export JSON    |
| 3     | MATLAB : pipeline, fantômes, validation           | export + vidéo |
| Après | Dashboards React des trois projets, READMEs, CV   |                |

Trois jours suffisent pour les trois noyaux, pas pour trois dashboards complets. Fais-les ensuite, un par un.

## 7. Ce que tu dois savoir expliquer en entretien

- Pourquoi U-Net et pas un autre réseau, et à quoi servent les skip connections.

- Ce que fait le dropout et pourquoi il est coupé en inférence.

- Pourquoi Dice + BCE, et ce que mesurent Dice et IoU.

- La différence entre FRE et TRE, et pourquoi le TRE compte.

- Pourquoi tu as validé sur des fantômes, et les limites de BUSI (pas d'identifiant patient).

- Ce qui n'est pas fait et figure dans la roadmap.

## 8. Règles d'honnêteté

- Chaque chiffre du README vient d'une exécution que tu as faite.

- Ne dis jamais « entraîné » pour un modèle que tu n'as pas entraîné.

- Mentionne les limites : petit dataset, pas de séparation par patient, usage non clinique.

## 9. Affichage et visualisation des images

Les images médicales restent en niveaux de gris et les résultats se superposent en couleur. Une couleur n'apparaît que pour dire quelque chose : un masque, une erreur, une mesure.

### 9.1 Ce que je retiens de tes captures

| Capture                                 | Ce que je garde                                                                                                                                         | Où je l'utilise                            |
|-----------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------|
| Les 5 dashboards (maquette)             | La structure : barre latérale, en-tête avec badge de technologie, image au centre, panneau de mesures, bandeau Fonctionnalités / Technologies           | Mise en page des 3 dashboards (section 10) |
| Les mêmes 5 dashboards                  | À éviter : chiffres inventés (96,2 %), jauges rondes toutes identiques, colorbar arc-en-ciel, bleu marine générique                                     | Toutes les pages                           |
| Viewer Canon (axial, coronal, rendu 3D) | Grille 2 × 3, texte dans les coins (fenêtre/niveau, épaisseur, position), préréglages 400/40 et 1600/−600                                               | Projet 3, vue Coupes                       |
| RSNA et cloudfactory                    | Même coupe sans masque puis avec masque, légende en pastilles, un organe = une couleur                                                                  | Légende cliquable des masques              |
| BraTS et Medium                         | Bande de coupes consécutives, une ligne vérité terrain et une ligne prédiction, numéro de coupe sous chaque image                                       | Projet 1, page Erreurs                     |
| Mediso, MDPI, Springer, PMC             | Grille de comparaison au même recadrage avec le nom de la méthode sous chaque image ; paire bruitée / propre ; méthodes contre la référence pleine dose | Projet 3, onglet Débruitage                |
| U-Net, DnCNN, MSTAC, CNN + LSTM         | Légende des flèches par type d'opération, dimensions des tenseurs sous chaque bloc, connexion résiduelle visible                                        | Schémas (9.5)                              |
| Captures de structures de dossiers      | tests/, configs/, reports/ séparés du code                                                                                                              | Structure des repos (section 1)            |
| Tableau des fonctions                   | ReLU, Softmax, Sigmoid, Cross-entropy, Dice + BCE, AdamW : cohérent avec mon plan ; pour un masque binaire, la sigmoid                                  | Projet 1                                   |

**Deux précautions.** Ces captures sont des figures d'articles et de sites : ne les mets pas dans tes repos (droits d'auteur), redessine-les avec tes propres données. Et la capture cloudfactory montre un identifiant patient et une date : sur tes écrans, rien de tel, anonymise tout.

### 9.2 Règles du viewer

- Coins annotés en police mono : fenêtre/niveau, numéro de coupe sur N, épaisseur, niveau de zoom. Lettres d'orientation A / P / L / R sur les bords pour CT et IRM.

- Règle d'échelle en mm quand l'espacement des pixels est connu. Pour BUSI il ne l'est pas : affiche des pixels et dis-le.

- Molette : changer de coupe. Glisser : déplacer. Ctrl + molette : zoom. Clic droit + glisser : fenêtre/niveau, comme sur le viewer Canon. Flèches du clavier : coupe suivante. Touche O : masque affiché ou caché.

- Superposition avec opacité réglable (40 % par défaut) ; la vérité terrain en contour, la prédiction en aire pleine ; bascule avant / après.

- Légende cliquable : pastille de couleur, nom, volume ; un clic cache ou montre le masque.

- Toute comparaison utilise la même fenêtre et le même recadrage.

- Le fond de la zone image reste sombre (les images ont un fond noir, comme sur Canon). Tout le reste de la page est clair.

- Aucune donnée patient affichée, tags DICOM identifiants supprimés avant export, bandeau fixe « Projet académique, non clinique ».

### 9.3 Couleurs des masques et des cartes

| Usage                                    | Couleur                                                                                  |
|------------------------------------------|------------------------------------------------------------------------------------------|
| Projet 1 : lésion prédite (aire, 40 %)   | Corail \#E4572E                                                                          |
| Projet 1 : vérité terrain (contour 2 px) | Sarcelle \#0E7C7B                                                                        |
| Projet 3 : os                            | Ivoire \#E8DCC8, axes principaux en terre cuite \#B8502F                                 |
| Plusieurs organes ou labels              | Palette Okabe-Ito : \#E69F00, \#56B4E9, \#009E73, \#F0E442, \#0072B2, \#D55E00, \#CC79A7 |
| Cartes de différence (signées)           | Divergente centrée sur 0 : bleu \#2166AC, blanc, rouge \#B2182B                          |
| Cartes d'erreur (positives)              | Séquentielle claire vers foncée (viridis ou cividis)                                     |

Pas de rouge et vert comme seule distinction (daltonisme), jamais de jet ni d'arc-en-ciel, et toute colorbar porte son unité (HU, mm, %).

### 9.4 Affichage selon le type d'image

| Type                       | Affichage                                                                     | Particularités                                                                                                           |
|----------------------------|-------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------|
| Échographie (BUSI, PNG 2D) | Canvas 2D, luminosité / contraste, zoom                                       | Pas d'unités HU, pas de fenêtre/niveau                                                                                   |
| IRM (ex. BraTS)            | Coupes axiales, bande de 5 coupes, onglets de séquences (T1, T1ce, T2, FLAIR) | Intensités arbitraires : normaliser par volume (percentiles 1 à 99). Format NIfTI à convertir en PNG ou WebP pour le web |
| CT (DICOM, HU)             | Fenêtre/niveau en HU avec préréglages                                         | HU = valeur × RescaleSlope + RescaleIntercept ; espacement des voxels pour toute mesure                                  |
| Volume 3D                  | Trois plans liés par un curseur en croix + rendu 3D du maillage               | Rendu 3D avec three.js, maillage exporté en STL                                                                          |
| Points 3D (projet 2)       | Nuage de points avant / après, axes en mm                                     | Vue orthographique en option                                                                                             |

Préréglages CT courants (fenêtre / niveau) : tissus mous 400 / 40, poumon 1600 / −600 (comme sur la capture Canon), os environ 1800 / 400, cerveau 80 / 40.

### 9.5 Schémas à produire (redessinés par toi)

| Schéma               | Contenu                                                                                                                                             | Outil                                      |
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------|
| U-Net du projet 1    | Blocs avec tailles de tenseurs, skip connections, branche auxiliaire, légende des flèches (conv 3×3 + BN + ReLU, max pool, up-conv, skip, conv 1×1) | Composant SVG dans le dashboard ou draw.io |
| Pipeline du projet 1 | Données → entraînement → modèle → API → dashboard                                                                                                   | Mermaid                                    |
| Recalage du projet 2 | P, Q → centroïdes → H → SVD → R, t → FRE / TRE                                                                                                      | Mermaid                                    |
| Pipeline du projet 3 | HU → lissage → seuil → morphologie → maillage → mesures                                                                                             | Mermaid                                    |

Version lisible de l'U-Net du projet 1 (tailles spatiales : 256, 128, 64, 32, 16) :

```text
256×256×1
E1 (32)  ──────────────────────────► U1 (32) ► conv 1×1 ► sigmoid ► masque
  ↓ max pool                              ↑ up-conv
E2 (64)  ─────────────────────► U2 (64)
  ↓                                  ↑
E3 (128) ───────────────► U3 (128)
  ↓                            ↑
E4 (256) ─────────► U4 (256)
  ↓                     ↑
Goulot (512) + Dropout 0,3
  └─► GAP ► Linear 64 ► ReLU ► Dropout 0,3 ► Linear 1 ► sigmoid (bénin / malin)
```

### 9.6 Projet 3 : onglet Débruitage (option)

Le débruitage compare des méthodes sur les fantômes bruités, puis mesure ce qui compte vraiment : l'effet sur l'erreur de volume.

- **Méthodes :** gaussien (imgaussfilt3), médian (medfilt3), bilatéral (imbilatfilt), non-local means (imnlmfilt), Wiener (wiener2). Les ondelettes (wdenoise2) demandent la Wavelet Toolbox : seulement si tu l'as.

- **Affichage :** grille de comparaison au même recadrage et à la même fenêtre (comme Mediso), nom de la méthode sous chaque image, zoom sur une région d'intérêt, carte de différence avec colorbar en HU.

- **Mesures :** PSNR et SSIM, mais surtout l'erreur de volume avant et après filtrage. Un PSNR élevé ne prouve pas que les contours utiles sont préservés.

- **Graphique clé :** profil d'intensité le long d'une ligne traversant un bord. Il montre si le filtre étale le contour.

## 10. Dashboards : structure de page et design

On garde la logique des 5 maquettes (barre latérale, image au centre, panneau de mesures) avec un autre langage visuel.

### 10.1 Squelette commun (CSS Grid)

```text
┌────────┬───────────────────────────────────┬──────────────┐
│ Nav    │ En-tête : titre, sous-titre,      │ Mesures      │
│ texte  │ version, « non clinique »         │ (colonne     │
│ seul   ├───────────────────────────────────┤ étroite)     │
│        │ Zone image / graphique principal  │              │
│        ├───────────────────────────────────┴──────────────┤
│        │ Bande de vignettes, coupes ou résultats          │
└────────┴──────────────────────────────────────────────────┘
Page À propos : Fonctionnalités · Technologies · Limites
```

**Ce qui change par rapport aux maquettes**

- Fond clair teinté, barre latérale claire avec un filet fin ; l'élément actif est souligné par l'accent, pas rempli d'une pastille.

- Pas de cartes arrondies identiques avec ombres : une grille asymétrique séparée par des filets de 1 px, coins de 2 à 4 px.

- Les mesures sont un tableau typographique (valeur en mono, unité, écart-type, nombre d'images), pas des jauges rondes.

- Aucun chiffre sans source : il vient du fichier de résultats ou il affiche « — ».

- Une couleur d'accent et une couleur d'alerte par projet.

- Pas d'icônes décoratives, seulement les icônes d'outils du viewer (zoom, fenêtre/niveau, règle).

- Des polices avec du caractère (IBM Plex, Source Serif), pas Inter partout.

- « Fonctionnalités / Technologies » devient une page À propos avec des puces de texte plates, pas des logos colorés.

### 10.2 Pages et composants

| Projet     | Pages                                                                   | Composants clés                                                    |
|------------|-------------------------------------------------------------------------|--------------------------------------------------------------------|
| 1 · Python | Analyse, Performance, Erreurs, Modèle, À propos                         | ImageViewer, OverlayControls, SliceStrip, MetricTable, ArchDiagram |
| 2 · C++    | Vue d'ensemble, Recalage, Monte Carlo, Géométrie, Tests et CI, À propos | PointCloud3D, HistogramTRE, ErrorBand, MatrixView                  |
| 3 · MATLAB | Coupes, 3D, Mesures, Validation, Débruitage, Vidéo, À propos            | MPRViewer, MeshViewer, CompareGrid, ColorbarSVG, LabelLegend       |

### 10.3 Tokens de style (exemple projet 1)

```css
:root {
  --bg: #F2F5F3;  --surface: #FFFFFF;  --text: #1E2B29;
  --accent: #0E7C7B;  --alert: #E4572E;
  --line: #D9E0DD;  --muted: #5C6B68;
  --font-sans: 'IBM Plex Sans', system-ui, sans-serif;
  --font-mono: 'IBM Plex Mono', ui-monospace, monospace;
  --radius: 3px;
}
```

--line et --muted sont des neutres ajoutés aux 5 couleurs. Vérifie les contrastes (WCAG AA) avec un outil de contraste.

## 11. Choix des graphiques

Règle : on choisit le graphique d'après la question posée, pas d'après ce qui est joli. Pas de 3D pour des barres, pas de camemberts, pas de jet.

| Question                             | Meilleur graphique                                         | Détail                                                                                          |
|--------------------------------------|------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| Le modèle apprend-il ?               | Courbes de loss train et validation (lignes)               | Échelle log, marque de l'époque d'arrêt                                                         |
| Dice par image ?                     | Nuage de points par image + boîtes à moustaches par classe | Montre la dispersion, pas seulement la moyenne                                                  |
| Le Dice dépend-il de la taille ?     | Nuage de points, aire en échelle log                       | Courbe lissée par-dessus                                                                        |
| Quel changement a vraiment aidé ?    | Points ou barres avec écart-type sur 5 plis                | Expériences A à E                                                                               |
| Bénin ou malin ?                     | Matrice de confusion annotée + courbe ROC                  |                                                                                                 |
| Dispersion du TRE                    | Histogramme + lignes p50, p95, p99                         |                                                                                                 |
| Erreur selon le bruit                | Ligne avec bande p5 à p95, axes log                        |                                                                                                 |
| Erreur selon le nombre de fiduciaux  | Ligne avec marqueurs, pointillé = formule théorique        |                                                                                                 |
| Le FRE prédit-il le TRE ?            | Nuage de points FRE contre TRE                             | Résultat connu de la littérature (Fitzpatrick) : peu de corrélation, à vérifier sur tes données |
| Où le TRE est-il grand ?             | Carte de chaleur d'une coupe, colorbar en mm               | Séquentielle                                                                                    |
| Erreur de volume selon la résolution | Ligne en échelle log avec barres d'erreur                  | Voxel 0,5 / 1 / 2 mm                                                                            |
| Méthodes de débruitage               | Tableau + barres groupées (PSNR, SSIM, erreur de volume)   |                                                                                                 |
| Le bord est-il préservé ?            | Profil d'intensité le long d'une ligne                     |                                                                                                 |
| Mesures sur fantômes                 | Boîtes à moustaches (boxchart)                             |                                                                                                 |

## 12. Bibliothèques par projet

Utiliser toutes les bibliothèques n'est pas un objectif. Chacune doit avoir un rôle que tu sais expliquer.

**Python**

| Rôle                           | Bibliothèques                                                                                               |
|--------------------------------|-------------------------------------------------------------------------------------------------------------|
| Réseau                         | torch, torchvision                                                                                          |
| Données et augmentation        | numpy, Pillow, opencv-python, albumentations                                                                |
| Métriques                      | scikit-learn (F1, AUC, ROC, confusion), torchmetrics (Dice, IoU), scipy (Hausdorff avec directed_hausdorff) |
| Mesures et morphologie         | scikit-image (label, regionprops, remove_small_objects), scipy.ndimage                                      |
| Statistiques et tableaux       | pandas, scipy.stats (test de Wilcoxon, intervalles bootstrap)                                               |
| Figures du rapport             | matplotlib, seaborn                                                                                         |
| Explicabilité (classification) | pytorch-grad-cam, optionnel                                                                                 |
| API                            | fastapi, uvicorn, pydantic, python-multipart                                                                |
| Qualité et tests               | pytest, httpx, ruff                                                                                         |
| Export optionnel               | onnx, onnxruntime                                                                                           |
| MONAI                          | Seulement si tu manipules NIfTI ou DICOM ; pas pour le nom                                                  |

**C++** : Eigen, nlohmann/json, GoogleTest, CLI11 (arguments), OpenMP (Monte Carlo en parallèle, une graine par thread pour rester reproductible), Google Benchmark (optionnel), CMake presets.

**MATLAB** : Image Processing Toolbox (dicomreadVolume, imgaussfilt3, medfilt3, imbilatfilt, imnlmfilt, wiener2, graythresh, multithresh, imopen, bwconncomp, regionprops3, isosurface, volshow), MATLAB de base (patch, camlight, VideoWriter, exportgraphics, jsonencode, boxchart, stlwrite, matlab.unittest), Statistics and Machine Learning Toolbox (pca, fitlm, optionnel), Parallel Computing Toolbox (parfor, optionnel). Vérifie ce que tu as avec la commande ver.

## 13. Front-end : pile technique

Vérifie les versions actuelles à l'installation (React 18 ou 19, React Router 6 ou 7 en mode bibliothèque).

| Besoin                 | Choix                                                                                                            | Pourquoi                                                                                                                          |
|------------------------|------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|
| Build et dev           | Vite + React, JSX, modules ES                                                                                    | Démarrage rapide, rechargement à chaud                                                                                            |
| Style                  | CSS natif : variables, Grid, Flexbox, CSS Modules                                                                | **Pas de Bootstrap** : son apparence par défaut est exactement le look que tu veux éviter. Tailwind n'est pas nécessaire non plus |
| Composants accessibles | Radix UI (slider, onglets, infobulle, dialogue), sans style                                                      | Accessibles, habillés par toi                                                                                                     |
| Graphiques             | Recharts pour lignes et barres simples ; D3 (échelles, formes) pour boîtes, cartes de chaleur et bandes d'erreur | Chaque graphique du tableau de la section 11 est faisable                                                                         |
| 3D                     | three.js + @react-three/fiber + drei (STLLoader, OrbitControls)                                                  | Maillage osseux et nuage de points                                                                                                |
| Image                  | Canvas 2D, OffscreenCanvas, Web Worker pour fenêtre/niveau                                                       | Fluide même sur grosses coupes                                                                                                    |
| État global            | Redux Toolkit dans le projet 1 ; Context + useReducer dans les projets 2 et 3                                    | Redux seulement là où l'état est partagé et complexe                                                                              |
| Routage                | React Router (HashRouter pour GitHub Pages)                                                                      | Pages, routes imbriquées, filtres dans l'URL                                                                                      |
| Qualité                | ESLint, Prettier, Vitest + React Testing Library, Playwright (optionnel)                                         |                                                                                                                                   |
| Typage                 | JavaScript + PropTypes ou JSDoc ; TypeScript plus tard                                                           | Tu as demandé JSX et ES6                                                                                                          |
| Déploiement            | GitHub Pages (projets 2 et 3), Docker + nginx (projet 1)                                                         |                                                                                                                                   |

### 13.1 Structure du dossier dashboard

```text
dashboard/
  index.html  vite.config.js  package.json
  src/
    main.jsx  App.jsx
    app/store.js
    features/
      viewer/      viewerSlice.js  ViewerCanvas.jsx  WindowLevel.jsx  LabelLegend.jsx
      analysis/    analysisSlice.js  api.js  UploadZone.jsx
      metrics/     MetricTable.jsx  charts/
    components/    Layout.jsx  Sidebar.jsx  Panel.jsx  Stat.jsx
    routes/        index.jsx  ProtectedRoute.jsx
    hooks/         useZoomPan.js  useKeyboardNav.js  useDebounce.js
    styles/        tokens.css  base.css
    utils/         colormaps.js  format.js
  public/          données exportées (JSON, WebP)
  tests/
```

### 13.2 Performance (le « boost »)

- Découpage du code : React.lazy et Suspense par page, pour que three.js ne charge que sur la page 3D.

- React.memo, useMemo et useCallback sur les composants lourds (canvas, graphiques), mesurés avec le Profiler avant d'optimiser.

- Rendu canvas avec requestAnimationFrame ; fenêtre/niveau dans un Web Worker.

- Coupes pré-calculées en WebP, préchargées autour de la coupe courante.

- Longues listes (galerie d'erreurs) virtualisées.

- Anti-rebond (debounce) sur les curseurs et les filtres.

## 14. Back-end

**Projet 1 : FastAPI.**

```text
api/
  main.py        application, CORS, routes
  schemas.py     modèles Pydantic (entrées, sorties)
  inference.py   chargement du modèle, prétraitement, prédiction
  settings.py    chemins, seuils, version
  tests/         pytest + httpx
```

| Endpoint        | Rôle                                                                                               |
|-----------------|----------------------------------------------------------------------------------------------------|
| POST /predict   | Image → masque (PNG en base64), aire en pixels, probabilité bénin / malin, temps d'inférence en ms |
| GET /health     | Vivant ou non, modèle chargé ou non                                                                |
| GET /model-info | Version, date d'entraînement, hyperparamètres, métriques du jeu de test                            |

- Modèle chargé une seule fois au démarrage (lifespan), mode eval, torch.no_grad().

- Validation : type de fichier, taille maximale, dimensions ; erreurs renvoyées en JSON avec un code clair ; jamais de trace brute.

- Une réponse « incertaine » est affichée comme telle, jamais comme une certitude.

- Docker Compose : un service api, un service dashboard (nginx).

```text
Navigateur (React) ──fetch──► FastAPI ──► modèle PyTorch
      ▲                          │
      └──── JSON + PNG base64 ◄──┘
```

**Projets 2 et 3 : pas de serveur.** Le programme exporte ses résultats, le dashboard les lit.

| Projet     | Fichiers exportés                                                                                                   |
|------------|---------------------------------------------------------------------------------------------------------------------|
| 2 · C++    | results.json (version, date, unités mm, résumé), mc.csv (essais Monte Carlo), points.json (fiduciaux avant / après) |
| 3 · MATLAB | results.json (mesures, erreurs), coupes en WebP, bone.stl, rotation.mp4, validation.csv                             |

Chaque fichier porte un champ de version et d'unités, pour que le dashboard refuse un format qu'il ne comprend pas.

## 15. Concepts : où chacun sert vraiment

Cette liste est un programme d'apprentissage relié à du concret. N'utilise un concept que là où il est justifié, et sache dire pourquoi.

### 15.1 React

| Notion                                                      | Où dans le dashboard                                                                                             |
|-------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------|
| Composants fonction, JSX                                    | Toute l'interface                                                                                                |
| Props, props par défaut (valeurs par défaut des paramètres) | Stat, Panel, MetricTable                                                                                         |
| Props children                                              | Panel qui enveloppe un contenu                                                                                   |
| Props de style                                              | Passer une variable CSS (style={{ '--accent': color }})                                                          |
| Props avancées                                              | Spread {...rest}, render props, composant composé Tabs / Tab                                                     |
| Événements DOM                                              | onChange (curseur d'opacité), onPointerDown / Move / Up (déplacer l'image), onKeyDown (flèches)                  |
| Molette                                                     | Écouteur natif avec { passive: false } dans un useEffect (React ne permet pas preventDefault sur onWheel)        |
| Attributs JSX                                               | className, htmlFor, data-\*, aria-label, style en objet camelCase                                                |
| Fichiers et src                                             | import d'assets, dossier public/ pour les données                                                                |
| Boucles et conditions                                       | map avec key, filter, ternaire, &&, retour anticipé                                                              |
| useState, useReducer                                        | Formulaire d'upload, état local du viewer                                                                        |
| useEffect (avec nettoyage)                                  | Chargement des données, écouteurs d'événements                                                                   |
| useRef, useLayoutEffect                                     | Canvas, mesure de la taille avant affichage                                                                      |
| useMemo, useCallback, React.memo                            | Calculs et composants lourds                                                                                     |
| useContext                                                  | Thème et préférences (projets 2 et 3)                                                                            |
| Hooks personnalisés                                         | useZoomPan, useKeyboardNav, useDebounce                                                                          |
| HOC                                                         | Un seul exemple, withLoading(Component) qui gère chargement et erreur. Les hooks ont remplacé la plupart des HOC |
| Virtual DOM et réconciliation                               | Les clés (key) stables dans les listes de coupes                                                                 |
| Error Boundary                                              | Seul usage d'une classe : isole un graphique qui plante                                                          |
| Suspense et lazy                                            | Pages chargées à la demande                                                                                      |
| Débogage                                                    | React DevTools (arbre, Profiler), StrictMode, console des clés manquantes, onglet Réseau                         |

### 15.2 Redux (projet 1)

| Notion                                  | Où                                                                                                                                                                                 |
|-----------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Store (configureStore)                  | app/store.js                                                                                                                                                                       |
| Slice (createSlice : actions + reducer) | viewerSlice (fenêtre/niveau, coupe, opacité, labels visibles), analysisSlice (statut, résultat, erreur)                                                                            |
| Actions et reducers                     | Générés par createSlice, avec Immer                                                                                                                                                |
| Thunk (createAsyncThunk)                | analyzeImage : appel de POST /predict, états pending, fulfilled, rejected                                                                                                          |
| Middleware                              | Le middleware thunk par défaut ; un listener middleware qui sauvegarde les préférences du viewer ; vérification de sérialisation (ne mets jamais un File ou un Blob dans le store) |
| Sélecteurs (createSelector)             | Dice moyen, pires cas, valeurs mémoïsées                                                                                                                                           |
| React-Redux                             | Provider, useSelector, useDispatch                                                                                                                                                 |
| Débogage                                | Redux DevTools (historique des actions, retour dans le temps)                                                                                                                      |

Redux est justifié ici parce que le viewer, le panneau de mesures et la galerie partagent le même état. Dans les projets 2 et 3, Context + useReducer suffit : dis-le en entretien, c'est un bon signe de jugement.

### 15.3 React Router

| Notion                       | Où                                                                                        |
|------------------------------|-------------------------------------------------------------------------------------------|
| Routes et liens              | / , /analyse, /performance, /erreurs, /modele                                             |
| Routes imbriquées et Outlet  | Layout (barre latérale + en-tête) qui contient les pages via Outlet                       |
| Paramètres d'URL (useParams) | /erreurs/:caseId : détail d'un cas                                                        |
| useNavigate                  | Clic sur une vignette de la galerie → détail du cas                                       |
| useSearchParams              | Filtres partageables : ?classe=malin&tri=dice                                             |
| Route protégée               | /resultats exige qu'une analyse ait réussi, sinon redirection vers /analyse avec Navigate |
| Route inconnue               | Route \* vers une page 404                                                                |
| HashRouter                   | Pour GitHub Pages ; BrowserRouter derrière nginx                                          |

### 15.4 ES6 et JavaScript avancé

Les méthodes de tableau et les objets de correspondance remplacent les longs switch et les boucles for.

```js
// objet de correspondance au lieu d'un switch
const presets = { poumon: { w: 1600, l: -600 }, tissus: { w: 400, l: 40 }, os: { w: 1800, l: 400 } };
const { w, l } = presets[name] ?? presets.tissus;

// méthodes de tableau au lieu de boucles
const worst = cases.filter(c => c.dice < 0.5).sort((a, b) => a.dice - b.dice).slice(0, 6);
const meanDice = cases.reduce((s, c) => s + c.dice, 0) / cases.length;
const byClass = cases.reduce((acc, c) => ({ ...acc, [c.label]: [...(acc[c.label] ?? []), c.dice] }), {});

// déstructuration, chaînage optionnel, coalescence, spread, template literal
const { dice = 0, iou = 0 } = result?.metrics ?? {};
const next = { ...state, opacity: 0.4 };
const label = `${(dice * 100).toFixed(1)} %`;
```

À maîtriser aussi : map, filter, reduce, find, some, every, flatMap, includes, Object.entries et Object.fromEntries, Set et Map, async / await, Promise.all (charger plusieurs JSON en parallèle), modules (import / export nommés et par défaut), rest et spread, Intl.NumberFormat pour les nombres.

## 16. Outils : à quoi sert chacun

| Outil                     | Sert à                                                                                                                                    | Dans ton cas                                                                              |
|---------------------------|-------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------|
| VS Code                   | Éditeur gratuit avec extensions                                                                                                           | Éditeur principal                                                                         |
| Cursor                    | VS Code avec IA intégrée, plusieurs modèles au choix                                                                                      | Projet 2 (C++) dans ta répartition                                              |
| Claude Code               | Agent qui lit tout le dépôt, modifie plusieurs fichiers, lance les tests et les commandes ; en terminal, application ou extension VS Code | Exécuter les plans : code, tests, corrections                                             |
| Application Claude (chat) | Réfléchir, planifier, expliquer, relire, rédiger README et CV                                                                             | Planification et compréhension                                                            |
| ChatGPT                   | Second avis, explications                                                                                                                 | Pas d'éditeur ; ne laisse pas deux IA modifier le même fichier sans commit entre les deux |

**Quel modèle ?** Sonnet 5.5 est le modèle équilibré : il convient au code courant et à la planification. Opus 5.5 est plus fort sur les problèmes difficiles (bug subtil, choix d'architecture) mais plus lent et plus gourmand en quota. Haiku 5.5 sert aux petites tâches rapides. Je ne peux pas comparer fiablement avec les modèles proposés dans Cursor, car ils changent vite : essaie sur une vraie tâche. Un plan précis et des tests comptent davantage que le choix du modèle.

**Extensions VS Code utiles**

| Domaine             | Extensions                                                                                       |
|---------------------|--------------------------------------------------------------------------------------------------|
| Python              | Python, Pylance, Jupyter, Ruff                                                                   |
| C++                 | C/C++ (Microsoft), CMake Tools (clangd est une alternative : n'installe pas les deux analyseurs) |
| MATLAB              | MATLAB (MathWorks)                                                                               |
| JavaScript et React | ESLint, Prettier, ES7+ React/Redux snippets                                                      |
| API et Docker       | Docker, Thunder Client ou REST Client                                                            |
| Git                 | GitLens, GitHub Pull Requests                                                                    |
| Confort             | Error Lens, Path Intellisense                                                                    |

Dans le navigateur : React Developer Tools et Redux DevTools.

## 17. Travailler les 3 projets en même temps

- **Le GPU n'est pas touché** par l'application Claude, Claude Code ou Cursor : ils envoient du texte à un serveur distant. Le GPU travaille seulement pendant l'entraînement PyTorch (projet 1) et, très légèrement, pour le rendu 3D du navigateur.

- **Ce qui ralentit vraiment :** la RAM (MATLAB, Docker Desktop avec WSL2, Chrome et Cursor sont gourmands), le processeur si l'entraînement tourne sans GPU, et ton disque C: presque plein.

- **Les conflits de fichiers :** si deux outils modifient le même dossier, l'un écrase l'autre. Règle : un projet = un dossier = un outil, avec des commits fréquents et une branche par tâche.

- **Le quota :** l'usage de l'application Claude et de Claude Code compte sur ton abonnement. Je ne connais pas tes limites exactes : regarde-les dans les réglages.

**Organisation sans perdre de temps**

1.  Lance d'abord l'entraînement du projet 1 (c'est le plus long), puis travaille sur autre chose pendant qu'il tourne.

2.  Un outil par projet et par dossier : Claude Code sur le projet 1 (Python), Cursor sur le projet 2 (C++), Codex sur le projet 3 (MATLAB) ; MATLAB ouvert à part.

3.  Sans GPU, limite le processeur pendant l'entraînement : torch.set_num_threads(4) et DataLoader avec num_workers = 2, sinon tout le reste devient lent.

4.  Ferme Docker Desktop quand tu ne testes pas l'API.

5.  Garde tous les caches, venvs et données sur D:.

## Annexe A. Exemples de code (complètent les sections 13 et 15)

### A.1 Slice Redux Toolkit du viewer (projet 1)

Redux Toolkit utilise Immer : on peut écrire `s.opacity = ...` dans un reducer sans muter réellement l'état. Les appels à l'API passent par `createAsyncThunk` (section 15.2).

```js
import { createSlice, configureStore } from '@reduxjs/toolkit';

const viewerSlice = createSlice({
  name: 'viewer',
  initialState: { slice: 0, opacity: 0.4, hidden: [] },
  reducers: {
    setOpacity: (s, { payload }) => { s.opacity = payload; },
    toggleLabel: (s, { payload }) => {
      s.hidden = s.hidden.includes(payload)
        ? s.hidden.filter((l) => l !== payload)
        : [...s.hidden, payload];
    },
  },
});

export const { setOpacity, toggleLabel } = viewerSlice.actions;
export const store = configureStore({ reducer: { viewer: viewerSlice.reducer } });
```

### A.2 Routes imbriquées avec Outlet (GitHub Pages)

`Layout` affiche la barre latérale et un `<Outlet />` où s'insèrent les pages. `createHashRouter` évite l'erreur 404 au rechargement sur GitHub Pages ; derrière nginx, utilise `createBrowserRouter`.

```js
const router = createHashRouter([
  { path: '/', element: <Layout />, children: [
    { index: true, element: <AnalysePage /> },
    { path: 'performance', element: <PerformancePage /> },
    { path: 'erreurs/:caseId', element: <ErrorDetail /> },
  ]},
]);
```
