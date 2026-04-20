import React, { useState, useEffect } from "react";
import "./HomePage.css";

export default function WorkSchedule() {
  const dayOptions = ["Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"];

  const readStored = () => {
    try {
      const raw = localStorage.getItem("workShifts");
      if (!raw) return [];
      return JSON.parse(raw);
    } catch (e) {
      console.warn("Failed to read workShifts", e);
      return [];
    }
  };

  const [shifts, setShifts] = useState(() => readStored());
  const [day, setDay] = useState("Mon");
  const [startTime, setStartTime] = useState("");
  const [endTime, setEndTime] = useState("");
  const [place, setPlace] = useState("");
  const [error, setError] = useState("");

  useEffect(() => {
    try {
      const raw = localStorage.getItem("workShifts");
      if (raw) setShifts(JSON.parse(raw));
    } catch (e) {
      
    }
  }, []);

  const persistAndNotify = (next) => {
    setShifts(next);
    try {
      localStorage.setItem("workShifts", JSON.stringify(next));
      window.dispatchEvent(new Event("workUpdated"));
    } catch (e) {
      console.warn("Failed to persist workShifts", e);
    }
  };

  const handleAdd = (e) => {
    e.preventDefault();
    if (!startTime || !endTime || !place || !day) {
      setError("Please fill day, start/end times and place.");
      return;
    }

    const time = `${startTime} - ${endTime}`;
    const newShift = { id: Date.now(), day, time, place };
    const next = [newShift, ...shifts];
    persistAndNotify(next);
    setStartTime("");
    setEndTime("");
    setPlace("");
    setError("");
  };

  const handleDelete = (id) => {
    const next = shifts.filter((s) => s.id !== id);
    persistAndNotify(next);
  };

  const goBack = () => {
    window.history.pushState({}, "", "/");
    window.dispatchEvent(new PopStateEvent("popstate"));
  };

  return (
    <div className="work-page" style={{ padding: 24 }}>
      <button onClick={goBack} style={{ marginBottom: 12 }}>
        ← Back
      </button>
      <h1>Work Schedule</h1>
      <p>View and manage your upcoming shifts here. Add a shift and it will appear on the weekly calendar.</p>

      <form onSubmit={handleAdd} style={{ display: "grid", gap: 8, maxWidth: 560 }}>
        <label>
          Day
          <select value={day} onChange={(e) => setDay(e.target.value)} style={{ padding: 8, marginTop: 4 }}>
            {dayOptions.map((d) => (
              <option key={d} value={d}>{d}</option>
            ))}
          </select>
        </label>

        <div style={{ display: "flex", gap: 8 }}>
          <label style={{ flex: 1 }}>
            Start
            <input type="time" value={startTime} onChange={(e) => setStartTime(e.target.value)} style={{ width: "100%", padding: 8, marginTop: 4 }} />
          </label>
          <label style={{ flex: 1 }}>
            End
            <input type="time" value={endTime} onChange={(e) => setEndTime(e.target.value)} style={{ width: "100%", padding: 8, marginTop: 4 }} />
          </label>
        </div>

        <label>
          Place
          <input type="text" value={place} onChange={(e) => setPlace(e.target.value)} placeholder="e.g. Library" style={{ width: "100%", padding: 8, marginTop: 4 }} />
        </label>

        {error && <div style={{ color: "#c00" }}>{error}</div>}

        <div style={{ display: "flex", gap: 8 }}>
          <button type="submit" style={{ padding: "8px 12px" }}>Add Shift</button>
        </div>
      </form>

      <div style={{ display: "grid", gap: 12, marginTop: 20 }}>
        {shifts.map((s) => (
          <div key={s.id} style={{ border: "1px solid #ddd", padding: 12, borderRadius: 8, display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <div>
              <h3 style={{ margin: 0 }}>{s.day}</h3>
              <small>{s.time} • {s.place}</small>
            </div>
            <div>
              <button onClick={() => handleDelete(s.id)} style={{ background: '#f44336', color: 'white', padding: '6px 10px', borderRadius: 6, border: 'none' }}>Delete</button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
