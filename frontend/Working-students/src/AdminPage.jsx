import { useCallback, useEffect, useMemo, useState } from 'react';
import { apiUrl } from './api';
import './AdminPage.css';

const dayOptions = [
  { value: 'M', label: 'M' },
  { value: 'T', label: 'T' },
  { value: 'W', label: 'W' },
  { value: 'R', label: 'R' },
  { value: 'F', label: 'F' },
  { value: 'S', label: 'S' },
  { value: 'U', label: 'U' },
];

function getInitialClassForm() {
  return {
    class_name: '',
    course_code: '',
    building: '',
    room_number: '',
    start_time: '',
    end_time: '',
    days_of_week: [],
  };
}

function getInitialAssignmentForm(classId = '') {
  return {
    assignment_id: null,
    class_id: classId,
    assignment_type_id: '',
    title: '',
    description: '',
    due_date: '',
    due_time: '',
    assignment_time_prediction: '',
  };
}

function normalizeTime(value) {
  if (!value) return '';
  return value.slice(0, 5);
}

function AdminPage({ user, token, onLogout }) {
  const [view, setView] = useState('dashboard');
  const [classes, setClasses] = useState([]);
  const [assignments, setAssignments] = useState([]);
  const [roster, setRoster] = useState([]);
  const [assignmentTypes, setAssignmentTypes] = useState([]);
  const [selectedClass, setSelectedClass] = useState(null);
  const [loading, setLoading] = useState(true);
  const [loadingRoster, setLoadingRoster] = useState(false);
  const [error, setError] = useState('');
  const [showAddStudentModal, setShowAddStudentModal] = useState(false);
  const [studentForm, setStudentForm] = useState({ name: '', email: '' });

  const [classForm, setClassForm] = useState(getInitialClassForm());
  const [assignmentForm, setAssignmentForm] = useState(getInitialAssignmentForm());

  const authHeaders = useMemo(
    () => ({
      'Content-Type': 'application/json',
      Authorization: `Bearer ${token}`,
    }),
    [token],
  );

  const loadAdminClassesAndTypes = useCallback(async (showError = true) => {
    try {
      setError('');
      setLoading(true);
      const [classesRes, typesRes] = await Promise.all([
        fetch(apiUrl('/admin/classes'), { headers: authHeaders }),
        fetch(apiUrl('/admin/assignment-types'), { headers: authHeaders }),
      ]);

      if (!classesRes.ok || !typesRes.ok) {
        throw new Error('Failed to load admin data');
      }

      const classesData = await classesRes.json();
      const typesData = await typesRes.json();
      setClasses(Array.isArray(classesData) ? classesData : []);
      setAssignmentTypes(Array.isArray(typesData) ? typesData : []);
    } catch (err) {
      if (showError) {
        setError('Failed to load admin data. Please refresh and try again.');
      }
    } finally {
      setLoading(false);
    }
  }, [authHeaders]);

  const loadAssignmentsForClass = useCallback(async (classId, showError = true) => {
    try {
      setError('');
      const res = await fetch(apiUrl('/admin/assignments/list'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify({ class_id: Number(classId) }),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to load assignments');
      }

      const data = await res.json();
      setAssignments(Array.isArray(data) ? data : []);
    } catch (err) {
      if (showError) {
        setError(err.message || 'Failed to load assignments');
      }
    }
  }, [authHeaders]);

  const loadRosterForClass = useCallback(async (classId, showError = true) => {
    try {
      setError('');
      setLoadingRoster(true);
      const res = await fetch(apiUrl('/admin/classes/roster'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify({ class_id: Number(classId) }),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to load roster');
      }

      const data = await res.json();
      setRoster(Array.isArray(data) ? data : []);
    } catch (err) {
      if (showError) {
        setError(err.message || 'Failed to load roster');
      }
    } finally {
      setLoadingRoster(false);
    }
  }, [authHeaders]);

  useEffect(() => {
    loadAdminClassesAndTypes();
  }, [loadAdminClassesAndTypes]);

  const openCreateClass = () => {
    setError('');
    setClassForm(getInitialClassForm());
    setView('createClass');
  };

  const openClassDetail = async (classItem) => {
    setSelectedClass(classItem);
    setAssignments([]);
    setRoster([]);
    setView('classDetail');
    await loadAssignmentsForClass(classItem.id);
  };

  const openRoster = async () => {
    if (!selectedClass) return;
    setError('');
    setView('roster');
    await loadRosterForClass(selectedClass.id);
  };

  const openCreateAssignment = () => {
    if (!selectedClass) return;
    setError('');
    setAssignmentForm(getInitialAssignmentForm(String(selectedClass.id)));
    setView('createAssignment');
  };

  const openEditAssignment = (assignment) => {
    setError('');
    setAssignmentForm({
      assignment_id: assignment.id,
      class_id: String(assignment.class_id),
      assignment_type_id: String(assignment.assignment_type_id),
      title: assignment.title || '',
      description: assignment.description || '',
      due_date: assignment.due_date || '',
      due_time: normalizeTime(assignment.due_time),
      assignment_time_prediction: assignment.assignment_time_prediction > 0
        ? String(assignment.assignment_time_prediction)
        : '',
    });
    setView('editAssignment');
  };

  const handleBackToDashboard = async () => {
    setError('');
    setSelectedClass(null);
    setAssignments([]);
    setRoster([]);
    setShowAddStudentModal(false);
    setStudentForm({ name: '', email: '' });
    setView('dashboard');
    await loadAdminClassesAndTypes(false);
  };

  const handleBackToClassDetail = async () => {
    if (!selectedClass) {
      setView('dashboard');
      return;
    }
    setError('');
    setView('classDetail');
    await loadAssignmentsForClass(selectedClass.id, false);
  };

  const openAddStudentModal = () => {
    setError('');
    setStudentForm({ name: '', email: '' });
    setShowAddStudentModal(true);
  };

  const closeAddStudentModal = () => {
    setShowAddStudentModal(false);
  };

  const submitAddStudent = async (event) => {
    event.preventDefault();
    setError('');

    if (!selectedClass?.id || !studentForm.email.trim()) {
      setError('Student email is required.');
      return;
    }

    try {
      const res = await fetch(apiUrl('/admin/classes/roster/add'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify({
          class_id: Number(selectedClass.id),
          name: studentForm.name.trim(),
          email: studentForm.email.trim(),
        }),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to add student');
      }

      setShowAddStudentModal(false);
      setStudentForm({ name: '', email: '' });
      await loadRosterForClass(Number(selectedClass.id), false);
      await loadAdminClassesAndTypes(false);
    } catch (err) {
      setError(err.message || 'Failed to add student');
    }
  };

  const toggleDay = (dayValue) => {
    setClassForm((prev) => {
      const days = prev.days_of_week.includes(dayValue)
        ? prev.days_of_week.filter((value) => value !== dayValue)
        : [...prev.days_of_week, dayValue];
      return { ...prev, days_of_week: days };
    });
  };

  const submitCreateClass = async (event) => {
    event.preventDefault();
    setError('');

    if (!classForm.class_name.trim()) {
      setError('Class name is required.');
      return;
    }

    const payload = {
      ...classForm,
      days_of_week: classForm.days_of_week.join(''),
    };

    try {
      const res = await fetch(apiUrl('/admin/classes'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify(payload),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to create class');
      }

      const data = await res.json();
      const createdClass = {
        id: data.class_id,
        class_name: classForm.class_name.trim(),
        course_code: classForm.course_code.trim(),
        building: classForm.building.trim(),
        room_number: classForm.room_number.trim(),
        start_time: classForm.start_time || '',
        end_time: classForm.end_time || '',
        days_of_week: classForm.days_of_week.join(''),
        enrollment_count: 0,
      };

      setClasses((prev) => [createdClass, ...prev.filter((item) => item.id !== createdClass.id)]);
      setClassForm(getInitialClassForm());
      setView('dashboard');
      await loadAdminClassesAndTypes(false);
    } catch (err) {
      setError(err.message || 'Failed to create class');
    }
  };

  const submitCreateAssignment = async (event) => {
    event.preventDefault();
    setError('');

    if (!selectedClass?.id || !assignmentForm.assignment_type_id || !assignmentForm.title.trim()) {
      setError('Assignment type and title are required.');
      return;
    }

    const payload = {
      class_id: Number(selectedClass.id),
      assignment_type_id: Number(assignmentForm.assignment_type_id),
      title: assignmentForm.title.trim(),
      description: assignmentForm.description.trim(),
      due_date: assignmentForm.due_date || '',
      due_time: assignmentForm.due_time || '',
      assignment_time_prediction: assignmentForm.assignment_time_prediction
        ? Number(assignmentForm.assignment_time_prediction)
        : 0,
    };

    try {
      const res = await fetch(apiUrl('/admin/assignments'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify(payload),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to create assignment');
      }

      await loadAssignmentsForClass(Number(selectedClass.id), false);
      setView('classDetail');
    } catch (err) {
      setError(err.message || 'Failed to create assignment');
    }
  };

  const submitEditAssignment = async (event) => {
    event.preventDefault();
    setError('');

    if (!assignmentForm.assignment_id || !selectedClass?.id || !assignmentForm.assignment_type_id || !assignmentForm.title.trim()) {
      setError('Assignment type and title are required.');
      return;
    }

    const payload = {
      assignment_id: Number(assignmentForm.assignment_id),
      class_id: Number(selectedClass.id),
      assignment_type_id: Number(assignmentForm.assignment_type_id),
      title: assignmentForm.title.trim(),
      description: assignmentForm.description.trim(),
      due_date: assignmentForm.due_date || '',
      due_time: assignmentForm.due_time || '',
      assignment_time_prediction: assignmentForm.assignment_time_prediction
        ? Number(assignmentForm.assignment_time_prediction)
        : 0,
    };

    try {
      const res = await fetch(apiUrl('/admin/assignments/update'), {
        method: 'POST',
        headers: authHeaders,
        body: JSON.stringify(payload),
      });

      if (!res.ok) {
        const text = await res.text();
        throw new Error(text || 'Failed to update assignment');
      }

      await loadAssignmentsForClass(Number(selectedClass.id), false);
      setView('classDetail');
    } catch (err) {
      setError(err.message || 'Failed to update assignment');
    }
  };

  const renderDashboard = () => (
    <>
      <section className="admin-section minimal-actions">
        <button type="button" onClick={openCreateClass}>Create Class</button>
      </section>

      <section className="admin-section">
        <h3>Your Classes</h3>
        {classes.length === 0 ? (
          <p>No classes created yet.</p>
        ) : (
          <div className="admin-card-list">
            {classes.map((item) => (
              <button
                key={item.id}
                type="button"
                className="entity-card"
                onClick={() => openClassDetail(item)}
              >
                <div className="entity-card-title">{item.class_name}</div>
                <div className="entity-card-meta">{item.course_code || 'No code'} - {item.enrollment_count} enrolled</div>
              </button>
            ))}
          </div>
        )}
      </section>
    </>
  );

  const renderCreateClass = () => (
    <section className="admin-section">
      <div className="minimal-actions">
        <button type="button" onClick={handleBackToDashboard}>Back</button>
      </div>
      <h3>Create Class</h3>
      <form onSubmit={submitCreateClass} className="admin-form-grid">
        <input
          type="text"
          placeholder="Class name *"
          value={classForm.class_name}
          onChange={(event) => setClassForm((prev) => ({ ...prev, class_name: event.target.value }))}
        />
        <input
          type="text"
          placeholder="Course code"
          value={classForm.course_code}
          onChange={(event) => setClassForm((prev) => ({ ...prev, course_code: event.target.value }))}
        />
        <input
          type="text"
          placeholder="Building"
          value={classForm.building}
          onChange={(event) => setClassForm((prev) => ({ ...prev, building: event.target.value }))}
        />
        <input
          type="text"
          placeholder="Room number"
          value={classForm.room_number}
          onChange={(event) => setClassForm((prev) => ({ ...prev, room_number: event.target.value }))}
        />
        <input
          type="time"
          value={classForm.start_time}
          onChange={(event) => setClassForm((prev) => ({ ...prev, start_time: event.target.value }))}
        />
        <input
          type="time"
          value={classForm.end_time}
          onChange={(event) => setClassForm((prev) => ({ ...prev, end_time: event.target.value }))}
        />
        <div className="day-selector">
          <span className="field-label">Days of week</span>
          <div className="day-grid">
            {dayOptions.map((day) => {
              const selected = classForm.days_of_week.includes(day.value);
              return (
                <button
                  key={day.value}
                  type="button"
                  className={`day-pill ${selected ? 'selected' : ''}`}
                  onClick={() => toggleDay(day.value)}
                  aria-pressed={selected}
                >
                  {day.label}
                </button>
              );
            })}
          </div>
        </div>
        <button type="submit">Save Class</button>
      </form>
    </section>
  );

  const renderClassDetail = () => (
    <section className="admin-section">
      <div className="minimal-actions split-actions">
        <button type="button" onClick={handleBackToDashboard}>Back</button>
        <div className="split-actions">
          <button type="button" onClick={openRoster}>Roster</button>
          <button type="button" onClick={openCreateAssignment}>Create Assignment</button>
        </div>
      </div>
      <h3>{selectedClass?.class_name || 'Class'}</h3>
      <p className="field-hint">Assignments are shown in creation order.</p>

      {assignments.length === 0 ? (
        <p>No assignments yet for this class.</p>
      ) : (
        <div className="admin-card-list vertical">
          {assignments.map((assignment) => (
            <button
              key={assignment.id}
              type="button"
              className="entity-card"
              onClick={() => openEditAssignment(assignment)}
            >
              <div className="entity-card-title">{assignment.title}</div>
              <div className="entity-card-meta">{assignment.type_name}</div>
              <div className="entity-card-meta">
                {assignment.due_date || 'No due date'} {assignment.due_time ? `at ${normalizeTime(assignment.due_time)}` : ''}
              </div>
            </button>
          ))}
        </div>
      )}
    </section>
  );

  const renderRoster = () => (
    <section className="admin-section">
      <div className="minimal-actions split-actions">
        <button type="button" onClick={handleBackToClassDetail}>Back</button>
        <button type="button" onClick={openAddStudentModal}>Add Student</button>
      </div>
      <h3>{selectedClass?.class_name || 'Class'} Roster</h3>

      {loadingRoster ? (
        <p>Loading roster...</p>
      ) : roster.length === 0 ? (
        <p>No students enrolled yet.</p>
      ) : (
        <div className="admin-card-list vertical">
          {roster.map((student) => (
            <div key={student.id} className="entity-card static-card">
              <div className="entity-card-title">{student.name || 'Unnamed student'}</div>
              <div className="entity-card-meta">{student.email}</div>
            </div>
          ))}
        </div>
      )}
    </section>
  );

  const renderAssignmentForm = (mode) => (
    <section className="admin-section">
      <div className="minimal-actions">
        <button type="button" onClick={handleBackToClassDetail}>Back</button>
      </div>
      <h3>{mode === 'edit' ? 'Edit Assignment' : 'Create Assignment'}</h3>
      <p className="field-hint">Class: {selectedClass?.class_name || 'Unknown class'}</p>
      <form onSubmit={mode === 'edit' ? submitEditAssignment : submitCreateAssignment} className="admin-form-grid">
        <select
          value={assignmentForm.assignment_type_id}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, assignment_type_id: event.target.value }))}
        >
          <option value="">Select assignment type *</option>
          {assignmentTypes.map((type) => (
            <option key={type.id} value={type.id}>
              {type.type_name} (avg {type.avg_completion_hours}h)
            </option>
          ))}
        </select>
        <input
          type="text"
          placeholder="Title *"
          value={assignmentForm.title}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, title: event.target.value }))}
        />
        <textarea
          placeholder="Description"
          value={assignmentForm.description}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, description: event.target.value }))}
        />
        <input
          type="date"
          value={assignmentForm.due_date}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, due_date: event.target.value }))}
        />
        <input
          type="time"
          value={assignmentForm.due_time}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, due_time: event.target.value }))}
        />
        <input
          type="number"
          min="0"
          step="1"
          placeholder="Assignment length (in hours)"
          value={assignmentForm.assignment_time_prediction}
          onChange={(event) => setAssignmentForm((prev) => ({ ...prev, assignment_time_prediction: event.target.value }))}
        />
        <p className="field-hint">Optional. Leave blank to use the saved average for this assignment type.</p>
        <button type="submit">{mode === 'edit' ? 'Save Changes' : 'Create Assignment'}</button>
      </form>
    </section>
  );

  return (
    <section className="admin-card">
      <header className="homepage-header">
        <div className="profile-container">
          <span>Profile</span>
          <div className="email-tooltip">{user.email}</div>
        </div>
        <button onClick={onLogout} className="logout-button">
          Log Out
        </button>
      </header>

      <h2>Your Dashboard</h2>
      {error && <p className="result error-message">{error}</p>}

      {loading && view === 'dashboard' ? (
        <p>Loading admin data...</p>
      ) : (
        <>
          {view === 'dashboard' && renderDashboard()}
          {view === 'createClass' && renderCreateClass()}
          {view === 'classDetail' && renderClassDetail()}
          {view === 'roster' && renderRoster()}
          {view === 'createAssignment' && renderAssignmentForm('create')}
          {view === 'editAssignment' && renderAssignmentForm('edit')}
        </>
      )}

      {showAddStudentModal && (
        <div className="modal-backdrop" role="presentation" onClick={closeAddStudentModal}>
          <div className="modal-card" role="dialog" aria-modal="true" onClick={(event) => event.stopPropagation()}>
            <h3>Add Student</h3>
            <form onSubmit={submitAddStudent} className="admin-form-grid">
              <input
                type="text"
                placeholder="Student name"
                value={studentForm.name}
                onChange={(event) => setStudentForm((prev) => ({ ...prev, name: event.target.value }))}
              />
              <input
                type="email"
                placeholder="Student email *"
                value={studentForm.email}
                onChange={(event) => setStudentForm((prev) => ({ ...prev, email: event.target.value }))}
              />
              <div className="minimal-actions split-actions">
                <button type="button" onClick={closeAddStudentModal}>Cancel</button>
                <button type="submit">Add</button>
              </div>
            </form>
          </div>
        </div>
      )}
    </section>
  );
}

export default AdminPage;
