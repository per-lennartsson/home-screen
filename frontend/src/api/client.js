const BASE = "/api";

async function request(path, options = {}) {
  const res = await fetch(`${BASE}${path}`, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  if (!res.ok) {
    const body = await res.text();
    throw new Error(`${options.method || "GET"} ${path} -> ${res.status}: ${body}`);
  }
  if (res.status === 204) return null;
  return res.json();
}

export const api = {
  listGateways: () => request("/gateways"),
  createGateway: (data) => request("/gateways", { method: "POST", body: JSON.stringify(data) }),
  deleteGateway: (id) => request(`/gateways/${id}`, { method: "DELETE" }),

  listDisplays: () => request("/displays"),
  createDisplay: (data) => request("/displays", { method: "POST", body: JSON.stringify(data) }),
  deleteDisplay: (id) => request(`/displays/${id}`, { method: "DELETE" }),
  assignDesign: (displayId, designId) =>
    request(`/displays/${displayId}/assign`, {
      method: "POST",
      body: JSON.stringify({ design_id: designId }),
    }),
  assignGateway: (displayId, gatewayId) =>
    request(`/displays/${displayId}/assign-gateway`, {
      method: "POST",
      body: JSON.stringify({ gateway_id: gatewayId }),
    }),
  setRotation: (displayId, rotate180) =>
    request(`/displays/${displayId}/rotate`, {
      method: "POST",
      body: JSON.stringify({ rotate_180: rotate180 }),
    }),
  setWakeInterval: (displayId, wakeIntervalS) =>
    request(`/displays/${displayId}/wake-interval`, {
      method: "POST",
      body: JSON.stringify({ wake_interval_s: wakeIntervalS }),
    }),
  forceFullRefresh: (displayId) => request(`/displays/${displayId}/force-full-refresh`, { method: "POST" }),
  setFullRefreshInterval: (displayId, fullRefreshIntervalS) =>
    request(`/displays/${displayId}/full-refresh-interval`, {
      method: "POST",
      body: JSON.stringify({ full_refresh_interval_s: fullRefreshIntervalS }),
    }),
  getPayload: (displayId) => request(`/displays/${displayId}/payload`),
  getBatteryHistory: (displayId) => request(`/displays/${displayId}/battery-history`),
  getBatteryEstimate: (displayId, wakeIntervalS) =>
    request(`/displays/${displayId}/battery-estimate${wakeIntervalS ? `?wake_interval_s=${wakeIntervalS}` : ""}`),

  listDesigns: () => request("/designs"),
  createDesign: (data) => request("/designs", { method: "POST", body: JSON.stringify(data) }),
  updateDesign: (id, data) => request(`/designs/${id}`, { method: "PUT", body: JSON.stringify(data) }),
  deleteDesign: (id) => request(`/designs/${id}`, { method: "DELETE" }),

  getHomeAssistantConfig: () => request("/integrations/home-assistant"),
  setHomeAssistantConfig: (data) =>
    request("/integrations/home-assistant", { method: "PUT", body: JSON.stringify(data) }),
  previewEntity: (entityId, attribute) =>
    request(
      `/integrations/home-assistant/entities/${encodeURIComponent(entityId)}${
        attribute ? `?attribute=${encodeURIComponent(attribute)}` : ""
      }`
    ),
};
