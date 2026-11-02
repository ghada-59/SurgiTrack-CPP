import { useMemo, useState } from "react";
import { findConfiguration, findTarget, showInt, showNumber } from "../format.js";
import { useResults } from "../ResultsContext.jsx";

export default function OverviewPage() {
  const { status, results, csvRows } = useResults();
  const counts = results?.fiducial_counts ?? [];
  const fles = results?.fle_levels_mm ?? [];
  const [fiducialCount, setFiducialCount] = useState(null);
  const [fle, setFle] = useState(null);
  const selectedCount = fiducialCount ?? (counts.includes(6) ? 6 : counts[0]);
  const selectedFle = fle ?? (fles.some((value) => Math.abs(value - 0.25) < 1e-9) ? 0.25 : fles[0]);
  const configuration = useMemo(
    () => findConfiguration(results, selectedCount, selectedFle),
    [results, selectedCount, selectedFle]
  );
  const target = findTarget(configuration, "centroid");
  const csvMatches =
    status === "ready" && results ? csvRows === results.csv_rows : null;

  return (
    <section>
      <p className="reg-lead">
        Figures on this page are read from <code>results/results.json</code> and{" "}
        <code>results/mc.csv</code>. Nothing here is typed in by hand.
      </p>
      <div className="reg-stats">
        <Stat label="Trials / configuration" value={showInt(results?.trials_per_configuration)} />
        <Stat label="Configurations" value={showInt(results?.configurations?.length)} />
        <Stat label="OpenMP threads" value={showInt(results?.openmp_threads)} />
        <Stat label="Seed" value={results ? String(results.seed) : "—"} />
        <Stat label="Max |rel| TRE²" value={showNumber(results?.max_abs_relative_error_tre2, 8)} />
        <Stat label="CSV data rows" value={showInt(csvRows)} />
        <Stat
          label="CSV rows match JSON"
          value={csvMatches === null ? "—" : csvMatches ? "yes" : "no"}
        />
        <Stat label="Generated (UTC)" value={results?.generated_at_utc ?? "—"} />
      </div>
      <div className="reg-controls">
        <label>
          Fiducials
          <select
            value={selectedCount ?? ""}
            onChange={(event) => setFiducialCount(Number(event.target.value))}
          >
            {counts.map((count) => (
              <option key={count} value={count}>
                {count}
              </option>
            ))}
          </select>
        </label>
        <label>
          FLE (mm)
          <select value={selectedFle ?? ""} onChange={(event) => setFle(Number(event.target.value))}>
            {fles.map((value) => (
              <option key={value} value={value}>
                {value}
              </option>
            ))}
          </select>
        </label>
      </div>
      <h2>Centroid target for the selection</h2>
      <div className="reg-stats">
        <Stat label="Mean TRE (mm)" value={showNumber(target?.mean_tre_mm)} />
        <Stat label="Median / p50 (mm)" value={showNumber(target?.p50_tre_mm)} />
        <Stat label="p95 (mm)" value={showNumber(target?.p95_tre_mm)} />
        <Stat label="p99 (mm)" value={showNumber(target?.p99_tre_mm)} />
        <Stat label="RMS TRE (mm)" value={showNumber(target?.rms_tre_mm)} />
        <Stat label="Fitzpatrick RMS (mm)" value={showNumber(target?.fitzpatrick_rms_tre_mm)} />
        <Stat label="Relative TRE²" value={showNumber(target?.relative_error_tre2, 8)} />
        <Stat label="Mean FRE (mm)" value={showNumber(configuration?.fre?.mean_mm)} />
      </div>
    </section>
  );
}

function Stat({ label, value }) {
  return (
    <article className="reg-stat">
      <span>{label}</span>
      <strong>{value || "—"}</strong>
    </article>
  );
}
