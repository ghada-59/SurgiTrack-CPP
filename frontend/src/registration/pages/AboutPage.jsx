export default function AboutPage() {
  return (
    <section className="reg-about">
      <h2>Scope</h2>
      <p>Academic rigid-registration exercise. Not a medical device and not for surgical guidance.</p>
      <h2>What the pages use</h2>
      <ul>
        <li>C++17, Eigen, GoogleTest, OpenMP, CMake</li>
        <li>React, Vite, React Router, Context and useReducer</li>
        <li>three.js through react-three-fiber for the fiducial view</li>
        <li>Exported JSON, CSV, and the Release ctest log</li>
      </ul>
      <h2>Limits</h2>
      <ul>
        <li>Synthetic fiducials and Gaussian noise only</li>
        <li>No tracker, no DICOM, no patient</li>
        <li>A missing export is shown as —</li>
        <li>p5 and p95 are Hyndman–Fan type 7 percentiles of the exported trials</li>
      </ul>
    </section>
  );
}
