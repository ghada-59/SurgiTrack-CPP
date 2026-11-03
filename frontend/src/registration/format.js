export function showNumber(value, digits = 6) {
  if (typeof value !== "number" || !Number.isFinite(value)) return "—";
  const fixed = value.toFixed(digits);
  if (value !== 0 && Number(fixed) === 0) return value.toExponential(6);
  return fixed;
}

export function showInt(value) {
  if (typeof value !== "number" || !Number.isFinite(value)) return "—";
  return String(Math.trunc(value));
}

export function findConfiguration(results, fiducialCount, fle) {
  if (!results) return null;
  return (
    results.configurations.find(
      (configuration) =>
        configuration.n_fiducials === fiducialCount &&
        Math.abs(configuration.fle_rms_mm - fle) < 1e-9
    ) ?? null
  );
}

export function findTarget(configuration, name) {
  return configuration?.targets?.find((target) => target.name === name) ?? null;
}
