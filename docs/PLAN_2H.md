# SurgiTrack-CPP v2 : plan de 2 heures

> Planning de 2 heures pour le **noyau** du projet. Le détail technique (architecture, validation, dashboard) est dans `docs/PLAN.md`, qui l'emporte en cas d'écart. Prompt mis à jour pour coller à PLAN.md : convention Eigen « un point = une colonne » (Matrix3Xd), 3 à 10 fiduciaux, FLE 0,1 à 1,0 mm, 10 000 essais, exports JSON.


Projet C++ pour les stages Stryker (navigation chirurgicale, robotique, IA)

## Objectif

Étendre ton repo SurgiTrack-CPP avec un module de recalage rigide par points et une étude Monte Carlo de la précision. C'est une simulation éducative, pas un système clinique. Elle montre trois choses aux recruteurs : de la géométrie 3D en C++ propre, des tests automatiques et une mesure honnête de l'erreur.

## Livrable final

- Une fonction registerRigid (SVD avec Eigen) qui renvoie rotation, translation et FRE

- Des tests GoogleTest et une CI verte sous Linux

- Une application Monte Carlo qui écrit un fichier CSV avec FRE et TRE

- Un README avec la méthode, les résultats et les limites

## Plan minute par minute

| Temps       | Étape                                                                                    | Résultat attendu       |
|-------------|------------------------------------------------------------------------------------------|------------------------|
| 0:00 à 0:10 | Cadrage : lire le repo, créer une branche, proposer les nouveaux fichiers                | Structure validée      |
| 0:10 à 0:40 | Recalage rigide : centroïdes, matrice de covariance, SVD, correction des réflexions, FRE | registerRigid compile  |
| 0:40 à 1:00 | Tests : transformation connue retrouvée, cas dégénérés, réflexion                        | Tous les tests passent |
| 1:00 à 1:30 | Monte Carlo : bruit gaussien sur les fiducielles, FRE et TRE sur des milliers d'essais   | CSV de résultats       |
| 1:30 à 1:45 | Figure : histogramme ou courbe TRE selon la distance aux fiducielles                     | Image pour le README   |
| 1:45 à 2:00 | README, CI, commit final                                                                 | Repo publiable         |

## Le prompt exact pour Claude Code

Colle ce texte au début de la session, dans le dossier du repo.

```cpp
You are helping me extend my existing repo SurgiTrack-CPP (C++17, Eigen, CMake, GoogleTest, GitHub Actions). Goal: add a point-based rigid registration module and a Monte Carlo accuracy study. This is an educational simulation, not a clinical system, and the README must say so.

Work in small steps. After each step, build, run the tests and show me the output. Do not move to the next step if a test fails.

Step 1. Read the repo. Summarise structure, build and tests in 10 lines. Propose the new files (header, source, tests, a Monte Carlo app, docs). Wait for my OK.

Step 2. Implement registerRigid(const Eigen::Matrix3Xd& moving, const Eigen::Matrix3Xd& fixed) (3 x N, one point per column, units mm) returning rotation, translation and FRE (RMS error over the fiducials). Use centroids plus SVD (Kabsch/Arun method). Force det(R) = +1 to avoid reflections. Throw std::invalid_argument for fewer than 3 points, size mismatch, or collinear points (sigma2 < 1e-6 * sigma1); warn if sigma2 < 1e-2 * sigma1.

Step 3. Write GoogleTest tests: (a) a known rotation and translation is recovered without noise (error below 1e-9), (b) FRE is about zero on perfect data, (c) a mirrored point set does not produce a reflection, (d) every invalid input throws.

Step 4. Write a Monte Carlo app. 3 to 10 non-collinear fiducials inside a 100 mm region, several target points at different distances from the fiducial centroid. Add Gaussian noise (FLE, total RMS, in 0.1, 0.25, 0.5 and 1.0 mm) to the fiducials, register, then compute FRE and TRE (target registration error). Run 10,000 trials per configuration with a seeded std::mt19937_64 (one seed per OpenMP thread) so results are reproducible. Write mc.csv, results.json and points.json, print mean, median and the 50th/95th/99th percentiles, and compare TRE with Fitzpatrick's formula (docs/PLAN.md section 3.5, about 5 % agreement).

Step 5. Update the README: goal, math summary, how to build and run, results table, one figure, and a Limitations section (simulation only, no real sensor, no clinical use). Make sure the CI runs the tests.

Rules: no new heavy dependencies, no code I cannot explain, comment the math briefly, keep functions small. At the end, list what you tested and what you did not.
```

## Vérifications avant de publier

- Lance les tests toi-même en local et vérifie que la CI est verte

- Relance l'application Monte Carlo avec la même graine et vérifie que tu obtiens les mêmes chiffres

- Regarde tes résultats : compare la FRE et la TRE, et vérifie comment la TRE change avec la distance aux fiducielles. Écris ce que tu observes réellement dans le README

- Ne garde dans le README aucune phrase que tu ne peux pas expliquer à l'oral

## Questions d'entretien probables

- Pourquoi utiliser la SVD, et pourquoi forcer le déterminant à +1 ?

- Pourquoi la FRE seule ne suffit pas à juger la précision d'une navigation ?

- Que se passe-t-il si les points sont colinéaires ou trop peu nombreux ?

- Comment passer à un recalage de surface (ICP) ou au temps réel ?
