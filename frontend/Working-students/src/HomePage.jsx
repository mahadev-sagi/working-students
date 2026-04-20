import WeeklyCalendar from "./calender1";
import "./HomePage.css";
import EventSidebar from "./sidebar";
import "./sidebar.css";
import TravelWidget from "./TravelWidget";
import { useState, useEffect, useRef } from "react";

function HomePage({ user, onLogout }) {
  const tasks = [
    {
      day: "Mon",
      title: "Reading HW",
      time: "50 min",
      color: "#fff176",
    },
    {
      day: "Tues",
      title: "Discussion",
      time: "25 min",
      color: "#80deea",
    },
    {
      day: "Thurs",
      title: "Exam Prep",
      time: "30 min",
      color: "#ff8a80",
    },
    {
      day: "Sat",
      title: "Work",
      time: "Shift",
      color: "#ff80ab",
    },
  ];

  const events = [
    { id: 1, name: "Assignments", color: "#ff8a80" },
    { id: 2, name: "Events", color: "#81c784" },
    { id: 3, name: "Work Schedule", color: "#ba68c8" },
    { id: 4, name: "Personal Notes", color: "#64b5f6" },
  ];

  const [notes, setNotes] = useState([]);
  const [editingId, setEditingId] = useState(null);
  const [editingText, setEditingText] = useState("");
  const [showProfileMenu, setShowProfileMenu] = useState(false);
  const profileRef = useRef(null);

  useEffect(() => {
    function handleClickOutside(e) {
      if (profileRef.current && !profileRef.current.contains(e.target)) {
        setShowProfileMenu(false);
      }
    }
    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, []);

  const [storedAssignments, setStoredAssignments] = useState([]);
  const [storedShifts, setStoredShifts] = useState([]);

  useEffect(() => {
    function loadData() {
      try {
        const rawA = localStorage.getItem("assignments");
        setStoredAssignments(rawA ? JSON.parse(rawA) : []);
      } catch (e) {
        console.warn("Failed to parse assignments from storage", e);
        setStoredAssignments([]);
      }

      try {
        const rawS = localStorage.getItem("workShifts");
        setStoredShifts(rawS ? JSON.parse(rawS) : []);
      } catch (e) {
        console.warn("Failed to parse workShifts from storage", e);
        setStoredShifts([]);
      }
    }

    loadData();
    window.addEventListener("assignmentsUpdated", loadData);
    window.addEventListener("workUpdated", loadData);
    return () => {
      window.removeEventListener("assignmentsUpdated", loadData);
      window.removeEventListener("workUpdated", loadData);
    };
  }, []);

  const dayLabels = ["Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"];

  function getStartOfWeek(date) {
    const d = new Date(date);
    d.setHours(0, 0, 0, 0);
    d.setDate(d.getDate() - d.getDay());
    return d;
  }

  function isDateInWeek(dateStr) {
    try {
      const d = new Date(dateStr);
      const start = getStartOfWeek(new Date());
      const end = new Date(start);
      end.setDate(start.getDate() + 6);
      d.setHours(0, 0, 0, 0);
      return d >= start && d <= end;
    } catch (e) {
      return false;
    }
  }

  const assignmentTasks = storedAssignments
    .filter((a) => isDateInWeek(a.due))
    .map((a) => {
      const d = new Date(a.due);
      const label = dayLabels[d.getDay()];
      return {
        day: label,
        title: a.title + (a.className ? ` (${a.className})` : ""),
        time: "Due",
        color: "#ff8a80",
      };
    });

  const shiftTasks = storedShifts.map((s) => ({
    day: s.day,
    title: `Shift • ${s.place}`,
    time: s.time,
    color: "#ba68c8",
  }));

  const calendarTasks = [...tasks, ...assignmentTasks, ...shiftTasks];

  const handleAddNote = (note) => {
    setNotes((prev) => [note, ...prev]);
  };

  const handleDeleteNote = (id) => {
    setNotes((prev) => prev.filter((n) => n.id !== id));
    if (editingId === id) {
      setEditingId(null);
      setEditingText("");
    }
  };

  const startEditNote = (note) => {
    setEditingId(note.id);
    setEditingText(note.text);
  };

  const saveEditNote = () => {
    if (editingId == null) return;
    setNotes((prev) =>
      prev.map((n) => (n.id === editingId ? { ...n, text: editingText, editedAt: new Date().toISOString() } : n))
    );
    setEditingId(null);
    setEditingText("");
  };

  const cancelEdit = () => {
    setEditingId(null);
    setEditingText("");
  };

  return (
    <div>
      <TravelWidget />
      <header className="home-header">
        <h1 className="site-title">WorkingStudents</h1>
        <div className="welcome-title">Welcome Back!</div>

        <div className="profile-wrap" ref={profileRef}>
          <div className="profile-circle" onClick={() => setShowProfileMenu((s) => !s)}>
            {user && user.email ? user.email[0].toUpperCase() : "U"}
          </div>
          {showProfileMenu && (
            <div className="profile-menu">
              <div className="profile-email">{user?.email || "user@domain"}</div>
              <button className="logout-button" onClick={() => { setShowProfileMenu(false); if (typeof onLogout === 'function') onLogout(); }}>
                Logout
              </button>
            </div>
          )}
        </div>
      </header>
      <WeeklyCalendar tasks={calendarTasks} />
      <h2 className="upcoming-events-title">Upcoming Events</h2>
      <div className="home-layout">
        <EventSidebar events={events} onAddNote={handleAddNote} />

        <div className="notes-container">
          <h3>Personal Notes</h3>
          {notes.length === 0 ? (
            <div className="note-empty">No notes yet. Save a note from Personal Notes.</div>
          ) : (
            notes.map((n) => (
              <div key={n.id} className="note-box">
                <div className="note-meta">
                  {new Date(n.createdAt).toLocaleString()}
                  {n.editedAt ? ` • edited ${new Date(n.editedAt).toLocaleString()}` : ""}
                </div>

                {editingId === n.id ? (
                  <div>
                    <textarea
                      className="note-textarea"
                      value={editingText}
                      onChange={(e) => setEditingText(e.target.value)}
                    />
                    <div className="note-actions">
                      <button className="note-button" onClick={saveEditNote}>
                        Save
                      </button>
                      <button className="note-button" onClick={cancelEdit}>
                        Cancel
                      </button>
                    </div>
                  </div>
                ) : (
                  <>
                    <div className="note-text">{n.text}</div>
                    <div className="note-actions">
                      <button className="note-button" onClick={() => startEditNote(n)}>
                        Edit
                      </button>
                      <button className="note-button" onClick={() => handleDeleteNote(n.id)}>
                        Delete
                      </button>
                    </div>
                  </>
                )}
              </div>
            ))
          )}
        </div>

      </div>
    </div>
  );
}

export default HomePage;