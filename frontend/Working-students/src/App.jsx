import { useState, useEffect } from 'react';
import { apiUrl } from './api';
import './App.css';
import HomePage from './HomePage';
import Assignments from './Assignments';
import Events from './Events';
import WorkSchedule from './WorkSchedule';

function App() {
 
  const [page, setPage] = useState('login');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [user, setUser] = useState(null);
  const [token, setToken] = useState(null);
  const [error, setError] = useState('');

  const handleLogin = async () => {
    if (email.trim() === '' || password.trim() === '') {
      setError('Email and password required');
      return;
    }
    if (!email.endsWith('@ufl.edu')) {
      setError('Only @ufl.edu emails are allowed to login.');
      return;
    }
    setError('');
    console.log('LOGIN DEBUG:', { email, password });
    try {
      const response = await fetch(apiUrl('/login'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password }),
      });
      if (!response.ok) {
        if (response.status === 401 || response.status === 404 || response.status === 403) {
           setError('Invalid Credentials!');
        } else {
           const errorMessage = await response.text();
           setError(errorMessage);
        }
        return;
      }
      const data = await response.json();
      setToken(data.token);
      setUser({ email });
      setPage('homepage');
    } catch (error) {
      setError('An error occurred during login. Please try again.');
    }
  };

  const handleSignup = async () => {
    if (email.trim() === '' || password.trim() === '') {
      setError('Email and password required');
      return;
    }
    if (!email.endsWith('@ufl.edu')) {
      setError('Only @ufl.edu emails are allowed to sign up.');
      return;
    }
    setError('');
    try {
      const signupResponse = await fetch(apiUrl('/signup'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password }),
      });
      if (!signupResponse.ok) {
        const errorMessage = await signupResponse.text();
        setError(errorMessage);
        return;
      }

      await handleLogin();
    } catch (error) {
      setError('An error occurred during signup. Please try again.');
    }
  };

  const handleLogout = () => {
    setUser(null);
    setToken(null);
    setPage('login');
    window.history.pushState({}, '', '/');
    setPath('/');
  };

  
  const [path, setPath] = useState(window.location.pathname || '/');

  useEffect(() => {
    const onPop = () => setPath(window.location.pathname || '/');
    window.addEventListener('popstate', onPop);
    return () => window.removeEventListener('popstate', onPop);
  }, []);

   return (
    <main className="page">
      {user ? (
        <HomePage user={user} token={token} onLogout={handleLogout} />
      ) : page === 'signup' ? (
        <section className="home-card">
          <h1>Working Students - Sign Up</h1>
          <p>Email:</p>
          <input
            type="text"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="Enter your @ufl.edu email"
          />
          <p>Password:</p>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Enter password"
          />
          {error && <p className="error-message">{error}</p>}
          <br /><br />
          <div className="auth-actions">
            <button onClick={handleSignup}>Sign Up</button>
            <p>
              Already have an account? <button onClick={() => { setError(''); setPage('login'); }}>Login!</button>
            </p>
          </div>
        </section>
      ) : (
        <section className="home-card">
          <h1>Working Students</h1>

          <p>Email:</p>
          <input
            type="text"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="Enter your @ufl.edu email"
          />

          <p>Password:</p>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Enter password"
          />
          {error && <p className="error-message">{error}</p>}

          <br />
          <br />

          <div className="auth-actions">
            <button onClick={handleLogin}>Login</button>
            <p>
              New user? <button onClick={() => { setError(''); setPage('signup'); }}>Sign up!</button>
            </p>
          </div>
        </section>
      )}
    </main>
  );
}

export default App;