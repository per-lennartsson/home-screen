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

  listDisplays: () => request("/displays"),
  createDisplay: (data) => request("/displays", { method: "POST", body: JSON.stringify(data) }),
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
  getPayload: (displayId) => request(`/displays/${displayId}/payload`),

  listDesigns: () => request("/designs"),
  createDesign: (data) => request("/designs", { method: "POST", body: JSON.stringify(data) }),
  updateDesign: (id, data) => request(`/designs/${id}`, { method: "PUT", body: JSON.stringify(data) }),

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
