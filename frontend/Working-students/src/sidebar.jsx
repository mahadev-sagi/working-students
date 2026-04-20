import "./sidebar.css";
import { useRef, useState, useEffect } from "react";

export default function EventSidebar({ events, onAddNote }) {
  const containerRef = useRef(null);
  const popupRef = useRef(null);
  const elRefs = useRef([]);

  const [selectedId, setSelectedId] = useState(null);
  const [popupPos, setPopupPos] = useState({ top: 0, left: 0 });
  const [details, setDetails] = useState({});
  const [editingText, setEditingText] = useState("");

  useEffect(() => {
    function handleClickOutside(e) {
      if (
        popupRef.current &&
        !popupRef.current.contains(e.target) &&
        containerRef.current &&
        !containerRef.current.contains(e.target)
      ) {
        setSelectedId(null);
      }
    }
    function handleEsc(e) {
      if (e.key === "Escape") setSelectedId(null);
    }
    document.addEventListener("mousedown", handleClickOutside);
    document.addEventListener("keydown", handleEsc);
    return () => {
      document.removeEventListener("mousedown", handleClickOutside);
      document.removeEventListener("keydown", handleEsc);
    };
  }, []);

  const onClickItem = (eventObj, idx) => {
    const el = elRefs.current[idx];
    if (!el || !containerRef.current) return;

    if (eventObj.name === "Assignments") {
      window.history.pushState({}, "", "/assignments");
      window.dispatchEvent(new PopStateEvent("popstate"));
      return;
    }
    if (eventObj.name === "Events") {
      window.history.pushState({}, "", "/events");
      window.dispatchEvent(new PopStateEvent("popstate"));
      return;
    }
    if (eventObj.name === "Work Schedule") {
      window.history.pushState({}, "", "/work-schedule");
      window.dispatchEvent(new PopStateEvent("popstate"));
      return;
    }

    const top = el.offsetTop;
    const left = el.offsetLeft + el.offsetWidth + 8;

    setPopupPos({ top, left });
    setSelectedId(eventObj.id);
    setEditingText(details[eventObj.id] || "");
  };

  const onSave = () => {
    if (selectedId == null) return;
    setDetails((d) => ({ ...d, [selectedId]: editingText }));

  const eventObj = events.find((e) => e.id === selectedId);
    if (eventObj && eventObj.name && eventObj.name.toLowerCase().includes("note")) {
      if (typeof onAddNote === "function") {
        onAddNote({
          id: Date.now(),
          text: editingText,
          createdAt: new Date().toISOString(),
        });
      }
    }

    setSelectedId(null);
  };

  return (
    <div className="sidebar" ref={containerRef}>
      <h3 className="sidebar-title">Events</h3>

      {events.map((event, idx) => (
        <div
          key={event.id}
          ref={(el) => (elRefs.current[idx] = el)}
          className="sidebar-item"
          style={{ backgroundColor: event.color }}
          onClick={() => onClickItem(event, idx)}
        >
          {event.name}
        </div>
      ))}

      {selectedId != null && (
        <div
          className="sidebar-popup"
          ref={popupRef}
          style={{ top: popupPos.top + "px", left: popupPos.left + "px" }}
        >
          <div className="popup-header">
            <strong>
              {events.find((e) => e.id === selectedId)?.name || "Details"}
            </strong>
          </div>
          <textarea
            className="popup-textarea"
            value={editingText}
            onChange={(e) => setEditingText(e.target.value)}
            placeholder="Write any notes for yourself..."
          />
          <div className="popup-actions">
            <button className="popup-save" onClick={onSave}>
              Save
            </button>
            <button className="popup-close" onClick={() => setSelectedId(null)}>
              Close
            </button>
          </div>
        </div>
      )}
    </div>
  );
}