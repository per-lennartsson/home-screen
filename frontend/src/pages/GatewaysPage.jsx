import { useEffect, useState } from "react";
import { api } from "../api/client.js";

export default function GatewaysPage() {
  const [gateways, setGateways] = useState([]);
  const [name, setName] = useState("");
  const [location, setLocation] = useState("");
  const [error, setError] = useState(null);

  const refresh = () => api.listGateways().then(setGateways).catch((e) => setError(e.message));

  useEffect(() => {
    refresh();
  }, []);

  const submit = async (e) => {
    e.preventDefault();
    setError(null);
    try {
      await api.createGateway({ name, location: location || null });
      setName("");
      setLocation("");
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  return (
    <>
      <h2>Gateways</h2>
      {error && <div className="error">{error}</div>}

      <div className="card">
        <form onSubmit={submit} className="row">
          <div className="field">
            <label>Name</label>
            <input value={name} onChange={(e) => setName(e.target.value)} required />
          </div>
          <div className="field">
            <label>Location</label>
            <input value={location} onChange={(e) => setLocation(e.target.value)} />
          </div>
          <button type="submit">Register gateway</button>
        </form>
      </div>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>Location</th>
              <th>Last check-in</th>
            </tr>
          </thead>
          <tbody>
            {gateways.map((g) => (
              <tr key={g.id}>
                <td>{g.id}</td>
                <td>{g.name}</td>
                <td>{g.location || "-"}</td>
                <td>{g.last_checkin_at ? new Date(g.last_checkin_at).toLocaleString() : "never"}</td>
              </tr>
            ))}
            {gateways.length === 0 && (
              <tr>
                <td colSpan={4} className="muted">
                  No gateways registered yet.
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </>
  );
}
