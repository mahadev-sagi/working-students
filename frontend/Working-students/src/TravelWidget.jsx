import { useEffect, useMemo, useState } from 'react';
import { apiUrl } from './api';
import './TravelWidget.css';

function TravelWidget() {
  const [locations, setLocations] = useState([]);
  const [fromCode, setFromCode] = useState('');
  const [toCode, setToCode] = useState('');
  const [result, setResult] = useState(null);
  const [error, setError] = useState('');

  const options = useMemo(() => {
    return locations.map((loc) => ({ value: loc.code, label: `${loc.code} - ${loc.name}` }));
  }, [locations]);

  useEffect(() => {
    let isMounted = true;

    async function loadLocations() {
      try {
        setError('');
        const res = await fetch(apiUrl('/travel/locations'));
        if (!res.ok) {
          throw new Error('Failed to load locations');
        }

        const data = await res.json();
        const list = Array.isArray(data) ? data : [];

        if (!isMounted) return;
        setLocations(list);

        if (list.length >= 2) {
          setFromCode((prev) => prev || list[0].code);
          setToCode((prev) => prev || list[1].code);
        }
      } catch (err) {
        if (isMounted) {
          setError(err.message || 'Unable to load travel data');
        }
      }
    }

    loadLocations();

    return () => {
      isMounted = false;
    };
  }, []);

  const handleCalculate = async () => {
    if (!fromCode || !toCode) {
      setError('Please select both locations.');
      return;
    }

    try {
      setError('');
      setResult(null);

      const res = await fetch(apiUrl('/travel/calculate'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ from_code: fromCode, to_code: toCode }),
      });

      if (!res.ok) {
        throw new Error('Failed to calculate route');
      }

      const data = await res.json();
      setResult(data);
    } catch (err) {
      setError(err.message || 'Unable to calculate route');
    }
  };

  return (
    <section className="travel-widget">
      <h3>Campus Travel</h3>
      <div className="travel-controls">
        <select value={fromCode} onChange={(event) => setFromCode(event.target.value)}>
          <option value="">From</option>
          {options.map((opt) => (
            <option key={`from-${opt.value}`} value={opt.value}>
              {opt.label}
            </option>
          ))}
        </select>

        <select value={toCode} onChange={(event) => setToCode(event.target.value)}>
          <option value="">To</option>
          {options.map((opt) => (
            <option key={`to-${opt.value}`} value={opt.value}>
              {opt.label}
            </option>
          ))}
        </select>

        <button type="button" onClick={handleCalculate}>Estimate</button>
      </div>

      {error && <p className="travel-error">{error}</p>}
      {result && (
        <p className="travel-result">
          {result.found
            ? `~${result.travel_time_minutes} min (${result.distance_meters} m)`
            : 'No route found'}
        </p>
      )}
    </section>
  );
}

export default TravelWidget;
