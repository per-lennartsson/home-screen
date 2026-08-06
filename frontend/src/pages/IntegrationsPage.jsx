import { useEffect, useState } from "react";
import { api } from "../api/client.js";

export default function IntegrationsPage() {
  const [config, setConfig] = useState(null);
  const [baseUrl, setBaseUrl] = useState("");
  const [token, setToken] = useState("");
  const [pollIntervalS, setPollIntervalS] = useState(30);
  const [error, setError] = useState(null);
  const [saved, setSaved] = useState(false);

  const refresh = () =>
    api
      .getHomeAssistantConfig()
      .then((c) => {
        setConfig(c);
        setBaseUrl(c.base_url || "");
        setPollIntervalS(c.poll_interval_s);
      })
      .catch((e) => setError(e.message));

  useEffect(() => {
    refresh();
  }, []);

  const save = async (e) => {
    e.preventDefault();
    setError(null);
    setSaved(false);
    try {
      await api.setHomeAssistantConfig({
        base_url: baseUrl,
        access_token: token || null,
        poll_interval_s: pollIntervalS,
      });
      setToken("");
      setSaved(true);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  return (
    <>
      <h2>Integrations</h2>
      {error && <div className="error">{error}</div>}

      <div className="card">
        <h3 style={{ marginTop: 0 }}>Home Assistant</h3>
        <p className="muted">
          Value elements can bind to a Home Assistant entity instead of a static value — set the
          connection here, then pick "Home Assistant entity" as the source when adding a value
          element in the design editor. Polled on the interval below; changes propagate to
          displays through the normal full/diff sync.
        </p>
        <form onSubmit={save}>
          <div className="field">
            <label>Base URL</label>
            <input
              value={baseUrl}
              onChange={(e) => setBaseUrl(e.target.value)}
              placeholder="http://homeassistant.local:8123"
            />
          </div>
          <div className="field">
            <label>
              Long-lived access token
              {config?.token_set && <span className="muted"> (currently set — leave blank to keep it)</span>}
            </label>
            <input
              type="password"
              value={token}
              onChange={(e) => setToken(e.target.value)}
              placeholder={config?.token_set ? "••••••••" : "paste a long-lived access token"}
            />
          </div>
          <div className="field">
            <label>Poll interval (seconds)</label>
            <input
              type="number"
              min={5}
              max={3600}
              value={pollIntervalS}
              onChange={(e) => setPollIntervalS(+e.target.value)}
              style={{ width: 90 }}
            />
          </div>
          <button type="submit">Save</button>
          {saved && <span className="muted" style={{ marginLeft: 8 }}>Saved.</span>}
        </form>
      </div>
    </>
  );
}
