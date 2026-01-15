// /js/library-table.element.js
// <library-table> rendered in LIGHT DOM (no Shadow DOM)

/**
 * Format seconds as HH:MM:SS, supporting negatives safely.
 * Returns "00:00:00" on NaN/Infinity/undefined.
 */
function toHHMMSS(totalSeconds) {
  const n = Number(totalSeconds);
  if (!Number.isFinite(n)) return "00:00:00"; // fallback for bad inputs
  const sign = n < 0 ? "-" : "";
  const sec = Math.abs(Math.trunc(n)); // drop fractional part, handle negatives
  const hours = Math.floor(sec / 3600);
  const minutes = Math.floor((sec % 3600) / 60);
  const seconds = sec % 60;
  return (
    sign +
    hours.toString().padStart(2, "0") + ":" +
    minutes.toString().padStart(2, "0") + ":" +
    seconds.toString().padStart(2, "0")
  );
}

class DHCPTableElement extends HTMLElement {
  static get observedAttributes() {
    return ['src', 'row-key'];
  }

  constructor() {
    super();
    // Render directly to the element (light DOM)
    this._root = this; // no shadow
    this._src = '/api/dhcp-info.json'; // default preserves original path

    // Refreshing & concurrency
    this._refreshHandle = null;
    this._busy = false;

    // Stable DOM references to avoid flicker
    this._summarySpan = null;
    this._table = null;
    this._tbody = null;

    // Row diffing
    this._rowKey = 'auto'; // 'auto' | 'id' | 'macAddress' | 'ipAddress'
    this._rowIndex = new Map(); // key -> <tr>
  }

  connectedCallback() {
    // Attributes
    if (this.hasAttribute('src')) {
      const s = this.getAttribute('src');
      if (s && s.trim()) this._src = s.trim();
    }
    if (this.hasAttribute('row-key')) {
      const k = (this.getAttribute('row-key') || '').trim();
      if (k) this._rowKey = k;
    }

    // Build static shell only once: summary + table + header
    if (!this._summarySpan) {
      const container = document.createElement('div');

      this._summarySpan = document.createElement('span'); // summary text
      container.appendChild(this._summarySpan);

      this._table = document.createElement('table');
      const thead = document.createElement('thead');
      const headerRow = document.createElement('tr');
      ['IP Address', 'MAC Address', 'Expires', 'Status'].forEach(text => {
        const th = document.createElement('th');
        th.scope = 'col';
        th.textContent = text;
        headerRow.appendChild(th);
      });
      thead.appendChild(headerRow);
      this._table.appendChild(thead);

      this._tbody = document.createElement('tbody');
      this._table.appendChild(this._tbody);
      container.appendChild(this._table);

      // Atomic insert, prevents flash on first mount
      this._root.replaceChildren(container);
    }

    // First data fill
    this.render();

    // Refresh every second
    if (!this._refreshHandle) {
      this._refreshHandle = setInterval(() => this.render(), 1000);
    }
  }

  disconnectedCallback() {
    // Clean up the refresh timer when element is removed
    if (this._refreshHandle) {
      clearInterval(this._refreshHandle);
      this._refreshHandle = null;
    }
  }

  attributeChangedCallback(name, _oldVal, newVal) {
    if (name === 'src') {
      this._src = (newVal && newVal.trim()) ? newVal.trim() : this._src;
      this.render();
    } else if (name === 'row-key') {
      const k = (newVal || '').trim();
      this._rowKey = k || 'auto';
      // Re-diff on next tick using the new key
      this.render();
    }
  }

  /**
   * Insert a status line below the summary without nuking the table.
   * Removes prior warn/error messages to prevent stacking.
   */
  insertStatus(message, type = 'warn') {
    // Remove previous status messages (direct children) to avoid duplicates
    const prior = this._root.querySelectorAll(':scope p.warn, :scope p.error');
    prior.forEach(p => p.remove());

    const p = document.createElement('p');
    p.className = type; // 'warn' or 'error' (matches your CSS)
    p.textContent = message;

    // Place status just below summary
    if (this._summarySpan && this._summarySpan.parentNode) {
      this._summarySpan.after(p);
    } else {
      // Fallback if shell not yet built
      this._root.appendChild(p);
    }
  }

  /**
   * Determine the key to identify a row across updates.
   * Order of precedence when 'auto': id -> macAddress -> ipAddress -> JSON fallback.
   */
  _getKey(item) {
    const k = this._rowKey;
    if (k && k !== 'auto') return String(item?.[k] ?? '');
    // auto
    if (item?.id != null && item.id !== '') return String(item.id);
    if (item?.macAddress != null && item.macAddress !== '') return String(item.macAddress);
    if (item?.ipAddress != null && item.ipAddress !== '') return String(item.ipAddress);
    // last resort (should be rare): a stable stringification of visible fields
    return JSON.stringify([
      item?.ipAddress ?? '',
      item?.macAddress ?? '',
      item?.expires ?? '',
      item?.status ?? ''
    ]);
  }

