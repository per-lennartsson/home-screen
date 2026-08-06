"""
Fetches entity state from a Home Assistant instance via its REST API
(https://developers.home-assistant.io/docs/api/rest/). One instance per system (spec 1's
"one location" v1 scope), configured through HomeAssistantConfig — see
api/integrations.py.
"""

import httpx

REQUEST_TIMEOUT_S = 10.0


class HomeAssistantError(Exception):
    pass


async def fetch_entity_value(base_url: str, access_token: str, entity_id: str, attribute: str | None = None) -> str:
    """Fetches an entity's current state (or a specific attribute of it) from Home
    Assistant. Raises HomeAssistantError with a message safe to show in the GUI."""
    url = f"{base_url.rstrip('/')}/api/states/{entity_id}"
    headers = {"Authorization": f"Bearer {access_token}"}

    try:
        async with httpx.AsyncClient(timeout=REQUEST_TIMEOUT_S) as client:
            resp = await client.get(url, headers=headers)
    except httpx.RequestError as exc:
        raise HomeAssistantError(f"could not reach Home Assistant: {exc}") from exc

    if resp.status_code == 401:
        raise HomeAssistantError("Home Assistant rejected the access token")
    if resp.status_code == 404:
        raise HomeAssistantError(f"entity '{entity_id}' not found")
    if resp.status_code != 200:
        raise HomeAssistantError(f"Home Assistant returned HTTP {resp.status_code}")

    data = resp.json()
    if attribute:
        if attribute not in data.get("attributes", {}):
            raise HomeAssistantError(f"entity '{entity_id}' has no attribute '{attribute}'")
        return str(data["attributes"][attribute])
    return str(data["state"])
