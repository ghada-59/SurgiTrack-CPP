import React, { useMemo, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { OrbitControls } from "@react-three/drei";
import { showNumber } from "../format.js";
import { useResults } from "../ResultsContext.jsx";

class ViewBoundary extends React.Component {
  constructor(props) {
    super(props);
    this.state = { failed: false };
  }

  static getDerivedStateFromError() {
    return { failed: true };
  }

  render() {
    if (this.state.failed) {
      return <p className="reg-alert">The 3D view failed. The table still uses the exported points.</p>;
    }
    return this.props.children;
  }
}

function Cloud({ points, color }) {
  const geometry = useMemo(() => {
    const positions = new Float32Array(points.length * 3);
    points.forEach((point, index) => {
      positions[index * 3] = point[0];
      positions[index * 3 + 1] = point[1];
      positions[index * 3 + 2] = point[2];
    });
    return positions;
  }, [points]);
  return (
    <points>
      <bufferGeometry>
        <bufferAttribute
          attach="attributes-position"
          count={geometry.length / 3}
          array={geometry}
          itemSize={3}
        />
      </bufferGeometry>
      <pointsMaterial color={color} size={6} sizeAttenuation={false} />
    </points>
  );
}

function maybeCloud(visible, points, color) {
  if (!visible || points.length === 0) return null;
  return <Cloud points={points} color={color} />;
}

export default function RegistrationPage() {
  const { points } = useResults();
  const [showMoving, setShowMoving] = useState(true);
  const [showFixed, setShowFixed] = useState(true);
  const [showRegistered, setShowRegistered] = useState(true);
  const moving = points?.moving_mm ?? [];
  const fixed = points?.fixed_mm ?? [];
  const registered = points?.registered_mm ?? [];

  return (
    <section>
      <p className="reg-lead">
        Noiseless registration of the first {points?.n_fiducials ?? "—"} reference fiducials,
        from <code>results/points.json</code>. Axes are millimetres.
      </p>
      <div className="reg-controls">
        <label><input type="checkbox" checked={showMoving} onChange={(event) => setShowMoving(event.target.checked)} /> Moving</label>
        <label><input type="checkbox" checked={showFixed} onChange={(event) => setShowFixed(event.target.checked)} /> Fixed</label>
        <label><input type="checkbox" checked={showRegistered} onChange={(event) => setShowRegistered(event.target.checked)} /> Registered</label>
      </div>
      <div className="reg-stage">
        <ViewBoundary>
          <Canvas camera={{ position: [160, 120, 180], fov: 40 }}>
            <color attach="background" args={["#f7f8f9"]} />
            <ambientLight intensity={0.8} />
            <axesHelper args={[70]} />
            {maybeCloud(showMoving, moving, "#35566F")}
            {maybeCloud(showFixed, fixed, "#B23A2E")}
            {maybeCloud(showRegistered, registered, "#D98E04")}
            <OrbitControls />
          </Canvas>
        </ViewBoundary>
      </div>
      <div className="reg-split">
        <Matrix title="Recovered R" matrix={points?.rotation} />
        <div>
          <h2>Recovered t (mm)</h2>
          <p className="reg-mono">{formatVector(points?.translation_mm, 15)}</p>
          <h2>True t (mm)</h2>
          <p className="reg-mono">{formatVector(points?.true_translation_mm, 15)}</p>
          <p>FRE RMS: {showNumber(points?.fre_rms_mm, 8)} mm</p>
          <p>det(R): {showNumber(points?.determinant, 16)}</p>
        </div>
      </div>
      <h2>Residual per fiducial (mm)</h2>
      <table className="reg-table">
        <thead>
          <tr>
            <th>Index</th>
            <th>Moving</th>
            <th>Fixed</th>
            <th>Registered</th>
            <th>Residual norm</th>
          </tr>
        </thead>
        <tbody>
          {moving.map((point, index) => (
            <tr key={index}>
              <td>{index}</td>
              <td>{formatVector(point)}</td>
              <td>{formatVector(fixed[index])}</td>
              <td>{formatVector(registered[index])}</td>
              <td>{showNumber(points?.residuals_mm?.[index], 8)}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </section>
  );
}

function formatVector(value, digits = 6) {
  if (!Array.isArray(value) || value.length < 3) return "—";
  return value.map((component) => showNumber(component, digits)).join(", ");
}

function Matrix({ title, matrix }) {
  return (
    <div>
      <h2>{title}</h2>
      {Array.isArray(matrix) ? (
        <table className="reg-table">
          <tbody>
            {matrix.map((row, rowIndex) => (
              <tr key={rowIndex}>
                {row.map((value, columnIndex) => (
                  <td key={columnIndex}>{showNumber(value, 8)}</td>
                ))}
              </tr>
            ))}
          </tbody>
        </table>
      ) : (
        <p>—</p>
      )}
    </div>
  );
}
