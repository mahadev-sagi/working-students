import './HomePage.css';

function AdminPage({ user, onLogout }) {
  return (
    <section className="calculator-card">
      <header className="homepage-header">
        <div className="profile-container">
          <span>Admin</span>
          <div className="email-tooltip">{user.email}</div>
        </div>
        <button onClick={onLogout} className="logout-button">
          Log Out
        </button>
      </header>

      <h2>Admin Dashboard</h2>
      <p>You are signed in with admin access.</p>
      <p>Use this page for admin-only features.</p>
    </section>
  );
}

export default AdminPage;
