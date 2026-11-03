import { createContext, useContext, useEffect, useReducer } from "react";

const ResultsContext = createContext(null);

const initialState = {
  status: "loading",
  results: null,
  points: null,
  csvText: "",
  csvRows: null,
  ctestText: "",
  error: "",
};

function reducer(state, action) {
  switch (action.type) {
    case "ready":
      return { ...state, status: "ready", error: "", ...action.payload };
    case "error":
      return { ...state, status: "error", error: action.message };
    default:
      return state;
  }
}

async function loadText(url) {
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`${url} returned ${response.status}`);
  }
  return response.text();
}

export function ResultsProvider({ children }) {
  const [state, dispatch] = useReducer(reducer, initialState);

  useEffect(() => {
    let cancelled = false;
    const base = import.meta.env.BASE_URL;
    Promise.all([
      loadText(`${base}data/results.json`),
      loadText(`${base}data/points.json`),
      loadText(`${base}data/mc.csv`),
      loadText(`${base}data/ctest_release.txt`),
    ])
      .then(([resultsText, pointsText, csvText, ctestText]) => {
        if (cancelled) return;
        const results = JSON.parse(resultsText);
        const points = JSON.parse(pointsText);
        if (results.units !== "mm" || results.version !== "1.0") {
          throw new Error("results.json version or units were not recognised");
        }
        if (points.units !== "mm" || points.version !== "1.0") {
          throw new Error("points.json version or units were not recognised");
        }
        const csvRows = csvText.trim() ? csvText.trim().split(/\r?\n/).length - 1 : 0;
        dispatch({
          type: "ready",
          payload: { results, points, csvText, csvRows, ctestText },
        });
      })
      .catch((error) => {
        if (!cancelled) {
          dispatch({ type: "error", message: error.message || "Could not load results" });
        }
      });
    return () => {
      cancelled = true;
    };
  }, []);

  return <ResultsContext.Provider value={state}>{children}</ResultsContext.Provider>;
}

export function useResults() {
  const value = useContext(ResultsContext);
  if (!value) {
    throw new Error("useResults must be used inside ResultsProvider");
  }
  return value;
}
