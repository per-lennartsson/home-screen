import { useEffect, useState } from "react";
import { NavLink, Route, Routes } from "react-router-dom";
import { api } from "./api/client.js";
import { relativeTime } from "./lib/format.js";
import { isGatewayOnline } from "./lib/status.js";
import DesignsPage from "./pages/DesignsPage.jsx";
import DisplaysPage from "./pages/DisplaysPage.jsx";
import GatewaysPage from "./pages/GatewaysPage.jsx";
import IntegrationsPage from "./pages/IntegrationsPage.jsx";

function NavItem({ to, label, count, dot }) {
  return (
    <NavLink to={to} className={({ isActive }) => `nav-item${isActive ? " active" : ""}`}>
      <span>{label}</span>
      {dot ? <span className={`status-dot ${dot}`} /> : <span className="nav-count">{count}</span>}
    </NavLink>
  );
}

export default function App() {
  const [displays, setDisplays] = useState([]);
  const [gateways, setGateways] = useState([]);
  const [designs, setDesigns] = useState([]);
  const [haConfig, setHaConfig] = useState(null);

  useEffect(() => {
    const refresh = () => {
      api.listDisplays().then(setDisplays).catch(() => {});
      api.listGateways().then(setGateways).catch(() => {});
      api.listDesigns().then(setDesigns).catch(() => {});
      api.getHomeAssistantConfig().then(setHaConfig).catch(() => {});
    };
    refresh();
    const interval = setInterval(refresh, 5000);
    return () => clearInterval(interval);
  }, []);

  const gatewaysDot = gateways.length === 0 || gateways.every(isGatewayOnline) ? "ok" : "warn";
  const haDot = haConfig?.base_url ? "ok" : "warn";

  const lastSeenTimes = displays.map((d) => d.last_seen_at).filter(Boolean);
  const mostRecentSeen = lastSeenTimes.length
    ? lastSeenTimes.reduce((a, b) => (new Date(a) > new Date(b) ? a : b))
    : null;
  const pendingCount = displays.filter((d) => d.last_seen_at && !d.in_sync).length;

  return (
    <div className="app-shell">
      <nav className="sidebar">
        <div className="sidebar-brand">
          <div className="sidebar-mark">
            <span />
            <span />
          </div>
          <div className="sidebar-title">ePaper</div>
        </div>

        <div className="sidebar-nav">
          <NavItem to="/" label="Displays" count={displays.length} />
          <NavItem to="/designs" label="Designs" count={designs.length} />
        </div>

        <div className="sidebar-section-label">Setup</div>
        <div className="sidebar-nav">
          <NavItem to="/gateways" label="Gateways" dot={gatewaysDot} />
          <NavItem to="/integrations" label="Home Assistant" dot={haDot} />
        </div>

        <div className="sidebar-footer">
          <div className="line">
            {displays.length ? `Last sync ${relativeTime(mostRecentSeen)}` : "No panels yet"}
          </div>
          <div className={`line${pendingCount > 0 ? " warn" : ""}`}>
            {pendingCount > 0 ? `${pendingCount} panel${pendingCount === 1 ? "" : "s"} pending update` : "All panels in sync"}
          </div>
        </div>
      </nav>
      <main className="main-content">
        <Routes>
          <Route path="/" element={<DisplaysPage />} />
          <Route path="/designs" element={<DesignsPage />} />
          <Route path="/gateways" element={<GatewaysPage />} />
          <Route path="/integrations" element={<IntegrationsPage />} />
        </Routes>
      </main>
    </div>
  );
}
