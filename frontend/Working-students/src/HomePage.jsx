import { useMemo, useState, useEffect } from 'react';
import './HomePage.css';

const assignmentTypes = [
  { key: 'homework', label: 'Homework', hoursPerAssignment: 2 },
  { key: 'quiz', label: 'Quiz', hoursPerAssignment: 1 },
  { key: 'essay', label: 'Essay', hoursPerAssignment: 5 },
  { key: 'researchPaper', label: 'Research Paper', hoursPerAssignment: 8 },
  { key: 'labReport', label: 'Lab Report', hoursPerAssignment: 4 },
  { key: 'discussionPost', label: 'Discussion Post', hoursPerAssignment: 1 },
];

const initialCounts = assignmentTypes.reduce((acc, type) => {
  acc[type.key] = '';
  return acc;
}, {});

function HomePage({ user, onLogout }) {
  const [counts, setCounts] = useState(initialCounts);
  const [totalHours, setTotalHours] = useState(null);
  const [routeStops, setRouteStops] = useState(['REI', 'MALA']);
  const [activeTab, setActiveTab] = useState('calculator');
  const [travelLocations, setTravelLocations] = useState([]);
  const [loadingLocations, setLoadingLocations] = useState(true);
  const [travelResult, setTravelResult] = useState({ error: '', totalMinutes: 0, segments: [] });

  // Load travel locations from API
  useEffect(() => {
    fetch('/api/travel/locations', { cache: 'no-store' })
      .then(async (res) => {
        if (!res.ok) {
          throw new Error(`Failed to load locations: ${res.status}`);
        }

        const text = await res.text();
        if (!text) {
          return [];
        }

        return JSON.parse(text);
      })
      .then(data => {
        setTravelLocations(Array.isArray(data) ? data : []);
        setLoadingLocations(false);
      })
      .catch(err => {
        console.error('Failed to load locations:', err);
        setTravelLocations([]);
        setLoadingLocations(false);
      });
  }, []);

  const rows = useMemo(
    () =>
      assignmentTypes.map((type) => ({
        ...type,
        count: Number.parseInt(counts[type.key], 10) || 0,
      })),
    [counts],
  );

  const travelOptions = useMemo(
    () =>
      travelLocations
        .map((location) => ({
          code: location.code,
          label: `${location.name} (${location.code})`,
        }))
        .sort((a, b) => a.label.localeCompare(b.label)),
    [travelLocations],
  );

  // Calculate travel time using API
  const calculateTravelTime = async () => {
    if (routeStops.length < 2) {
      setTravelResult({ error: 'Please select at least 2 locations', totalMinutes: 0, segments: [] });
      return;
    }

    let totalMinutes = 0;
    let error = '';
    const segments = [];

    try {
      for (let index = 0; index < routeStops.length - 1; index += 1) {
        const fromCode = routeStops[index];
        const toCode = routeStops[index + 1];

        const response = await fetch('/api/travel/calculate', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ from_code: fromCode, to_code: toCode })
        });

        const route = await response.json();

        if (!route.found) {
          error = `No route found from ${fromCode} to ${toCode}.`;
          break;
        }

        segments.push({ fromCode, toCode, minutes: route.travel_time_minutes });
        totalMinutes += route.travel_time_minutes;
      }
    } catch (err) {
      error = 'Failed to calculate travel time. Please try again.';
    }

    setTravelResult({ totalMinutes, error, segments });
  };

  // Recalculate when route stops change
  useEffect(() => {
    if (routeStops.length >= 2 && travelLocations.length > 0) {
      calculateTravelTime();
    } else {
      setTravelResult({ error: '', totalMinutes: 0, segments: [] });
    }
  }, [routeStops, travelLocations]);

  const handleChange = (key, value) => {
    if (value === '') {
      setCounts((prev) => ({ ...prev, [key]: '' }));
      return;
    }

    const parsed = Number.parseInt(value, 10);
    if (Number.isNaN(parsed) || parsed < 0) return;
    setCounts((prev) => ({ ...prev, [key]: String(parsed) }));
  };

  const handleCalculate = (event) => {
    event.preventDefault();
    const hours = rows.reduce(
      (sum, row) => sum + row.count * row.hoursPerAssignment,
      0,
    );
    setTotalHours(hours);
  };

  const handleStopChange = (index, value) => {
    setRouteStops((prev) => {
      const next = [...prev];
      next[index] = value;
      return next;
    });
  };

  const addRouteStop = () => {
    if (travelOptions.length === 0) return;
    setRouteStops((prev) => [...prev, travelOptions[0].code]);
  };

  const removeRouteStop = (index) => {
    setRouteStops((prev) => prev.filter((_, idx) => idx !== index));
  };

  return (
    <section className="calculator-card">
      <header className="homepage-header">
        <div className="profile-container">
          <span>Profile</span>
          <div className="email-tooltip">{user.email}</div>
        </div>
        <button onClick={onLogout} className="logout-button">
          Log Out
        </button>
      </header>
      
      <div className="tab-buttons">
        <button 
          className={`tab-button ${activeTab === 'calculator' ? 'active' : ''}`}
          onClick={() => setActiveTab('calculator')}
        >
          Assignment Calculator
        </button>
        <button 
          className={`tab-button ${activeTab === 'travel' ? 'active' : ''}`}
          onClick={() => setActiveTab('travel')}
        >
          Travel Time Calculator
        </button>
      </div>

      {activeTab === 'calculator' && (
        <>
          <h2>Assignment Hours Calculator</h2>
          <p>Enter how many assignments you have for each type.</p>
          <form onSubmit={handleCalculate}>
            {rows.map((row) => (
              <label key={row.key} className="input-row">
                <span>{row.label}</span>
                <input
                  type="number"
                  min="0"
                  step="1"
                  value={counts[row.key]}
                  onChange={(event) => handleChange(row.key, event.target.value)}
                  placeholder="0"
                />
              </label>
            ))}
            <button type="submit">Calculate</button>
          </form>
          {totalHours !== null && (
            <p className="result">Total assignment hours: {totalHours}</p>
          )}
        </>
      )}

      {activeTab === 'travel' && (
        <>
          <h2>Travel Time Calculator</h2>
          <p>Select multiple campus locations and calculate the total travel time between them.</p>
          <div className="travel-section">
            {loadingLocations ? (
              <p>Loading campus locations...</p>
            ) : travelOptions.length === 0 ? (
              <p className="result error-message">Campus locations are unavailable right now.</p>
            ) : (
              <>
            {routeStops.map((stopCode, index) => (
              <div key={index} className="travel-row">
                <label>
                  Location {index + 1}
                  <select
                    value={stopCode}
                    onChange={(event) => handleStopChange(index, event.target.value)}
                  >
                    {travelOptions.map((option) => (
                      <option key={option.code} value={option.code}>
                        {option.label}
                      </option>
                    ))}
                  </select>
                </label>
                {routeStops.length > 1 && (
                  <button
                    type="button"
                    className="remove-stop-button"
                    onClick={() => removeRouteStop(index)}
                  >
                    ×
                  </button>
                )}
              </div>
            ))}
            <div className="travel-actions">
              <button type="button" onClick={addRouteStop}>
                Add Location
              </button>
            </div>
            {travelResult.error ? (
              <p className="result error-message">{travelResult.error}</p>
            ) : (
              <>
                <div className="travel-summary">
                  <p className="result">Total travel time: {travelResult.totalMinutes} minutes</p>
                  {travelResult.segments.length > 0 && (
                    <ul className="travel-segments">
                      {travelResult.segments.map((segment, idx) => (
                        <li key={idx}>
                          {segment.fromCode} → {segment.toCode}: {segment.minutes} min
                        </li>
                      ))}
                    </ul>
                  )}
                </div>
              </>
            )}
              </>
            )}
          </div>
        </>
      )}
    </section>
  );
}

export default HomePage;
