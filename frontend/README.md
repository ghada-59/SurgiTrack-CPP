# SurgiTrack — Spatial Safety Console

An optional React/Vite visual companion for exploring the repository's coordinate-transform and safety-geofencing concepts.

## Start the interface

From the repository root:

~~~bash
cd frontend
npm install
npm run dev
~~~

Open the local URL printed by Vite, normally http://127.0.0.1:5173/.

No backend is required. The safety view computes an illustrative straight-line trajectory in the browser. The registration pages (`#/overview`, `#/registration`, `#/monte-carlo`, `#/geometry`, `#/tests`, `#/about`) read the C++ exports in `../results` (`results.json`, `points.json`, `mc.csv`, and `ctest_release.txt`). They do not invent metrics: a missing value is shown as —.

## Important scope

This UI is a separate mathematical illustration. It does not invoke the C++ executable, and it does not validate or replace the C++ classes. The displayed trajectory and protected structure are synthetic; the projection is schematic, not a true 3D rendering. This is an educational software exercise, not a medical device or surgical-planning tool.
