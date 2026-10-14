import { useMemo, useState } from "react";
import { Activity, AlertOctagon, RotateCcw, ShieldCheck, Target, Waypoints } from "lucide-react";

const initial = { radius: 5, buffer: 3, startX: -16, endX: 16, startY: 10, endY: -10, steps: 80 };
const fmt = (value) => Number(value).toFixed(2);

export default function App() {
  const [settings, setSettings] = useState(initial);
  const [sample, setSample] = useState(0);
  const path = useMemo(() => Array.from({ length: settings.steps + 1 }, (_, i) => {
    const t = i / settings.steps;
    const x = settings.startX + (settings.endX - settings.startX) * t;
    const y = settings.startY + (settings.endY - settings.startY) * t;
    const distance = Math.hypot(x, y);
    const status = distance <= settings.radius ? "CRITICAL" : distance <= settings.radius + settings.buffer ? "WARNING" : "SAFE";
    return { i, progress: t * 100, x, y, z: 0, distance, status };
  }), [settings]);
  const current = path[Math.min(sample, path.length - 1)];
  const update = (key, value) => {
    setSettings((old) => ({ ...old, [key]: Number(value) }));
    setSample(0);
  };
  const reset = () => { setSettings(initial); setSample(0); };
  const exportCsv = () => {
    const rows = [["sample", "progress_percent", "x_mm", "y_mm", "z_mm", "distance_mm", "status"], ...path.map(p => [p.i, fmt(p.progress), fmt(p.x), fmt(p.y), fmt(p.z), fmt(p.distance), p.status])];
    const blob = new Blob([rows.map(row => row.join(",")).join("\n")], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "surgitrack-synthetic-trajectory.csv";
    link.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="shell">
      <aside className="sidebar">
        <a className="brand" href="#top"><span className="brand-icon"><Target size={22} /></span><span><b>SURGITRACK</b><small>SPATIAL SAFETY CONSOLE</small></span></a>
        <div className="side-label">SIMULATION</div>
        <div className="nav-active"><Waypoints size={16} /> Trajectory workspace <span>01</span></div>
        <div className="side-note"><small>GEOMETRY MODEL</small><b>2D radial model</b><span>Euclidean distance</span><span>Synthetic straight path</span></div>
        <div className="side-status"><ShieldCheck size={15} /><div><b>Educational simulation</b><small>No patient or device connection</small></div></div>
        <div className="side-footer">C++ core · React visual companion</div>
      </aside>
      <main id="top">
        <header className="topbar"><div><span>MEDICAL NAVIGATION CONCEPTS</span><i>/</i><strong>Geometric safety simulation</strong></div><button className="reset" onClick={reset}><RotateCcw size={14} /> Reset scenario</button></header>
        <section className="hero"><div><span className="eyebrow">SAFETY GEOFENCING / SYNTHETIC DATA</span><h1>Know the<br /><em>clearance.</em></h1><p>Explore a synthetic trajectory and inspect how its distance from an illustrative protected centre changes across sampled positions.</p></div><div className="hero-badge"><Activity size={30} /><span>XY / MM</span></div></section>
        <div className="disclaimer"><AlertOctagon size={17} /><p><b>Simulation, not surgical guidance.</b> This browser companion uses a simplified 2D mathematical model. Values are illustrative and are not validated clinical limits.</p></div>
        <div className="section-head"><span>01</span><h2>Trajectory workspace</h2><small>{path.length} SYNTHETIC SAMPLES</small></div>
        <section className="workspace">
          <div className="scene-card">
            <div className="scene-head"><div><small>XY VIEW</small><h3>Trajectory and protected radius</h3></div><span className={"state-chip " + current.status.toLowerCase()}><i />{current.status}</span></div>
            <div className="scene">
              <svg viewBox="0 0 540 340" role="img" aria-label="Synthetic two-dimensional trajectory and protected zone">
                <defs><pattern id="grid" width="24" height="24" patternUnits="userSpaceOnUse"><path d="M 24 0 L 0 0 0 24" fill="none" className="grid-stroke" strokeWidth=".7" /></pattern></defs>
                <rect width="540" height="340" fill="url(#grid)" rx="8" />
                <line x1="30" y1="170" x2="510" y2="170" className="axes" /><line x1="270" y1="20" x2="270" y2="320" className="axes" />
                <circle cx="270" cy="170" r={settings.radius * 7} className="protected-zone" />
                <circle cx="270" cy="170" r={(settings.radius + settings.buffer) * 7} className="buffer-ring" />
                <polyline points={path.map(p => (270 + p.x * 7) + "," + (170 - p.y * 7)).join(" ")} className="trajectory-line" />
                <circle cx={270 + current.x * 7} cy={170 - current.y * 7} r="7" className={"tool-point " + current.status.toLowerCase()} />
                <circle cx="270" cy="170" r="4" className="center-point" />
                <text x="282" y="158" className="annotation">Protected centre</text>
                <text x="18" y="326" className="axis-label">SCHEMATIC VIEW · NOT TO SCALE</text>
              </svg>
              <div className="scene-legend"><span><i className="leg-tool" />Trajectory</span><span><i className="leg-zone" />Protected radius</span><span><i className="leg-buffer" />Warning buffer</span></div>
            </div>
            <div className="scrubber"><div><label htmlFor="progress">Trajectory position</label><b>{fmt(current.progress)}%</b></div><input id="progress" type="range" min="0" max={settings.steps} value={sample} onChange={e => setSample(Number(e.target.value))} /><div className="scrub-foot"><span>START</span><span>Sample {current.i + 1} / {path.length}</span><span>END</span></div></div>
          </div>
          <div className="readout"><div className="readout-head"><small>POSITION READOUT</small><span className={"status-label " + current.status.toLowerCase()}>{current.status}</span></div><div className="distance-display"><b>{fmt(current.distance)}</b><span>mm <small>distance to centre</small></span></div><div className="coords"><div><small>X</small><b>{fmt(current.x)}</b><span>mm</span></div><div><small>Y</small><b>{fmt(current.y)}</b><span>mm</span></div><div><small>Z</small><b>0.00</b><span>mm</span></div></div><div className="readout-rule" /><div className="rule-row"><span>Protected radius</span><b>{settings.radius} mm</b></div><div className="rule-row"><span>Warning threshold</span><b>{settings.radius + settings.buffer} mm</b></div><div className="readout-message"><span className={"message-icon " + current.status.toLowerCase()}><ShieldCheck size={17} /></span><div><b>{current.status === "CRITICAL" ? "Inside illustrative radius" : current.status === "WARNING" ? "Within warning buffer" : "Outside warning boundary"}</b><p>State is calculated from the sample's Euclidean distance to the origin.</p></div></div></div>
        </section>
        <div className="section-head section-gap"><span>02</span><h2>Geometry parameters</h2><small>EDIT VALUES TO RECOMPUTE</small></div>
        <section className="parameter-grid">
          <article className="parameter-card"><div className="parameter-title"><span><Target size={17} /></span><div><b>Protected structure</b><small>Illustrative radial boundary</small></div></div>
            <label>Safety radius <b>{settings.radius} mm</b><input type="range" min="2" max="10" step=".5" value={settings.radius} onChange={e => update("radius", e.target.value)} /></label>
            <label>Warning buffer <b>{settings.buffer} mm</b><input type="range" min="0" max="8" step=".5" value={settings.buffer} onChange={e => update("buffer", e.target.value)} /></label>
          </article>
          <article className="parameter-card"><div className="parameter-title"><span><Waypoints size={17} /></span><div><b>Trajectory endpoints</b><small>Linear path in the XY plane</small></div></div>
            <label>Start X <b>{settings.startX} mm</b><input type="range" min="-30" max="30" value={settings.startX} onChange={e => update("startX", e.target.value)} /></label>
            <label>End X <b>{settings.endX} mm</b><input type="range" min="-30" max="30" value={settings.endX} onChange={e => update("endX", e.target.value)} /></label>
            <label>Start Y <b>{settings.startY} mm</b><input type="range" min="-30" max="30" value={settings.startY} onChange={e => update("startY", e.target.value)} /></label>
            <label>End Y <b>{settings.endY} mm</b><input type="range" min="-30" max="30" value={settings.endY} onChange={e => update("endY", e.target.value)} /></label>
          </article>
        </section>
        <div className="section-head section-gap"><span>03</span><h2>Path assessment</h2><small>GEOMETRIC COUNTS · NOT CLINICAL VALIDATION</small></div>
        <section className="assessment-grid">
          {["SAFE", "WARNING", "CRITICAL"].map(status => { const count = path.filter(p => p.status === status).length; return <div className="assessment-card" key={status}><small>{status} SAMPLES</small><b>{count}</b><span>{status === "SAFE" ? "Outside warning boundary" : status === "WARNING" ? "Within warning buffer" : "Inside protected radius"}</span><div className={"assess-line " + status.toLowerCase() + "-line"} style={{ width: (count / path.length * 100) + "%" }} /></div>; })}
          <div className="assessment-card"><small>MINIMUM CLEARANCE</small><b>{fmt(Math.min(...path.map(p => p.distance)) - settings.radius)} <i>mm</i></b><span>Minimum distance minus radius</span></div>
        </section>
        <section className="table-card"><div className="table-head"><div><small>DISCRETE TRAJECTORY SAMPLES</small><h3>Computed positions and states</h3></div><button onClick={exportCsv}>Export CSV</button></div><div className="table-wrap"><table><thead><tr><th>Sample</th><th>Progress</th><th>X (mm)</th><th>Y (mm)</th><th>Distance (mm)</th><th>Status</th></tr></thead><tbody>{path.filter((p, i) => i % 8 === 0 || i === path.length - 1).map(p => <tr key={p.i} onClick={() => setSample(p.i)} className={p.i === sample ? "row-selected" : ""}><td>{String(p.i + 1).padStart(2, "0")}</td><td>{fmt(p.progress)}%</td><td>{fmt(p.x)}</td><td>{fmt(p.y)}</td><td>{fmt(p.distance)}</td><td><span className={"table-status " + p.status.toLowerCase()}>{p.status}</span></td></tr>)}</tbody></table></div></section>
        <div className="disclaimer footnote"><ShieldCheck size={16} /><p>The existing C++ repository contains its own coordinate-transform and safety-geofencing classes. This separate visual companion does not invoke or validate that compiled implementation. Do not use for clinical planning.</p></div>
        <footer><span>SURGITRACK <i>/</i> SPATIAL SAFETY CONSOLE</span><span>GEOMETRIC SIMULATION · NOT A MEDICAL DEVICE</span></footer>
      </main>
    </div>
  );
}
