const API_URL = import.meta.env.VITE_API_URL || '/api';

export function apiUrl(path) {
    return `${API_URL}${path}`;
}