import { useMemo, useState } from "react";
import { findConfiguration, findTarget, showNumber } from "../format.js";
import { useResults } from "../ResultsContext.jsx";

export default function MonteCarloPage() {
  const { results, csvText } = useResults();
  const counts = results?.fiducial_counts ?? [];
  const fles = results?.fle_levels_mm ?? [];
  const [fiducialCount, setFiducialCount] = useState(null);
  const [fle, setFle] = useState(null);
  const [targetName, setTargetName] = useState("centroid");
  const selectedCount = fiducialCount ?? (counts.includes(6) ? 6 : counts[0]);
  const selectedFle = fle ?? (fles.some((value) => Math.abs(value - 0.25) < 1e-9) ? 0.25 : fles[0]);
  const configuration = findConfiguration(results, selectedCount, selectedFle);
  const targetNames = configuration?.targets?.map((target) => target.name) ?? [];
  const activeName = targetNames.includes(targetName) ? targetName : targetNames[0];
  const target = findTarget(configuration, activeName);
  const preview = useMemo(() => csvText.split(/\r?\n/).slice(0, 6).join("\n"), [csvText]);
  const series = useMemo(() => {
    if (!results || !activeName) return [];
    return fles
      .map((value) => {
        const row = findConfiguration(results, selectedCount, value);
        const item = findTarget(row, activeName);
        return item
          ? {
              fle: value,
              mean: item.mean_tre_mm,
              formula: item.fitzpatrick_rms_tre_mm,
              p5: item.p5_tre_mm,
              p95: item.p95_tre_mm,
            }
          : null;
      })
      .filter(Boolean);
  }, [results, fles, selectedCount, activeName]);

  return (
    <section>
      <p className="reg-lead">
        Histogram counts and summary statistics come from <code>results.json</code>. The noise
        chart uses a log scale. Its band is the measured p5 to p95 of the same trials, or — if
        either quantile is missing.
      </p>
      <div className="reg-controls">
        <label>
          Fiducials
          <input
            type="range"
            min={counts[0] ?? 0}
            max={counts[counts.length - 1] ?? 0}
            step="1"
            value={selectedCount ?? 0}
            onChange={(event) => setFiducialCount(Number(event.target.value))}
          />
          <b>{selectedCount ?? "—"}</b>
        </label>
        <label>
          FLE (mm)
          <select value={selectedFle ?? ""} onChange={(event) => setFle(Number(event.target.value))}>
            {fles.map((value) => (
              <option key={value} value={value}>{value}</option>
            ))}
          </select>
        </label>
        <label>
          Target
          <select value={activeName ?? ""} onChange={(event) => setTargetName(event.target.value)}>
            {targetNames.map((name) => (
              <option key={name} value={name}>{name}</option>
            ))}
          </select>
        </label>
      </div>
      <Histogram target={target} />
      <h2>Mean TRE against FLE</h2>
      <p className="reg-note">Solid: measured mean. Dashed: Fitzpatrick RMS. Shaded: measured p5–p95. Both axes are log10.</p>
      <LineChart series={series} />
      <h2>Selected summary</h2>
      <table className="reg-table">
        <tbody>
          <tr><th>Mean TRE (mm)</th><td>{showNumber(target?.mean_tre_mm)}</td></tr>
          <tr><th>p5 (mm)</th><td>{showNumber(target?.p5_tre_mm)}</td></tr>
          <tr><th>p50 (mm)</th><td>{showNumber(target?.p50_tre_mm)}</td></tr>
          <tr><th>p95 (mm)</th><td>{showNumber(target?.p95_tre_mm)}</td></tr>
          <tr><th>p99 (mm)</th><td>{showNumber(target?.p99_tre_mm)}</td></tr>
          <tr><th>Fitzpatrick RMS (mm)</th><td>{showNumber(target?.fitzpatrick_rms_tre_mm)}</td></tr>
          <tr><th>Relative TRE²</th><td>{showNumber(target?.relative_error_tre2, 8)}</td></tr>
          <tr><th>p5–p95 band (mm)</th><td>{formatBand(target)}</td></tr>
        </tbody>
      </table>
      <h2>mc.csv preview</h2>
      <pre className="reg-pre">{preview || "—"}</pre>
    </section>
  );
}

