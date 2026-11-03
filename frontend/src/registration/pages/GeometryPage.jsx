import { useMemo, useState } from "react";
import { findConfiguration, findTarget, showNumber } from "../format.js";
import { useResults } from "../ResultsContext.jsx";

export default function GeometryPage() {
  const { results, points } = useResults();
  const fles = results?.fle_levels_mm ?? [];
  const [fle, setFle] = useState(null);
  const [targetName, setTargetName] = useState("centroid");
  const selectedFle = fle ?? (fles.some((value) => Math.abs(value - 0.25) < 1e-9) ? 0.25 : fles[0]);
  const names = results?.configurations?.[0]?.targets?.map((target) => target.name) ?? [];
  const activeName = names.includes(targetName) ? targetName : names[0];
  const series = useMemo(() => {
    if (!results || !activeName) return [];
    return (results.fiducial_counts ?? [])
      .map((count) => {
        const configuration = findConfiguration(results, count, selectedFle);
        const target = findTarget(configuration, activeName);
        return target
          ? { count, mean: target.mean_tre_mm, formula: target.fitzpatrick_rms_tre_mm }
          : null;
      })
      .filter(Boolean);
  }, [results, selectedFle, activeName]);
  const fiducials = points?.reference_fiducials_mm ?? [];

  return (
    <section>
      <p className="reg-lead">
        Subsets use the first N columns of the reference cloud in <code>points.json</code>.
        The curve uses the selected FLE from <code>results.json</code>.
      </p>
      <div className="reg-controls">
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
            {names.map((name) => (
              <option key={name} value={name}>{name}</option>
            ))}
          </select>
        </label>
      </div>
      <CountChart series={series} />
      <h2>Reference fiducials, XY (mm)</h2>
      <FiducialPlot points={fiducials} />
      <table className="reg-table">
        <thead>
          <tr><th>N</th><th>Mean TRE (mm)</th><th>Fitzpatrick RMS (mm)</th></tr>
        </thead>
        <tbody>
          {series.map((item) => (
            <tr key={item.count}>
              <td>{item.count}</td>
              <td>{showNumber(item.mean)}</td>
              <td>{showNumber(item.formula)}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </section>
  );
}

function CountChart({ series }) {
  if (series.length === 0) return <p className="reg-note">Geometry curve: —</p>;
  const maxTre = Math.max(...series.map((item) => Math.max(item.mean, item.formula)), 1e-9) * 1.15;
  const minN = series[0].count;
  const maxN = series[series.length - 1].count;
  const span = Math.max(maxN - minN, 1);
  const xOf = (count) => 50 + ((count - minN) / span) * 540;
  const yOf = (tre) => 230 - (tre / maxTre) * 190;
  return (
    <svg className="reg-chart" viewBox="0 0 640 280" role="img" aria-label="Mean TRE against fiducial count">
      <line x1="50" y1="230" x2="590" y2="230" stroke="#1B2631" />
      <line x1="50" y1="40" x2="50" y2="230" stroke="#1B2631" />
      <polyline fill="none" stroke="#35566F" strokeWidth="2" points={series.map((item) => `${xOf(item.count)},${yOf(item.mean)}`).join(" ")} />
      <polyline fill="none" stroke="#B23A2E" strokeWidth="2" strokeDasharray="6 4" points={series.map((item) => `${xOf(item.count)},${yOf(item.formula)}`).join(" ")} />
      {series.map((item) => (
        <g key={item.count}>
          <circle cx={xOf(item.count)} cy={yOf(item.mean)} r="4" fill="#35566F" />
          <text x={xOf(item.count) - 4} y="252" fill="#1B2631" fontSize="12">{item.count}</text>
        </g>
      ))}
      <text x="250" y="272" fill="#1B2631" fontSize="12">Fiducial count</text>
    </svg>
  );
}

function FiducialPlot({ points }) {
  if (points.length === 0) return <p className="reg-note">Fiducial plot: —</p>;
  const xs = points.map((point) => point[0]);
  const ys = points.map((point) => point[1]);
  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);
  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);
  const span = Math.max(maxX - minX, maxY - minY, 1);
  const project = (point) => {
    const x = 40 + ((point[0] - minX) / span) * 360;
    const y = 400 - ((point[1] - minY) / span) * 360;
    return [x, y];
  };
  return (
    <svg className="reg-chart" viewBox="0 0 460 440" role="img" aria-label="Fiducial XY projection">
      {points.map((point, index) => {
        const [x, y] = project(point);
        return (
          <g key={index}>
            <circle cx={x} cy={y} r="5" fill={index < 6 ? "#35566F" : "#D98E04"} />
            <text x={x + 8} y={y + 4} fill="#1B2631" fontSize="12">{index}</text>
          </g>
        );
      })}
    </svg>
  );
}
