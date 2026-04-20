import React from "react";
import "./HomePage.css";

export default function Events() {
  const sampleEvents = [
    { id: 1, title: "Campus Fair", date: "2026-05-01", location: "Plaza" },
    { id: 2, title: "Guest Lecture", date: "2026-05-08", location: "Auditorium" },
  ];

  const goBack = () => {
    window.history.pushState({}, "", "/");
    window.dispatchEvent(new PopStateEvent("popstate"));
  };

  return (
    <div className="events-page" style={{ padding: 24 }}>
      <button onClick={goBack} style={{ marginBottom: 12 }}>
        ← Back
      </button>
      <h1>Events</h1>
      <p>List of upcoming on campus events.</p>

      <div style={{ display: "grid", gap: 12, marginTop: 12 }}>
        {sampleEvents.map((e) => (
          <div key={e.id} style={{ border: "1px solid #ddd", padding: 12, borderRadius: 8 }}>
            <h3 style={{ margin: 0 }}>{e.title}</h3>
            <small>
              {e.date} • {e.location}
            </small>
          </div>
        ))}
      </div>
    </div>
  );
}
