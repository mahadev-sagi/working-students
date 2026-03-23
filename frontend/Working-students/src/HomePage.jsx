import { useMemo, useState } from 'react';
import './HomePage.css';

const assignmentTypes = [
  { key: 'homework', label: 'Homework', hoursPerAssignment: 2 },
  { key: 'quiz', label: 'Quiz', hoursPerAssignment: 1 },
  { key: 'essay', label: 'Essay', hoursPerAssignment: 5 },
  { key: 'researchPaper', label: 'Research Paper', hoursPerAssignment: 8 },
  { key: 'labReport', label: 'Lab Report', hoursPerAssignment: 4 },
  { key: 'discussionPost', label: 'Discussion Post', hoursPerAssignment: 1 },
];

const initialCounts = assignmentTypes.reduce((acc, type) => {
  acc[type.key] = '';
  return acc;
}, {});

function HomePage({ user, onLogout }) {
  const [counts, setCounts] = useState(initialCounts);
  const [totalHours, setTotalHours] = useState(null);

  const rows = useMemo(
    () =>
      assignmentTypes.map((type) => ({
        ...type,
        count: Number.parseInt(counts[type.key], 10) || 0,
      })),
    [counts],
  );

  const handleChange = (key, value) => {
    if (value === '') {
      setCounts((prev) => ({ ...prev, [key]: '' }));
      return;
    }

    const parsed = Number.parseInt(value, 10);
    if (Number.isNaN(parsed) || parsed < 0) return;
    setCounts((prev) => ({ ...prev, [key]: String(parsed) }));
  };

  const handleCalculate = (event) => {
    event.preventDefault();
    const hours = rows.reduce(
      (sum, row) => sum + row.count * row.hoursPerAssignment,
      0,
    );
    setTotalHours(hours);
  };

  return (
    <section className="calculator-card">
      <header className="homepage-header">
        <div className="profile-container">
          <span>Profile</span>
          <div className="email-tooltip">{user.email}</div>
        </div>
        <button onClick={onLogout} className="logout-button">
          Log Out
        </button>
      </header>
      <h2>Assignment Hours Calculator</h2>
      <p>Enter how many assignments you have for each type.</p>
      <form onSubmit={handleCalculate}>
        {rows.map((row) => (
          <label key={row.key} className="input-row">
            <span>{row.label}</span>
            <input
              type="number"
              min="0"
              step="1"
              value={counts[row.key]}
              onChange={(event) => handleChange(row.key, event.target.value)}
              placeholder="0"
            />
          </label>
        ))}
        <button type="submit">Calculate</button>
      </form>
      {totalHours !== null && (
        <p className="result">Total assignment hours: {totalHours}</p>
      )}
    </section>
  );
}

export default HomePage;