function Histogram({ target }) {
  const counts = target?.histogram?.counts;
  if (!Array.isArray(counts) || counts.length === 0) {
    return <p className="reg-note">Histogram: —</p>;
  }
  const peak = Math.max(...counts, 1);
  const width = target.histogram.bin_width_mm;
  return (
    <svg className="reg-chart" viewBox="0 0 640 280" role="img" aria-label="TRE histogram">
      {counts.map((count, index) => {
        const barHeight = (count / peak) * 220;
        return (
          <rect
            key={index}
            x={40 + index * (560 / counts.length)}
            y={240 - barHeight}
            width={560 / counts.length - 1}
            height={barHeight}
            fill="#35566F"
          />
        );
      })}
      <Marker target={target} field="p50_tre_mm" color="#1B2631" label="p50" width={width} bins={counts.length} />
      <Marker target={target} field="p95_tre_mm" color="#D98E04" label="p95" width={width} bins={counts.length} />
      <Marker target={target} field="p99_tre_mm" color="#B23A2E" label="p99" width={width} bins={counts.length} />
      <text x="40" y="268" fill="#1B2631" fontSize="12">0</text>
      <text x="520" y="268" fill="#1B2631" fontSize="12">{showNumber(width * counts.length, 3)} mm</text>
    </svg>
  );
}

function Marker({ target, field, color, label, width, bins }) {
  const value = target?.[field];
  if (typeof value !== "number" || !(width > 0)) return null;
  const x = 40 + Math.min(bins - 0.5, value / width) * (560 / bins);
  return (
    <g>
      <line x1={x} y1="16" x2={x} y2="240" stroke={color} strokeDasharray="4 3" />
      <text x={x + 4} y="28" fill={color} fontSize="12">{label}</text>
    </g>
  );
}

function formatBand(target) {
  const low = target?.p5_tre_mm;
  const high = target?.p95_tre_mm;
  if (typeof low !== "number" || typeof high !== "number" || !Number.isFinite(low) || !Number.isFinite(high)) {
    return "—";
  }
  return `${showNumber(low)} – ${showNumber(high)}`;
}

function positive(value) {
  return typeof value === "number" && Number.isFinite(value) && value > 0;
}

function LineChart({ series }) {
  const usable = series.filter(
    (item) => positive(item.fle) && positive(item.mean) && positive(item.formula) && positive(item.p5) && positive(item.p95)
  );
  if (usable.length === 0) return <p className="reg-note">Log curve: —</p>;
  const log = (value) => Math.log10(value);
  const fleLogs = usable.map((item) => log(item.fle));
  const treLogs = usable.flatMap((item) => [item.mean, item.formula, item.p5, item.p95].map(log));
  const minFle = Math.min(...fleLogs);
  const maxFle = Math.max(...fleLogs);
  const minTre = Math.min(...treLogs);
  const maxTre = Math.max(...treLogs);
  const fleSpan = Math.max(maxFle - minFle, 1e-9);
  const treSpan = Math.max(maxTre - minTre, 1e-9);
  const xOf = (fle) => 70 + ((log(fle) - minFle) / fleSpan) * 500;
  const yOf = (tre) => 220 - ((log(tre) - minTre) / treSpan) * 170;
  const meanPoints = usable.map((item) => `${xOf(item.fle)},${yOf(item.mean)}`).join(" ");
  const formulaPoints = usable.map((item) => `${xOf(item.fle)},${yOf(item.formula)}`).join(" ");
  const band = `${usable.map((item) => `${xOf(item.fle)},${yOf(item.p95)}`).join(" ")} ${[...usable].reverse().map((item) => `${xOf(item.fle)},${yOf(item.p5)}`).join(" ")}`;
  return (
    <svg className="reg-chart" viewBox="0 0 640 280" role="img" aria-label="Mean TRE against FLE on log axes">
      <line x1="70" y1="220" x2="570" y2="220" stroke="#1B2631" />
      <line x1="70" y1="50" x2="70" y2="220" stroke="#1B2631" />
      <polygon points={band} fill="#35566F" opacity="0.18" />
      <polyline fill="none" stroke="#35566F" strokeWidth="2" points={meanPoints} />
      <polyline fill="none" stroke="#B23A2E" strokeWidth="2" strokeDasharray="6 4" points={formulaPoints} />
      {usable.map((item) => (
        <circle key={item.fle} cx={xOf(item.fle)} cy={yOf(item.mean)} r="4" fill="#35566F" />
      ))}
      <text x="250" y="258" fill="#1B2631" fontSize="12">log10 FLE (mm)</text>
      <text x="78" y="42" fill="#35566F" fontSize="12">log10 TRE</text>
      <text x="160" y="42" fill="#B23A2E" fontSize="12">Fitzpatrick RMS</text>
    </svg>
  );
}
