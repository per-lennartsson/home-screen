import { useEffect, useState } from "react";
import { api } from "../api/client.js";

export default function DisplaysPage() {
  const [displays, setDisplays] = useState([]);
  const [gateways, setGateways] = useState([]);
  const [designs, setDesigns] = useState([]);
  const [name, setName] = useState("");
  const [mac, setMac] = useState("");
  const [gatewayId, setGatewayId] = useState("");
  const [error, setError] = useState(null);
  const [payloadPreview, setPayloadPreview] = useState(null);

  const refresh = () => {
    api.listDisplays().then(setDisplays).catch((e) => setError(e.message));
    api.listGateways().then(setGateways).catch(() => {});
    api.listDesigns().then(setDesigns).catch(() => {});
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
      await api.createDisplay({ name, mac_address: mac, gateway_id: gatewayId ? +gatewayId : null });
      setName("");
      setMac("");
      setGatewayId("");
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const assign = async (displayId, designId) => {
    if (!designId) return;
    try {
      await api.assignDesign(displayId, +designId);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const viewPayload = async (displayId) => {
    try {
      const payload = await api.getPayload(displayId);
      setPayloadPreview({ displayId, payload });
    } catch (e) {
      setError(e.message);
    }
  };

  return (
    <>
      <h2>Dashboard</h2>
      {error && <div className="error">{error}</div>}

      <div className="card">
        <form onSubmit={submit} className="row">
          <div className="field">
            <label>Name</label>
            <input value={name} onChange={(e) => setName(e.target.value)} required />
          </div>
          <div className="field">
            <label>MAC address</label>
            <input
              value={mac}
              onChange={(e) => setMac(e.target.value)}
              placeholder="AA:BB:CC:DD:EE:FF"
              required
            />
          </div>
          <div className="field">
            <label>Gateway</label>
            <select value={gatewayId} onChange={(e) => setGatewayId(e.target.value)}>
              <option value="">(unassigned)</option>
              {gateways.map((g) => (
                <option key={g.id} value={g.id}>
                  {g.name}
                </option>
              ))}
            </select>
          </div>
          <button type="submit">Register display</button>
        </form>
      </div>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>Name</th>
              <th>MAC</th>
              <th>Design</th>
              <th>Battery</th>
              <th>Last seen</th>
              <th>Status</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {displays.map((d) => (
              <tr key={d.id}>
                <td>{d.name}</td>
                <td>{d.mac_address}</td>
                <td>
                  <select defaultValue="" onChange={(e) => assign(d.id, e.target.value)}>
                    <option value="" disabled>
                      {designs.find((x) => x.id === d.design_id)?.name || "assign design..."}
                    </option>
                    {designs.map((des) => (
                      <option key={des.id} value={des.id}>
                        {des.name}
                      </option>
                    ))}
                  </select>
                </td>
                <td>{d.battery_pct != null ? `${d.battery_pct}% (${d.battery_mv}mV)` : "-"}</td>
                <td>{d.last_seen_at ? new Date(d.last_seen_at).toLocaleString() : "never"}</td>
                <td>
                  <span className={`badge ${d.in_sync ? "ok" : "warn"}`}>
                    {d.in_sync ? "in sync" : "pending update"}
                  </span>
                </td>
                <td>
                  <button type="button" className="secondary" onClick={() => viewPayload(d.id)}>
                    View payload
                  </button>
                </td>
              </tr>
            ))}
            {displays.length === 0 && (
              <tr>
                <td colSpan={7} className="muted">
                  No displays registered yet.
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>

      {payloadPreview && (
        <div className="card">
          <div className="muted" style={{ marginBottom: 6 }}>
            GET /api/displays/{payloadPreview.displayId}/payload — this is what the gateway would fetch
            and push over BLE
          </div>
          <pre style={{ margin: 0, fontSize: 12, overflowX: "auto" }}>
            {JSON.stringify(payloadPreview.payload, null, 2)}
          </pre>
        </div>
      )}
    </>
  );
}
