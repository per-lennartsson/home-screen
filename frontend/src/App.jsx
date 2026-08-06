import { NavLink, Route, Routes } from "react-router-dom";
import DesignsPage from "./pages/DesignsPage.jsx";
import DisplaysPage from "./pages/DisplaysPage.jsx";
import GatewaysPage from "./pages/GatewaysPage.jsx";
import IntegrationsPage from "./pages/IntegrationsPage.jsx";

export default function App() {
  return (
    <div className="layout">
      <nav className="sidebar">
        <h1>ePaper Displays</h1>
        <NavLink to="/" end className={({ isActive }) => (isActive ? "active" : "")}>
          Dashboard
        </NavLink>
        <NavLink to="/designs" className={({ isActive }) => (isActive ? "active" : "")}>
          Designs
        </NavLink>
        <NavLink to="/gateways" className={({ isActive }) => (isActive ? "active" : "")}>
          Gateways
        </NavLink>
        <NavLink to="/integrations" className={({ isActive }) => (isActive ? "active" : "")}>
          Integrations
        </NavLink>
      </nav>
      <main>
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
