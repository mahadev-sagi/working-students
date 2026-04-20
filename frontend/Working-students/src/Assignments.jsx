import React, { useState } from "react";
import "./HomePage.css";

export default function Assignments() {
  const initialAssignments = [
    { id: 1, title: "Math Homework 5", due: "2026-04-25", className: "MATH 2010" },
    { id: 2, title: "CS Project Milestone", due: "2026-05-02", className: "CIS 4930" },
  ];

  const readStored = () => {
    try {
      const raw = localStorage.getItem("assignments");
      if (!raw) return initialAssignments;
      return JSON.parse(raw);
    } catch (e) {
      console.warn("Failed to read assignments from storage", e);
      return initialAssignments;
    }
  };

  const [assignments, setAssignments] = useState(() => readStored());
  const [title, setTitle] = useState("");
  const [due, setDue] = useState("");
  const [className, setClassName] = useState("");
  const [error, setError] = useState("");

  const goBack = () => {
    window.history.pushState({}, "", "/");
    window.dispatchEvent(new PopStateEvent("popstate"));
  };

  const clearForm = () => {
    setTitle("");
    setDue("");
    setClassName("");
    setError("");
  };

  const handleAdd = (e) => {
    e.preventDefault();
    if (!title.trim() || !due.trim() || !className.trim()) {
      setError("Please enter title, due date and class name.");
      return;
    }

    const newAssignment = {
      id: Date.now(),
      title: title.trim(),
      due: due.trim(),
      className: className.trim(),
    };

    setAssignments((prev) => {
      const next = [newAssignment, ...prev];
      try {
  localStorage.setItem("assignments", JSON.stringify(next));
  window.dispatchEvent(new Event("assignmentsUpdated"));
      } catch (e) {
        console.warn("Failed to save assignments", e);
      }
      return next;
    });
    clearForm();
  };

  

  return (
    <div className="assignments-page" style={{ padding: 24 }}>
      <button onClick={goBack} style={{ marginBottom: 12 }}>
        ← Back
      </button>
      <h1>Assignments</h1>
      <p>Below are your upcoming assignments. Add a new assignment using the form.</p>

      <form onSubmit={handleAdd} style={{ display: "grid", gap: 8, maxWidth: 560 }}>
        <label>
          Name
          <input
            type="text"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            placeholder="e.g. Problem Set 4"
            style={{ width: "100%", padding: 8, marginTop: 4 }}
          />
        </label>

        <label>
          Due date
          <input
            type="date"
            value={due}
            onChange={(e) => setDue(e.target.value)}
            style={{ width: "100%", padding: 8, marginTop: 4 }}
          />
        </label>

        <label>
          Class name
          <input
            type="text"
            value={className}
            onChange={(e) => setClassName(e.target.value)}
            placeholder="e.g. CEN 3031"
            style={{ width: "100%", padding: 8, marginTop: 4 }}
          />
        </label>

        {error && <div style={{ color: "#c00" }}>{error}</div>}

        <div style={{ display: "flex", gap: 8 }}>
          <button type="submit" style={{ padding: "8px 12px" }}>
            Add Assignment
          </button>
          <button type="button" onClick={clearForm} style={{ padding: "8px 12px" }}>
            Clear
          </button>
        </div>
      </form>

      <div style={{ display: "grid", gap: 12, marginTop: 20 }}>
        {assignments.map((a) => (
          <div key={a.id} style={{ border: "1px solid #ddd", padding: 12, borderRadius: 8 }}>
            <h3 style={{ margin: 0 }}>{a.title}</h3>
            <small>
              Due: {a.due} • {a.className}
            </small>
          </div>
        ))}
      </div>
    </div>
  );
}