  /** Create a <tr> with attached cell refs for efficient updates. */
  _createRow(item, key) {
    const tr = document.createElement('tr');
    tr.dataset.key = key;

    const ip = document.createElement('td');
    const mac = document.createElement('td');
    const exp = document.createElement('td');
    const st = document.createElement('td');

    // Initial fill
    ip.textContent = item?.ipAddress ?? '';
    mac.textContent = item?.macAddress ?? '';
    exp.textContent = item?.expires ?? '';

    const expiresVal = Boolean(item?.exp);
    if (expiresVal) exp.classList.add('error');
    else exp.classList.remove('error');

    const stValue = Number(item?.stat);
    if (Number.isFinite(stValue)) {
      if (stValue == 0) {
        st.classList.remove('warn');
        st.classList.add('error');
      } else if (stValue == 1) {
        st.classList.remove('error');
        st.classList.add('warn');
      } else {
        st.classList.remove('error');
        st.classList.remove('warn');
      }
    } else {
      st.classList.remove('error');
      st.classList.remove('warn');
    }
    st.textContent = item?.status ?? '';

    tr.append(ip, mac, exp, st);
    // Keep references for fast updates
    tr._cells = {
      ip,
      mac,
      exp,
      st
    };
    return tr;
  }

  /** Update cell text/classes only when actually changed. */
  _updateRow(tr, item) {
    const {
      ip,
      mac,
      exp,
      st
    } = tr._cells;

    const ipText = item?.ipAddress ?? '';
    if (ip.textContent !== String(ipText)) ip.textContent = String(ipText);

    const macText = item?.macAddress ?? '';
    if (mac.textContent !== String(macText)) mac.textContent = String(macText);

    const expText = item?.expires ?? '';
    if (exp.textContent !== String(expText)) {
      exp.textContent = String(expText);
      const expiresVal = Boolean(item?.exp);
      if (expiresVal) exp.classList.add('error');
      else exp.classList.remove('error');
    }

    const stText = item?.status ?? '';
    if (st.textContent !== String(stText)) {
      st.textContent = String(stText);
      const stValue = Number(item?.stat);
      if (Number.isFinite(stValue)) {
        if (stValue == 0) {
          st.classList.remove('warn');
          st.classList.add('error');
        } else if (stValue == 1) {
          st.classList.remove('error');
          st.classList.add('warn');
        } else {
          st.classList.remove('error');
          st.classList.remove('warn');
        }
      } else {
        st.classList.remove('error');
        st.classList.remove('warn');
      }
    }
  }

  async render() {
    if (this._busy) return;
    this._busy = true;

    let data;
    try {
      const res = await fetch(this._src, {});
      if (!res.ok) {
        this.insertStatus(`Failed to fetch dhcp info (HTTP ${res.status}).`, 'error');
        this._busy = false;
        return;
      }
      data = await res.json();
    } catch (err) {
      console.error(err);
      this.insertStatus('Unable to load dhcp info (network or JSON parse error).', 'error');
      this._busy = false;
      return;
    }

    // Normalize / summary
    const rows = Array.isArray(data?.dhcptable) ? data.dhcptable : [];
    const leaseSecs = Number.isFinite(data?.leasetime) ? data.leasetime : 86400;
    const start = Number.isFinite(data?.startOctet) ? data.startOctet : 101;
    const last = Number.isFinite(data?.lastOctet) ? data.lastOctet : 200;

    // Update summary text only if needed
    const newSummary = `Lease Time: ${toHHMMSS(leaseSecs)} Availability: ${start} - ${last}`;
    if (this._summarySpan && this._summarySpan.textContent !== newSummary) {
      this._summarySpan.textContent = newSummary;
    }

    // Status: empty data
    if (rows.length === 0) {
      // Remove all existing rows (if any) without rebuilding the table
      if (this._tbody && this._tbody.firstChild) {
        this._tbody.replaceChildren(); // clear fast, no flash
      }
      this._rowIndex.clear();
      this.insertStatus('No dhcp data to display.');
      this._busy = false;
      return;
    }

    // Keyed diff: add/update/move rows
    const seen = new Set();
    for (let i = 0; i < rows.length; i++) {
      const item = rows[i];
      const key = this._getKey(item);
      let tr = this._rowIndex.get(key);

      if (!tr) {
        // New row
        tr = this._createRow(item, key);
        this._rowIndex.set(key, tr);
      } else {
        // Existing row: update per-cell if changed
        this._updateRow(tr, item);
      }

      // Ensure the row is at position i; move only if needed
      const atIndex = this._tbody.children[i];
      if (tr !== atIndex) {
        this._tbody.insertBefore(tr, atIndex || null); // null = append
      }

      seen.add(key);
    }

    // Remove rows that disappeared
    for (const [key, tr] of Array.from(this._rowIndex.entries())) {
      if (!seen.has(key)) {
        tr.remove();
        this._rowIndex.delete(key);
      }
    }

    this._busy = false;
  }
}

// Register the element
customElements.define('dhcp-table', DHCPTableElement);