import { useState } from 'react'
import './App.css'

function App() {
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')

  const handleLogin = () => {
    alert(`Username: ${username}\nPassword: ${password}`)
  }

  const handleSignup = () => {
    alert("Redirecting to sign up...")
  }

  return (
    <div>
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

      <br /><br />

      <button onClick={handleLogin}>Login</button>

      <p>
        New user?{' '}
        <button onClick={handleSignup}>
          Sign up!
        </button>
      </p>
    </div>
  )
}

export default App