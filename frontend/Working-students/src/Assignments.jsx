import React, { useEffect, useState } from "react";
import { apiUrl } from "./api";
import "./HomePage.css";

export default function Assignments({ token }) {
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
  const [enrolledAssignments, setEnrolledAssignments] = useState([]);
  const [loadingEnrolled, setLoadingEnrolled] = useState(false);
  const [enrolledError, setEnrolledError] = useState("");

  useEffect(() => {
    let isMounted = true;

    async function loadEnrolledAssignments() {
      if (!token) return;
      try {
        setLoadingEnrolled(true);
        setEnrolledError("");
        const response = await fetch(apiUrl("/assignments"), {
          headers: {
            Authorization: `Bearer ${token}`,
          },
        });

        if (!response.ok) {
          throw new Error("Failed to load class assignments");
        }

        const data = await response.json();
        if (!isMounted) return;
        setEnrolledAssignments(Array.isArray(data) ? data : []);
      } catch (e) {
        if (!isMounted) return;
        setEnrolledAssignments([]);
        setEnrolledError(e.message || "Failed to load class assignments");
      } finally {
        if (isMounted) {
          setLoadingEnrolled(false);
        }
      }
    }

    loadEnrolledAssignments();
    return () => {
      isMounted = false;
    };
  }, [token]);

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

  const handleDeleteManual = (id) => {
    setAssignments((prev) => {
      const next = prev.filter((a) => a.id !== id);
      try {
        localStorage.setItem("assignments", JSON.stringify(next));
        window.dispatchEvent(new Event("assignmentsUpdated"));
      } catch (e) {
        console.warn("Failed to delete assignment", e);
      }
      return next;
    });
  };

  

  return (
    <div className="assignments-page" style={{ padding: 24 }}>
      <button onClick={goBack} style={{ marginBottom: 12 }}>
        ← Back
      </button>
      <h1>Assignments</h1>
      <p>Class assignments are pulled from classes you are enrolled in. You can still add manual assignments below for other classes.</p>

      <h2 style={{ marginTop: 16 }}>From Enrolled Classes</h2>
      {loadingEnrolled ? <p>Loading class assignments...</p> : null}
      {enrolledError ? <p style={{ color: "#c00" }}>{enrolledError}</p> : null}
      {!loadingEnrolled && !enrolledError && enrolledAssignments.length === 0 ? (
        <p>No class assignments found yet.</p>
      ) : null}

      <div style={{ display: "grid", gap: 12, marginTop: 12, marginBottom: 20 }}>
        {enrolledAssignments.map((a) => (
          <div key={a.assignment_id} style={{ border: "1px solid #ddd", padding: 12, borderRadius: 8 }}>
            <h3 style={{ margin: 0 }}>{a.title}</h3>
            <small>
              Type: {a.type || "Unknown"} • Due: {a.due_date || "No due date"} • Predicted: {a.predicted_hours || 0}h
            </small>
          </div>
        ))}
      </div>

      <h2>Manual Assignments</h2>

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
          <div key={a.id} style={{ border: "1px solid #ddd", padding: 12, borderRadius: 8, display: "flex", justifyContent: "space-between", alignItems: "start" }}>
            <div>
              <h3 style={{ margin: 0 }}>{a.title}</h3>
              <small>
                Due: {a.due} • {a.className}
              </small>
            </div>
            <button onClick={() => handleDeleteManual(a.id)} style={{ padding: "6px 8px", backgroundColor: "#ffcbcb", border: "1px solid #ff6b6b", borderRadius: 4, cursor: "pointer", marginLeft: 12 }}>
              Delete
            </button>
          </div>
        ))}
      </div>
    </div>
  );
}
