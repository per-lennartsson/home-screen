import { useEffect, useState } from "react";
import { api } from "../api/client.js";

export function entityValueKey(entityId, attribute) {
  return `${entityId}::${attribute || ""}`;
}

// Debounced, cached lookup of Home Assistant entity values for a set of {entityId, attribute}
// refs. Cache is keyed by entityValueKey and only grows — callers that need a fresh read (e.g.
// after editing an entity id) call api.previewEntity directly and merge the result themselves.
export function useEntityValues(entityRefs) {
  const [entityValues, setEntityValues] = useState({});

  useEffect(() => {
    const missing = entityRefs.filter(
      (ref) => ref.entityId && !(entityValueKey(ref.entityId, ref.attribute) in entityValues)
    );
    if (missing.length === 0) return;

    const timer = setTimeout(() => {
      missing.forEach((ref) => {
        const key = entityValueKey(ref.entityId, ref.attribute);
        api
          .previewEntity(ref.entityId, ref.attribute || undefined)
          .then((result) =>
            setEntityValues((prev) => ({
              ...prev,
              [key]: result.error
                ? { error: result.error, fetchedAt: new Date().toISOString() }
                : { value: result.value, fetchedAt: new Date().toISOString() },
            }))
          )
          .catch((e) =>
            setEntityValues((prev) => ({ ...prev, [key]: { error: e.message, fetchedAt: new Date().toISOString() } }))
          );
      });
    }, 500);
    return () => clearTimeout(timer);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [JSON.stringify(entityRefs), entityValues]);

  return [entityValues, setEntityValues];
}
