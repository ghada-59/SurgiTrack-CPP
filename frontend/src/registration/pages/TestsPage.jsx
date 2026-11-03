import { useResults } from "../ResultsContext.jsx";

export default function TestsPage() {
  const { ctestText, status } = useResults();
  return (
    <section>
      <p className="reg-lead">
        The log below is <code>results/ctest_release.txt</code> from the Release build on this
        machine. GitHub Actions for this branch was not run here, so the remote status is —.
        Durations that are absent from the log are —.
      </p>
      <p>Load status: {status === "ready" ? "file loaded" : status === "error" ? "—" : status}</p>
      <pre className="reg-pre">{ctestText || "—"}</pre>
    </section>
  );
}
