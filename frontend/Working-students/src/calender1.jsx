import "./calender1.css";
import { useState } from "react";


const dayLabels = ["Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"];

function getCurrentWeekDates() {
  const today = new Date();
  
  const sunday = new Date(today);
  sunday.setHours(0, 0, 0, 0);
  sunday.setDate(today.getDate() - today.getDay());

  const dates = [];
  for (let i = 0; i < 7; i++) {
    const d = new Date(sunday);
    d.setDate(sunday.getDate() + i);
    dates.push(d);
  }
  return dates;
}

export default function WeeklyCalendar({ tasks, onAssignmentDrop, onCompleteAssignment }) {
  const weekDates = getCurrentWeekDates();
  const todayIndex = new Date().getDay();
  const [activeDropSlot, setActiveDropSlot] = useState("");
  const [draggedAssignmentId, setDraggedAssignmentId] = useState("");

  function sortFixedTasks(items) {
    return [...items].sort((a, b) => {
      const aStart = Number.isFinite(a.startMinutes) ? a.startMinutes : Number.MAX_SAFE_INTEGER;
      const bStart = Number.isFinite(b.startMinutes) ? b.startMinutes : Number.MAX_SAFE_INTEGER;
      if (aStart !== bStart) return aStart - bStart;
      return String(a.title || "").localeCompare(String(b.title || ""));
    });
  }

  function getDraggedPayload(event) {
    try {
      const raw = event.dataTransfer.getData("application/json");
      if (!raw) return null;
      return JSON.parse(raw);
    } catch (e) {
      return null;
    }
  }

  function getSharedDragPayload() {
    if (typeof window === "undefined") return null;
    return window.__workingStudentsDragPayload || null;
  }

  function mergeDayTasks(dayTasks) {
    const fixedTasks = sortFixedTasks(dayTasks.filter((task) => task.kind !== "assignment"));
    const assignmentTasks = [...dayTasks.filter((task) => task.kind === "assignment")].sort(
      (a, b) => (a.order || 0) - (b.order || 0)
    );

    const sequence = [...fixedTasks];

    for (const assignmentTask of assignmentTasks) {
      const anchorKey = assignmentTask.anchorKey || "__END__";
      if (anchorKey === "__START__") {
        sequence.unshift(assignmentTask);
        continue;
      }

      if (anchorKey === "__END__") {
        sequence.push(assignmentTask);
        continue;
      }

      const anchorIndex = sequence.findIndex((task) => task.taskKey === anchorKey);
      if (anchorIndex === -1) {
        sequence.push(assignmentTask);
      } else {
        sequence.splice(anchorIndex + 1, 0, assignmentTask);
      }
    }

    return sequence;
  }

  function startDrag(event, task) {
    event.dataTransfer.effectAllowed = "move";
    setDraggedAssignmentId(task.assignmentId || "");
    if (typeof window !== "undefined") {
      window.__workingStudentsDragPayload = { kind: "assignment", assignmentId: task.assignmentId, blockId: task.blockId };
    }
    event.dataTransfer.setData(
      "application/json",
      JSON.stringify({ kind: "assignment", assignmentId: task.assignmentId, blockId: task.blockId })
    );
  }

  function slotKey(day, anchorKey) {
    return `${day}|${anchorKey}`;
  }

  function commitDrop(event, day, anchorKey) {
    event.preventDefault();
    event.stopPropagation();
    const payload = getDraggedPayload(event);
    if (payload && typeof onAssignmentDrop === "function") {
      onAssignmentDrop(payload, day, anchorKey);
    }
    if (typeof window !== "undefined") {
      window.__workingStudentsDragPayload = null;
    }
    setActiveDropSlot("");
    setDraggedAssignmentId("");
  }

  function renderDropSlot(day, anchorKey, empty = false) {
    const key = slotKey(day, anchorKey);
    return (
      <div
        className={`calendar-drop-slot ${empty ? "calendar-drop-slot-empty" : ""} ${activeDropSlot === key ? "calendar-drop-slot-active" : ""}`}
        onDragEnter={(e) => {
          const payload = getSharedDragPayload() || getDraggedPayload(e);
          if (payload && payload.kind === "assignment") {
            setActiveDropSlot(key);
          }
        }}
        onDragOver={(e) => {
          e.preventDefault();
          const payload = getSharedDragPayload() || getDraggedPayload(e);
          if (payload && payload.kind === "assignment") {
            setActiveDropSlot(key);
          }
        }}
        onDragLeave={() => {
          if (activeDropSlot === key) {
            setActiveDropSlot("");
          }
        }}
        onDrop={(e) => commitDrop(e, day, anchorKey)}
      />
    );
  }

  return (
    <div className="calendar-container">
      <h2 className="calendar-title">Your Week</h2>

      <div className="calendar-grid">
        {dayLabels.map((label, idx) => {
          const date = weekDates[idx];
          const dateNum = date.getDate();
          const monthShort = date.toLocaleString(undefined, { month: 'short' });
          const isToday = idx === todayIndex;
          const dayTasks = tasks.filter((task) => task.day === label);
          const daySequence = mergeDayTasks(dayTasks);

          return (
            <div key={label} className={`calendar-column ${isToday ? 'today' : ''}`} onDragOver={(e) => e.preventDefault()}>
              <div className="calendar-header">
                <div className="day-label">{label}</div>
                <div className="day-date">{monthShort} {dateNum}</div>
              </div>

              <div className="calendar-body">
                {daySequence.length === 0 ? (
                  renderDropSlot(label, "__END__", true)
                ) : (
                  <>
                    {renderDropSlot(label, "__START__")}

                    {daySequence.map((task, index) => (
                      <div key={task.taskKey || `${label}-${index}`}>
                        <div
                          className={`calendar-task ${task.kind === "assignment" ? "calendar-task-assignment" : ""} ${task.kind === "assignment" && draggedAssignmentId === task.assignmentId ? "calendar-task-dragging" : ""}`}
                          style={{ backgroundColor: task.color }}
                          draggable={task.kind === "assignment"}
                          onDragStart={task.kind === "assignment" ? (e) => startDrag(e, task) : undefined}
                          onDragEnd={() => {
                            setActiveDropSlot("");
                            setDraggedAssignmentId("");
                            if (typeof window !== "undefined") {
                              window.__workingStudentsDragPayload = null;
                            }
                          }}
                        >
                          <p>{task.title}</p>
                          {task.building && task.room ? <small>{task.building} • {task.room}</small> : null}
                          {task.className ? <small>{task.className}</small> : null}
                          {task.timeRange ? <small>{task.timeRange}</small> : null}
                          {task.duration ? <small>Duration: {task.duration}</small> : null}
                          {task.kind === "assignment" ? (
                            <>
                              <button
                                className="calendar-assignment-complete"
                                title="Mark assignment complete"
                                onClick={(e) => {
                                  e.preventDefault();
                                  e.stopPropagation();
                                  if (typeof onCompleteAssignment === "function") {
                                    onCompleteAssignment(task.assignmentId);
                                  }
                                }}
                              >
                                ✓
                              </button>
                              <small><strong>{task.predictedHours ? `${task.predictedHours}h` : "Time TBD"}</strong></small>
                              <small>{task.due || "No due date"}</small>
                            </>
                          ) : (!task.timeRange && !task.duration && task.time ? <small>{task.time}</small> : null)}
                        </div>

                        {renderDropSlot(label, task.taskKey || "__END__")}
                      </div>
                    ))}
                  </>
                )}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}