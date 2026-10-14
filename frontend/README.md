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

No backend is required for this visual companion. It computes an illustrative straight-line trajectory, Euclidean distance to a spherical protected zone, and SAFE / WARNING / CRITICAL states in the browser. Parameters can be adjusted and the sampled path exported as CSV.

## Important scope

This UI is a separate mathematical illustration. It does not invoke the C++ executable, and it does not validate or replace the C++ classes. The displayed trajectory and protected structure are synthetic; the projection is schematic, not a true 3D rendering. This is an educational software exercise, not a medical device or surgical-planning tool.
