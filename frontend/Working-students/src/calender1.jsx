import "./calender1.css";


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

export default function WeeklyCalendar({ tasks }) {
  const weekDates = getCurrentWeekDates();
  const todayIndex = new Date().getDay();

  return (
    <div className="calendar-container">
      <h2 className="calendar-title">Your Week</h2>

      <div className="calendar-grid">
        {dayLabels.map((label, idx) => {
          const date = weekDates[idx];
          const dateNum = date.getDate();
          const monthShort = date.toLocaleString(undefined, { month: 'short' });
          const isToday = idx === todayIndex;

          return (
            <div key={label} className={`calendar-column ${isToday ? 'today' : ''}`}>
              <div className="calendar-header">
                <div className="day-label">{label}</div>
                <div className="day-date">{monthShort} {dateNum}</div>
              </div>

              <div className="calendar-body">
                {tasks
                  .filter((task) => task.day === label)
                  .map((task, index) => (
                    <div
                      key={index}
                      className="calendar-task"
                      style={{ backgroundColor: task.color }}
                    >
                      <p>{task.title}</p>
                      <small>{task.time}</small>
                    </div>
                  ))}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}