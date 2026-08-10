import { useEffect, useMemo, useRef, useState } from "react";
import { api } from "../api/client.js";
import DesignCanvas from "../components/DesignCanvas.jsx";
import { fromLayoutJson, haEntityRefs } from "../lib/layout.js";
import { parseUtc, relativeTime } from "../lib/format.js";
import { displayStatus, isGatewayOnline } from "../lib/status.js";
import { useEntityValues } from "../lib/useEntityValues.js";

const THUMB_WIDTH = 284;
const STALE_HOURS = 6;
const LOW_BATTERY_PCT = 20;

const STATUS_LABEL = { in_sync: "In sync", pending: "Update pending", never: "Never seen" };

function warningFor(display) {
  const lowBattery = display.battery_pct != null && display.battery_pct < LOW_BATTERY_PCT;
  const hoursSinceSeen = display.last_seen_at
    ? (Date.now() - parseUtc(display.last_seen_at).getTime()) / 3600000
    : null;
  const stale = hoursSinceSeen != null && hoursSinceSeen > STALE_HOURS;
  if (!lowBattery && !stale) return null;

  const parts = [];
  if (lowBattery) parts.push(`is at ${display.battery_pct}% battery`);
  if (stale) parts.push(`hasn't checked in for ${relativeTime(display.last_seen_at)}`);
  return `${display.name} ${parts.join(" and ")}.`;
}

