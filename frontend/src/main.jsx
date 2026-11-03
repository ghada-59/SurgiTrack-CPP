import React, { Suspense, lazy } from "react";
import { createRoot } from "react-dom/client";
import { createHashRouter, RouterProvider } from "react-router-dom";
import App from "./App.jsx";
import DashboardLayout from "./registration/DashboardLayout.jsx";
import OverviewPage from "./registration/pages/OverviewPage.jsx";
import MonteCarloPage from "./registration/pages/MonteCarloPage.jsx";
import GeometryPage from "./registration/pages/GeometryPage.jsx";
import TestsPage from "./registration/pages/TestsPage.jsx";
import AboutPage from "./registration/pages/AboutPage.jsx";
import NotFoundPage from "./registration/pages/NotFoundPage.jsx";
import "./styles.css";
import "./registration/dashboard.css";

const RegistrationPage = lazy(() => import("./registration/pages/RegistrationPage.jsx"));

const router = createHashRouter([
  { path: "/", element: <App /> },
  {
    element: <DashboardLayout />,
    children: [
      { path: "overview", element: <OverviewPage /> },
      {
        path: "registration",
        element: (
          <Suspense fallback={<p className="reg-note">Loading the 3D view…</p>}>
            <RegistrationPage />
          </Suspense>
        ),
      },
      { path: "monte-carlo", element: <MonteCarloPage /> },
      { path: "geometry", element: <GeometryPage /> },
      { path: "tests", element: <TestsPage /> },
      { path: "about", element: <AboutPage /> },
      { path: "*", element: <NotFoundPage /> },
    ],
  },
]);

createRoot(document.getElementById("root")).render(
  <React.StrictMode>
    <RouterProvider router={router} />
  </React.StrictMode>
);
