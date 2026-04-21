import TravelWidget from "./TravelWidget";
import "./TravelTimePage.css";

function TravelTimePage() {
  const goBack = () => {
    window.history.pushState({}, "", "/");
    window.dispatchEvent(new PopStateEvent("popstate"));
  };

  return (
    <div className="travel-time-page" style={{ padding: 24 }}>
      <button type="button" onClick={goBack} style={{ marginBottom: 12 }}>
        ← Back
      </button>
      <h1>Travel Time Algorithm</h1>
      <p>Select two or more campus locations to estimate the total time for your trip.</p>
      <TravelWidget fullPage />
    </div>
  );
}

export default TravelTimePage;