export default function DisplaysPage() {
  const [displays, setDisplays] = useState([]);
  const [gateways, setGateways] = useState([]);
  const [designs, setDesigns] = useState([]);
  const [error, setError] = useState(null);

  const [showAddPanel, setShowAddPanel] = useState(false);
  const [name, setName] = useState("");
  const [mac, setMac] = useState("");
  const [gatewayId, setGatewayId] = useState("");
  const [width, setWidth] = useState(400);
  const [height, setHeight] = useState(300);

  const [changingDesignFor, setChangingDesignFor] = useState(null);
  const [assigningGatewayFor, setAssigningGatewayFor] = useState(null);
  const [detailsFor, setDetailsFor] = useState(null);
  const [payload, setPayload] = useState(null);

  const cardRefs = useRef({});

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

  const designsById = useMemo(() => new Map(designs.map((d) => [d.id, d])), [designs]);
  const gatewaysById = useMemo(() => new Map(gateways.map((g) => [g.id, g])), [gateways]);

  const elementsByDesignId = useMemo(() => {
    const map = new Map();
    for (const design of designs) map.set(design.id, fromLayoutJson(design.layout_json));
    return map;
  }, [designs]);

  const entityRefs = useMemo(() => {
    const refs = [];
    const seen = new Set();
    for (const elements of elementsByDesignId.values()) {
      for (const ref of haEntityRefs(elements)) {
        const key = `${ref.entityId}::${ref.attribute || ""}`;
        if (seen.has(key)) continue;
        seen.add(key);
        refs.push(ref);
      }
    }
    return refs;
  }, [elementsByDesignId]);
  const [entityValues] = useEntityValues(entityRefs);

  const onlineGateways = gateways.filter(isGatewayOnline).length;
  const warningDisplay = displays.find(warningFor);

  const submit = async (e) => {
    e.preventDefault();
    setError(null);
    try {
      await api.createDisplay({ name, mac_address: mac, gateway_id: gatewayId ? +gatewayId : null, width, height });
      setName("");
      setMac("");
      setGatewayId("");
      setWidth(400);
      setHeight(300);
      setShowAddPanel(false);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const assignDesign = async (displayId, designId) => {
    if (!designId) return;
    try {
      await api.assignDesign(displayId, +designId);
      setChangingDesignFor(null);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const assignGateway = async (displayId, gwId) => {
    if (!gwId) return;
    try {
      await api.assignGateway(displayId, +gwId);
      setAssigningGatewayFor(null);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const toggleRotation = async (display) => {
    setError(null);
    try {
      await api.setRotation(display.id, !display.rotate_180);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const remove = async (display) => {
    if (!window.confirm(`Delete display "${display.name}"? This can't be undone.`)) return;
    setError(null);
    try {
      await api.deleteDisplay(display.id);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const toggleDetails = async (displayId) => {
    if (detailsFor === displayId) {
      setDetailsFor(null);
      return;
    }
    try {
      const result = await api.getPayload(displayId);
      setPayload(result);
      setDetailsFor(displayId);
    } catch (e) {
      setError(e.message);
    }
  };

  return (
    <>
      <header className="page-header">
        <div className="page-header-titles">
          <h1>Displays</h1>
          <span className="page-header-meta">
            {displays.length} panel{displays.length === 1 ? "" : "s"} · {onlineGateways} of {gateways.length} gateways
            online
          </span>
        </div>
        <div className="page-header-actions">
          <button type="button" className="btn secondary" onClick={refresh}>
            Sync all now
          </button>
          <button type="button" className="btn" onClick={() => setShowAddPanel((v) => !v)}>
            Add display
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
              <div className="field">
                <label>Resolution</label>
                <div style={{ display: "flex", gap: 4, alignItems: "center" }}>
                  <input type="number" value={width} onChange={(e) => setWidth(+e.target.value)} style={{ width: 60 }} />
                  <span className="muted">×</span>
                  <input type="number" value={height} onChange={(e) => setHeight(+e.target.value)} style={{ width: 60 }} />
                </div>
              </div>
              <button type="submit" className="btn">
                Register display
              </button>
            </form>
          </div>
        )}

        {warningDisplay && (
          <div className="banner warn">
            <span className="dot" />
            <div>{warningFor(warningDisplay)}</div>
            <a
              href="#"
              onClick={(e) => {
                e.preventDefault();
                cardRefs.current[warningDisplay.id]?.scrollIntoView({ behavior: "smooth", block: "center" });
              }}
            >
              Troubleshoot
            </a>
          </div>
        )}

        <div className="card-grid">
          {displays.map((d) => {
            const design = d.design_id ? designsById.get(d.design_id) : null;
            const gateway = d.gateway_id ? gatewaysById.get(d.gateway_id) : null;
            const status = displayStatus(d);
            const elements = design ? elementsByDesignId.get(design.id) || [] : [];
            const batteryClass = d.battery_pct != null && d.battery_pct < LOW_BATTERY_PCT ? "warn" : "ok";

            return (
              <div
                key={d.id}
                className={`entity-card${status === "pending" ? " attention" : ""}`}
                ref={(el) => (cardRefs.current[d.id] = el)}
              >
                <div className="entity-card-head">
                  <div>
                    <div className="entity-card-title">{d.name}</div>
                    <div className="entity-card-sub">{gateway ? `via ${gateway.name}` : "No gateway assigned"}</div>
                  </div>
                  <span className={`status-badge ${status === "in_sync" ? "ok" : status === "pending" ? "warn" : "muted"}`}>
                    <span className="status-dot" />
                    {STATUS_LABEL[status]}
                  </span>
                </div>

                <div className="thumbnail-wrap">
                  {design ? (
                    <div className={status === "pending" ? "thumbnail-stale" : undefined} style={{ position: "relative" }}>
                      <DesignCanvas
                        elements={elements}
                        width={design.width}
                        height={design.height}
                        cssWidth={THUMB_WIDTH}
                        entityValues={entityValues}
                        className="card-thumb"
                      />
                    </div>
                  ) : (
                    <div className="thumbnail-empty">
                      <div className="muted">No design assigned</div>
                      <button type="button" className="btn secondary small" onClick={() => setChangingDesignFor(d.id)}>
                        Assign a design
                      </button>
                    </div>
                  )}
                </div>

                {changingDesignFor === d.id && (
                  <div className="entity-card-body">
                    <select defaultValue="" onChange={(e) => assignDesign(d.id, e.target.value)} autoFocus>
                      <option value="" disabled>
                        choose a design…
                      </option>
                      {designs.map((des) => {
                        const mismatch = des.width !== d.width || des.height !== d.height;
                        return (
                          <option key={des.id} value={des.id} disabled={mismatch}>
                            {des.name} ({des.width}×{des.height}
                            {mismatch ? " — mismatch" : ""})
                          </option>
                        );
                      })}
                    </select>
                  </div>
                )}

                {assigningGatewayFor === d.id && (
                  <div className="entity-card-body">
                    <select defaultValue="" onChange={(e) => assignGateway(d.id, e.target.value)} autoFocus>
                      <option value="" disabled>
                        choose a gateway…
                      </option>
                      {gateways.map((g) => (
                        <option key={g.id} value={g.id}>
                          {g.name}
                        </option>
                      ))}
                    </select>
                  </div>
                )}

                <div className="entity-card-body">
                  <div className="battery-row">
                    <div className="battery-track">
                      {d.battery_pct != null && (
                        <div className={`battery-fill ${batteryClass}`} style={{ width: `${d.battery_pct}%` }} />
                      )}
                    </div>
                    <div className={`battery-label ${batteryClass === "warn" ? "warn" : ""}`}>
                      {d.battery_pct != null ? `${d.battery_pct}% · ${(d.battery_mv / 1000).toFixed(2)} V` : "Battery unknown"}
                    </div>
                  </div>
                  <div className="muted">
                    {d.last_seen_at ? `Refreshed ${relativeTime(d.last_seen_at)}` : "Waiting for first check-in"}
                  </div>
                </div>

                <div className="entity-card-footer">
                  {design && (
                    <button type="button" className="btn secondary small" onClick={() => setChangingDesignFor((v) => (v === d.id ? null : d.id))}>
                      Change design
                    </button>
                  )}
                  {!gateway && (
                    <button type="button" className="btn secondary small" onClick={() => setAssigningGatewayFor((v) => (v === d.id ? null : d.id))}>
                      Assign gateway
                    </button>
                  )}
                  <button
                    type="button"
                    className="btn secondary small"
                    title="Flip the rendered image 180° to match how this panel is physically mounted"
                    onClick={() => toggleRotation(d)}
                  >
                    {d.rotate_180 ? "Undo 180° rotation" : "Rotate 180°"}
                  </button>
                  <button type="button" className="btn ghost" onClick={() => toggleDetails(d.id)}>
                    Details
                  </button>
                  <button type="button" className="btn danger small" onClick={() => remove(d)}>
                    Delete
                  </button>
                </div>
              </div>
            );
          })}
        </div>

        {displays.length === 0 && <div className="muted">No displays registered yet.</div>}

        {detailsFor != null && payload && (
          <div className="panel" style={{ marginTop: 20 }}>
            <div className="muted" style={{ marginBottom: 6 }}>
              GET /api/displays/{detailsFor}/payload — this is what the gateway would fetch and push over BLE
            </div>
            <pre style={{ margin: 0, fontSize: 12, overflowX: "auto" }}>{JSON.stringify(payload, null, 2)}</pre>
          </div>
        )}
      </div>
    </>
  );
}
