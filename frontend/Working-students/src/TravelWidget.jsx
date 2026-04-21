import { useEffect, useMemo, useState } from 'react';
import { apiUrl } from './api';
import './TravelWidget.css';

function TravelWidget({ title, fullPage = false }) {
  const [locations, setLocations] = useState([]);
  const [stopCodes, setStopCodes] = useState(['', '']);
  const [result, setResult] = useState(null);
  const [error, setError] = useState('');

  const options = useMemo(() => {
    return [...locations]
      .sort((a, b) => a.name.localeCompare(b.name))
      .map((loc) => ({ value: loc.code, label: `${loc.name} (${loc.code})` }));
  }, [locations]);

  const optionByCode = useMemo(() => {
    return options.reduce((map, opt) => {
      map[opt.value] = opt.label;
      return map;
    }, {});
  }, [options]);

  const selectedLabels = stopCodes.map((code) => optionByCode[code] || code);
  const totalTripMinutes = result?.total_trip_time_minutes ?? 0;

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
          setStopCodes((prev) => {
            const next = [...prev];
            if (!next[0]) next[0] = list[0].code;
            if (!next[1]) next[1] = list[1].code;
            return next;
          });
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

  const updateStop = (index, code) => {
    setStopCodes((prev) => prev.map((existing, idx) => (idx === index ? code : existing)));
    setResult(null);
  };

  const addStop = () => {
    setStopCodes((prev) => [...prev, '']);
    setResult(null);
  };

  const removeStop = (index) => {
    setStopCodes((prev) => prev.filter((_, idx) => idx !== index));
    setResult(null);
  };

  const handleCalculate = async () => {
    const selectedCodes = stopCodes.filter(Boolean);
    if (selectedCodes.length < 2) {
      setError('Please select at least two locations.');
      return;
    }

    try {
      setError('');
      setResult(null);

      const res = await fetch(apiUrl('/travel/calculate'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ locations: selectedCodes }),
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
    <section className={`travel-widget${fullPage ? ' travel-widget-full' : ''}`}>
      {title ? <h2>{title}</h2> : null}
      <div className="travel-stops">
        {stopCodes.map((code, index) => (
          <div className="travel-stop-row" key={`stop-${index}`}>
            <select
              value={code}
              onChange={(event) => updateStop(index, event.target.value)}
              aria-label={`Stop ${index + 1}`}
            >
              <option value={index === 0 ? '' : ''}>
                {index === 0 ? 'Starting location' : index === stopCodes.length - 1 ? 'Destination' : `Stop ${index + 1}`}
              </option>
              {options.map((opt) => (
                <option key={`stop-${index}-${opt.value}`} value={opt.value}>
                  {opt.label}
                </option>
              ))}
            </select>

            {stopCodes.length > 2 && (
              <button
                className="travel-remove-stop"
                type="button"
                onClick={() => removeStop(index)}
                aria-label={`Remove stop ${index + 1}`}
                title="Remove stop"
              >
                -
              </button>
            )}
          </div>
        ))}
      </div>

      <div className="travel-controls">
        <button type="button" onClick={addStop}>Add Stop</button>
        <button type="button" onClick={handleCalculate}>Estimate</button>
      </div>

      {error && <p className="travel-error">{error}</p>}
      {result && (
        result.found ? (
          <>
            <div className="travel-result">
              <div>
                <span>Route</span>
                <strong>{selectedLabels.filter(Boolean).join(' to ')}</strong>
              </div>
              <div>
                <span>Total trip time</span>
                <strong>{totalTripMinutes} min</strong>
              </div>
              <div>
                <span>Distance</span>
                <strong>{result.distance_meters} m</strong>
              </div>
            </div>
            {Array.isArray(result.legs) && result.legs.length > 0 && (
              <div className="travel-legs">
                {result.legs.map((leg, index) => (
                  <div className="travel-leg" key={`leg-${index}`}>
                    <strong>{selectedLabels[index]} to {selectedLabels[index + 1]}</strong>
                    <span>{leg.total_trip_time_minutes} min total, {leg.distance_meters} m</span>
                  </div>
                ))}
              </div>
            )}
          </>
        ) : (
          <p className="travel-result travel-result-empty">No route found</p>
        )
      )}
    </section>
  );
}

export default TravelWidget;
