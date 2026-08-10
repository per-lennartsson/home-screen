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

// Presets for the recurring full-refresh schedule, in seconds. "" maps to null (off).
const FULL_REFRESH_PRESETS = [
  { value: "", label: "Off" },
  { value: "3600", label: "Every hour" },
  { value: "21600", label: "Every 6 hours" },
  { value: "86400", label: "Every day" },
  { value: "604800", label: "Every week" },
];

function BatterySparkline({ history }) {
  if (history.length < 2) return null;
  const w = 260;
  const h = 48;
  const pad = 4;
  const mvs = history.map((r) => r.battery_mv);
  const min = Math.min(...mvs);
  const max = Math.max(...mvs);
  const range = max - min || 1;
  const points = history
    .map((r, i) => {
      const x = pad + (i / (history.length - 1)) * (w - pad * 2);
      const y = pad + (1 - (r.battery_mv - min) / range) * (h - pad * 2);
      return `${x},${y}`;
    })
    .join(" ");
  return (
    <svg width={w} height={h} viewBox={`0 0 ${w} ${h}`}>
      <polyline points={points} fill="none" stroke="currentColor" strokeWidth="1.5" />
    </svg>
  );
}

function batteryEstimateLabel(estimate, historyLength) {
  if (!estimate) return null;
  const at = `at a ${estimate.wake_interval_s}s wake interval`;
  if (estimate.status === "insufficient_data") {
    if (historyLength === 0) return "No battery readings logged yet.";
    if (estimate.sample_count > 0) {
      const span = estimate.span_hours != null ? `over ${estimate.span_hours.toFixed(2)}h so far` : "so far";
      return (
        `${estimate.sample_count} readings logged ${at} ${span} — not enough elapsed time yet to trust a drain ` +
        `estimate, and no other wake intervals logged yet to project one from instead. Check back later.`
      );
    }
    return `No readings logged yet ${at}, and no other wake intervals logged to project an estimate from instead.`;
  }
  if (estimate.status === "not_draining") {
    return `Battery isn't trending down ${at} — no estimate yet.`;
  }
  const days = estimate.estimated_days_remaining;
  const remaining = days < 1 ? `${Math.round(days * 24)} hours` : `${days.toFixed(1)} days`;
  if (estimate.status === "modeled") {
    return (
      `~${remaining} remaining ${at} (projected — this display hasn't run at that interval; ` +
      `estimate is fit from ${estimate.sample_count} readings logged at other intervals).`
    );
  }
  return (
    `~${remaining} remaining ${at}, based on ` +
    `${estimate.drain_mv_per_hour.toFixed(2)} mV/hour drain over ${estimate.span_hours.toFixed(1)}h of history ` +
    `(${estimate.sample_count} readings).`
  );
}

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
  const [editingWakeIntervalFor, setEditingWakeIntervalFor] = useState(null);
  const [wakeIntervalDraft, setWakeIntervalDraft] = useState(15);
  const [editingFullRefreshFor, setEditingFullRefreshFor] = useState(null);
  const [fullRefreshIntervalDraft, setFullRefreshIntervalDraft] = useState("");
  const [refreshingNowFor, setRefreshingNowFor] = useState(null);
  const [detailsFor, setDetailsFor] = useState(null);
  const [payload, setPayload] = useState(null);
  const [batteryLogFor, setBatteryLogFor] = useState(null);
  const [batteryHistory, setBatteryHistory] = useState([]);
  const [batteryEstimate, setBatteryEstimate] = useState(null);
  const [estimateIntervalDraft, setEstimateIntervalDraft] = useState(15);

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

  const startEditingWakeInterval = (display) => {
    setWakeIntervalDraft(display.wake_interval_s);
    setEditingWakeIntervalFor((v) => (v === display.id ? null : display.id));
  };

  const saveWakeInterval = async (displayId) => {
    setError(null);
    try {
      await api.setWakeInterval(displayId, +wakeIntervalDraft);
      setEditingWakeIntervalFor(null);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const forceFullRefresh = async (display) => {
    setError(null);
    setRefreshingNowFor(display.id);
    try {
      await api.forceFullRefresh(display.id);
      refresh();
    } catch (e) {
      setError(e.message);
    } finally {
      setRefreshingNowFor(null);
    }
  };

  const startEditingFullRefresh = (display) => {
    setFullRefreshIntervalDraft(display.full_refresh_interval_s ? String(display.full_refresh_interval_s) : "");
    setEditingFullRefreshFor((v) => (v === display.id ? null : display.id));
  };

  const saveFullRefreshInterval = async (displayId) => {
    setError(null);
    try {
      await api.setFullRefreshInterval(displayId, fullRefreshIntervalDraft === "" ? null : +fullRefreshIntervalDraft);
      setEditingFullRefreshFor(null);
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

  const toggleBatteryLog = async (display) => {
    if (batteryLogFor === display.id) {
      setBatteryLogFor(null);
      return;
    }
    try {
      const [history, estimate] = await Promise.all([
        api.getBatteryHistory(display.id),
        api.getBatteryEstimate(display.id),
      ]);
      setBatteryHistory(history);
      setBatteryEstimate(estimate);
      setEstimateIntervalDraft(display.wake_interval_s);
      setBatteryLogFor(display.id);
    } catch (e) {
      setError(e.message);
    }
  };

  const updateBatteryEstimate = async (displayId) => {
    setError(null);
    try {
      setBatteryEstimate(await api.getBatteryEstimate(displayId, +estimateIntervalDraft));
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
                    {" · "}
                    wakes every {d.wake_interval_s}s
                  </div>
                  <div className="muted">
                    {d.last_full_refresh_at
                      ? `Full refresh ${relativeTime(d.last_full_refresh_at)}`
                      : "Never fully refreshed"}
                    {" · "}
                    {d.full_refresh_interval_s
                      ? FULL_REFRESH_PRESETS.find((p) => +p.value === d.full_refresh_interval_s)?.label ||
                        `every ${d.full_refresh_interval_s}s`
                      : "no schedule"}
                    {d.full_refresh_due && " · due on next wake"}
                  </div>
                </div>

                {editingWakeIntervalFor === d.id && (
                  <div className="entity-card-body">
                    <div className="field">
                      <label>Wake interval</label>
                      <div style={{ display: "flex", alignItems: "center", gap: 9 }}>
                        <input
                          type="number"
                          min={5}
                          max={3600}
                          value={wakeIntervalDraft}
                          onChange={(e) => setWakeIntervalDraft(e.target.value)}
                          style={{ width: 90 }}
                          autoFocus
                        />
                        <span className="muted" style={{ fontSize: "12.5px" }}>
                          seconds
                        </span>
                        <button type="button" className="btn small" onClick={() => saveWakeInterval(d.id)}>
                          Save
                        </button>
                      </div>
                      <div className="muted" style={{ marginTop: 4 }}>
                        How often this panel wakes from deep sleep to check in and poll for new content. Lower
                        values mean fresher data at the cost of battery life.
                      </div>
                    </div>
                  </div>
                )}

                {batteryLogFor === d.id && (
                  <div className="entity-card-body">
                    <div className="field">
                      <label>Battery log</label>
                      {batteryHistory.length >= 2 ? (
                        <div className="muted" style={{ color: "var(--fg)" }}>
                          <BatterySparkline history={batteryHistory} />
                        </div>
                      ) : (
                        <div className="muted">Not enough readings yet to chart — check back after a few wake cycles.</div>
                      )}
                      <div className="muted" style={{ marginTop: 4 }}>
                        {batteryEstimateLabel(batteryEstimate, batteryHistory.length)}
                      </div>
                      <div style={{ display: "flex", alignItems: "center", gap: 9, marginTop: 8 }}>
                        <input
                          type="number"
                          min={5}
                          max={3600}
                          value={estimateIntervalDraft}
                          onChange={(e) => setEstimateIntervalDraft(e.target.value)}
                          style={{ width: 90 }}
                        />
                        <span className="muted" style={{ fontSize: "12.5px" }}>
                          seconds
                        </span>
                        <button type="button" className="btn small" onClick={() => updateBatteryEstimate(d.id)}>
                          Estimate for this interval
                        </button>
                      </div>
                      <div className="muted" style={{ marginTop: 4 }}>
                        Estimate battery life for a wake interval you haven't used yet (e.g. 900s for 15 minutes) —
                        projected from drain rates measured at whichever intervals this panel has actually run at.
                      </div>
                    </div>
                  </div>
                )}

                {editingFullRefreshFor === d.id && (
                  <div className="entity-card-body">
                    <div className="field">
                      <label>Full refresh schedule</label>
                      <div style={{ display: "flex", alignItems: "center", gap: 9 }}>
                        <select
                          value={fullRefreshIntervalDraft}
                          onChange={(e) => setFullRefreshIntervalDraft(e.target.value)}
                          autoFocus
                        >
                          {FULL_REFRESH_PRESETS.map((p) => (
                            <option key={p.value} value={p.value}>
                              {p.label}
                            </option>
                          ))}
                        </select>
                        <button type="button" className="btn small" onClick={() => saveFullRefreshInterval(d.id)}>
                          Save
                        </button>
                      </div>
                      <div className="muted" style={{ marginTop: 4 }}>
                        Automatically does a hardware flash-and-redraw on this schedule, in addition to any manual
                        "Full refresh now" presses, to clear ghosting that partial refreshes accumulate over time.
                      </div>
                    </div>
                  </div>
                )}

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
                  <button
                    type="button"
                    className="btn secondary small"
                    title="Configure how often this panel wakes up to check in and poll for new content"
                    onClick={() => startEditingWakeInterval(d)}
                  >
                    Wake interval
                  </button>
                  <button
                    type="button"
                    className="btn secondary small"
                    title="Flash and redraw the panel now to clear e-paper ghosting"
                    disabled={refreshingNowFor === d.id}
                    onClick={() => forceFullRefresh(d)}
                  >
                    {refreshingNowFor === d.id ? "Refreshing…" : "Full refresh now"}
                  </button>
                  <button
                    type="button"
                    className="btn secondary small"
                    title="Automatically clear ghosting on a recurring schedule"
                    onClick={() => startEditingFullRefresh(d)}
                  >
                    Refresh schedule
                  </button>
                  <button
                    type="button"
                    className="btn secondary small"
                    title="View logged battery history and an estimated remaining life"
                    onClick={() => toggleBatteryLog(d)}
                  >
                    Battery log
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
