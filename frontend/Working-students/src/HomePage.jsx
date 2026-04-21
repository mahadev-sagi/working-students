import WeeklyCalendar from "./calender1";
import "./HomePage.css";
import EventSidebar from "./sidebar";
import "./sidebar.css";
import { apiUrl } from "./api";
import { useState, useEffect, useRef } from "react";

function HomePage({ user, token, onLogout }) {
  const events = [
    { id: 1, name: "Assignments", color: "#ff8a80" },
    { id: 2, name: "Events", color: "#81c784" },
    { id: 3, name: "Work Schedule", color: "#ba68c8" },
    { id: 4, name: "Personal Notes", color: "#64b5f6" },
    { id: 5, name: "Travel Time", color: "#4db6ac" },
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

  const [storedShifts, setStoredShifts] = useState([]);
  const [backendShifts, setBackendShifts] = useState([]);
  const [enrolledClasses, setEnrolledClasses] = useState([]);
  const [manualAssignments, setManualAssignments] = useState([]);
  const [enrolledAssignments, setEnrolledAssignments] = useState([]);
  const [scheduledAssignments, setScheduledAssignments] = useState([]);
  const [completedAssignments, setCompletedAssignments] = useState([]);

  const createBlockId = () => `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;

  const normalizeScheduledAssignments = (items) => {
    let changed = false;
    const normalized = (Array.isArray(items) ? items : []).map((entry) => {
      const blockId = entry.blockId || createBlockId();
      if (!entry.blockId) changed = true;
      return {
        blockId,
        assignmentId: entry.assignmentId,
        day: entry.day,
        anchorKey: entry.anchorKey || "__END__",
        order: entry.order || Date.now(),
      };
    });
    return { normalized, changed };
  };

  useEffect(() => {
    function loadData() {
      try {
        const rawS = localStorage.getItem("workShifts");
        setStoredShifts(rawS ? JSON.parse(rawS) : []);
      } catch (e) {
        console.warn("Failed to parse workShifts from storage", e);
        setStoredShifts([]);
      }
    }

    loadData();
    window.addEventListener("workUpdated", loadData);
    return () => {
      window.removeEventListener("workUpdated", loadData);
    };
  }, []);

  useEffect(() => {
    let isMounted = true;

    async function loadClassesAndShifts() {
      if (!token) return;
      try {
        const [classesRes, shiftsRes] = await Promise.all([
          fetch(apiUrl("/classes"), {
            headers: {
              Authorization: `Bearer ${token}`,
            },
          }),
          fetch(apiUrl("/shifts"), {
            headers: {
              Authorization: `Bearer ${token}`,
            },
          }),
        ]);

        if (!classesRes.ok || !shiftsRes.ok) {
          throw new Error("Failed to load classes");
        }

        const [classesData, shiftsData] = await Promise.all([
          classesRes.json(),
          shiftsRes.json(),
        ]);

        if (isMounted) {
          setEnrolledClasses(Array.isArray(classesData) ? classesData : []);
          setBackendShifts(Array.isArray(shiftsData) ? shiftsData : []);
        }
      } catch (e) {
        if (isMounted) {
          setEnrolledClasses([]);
          setBackendShifts([]);
        }
      }
    }

    loadClassesAndShifts();
    return () => {
      isMounted = false;
    };
  }, [token]);

  useEffect(() => {
    try {
      const raw = localStorage.getItem("scheduledAssignments");
      const parsed = raw ? JSON.parse(raw) : [];
      const { normalized, changed } = normalizeScheduledAssignments(parsed);
      setScheduledAssignments(normalized);
      if (changed) {
        localStorage.setItem("scheduledAssignments", JSON.stringify(normalized));
      }
    } catch (e) {
      console.warn("Failed to load scheduled assignments from storage", e);
      setScheduledAssignments([]);
    }
  }, []);

  useEffect(() => {
    try {
      const raw = localStorage.getItem("completedAssignments");
      setCompletedAssignments(raw ? JSON.parse(raw) : []);
    } catch (e) {
      console.warn("Failed to load completed assignments from storage", e);
      setCompletedAssignments([]);
    }
  }, []);

  useEffect(() => {
    function loadAssignments() {
      try {
        const raw = localStorage.getItem("assignments");
        const initialAssignments = [
          { id: 1, title: "Math Homework 5", due: "2026-04-25", className: "MATH 2010" },
          { id: 2, title: "CS Project Milestone", due: "2026-05-02", className: "CIS 4930" },
        ];
        setManualAssignments(raw ? JSON.parse(raw) : initialAssignments);
      } catch (e) {
        console.warn("Failed to load assignments from storage", e);
        setManualAssignments([]);
      }
    }

    loadAssignments();
    window.addEventListener("assignmentsUpdated", loadAssignments);
    return () => {
      window.removeEventListener("assignmentsUpdated", loadAssignments);
    };
  }, []);

  useEffect(() => {
    let isMounted = true;

    async function loadEnrolledAssignments() {
      if (!token) return;
      try {
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
      }
    }

    loadEnrolledAssignments();
    return () => {
      isMounted = false;
    };
  }, [token]);

  const dayCodeToLabel = {
    M: "Mon",
    T: "Tues",
    W: "Wed",
    R: "Thurs",
    F: "Fri",
    S: "Sat",
    U: "Sun",
  };

  function normalizeDayLabel(rawDay) {
    const value = String(rawDay || "").trim().toLowerCase();
    if (!value) return "";
    const cleaned = value.replace(/[^a-z0-9]/g, "");
    if (!cleaned) return "";
    if (cleaned === "0" || cleaned === "7") return "Sun";
    if (cleaned === "1") return "Mon";
    if (cleaned === "2") return "Tues";
    if (cleaned === "3") return "Wed";
    if (cleaned === "4") return "Thurs";
    if (cleaned === "5") return "Fri";
    if (cleaned === "6") return "Sat";
    if (["m", "mon", "monday"].includes(cleaned)) return "Mon";
    if (["t", "tu", "tue", "tues", "tuesday"].includes(cleaned)) return "Tues";
    if (["w", "wed", "wednesday", "weds"].includes(cleaned)) return "Wed";
    if (["r", "th", "thu", "thur", "thurs", "thursday"].includes(cleaned)) return "Thurs";
    if (["f", "fri", "friday"].includes(cleaned)) return "Fri";
    if (["s", "sa", "sat", "saturday"].includes(cleaned)) return "Sat";
    if (["u", "su", "sun", "sunday"].includes(cleaned)) return "Sun";
    return "";
  }

  function expandClassDayLabels(rawDays) {
    const source = String(rawDays || "").trim();
    if (!source) return [];

    const wordMatches = source
      .toLowerCase()
      .match(/monday|mon|tuesday|tues|tue|wednesday|weds|wed|thursday|thurs|thur|thu|friday|fri|saturday|sat|sunday|sun/g);
    if (wordMatches && wordMatches.length > 0) {
      return [...new Set(wordMatches.map((token) => normalizeDayLabel(token)).filter(Boolean))];
    }

    const compact = source.replace(/[^A-Za-z]/g, "").toUpperCase();
    if (/^[MTWRFSU]+$/.test(compact)) {
      return [...new Set(compact.split("").map((code) => dayCodeToLabel[code]).filter(Boolean))];
    }

    const tokenized = source.split(/[\s,\/-]+/);
    return [...new Set(tokenized.map((token) => normalizeDayLabel(token)).filter(Boolean))];
  }

  function normalizeClock(value) {
    if (!value) return "";
    return value.slice(0, 5);
  }

  function parseRangeDuration(start, end) {
    const normStart = normalizeClock(start);
    const normEnd = normalizeClock(end);
    if (!normStart || !normEnd) {
      return {
        timeRange: normStart && normEnd ? `${normStart} - ${normEnd}` : "",
        duration: "",
        startMinutes: Number.MAX_SAFE_INTEGER,
      };
    }

    const [sh, sm] = normStart.split(":").map(Number);
    const [eh, em] = normEnd.split(":").map(Number);
    if (Number.isNaN(sh) || Number.isNaN(sm) || Number.isNaN(eh) || Number.isNaN(em)) {
      return { timeRange: `${normStart} - ${normEnd}`, duration: "", startMinutes: Number.MAX_SAFE_INTEGER };
    }

    let minutes = (eh * 60 + em) - (sh * 60 + sm);
    if (minutes < 0) minutes += 24 * 60;
    const hours = Math.floor(minutes / 60);
    const remMinutes = minutes % 60;

    let duration = "";
    if (hours > 0 && remMinutes > 0) {
      duration = `${hours}h ${remMinutes}m`;
    } else if (hours > 0) {
      duration = `${hours}h`;
    } else {
      duration = `${remMinutes}m`;
    }

    return { timeRange: `${normStart} - ${normEnd}`, duration, startMinutes: sh * 60 + sm };
  }

  const backendShiftTasks = backendShifts
    .map((s) => {
      const day = normalizeDayLabel(s.day_of_week);
      if (!day) return null;
      const { timeRange, duration, startMinutes } = parseRangeDuration(s.start_time, s.end_time);
      const taskKey = `shift-${s.shift_id}`;
      return {
        day,
        id: taskKey,
        taskKey,
        title: `Shift • ${s.location || "Work"}`,
        time: duration || "Time TBD",
        timeRange,
        duration,
        startMinutes,
        color: "#ba68c8",
        kind: "fixed",
        draggable: false,
      };
    })
    .filter(Boolean);

  const localShiftTasks = storedShifts
    .map((s) => {
      const day = normalizeDayLabel(s.day);
      if (!day) return null;
      const parts = String(s.time || "").split("-");
      const start = parts[0]?.trim() || "";
      const end = parts[1]?.trim() || "";
      const { timeRange, duration, startMinutes } = parseRangeDuration(start, end);
      const taskKey = `local-shift-${s.id ?? `${s.day}-${s.place || "work"}-${start}-${end}`}`;
      return {
        day,
        id: taskKey,
        taskKey,
        title: `Shift • ${s.place || "Work"}`,
        time: duration || s.time || "Time TBD",
        timeRange,
        duration,
        startMinutes,
        color: "#ba68c8",
        kind: "fixed",
        draggable: false,
      };
    })
    .filter(Boolean);

  const classTasks = enrolledClasses.flatMap((c) => {
    const dayLabels = expandClassDayLabels(c.days_of_week);
    const { timeRange, duration, startMinutes } = parseRangeDuration(c.start_time, c.end_time);
    const taskKey = `class-${c.class_id ?? c.id ?? `${c.course_code || "class"}-${c.class_name}`}`;

    return dayLabels
      .map((dayLabel) => ({
        day: dayLabel,
        id: taskKey,
        taskKey,
        title: c.course_code ? `${c.course_code} • ${c.class_name}` : c.class_name,
        time: duration || "Time TBD",
        timeRange,
        duration,
        startMinutes,
        building: c.building,
        room: c.room_number,
        color: "#80deea",
        kind: "fixed",
        draggable: false,
      }));
  });

  const dedupeKey = (task) => `${task.day}|${task.title}|${task.timeRange || ""}`;
  const shiftTasksMap = new Map();
  for (const item of [...backendShiftTasks, ...localShiftTasks]) {
    const key = dedupeKey(item);
    if (!shiftTasksMap.has(key)) {
      shiftTasksMap.set(key, item);
    }
  }
  const shiftTasks = Array.from(shiftTasksMap.values());

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

  const formatPredictedHours = (hours) => {
    const value = Number(hours);
    if (!Number.isFinite(value) || value <= 0) return "Time TBD";
    if (Number.isInteger(value)) return `${value}h`;
    return `${value.toFixed(1)}h`;
  };

  const demoAssignments = Array.from({ length: 30 }, (_, idx) => {
    const day = String((idx % 28) + 1).padStart(2, "0");
    const due = `2026-05-${day}`;
    return {
      id: `demo-${idx + 1}`,
      title: `Demo Assignment ${idx + 1}`,
      due,
      className: `DEMO ${1000 + (idx % 8)}`,
      predictedHours: 2,
      source: "demo",
    };
  });

  const normalizedEnrolledAssignments = enrolledAssignments.map((assignment) => ({
    id: `enrolled-${assignment.assignment_id}`,
    title: assignment.title,
    due: assignment.due_date,
    className: assignment.type ? `${assignment.type}` : "Assignment",
    predictedHours: assignment.predicted_hours,
    source: "enrolled",
  }));

  const normalizedManualAssignments = manualAssignments.map((assignment) => ({
    id: `manual-${assignment.id}`,
    title: assignment.title,
    due: assignment.due,
    className: assignment.className,
    predictedHours: assignment.predictedHours ?? 2,
    source: "manual",
  }));

  const allAssignments = [...normalizedEnrolledAssignments, ...normalizedManualAssignments, ...demoAssignments];
  const assignmentLookup = new Map(allAssignments.map((assignment) => [assignment.id, assignment]));
  const completedAssignmentSet = new Set(completedAssignments);

  const isUpcomingDueDate = (due) => {
    const raw = String(due || "").trim();
    if (!raw) return false;

    const [year, month, day] = raw.split("-").map(Number);
    if (!year || !month || !day) return false;

    const dueDate = new Date(year, month - 1, day);
    if (Number.isNaN(dueDate.getTime())) return false;

    const today = new Date();
    const todayStart = new Date(today.getFullYear(), today.getMonth(), today.getDate());
    return dueDate >= todayStart;
  };

  const widgetAssignments = [...allAssignments].sort((a, b) => {
      const dateA = new Date(a.due || "9999-12-31");
      const dateB = new Date(b.due || "9999-12-31");
      return dateA - dateB;
    })
    .filter((assignment) => isUpcomingDueDate(assignment.due));

  const scheduledAssignmentTasks = scheduledAssignments
    .map((schedule) => {
      const assignment = assignmentLookup.get(schedule.assignmentId);
      if (!assignment || completedAssignmentSet.has(schedule.assignmentId)) return null;
      const day = normalizeDayLabel(schedule.day);
      if (!day) return null;
      return {
        id: `assignment-${schedule.blockId}`,
        taskKey: `assignment-${schedule.blockId}`,
        blockId: schedule.blockId,
        assignmentId: assignment.id,
        day,
        title: assignment.title,
        className: assignment.className,
        predictedHours: assignment.predictedHours,
        timeRange: "",
        duration: "",
        startMinutes: Number.MAX_SAFE_INTEGER,
        due: assignment.due,
           color: "#ff8a80",
        kind: "assignment",
        anchorKey: schedule.anchorKey || "__END__",
        order: schedule.order || 0,
        draggable: true,
      };
    })
    .filter(Boolean);

  const calendarTasks = [...classTasks, ...shiftTasks, ...scheduledAssignmentTasks];

  const persistScheduledAssignments = (updater) => {
    setScheduledAssignments((prev) => {
      const next = typeof updater === "function" ? updater(prev) : updater;
      try {
        localStorage.setItem("scheduledAssignments", JSON.stringify(next));
      } catch (e) {
        console.warn("Failed to persist scheduled assignments", e);
      }
      return next;
    });
  };

  const persistCompletedAssignments = (updater) => {
    setCompletedAssignments((prev) => {
      const next = typeof updater === "function" ? updater(prev) : updater;
      try {
        localStorage.setItem("completedAssignments", JSON.stringify(next));
      } catch (e) {
        console.warn("Failed to persist completed assignments", e);
      }
      return next;
    });
  };

  const scheduleAssignment = (assignmentId, day, anchorKey, blockId) => {
    if (!assignmentLookup.has(assignmentId)) return;
    if (completedAssignmentSet.has(assignmentId)) return;
    persistScheduledAssignments((prev) => [
      ...prev.filter((assignment) => assignment.blockId !== blockId),
      {
        blockId: blockId || createBlockId(),
        assignmentId,
        day,
        anchorKey: anchorKey || "__END__",
        order: Date.now(),
      },
    ]);
  };

  const unscheduleAssignmentBlock = (blockId) => {
    persistScheduledAssignments((prev) => prev.filter((assignment) => assignment.blockId !== blockId));
  };

  const completeAssignment = (assignmentId) => {
    const assignment = assignmentLookup.get(assignmentId);
    if (!assignment) return;
    const confirmed = window.confirm(`Mark \"${assignment.title}\" as complete? This clears all scheduled blocks.`);
    if (!confirmed) return;

    persistCompletedAssignments((prev) => {
      if (prev.includes(assignmentId)) return prev;
      return [...prev, assignmentId];
    });
    persistScheduledAssignments((prev) => prev.filter((entry) => entry.assignmentId !== assignmentId));
  };

  const reopenAssignment = (assignmentId) => {
    persistCompletedAssignments((prev) => prev.filter((id) => id !== assignmentId));
  };

  const readDragPayload = (event) => {
    try {
      const raw = event.dataTransfer.getData("application/json");
      if (!raw) return null;
      return JSON.parse(raw);
    } catch (e) {
      return null;
    }
  };

  const handleWidgetDragStart = (event, assignment) => {
    if (completedAssignmentSet.has(assignment.id)) return;
    event.dataTransfer.effectAllowed = "move";
    window.__workingStudentsDragPayload = { kind: "assignment", assignmentId: assignment.id };
    event.dataTransfer.setData(
      "application/json",
      JSON.stringify({ kind: "assignment", assignmentId: assignment.id })
    );
  };

  const handleWidgetDragEnd = () => {
    window.__workingStudentsDragPayload = null;
  };

  const handleWidgetDrop = (event) => {
    event.preventDefault();
    const payload = readDragPayload(event);
    if (!payload || !payload.assignmentId || !payload.blockId) return;
    unscheduleAssignmentBlock(payload.blockId);
    window.__workingStudentsDragPayload = null;
  };

  const handleCalendarDrop = (payload, day, anchorKey) => {
    if (!payload || !payload.assignmentId) return;
    scheduleAssignment(payload.assignmentId, day, anchorKey, payload.blockId);
  };

  const handleCalendarCompleteAssignment = (assignmentId) => {
    completeAssignment(assignmentId);
  };

  return (
    <div>
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
      <WeeklyCalendar
        tasks={calendarTasks}
        onAssignmentDrop={handleCalendarDrop}
        onCompleteAssignment={handleCalendarCompleteAssignment}
      />

      <div className="below-calendar-layout">
        <div className="sidebar-buttons-container">
          <EventSidebar events={events} onAddNote={handleAddNote} />
        </div>

        <div className="notes-container">
          <h3>Personal Notes</h3>
          <div className="notes-list">
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

        <div className="assignments-widget-container" onDragOver={(e) => e.preventDefault()} onDrop={handleWidgetDrop}>
          <h3>Upcoming Assignments</h3>
          <div className="assignments-list">
            {widgetAssignments.length === 0 ? (
              <div className="assignment-empty">No assignments yet</div>
            ) : (
              widgetAssignments.map((assignment) => (
                <div
                  key={assignment.id}
                  className={`assignment-item ${completedAssignmentSet.has(assignment.id) ? "assignment-complete" : "assignment-draggable"}`}
                  draggable={!completedAssignmentSet.has(assignment.id)}
                  onDragStart={!completedAssignmentSet.has(assignment.id) ? (e) => handleWidgetDragStart(e, assignment) : undefined}
                  onDragEnd={handleWidgetDragEnd}
                >
                  <div className="assignment-info">
                    <div className="assignment-title">{assignment.title}</div>
                    <div className="assignment-meta">
                      <span>{assignment.className}</span>
                         <span className="assignment-due"><strong>{assignment.due || "No due date"}</strong></span>
                    </div>
                  </div>
                  <div className="assignment-actions">
                    <div className="assignment-predicted">
                      <strong>{formatPredictedHours(assignment.predictedHours)}</strong>
                    </div>
                    {completedAssignmentSet.has(assignment.id) ? (
                      <button
                        className="assignment-toggle assignment-reopen"
                        onClick={() => reopenAssignment(assignment.id)}
                        title="Mark incomplete"
                      >
                        ↩
                      </button>
                    ) : (
                      <button
                        className="assignment-toggle assignment-complete-button"
                        onClick={() => completeAssignment(assignment.id)}
                        title="Mark complete"
                      >
                        ✓
                      </button>
                    )}
                  </div>
                </div>
              ))
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default HomePage;
