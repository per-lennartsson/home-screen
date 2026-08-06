import { useEffect, useState } from "react";
import { api } from "../api/client.js";

const CANVAS_W = 296;
const CANVAS_H = 152;

function emptyElement() {
  return { type: "text", x: 10, y: 10, w: 100, h: 20, text: "" };
}

function Canvas({ elements }) {
  return (
    <div className="canvas-preview">
      {elements.map((el, i) => (
        <div
          key={i}
          className={`canvas-element ${el.type}`}
          style={{ left: el.x, top: el.y, width: el.w, height: el.h }}
        >
          {el.type === "text" ? el.text : el.value}
        </div>
      ))}
    </div>
  );
}

function toLayoutJson(elements) {
  return {
    elements: elements.map((el, i) => ({
      id: i + 1,
      type: el.type,
      x: el.x,
      y: el.y,
      w: el.w,
      h: el.h,
      props: el.type === "text" ? { text: el.text } : { value: el.value },
    })),
  };
}

function fromLayoutJson(layout) {
  return (layout.elements || []).map((el) => ({
    type: el.type,
    x: el.x,
    y: el.y,
    w: el.w,
    h: el.h,
    text: el.props?.text || "",
    value: el.props?.value || "",
  }));
}

export default function DesignsPage() {
  const [designs, setDesigns] = useState([]);
  const [name, setName] = useState("");
  const [elements, setElements] = useState([]);
  const [draft, setDraft] = useState(emptyElement());
  const [error, setError] = useState(null);
  const [editingId, setEditingId] = useState(null);

  const refresh = () => api.listDesigns().then(setDesigns).catch((e) => setError(e.message));

  useEffect(() => {
    refresh();
  }, []);

  const addElement = () => {
    setElements([...elements, draft]);
    setDraft(emptyElement());
  };

  const removeElement = (i) => setElements(elements.filter((_, idx) => idx !== i));

  const save = async () => {
    setError(null);
    try {
      if (editingId) {
        await api.updateDesign(editingId, { name, layout_json: toLayoutJson(elements) });
      } else {
        await api.createDesign({ name, layout_json: toLayoutJson(elements) });
      }
      setName("");
      setElements([]);
      setEditingId(null);
      refresh();
    } catch (e) {
      setError(e.message);
    }
  };

  const edit = (design) => {
    setEditingId(design.id);
    setName(design.name);
    setElements(fromLayoutJson(design.layout_json));
  };

  return (
    <>
      <h2>Designs</h2>
      {error && <div className="error">{error}</div>}

      <div className="card">
        <div className="field">
          <label>Design name</label>
          <input value={name} onChange={(e) => setName(e.target.value)} placeholder="e.g. desk-status" />
        </div>

        <div className="row" style={{ alignItems: "flex-start", gap: 24 }}>
          <div style={{ flex: 1 }}>
            <div className="row">
              <div className="field">
                <label>Type</label>
                <select value={draft.type} onChange={(e) => setDraft({ ...draft, type: e.target.value })}>
                  <option value="text">text (static)</option>
                  <option value="value">value (bound, diffable)</option>
                </select>
              </div>
              <div className="field">
                <label>{draft.type === "text" ? "Text" : "Value"}</label>
                <input
                  value={draft.type === "text" ? draft.text : draft.value}
                  onChange={(e) =>
                    setDraft({ ...draft, [draft.type === "text" ? "text" : "value"]: e.target.value })
                  }
                />
              </div>
            </div>
            <div className="row">
              <div className="field">
                <label>x</label>
                <input type="number" value={draft.x} onChange={(e) => setDraft({ ...draft, x: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>y</label>
                <input type="number" value={draft.y} onChange={(e) => setDraft({ ...draft, y: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>w</label>
                <input type="number" value={draft.w} onChange={(e) => setDraft({ ...draft, w: +e.target.value })} style={{ width: 60 }} />
              </div>
              <div className="field">
                <label>h</label>
                <input type="number" value={draft.h} onChange={(e) => setDraft({ ...draft, h: +e.target.value })} style={{ width: 60 }} />
              </div>
              <button type="button" className="secondary" onClick={addElement}>
                Add element
              </button>
            </div>

            <ul style={{ paddingLeft: 16, fontSize: 13 }}>
              {elements.map((el, i) => (
                <li key={i}>
                  {el.type} "{el.type === "text" ? el.text : el.value}" @ ({el.x},{el.y})
                  <button type="button" className="secondary" style={{ marginLeft: 8, padding: "2px 6px" }} onClick={() => removeElement(i)}>
                    remove
                  </button>
                </li>
              ))}
            </ul>
          </div>

          <div>
            <div className="muted" style={{ marginBottom: 6 }}>
              Live preview ({CANVAS_W}x{CANVAS_H})
            </div>
            <Canvas elements={elements} />
          </div>
        </div>

        <button type="button" onClick={save} disabled={!name || elements.length === 0} style={{ marginTop: 12 }}>
          {editingId ? "Save changes" : "Create design"}
        </button>
        {editingId && (
          <button
            type="button"
            className="secondary"
            style={{ marginLeft: 8 }}
            onClick={() => {
              setEditingId(null);
              setName("");
              setElements([]);
            }}
          >
            Cancel edit
          </button>
        )}
      </div>

      <div className="card">
        <table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>Elements</th>
              <th>Updated</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {designs.map((d) => (
              <tr key={d.id}>
                <td>{d.id}</td>
                <td>{d.name}</td>
                <td>{(d.layout_json.elements || []).length}</td>
                <td>{new Date(d.updated_at).toLocaleString()}</td>
                <td>
                  <button type="button" className="secondary" onClick={() => edit(d)}>
                    Edit
                  </button>
                </td>
              </tr>
            ))}
            {designs.length === 0 && (
              <tr>
                <td colSpan={5} className="muted">
                  No designs yet.
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </>
  );
}
