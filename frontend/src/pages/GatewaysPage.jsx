import { useEffect, useState } from "react";
import { api } from "../api/client.js";
import { relativeTime } from "../lib/format.js";
import { isGatewayOnline } from "../lib/status.js";

export default function GatewaysPage() {
  const [gateways, setGateways] = useState([]);
  const [displays, setDisplays] = useState([]);
  const [showAddPanel, setShowAddPanel] = useState(false);
  const [name, setName] = useState("");
  const [location, setLocation] = useState("");
  const [error, setError] = useState(null);

  const refresh = () => {
    api.listGateways().then(setGateways).catch((e) => setError(e.message));
    api.listDisplays().then(setDisplays).catch(() => {});
  };

  useEffect(() => {
    refresh();
    const interval = setInterval(refresh, 5000);
    return () => clearInterval(interval);
  }, []);

  const submit = async (e) => {
    e.preventDefault();
    setError(null);
    try {
      await api.createGateway({ name, location: location || null });
      setName("");
      setLocation("");
      setShowAddPanel(false);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const remove = async (gateway) => {
    if (!window.confirm(`Delete gateway "${gateway.name}"? This can't be undone.`)) return;
    setError(null);
    try {
      await api.deleteGateway(gateway.id);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  return (
    <>
      <header className="page-header">
        <div className="page-header-titles">
          <h1>Gateways</h1>
          <span className="page-header-meta">Bridges between this server and the panels over BLE</span>
        </div>
        <div className="page-header-actions">
          <button type="button" className="btn" onClick={() => setShowAddPanel((v) => !v)}>
            Register gateway
          </button>
        </div>
      </header>

      <div className="page-body">
        {error && <div className="error">{error}</div>}

        {showAddPanel && (
          <div className="panel add-panel">
            <form onSubmit={submit} className="row">
              <div className="field">
                <label>Name</label>
                <input value={name} onChange={(e) => setName(e.target.value)} required />
              </div>
              <div className="field">
                <label>Location</label>
                <input value={location} onChange={(e) => setLocation(e.target.value)} />
              </div>
              <button type="submit" className="btn">
                Register gateway
              </button>
            </form>
          </div>
        )}

        <div className="card-grid narrow">
          {gateways.map((g) => {
            const online = isGatewayOnline(g);
            const served = displays.filter((d) => d.gateway_id === g.id);
            const dependents = served.map((d) => d.name);

            return (
              <div key={g.id} className={`entity-card${online ? "" : " attention"}`}>
                <div className="entity-card-head">
                  <div>
                    <div className="entity-card-title">{g.name}</div>
                    <div className="entity-card-sub">
                      {g.location || "No location set"} · ID {g.id}
                    </div>
                  </div>
                  <span className={`status-badge ${online ? "ok" : "warn"}`}>
                    <span className="status-dot" />
                    {online ? "Online" : "Offline"}
                  </span>
                </div>
                <div className="card-stats">
                  <div>
                    <div className="card-stat-label">Last check-in</div>
                    <div className="card-stat-value" style={{ color: g.last_checkin_at ? undefined : "var(--warn)" }}>
                      {relativeTime(g.last_checkin_at)}
                    </div>
                  </div>
                  <div>
                    <div className="card-stat-label">Panels served</div>
                    <div className="card-stat-value">
                      {served.length} of {displays.length}
                    </div>
                  </div>
                </div>
                {!online && dependents.length > 0 && (
                  <div className="card-note">
                    {dependents.join(", ")} depend{dependents.length === 1 ? "s" : ""} on this gateway. Check that the
                    agent is running and can reach this server.
                  </div>
                )}
                <div className="entity-card-footer">
                  <button
                    type="button"
                    className="btn danger small"
                    disabled={served.length > 0}
                    title={served.length > 0 ? "Unassign its displays first" : undefined}
                    onClick={() => remove(g)}
                  >
                    Delete
                  </button>
                </div>
              </div>
            );
          })}
        </div>

        {gateways.length === 0 && <div className="muted">No gateways registered yet.</div>}
      </div>
    </>
  );
}
