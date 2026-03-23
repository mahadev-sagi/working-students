import { useMemo, useState } from 'react'
import './App.css'

const assignmentTypes = [
  { key: 'homework', label: 'Homework', hoursPerAssignment: 2 },
  { key: 'quiz', label: 'Quiz', hoursPerAssignment: 1 },
  { key: 'essay', label: 'Essay', hoursPerAssignment: 5 },
  { key: 'researchPaper', label: 'Research Paper', hoursPerAssignment: 8 },
  { key: 'labReport', label: 'Lab Report', hoursPerAssignment: 4 },
  { key: 'discussionPost', label: 'Discussion Post', hoursPerAssignment: 1 },
]

const initialCounts = assignmentTypes.reduce((acc, type) => {
  acc[type.key] = ''
  return acc
}, {})

function App() {
  const [page, setPage] = useState('login')
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [counts, setCounts] = useState(initialCounts)
  const [totalHours, setTotalHours] = useState(null)

  const rows = useMemo(
    () =>
      assignmentTypes.map((type) => ({
        ...type,
        count: Number.parseInt(counts[type.key], 10) || 0,
      })),
    [counts],
  )

  const handleChange = (key, value) => {
    if (value === '') {
      setCounts((prev) => ({ ...prev, [key]: '' }))
      return
    }

    const parsed = Number.parseInt(value, 10)
    if (Number.isNaN(parsed) || parsed < 0) return
    setCounts((prev) => ({ ...prev, [key]: String(parsed) }))
  }

  const handleCalculate = (event) => {
    event.preventDefault()
    const hours = rows.reduce(
      (sum, row) => sum + row.count * row.hoursPerAssignment,
      0,
    )
    setTotalHours(hours)
  }

  const handleLogin = () => {
    alert(`Username: ${username}\nPassword: ${password}`)
  }

  const handleSignup = () => {
    alert('Redirecting to sign up...')
  }

  return (
    <main className="page">
      <button
        className="top-right-nav"
        onClick={() => setPage((current) => (current === 'login' ? 'calculator' : 'login'))}
      >
        {page === 'login' ? 'Open Calculator' : 'Back to Login'}
      </button>

      {page === 'login' ? (
        <section className="home-card">
          <h1>Working Students</h1>

          <p>Username:</p>
          <input
            type="text"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            placeholder="Enter username"
          />

          <p>Password:</p>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Enter password"
          />

          <br />
          <br />

          <div className="auth-actions">
            <button onClick={handleLogin}>Login</button>
            <p>
              New user? <button onClick={handleSignup}>Sign up!</button>
            </p>
          </div>
        </section>
      ) : (
        <section className="calculator-card">
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
      )}
    </main>
  )
}

export default App
