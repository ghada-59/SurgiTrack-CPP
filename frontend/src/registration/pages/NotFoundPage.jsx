import { Link } from "react-router-dom";

export default function NotFoundPage() {
  return (
    <section>
      <h2>Page not found</h2>
      <p><Link to="/overview">Back to the overview</Link></p>
    </section>
  );
}
