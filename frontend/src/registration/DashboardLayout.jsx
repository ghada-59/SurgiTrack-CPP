import { NavLink, Outlet } from "react-router-dom";
import { ResultsProvider, useResults } from "./ResultsContext.jsx";

const links = [
  ["overview", "Vue d'ensemble"],
  ["registration", "Recalage"],
  ["monte-carlo", "Monte Carlo"],
  ["geometry", "Géométrie"],
  ["tests", "Tests et CI"],
  ["about", "À propos"],
];

function Shell() {
  const { status, error } = useResults();
  return (
    <div className="reg-shell">
      <aside className="reg-nav">
        <p className="reg-brand">SurgiTrack</p>
        <p className="reg-brand-sub">Recalage rigide</p>
        <nav>
          {links.map(([path, label]) => (
            <NavLink key={path} to={`/${path}`}>
              {label}
            </NavLink>
          ))}
          <NavLink to="/">Safety console</NavLink>
        </nav>
        <p className="reg-academic">Projet académique, non clinique</p>
      </aside>
      <div className="reg-main">
        <header className="reg-top">
          <div>
            <p className="reg-kicker">SurgiTrack-CPP</p>
            <h1>Registration study</h1>
          </div>
          <p className="reg-badge">Academic simulation</p>
        </header>
        {status === "loading" && <p className="reg-note">Loading measured results…</p>}
        {status === "error" && (
          <p className="reg-alert">
            Results did not load ({error}). Displayed metrics stay at —.
          </p>
        )}
        <Outlet />
      </div>
    </div>
  );
}

export default function DashboardLayout() {
  return (
    <ResultsProvider>
      <Shell />
    </ResultsProvider>
  );
}
