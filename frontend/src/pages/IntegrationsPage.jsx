import { useEffect, useMemo, useState } from "react";
import { Link } from "react-router-dom";
import { api } from "../api/client.js";
import { relativeTime } from "../lib/format.js";
import { fromLayoutJson, haEntityRefs } from "../lib/layout.js";
import { entityValueKey, useEntityValues } from "../lib/useEntityValues.js";

export default function IntegrationsPage() {
  const [config, setConfig] = useState(null);
  const [designs, setDesigns] = useState([]);
  const [baseUrl, setBaseUrl] = useState("");
  const [token, setToken] = useState("");
  const [pollIntervalS, setPollIntervalS] = useState(30);
  const [error, setError] = useState(null);

  const refresh = () => {
    api
      .getHomeAssistantConfig()
      .then((c) => {
        setConfig(c);
        setBaseUrl(c.base_url || "");
        setPollIntervalS(c.poll_interval_s);
      })
      .catch((e) => setError(e.message));
    api.listDesigns().then(setDesigns).catch(() => {});
  };

  useEffect(() => {
    refresh();
  }, []);

  const boundRefs = useMemo(() => {
    const seen = new Set();
    const refs = [];
    for (const design of designs) {
      for (const ref of haEntityRefs(fromLayoutJson(design.layout_json))) {
        const key = `${ref.entityId}::${ref.attribute || ""}`;
        if (seen.has(key)) continue;
        seen.add(key);
        refs.push(ref);
      }
    }
    return refs;
  }, [designs]);
  const [entityValues] = useEntityValues(boundRefs);

  const save = async (e) => {
    e.preventDefault();
    setError(null);
    try {
      await api.setHomeAssistantConfig({
        base_url: baseUrl,
        access_token: token || null,
        poll_interval_s: pollIntervalS,
      });
      setToken("");
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const configured = !!config?.base_url;

  return (
    <>
      <header className="page-header">
        <div className="page-header-titles">
          <h1>Home Assistant</h1>
        </div>
      </header>

      <div className="page-body" style={{ maxWidth: 560 }}>
        {error && <div className="error">{error}</div>}

        <div className="panel">
          <div
            style={{
              display: "flex",
              alignItems: "center",
              justifyContent: "space-between",
              gap: 12,
              paddingBottom: 14,
              borderBottom: "1px solid var(--border)",
            }}
          >
            <div style={{ display: "flex", alignItems: "center", gap: 9 }}>
              <span className={`status-dot ${configured ? "ok" : "warn"}`} />
              <span style={{ fontSize: 13.5, fontWeight: 550 }}>{configured ? "Configured" : "Not configured"}</span>
              {configured && (
                <span className="muted">
                  {boundRefs.length} bound entit{boundRefs.length === 1 ? "y" : "ies"} · polled every {pollIntervalS} s
                </span>
              )}
            </div>
          </div>

          <p className="muted" style={{ lineHeight: 1.5 }}>
            Value elements can bind to a Home Assistant entity instead of a static value — set the connection here,
            then pick "Home Assistant" as the source when adding a value element in the design editor. Polled on the
            interval below; changes propagate to displays through the normal full/diff sync.
          </p>

          <form onSubmit={save} style={{ paddingTop: 4 }}>
            <div className="field">
              <label>Base URL</label>
              <input
                value={baseUrl}
                onChange={(e) => setBaseUrl(e.target.value)}
                placeholder="http://homeassistant.local:8123"
                style={{ fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace", width: "100%" }}
              />
            </div>
            <div className="field">
              <label>
                Long-lived access token
                {config?.token_set && <span className="muted"> (currently set — leave blank to keep it)</span>}
              </label>
              <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
                <input
                  type="password"
                  value={token}
                  onChange={(e) => setToken(e.target.value)}
                  placeholder={config?.token_set ? "••••••••" : "paste a long-lived access token"}
                  style={{ flex: 1 }}
                />
                {config?.token_set && <span style={{ fontSize: 12, color: "var(--ok)", whiteSpace: "nowrap" }}>Token saved</span>}
              </div>
            </div>
            <div className="field">
              <label>Poll interval</label>
              <div style={{ display: "flex", alignItems: "center", gap: 9 }}>
                <input
                  type="number"
                  min={5}
                  max={3600}
                  value={pollIntervalS}
                  onChange={(e) => setPollIntervalS(+e.target.value)}
                  style={{ width: 90 }}
                />
                <span className="muted" style={{ fontSize: "12.5px" }}>
                  seconds
                </span>
              </div>
              <div className="muted" style={{ marginTop: 4 }}>
                Panels only redraw when a bound value actually changes, so a short interval costs little battery.
              </div>
            </div>
            <div style={{ display: "flex", alignItems: "center", gap: 10, paddingTop: 4 }}>
              <button type="submit" className="btn">
                Save
              </button>
              <span className="muted">Saved {relativeTime(config?.updated_at)}</span>
            </div>
          </form>
        </div>

        {boundRefs.length > 0 && (
          <div className="panel" style={{ background: "var(--bg2)" }}>
            <div style={{ fontSize: 13, fontWeight: 550, marginBottom: 10 }}>Bound entities in use</div>
            <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
              {boundRefs.map((ref) => {
                const cached = entityValues[entityValueKey(ref.entityId, ref.attribute)];
                return (
                  <div
                    key={entityValueKey(ref.entityId, ref.attribute)}
                    style={{ display: "flex", justifyContent: "space-between", gap: 8, fontSize: 12.5 }}
                  >
                    <span style={{ fontFamily: "ui-monospace, SFMono-Regular, Menlo, monospace" }}>{ref.entityId}</span>
                    <span style={{ color: cached?.error ? "var(--warn)" : "var(--muted)" }}>
                      {!cached ? "…" : cached.error || cached.value}
                    </span>
                  </div>
                );
              })}
            </div>
            <div className="muted" style={{ marginTop: 10 }}>
              Used across {designs.filter((d) => haEntityRefs(fromLayoutJson(d.layout_json)).length > 0).length} design(s)
              — manage bindings from <Link to="/designs">Designs</Link>.
            </div>
          </div>
        )}
      </div>
    </>
  );
}
