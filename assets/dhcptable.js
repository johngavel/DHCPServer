// dhcptable.js (wrapper)
// Thin wrapper that instantiates <data-table> with DHCP-specific config

class DHCPTableWrapper extends HTMLElement {
  connectedCallback() {
    const el = document.createElement('data-table');
    el.setAttribute('src', this.getAttribute('src') || '/api/dhcp-info.json');
    el.setAttribute('interval-ms', this.getAttribute('interval-ms') || '1000');
    el.setAttribute('row-key', this.getAttribute('row-key') || 'auto');
    el.setAttribute('summary', JSON.stringify({
      text: 'dhcpSummary'
    }));
    el.setAttribute('columns', JSON.stringify([{
      header: 'IP Address',
      field: 'ipAddress'
    }, {
      header: 'MAC Address',
      field: 'macAddress'
    }, {
      header: 'Expires',
      field: 'expires',
      classFor: 'dhcpExpireClass'
    }, {
      header: 'Status',
      field: 'status',
      classFor: 'dhcpStatusClass'
    }]));
    this.replaceChildren(el);
  }
}
customElements.define('dhcp-table', DHCPTableWrapper);

// DHCP-specific function registry kept local to this file
window._datatableFns = Object.assign(window._datatableFns || {}, {
  dhcpStatusClass: (_v, row) => {
    const s = Number(row?.stat);
    if (Number.isFinite(s)) {
      if (s == 0) return 'error';
      if (s == 1) return 'warn';
    }
    return '';
  },
  dhcpExpireClass: (_v, row) => (row?.exp ? 'error' : ''),
  dhcpSummary: (data) => {
    const toHHMMSS = (secs) => {
      const n = Number(secs);
      if (!Number.isFinite(n)) return '00:00:00';
      const sign = n < 0 ? '-' : '';
      const s = Math.abs(Math.trunc(n));
      const h = Math.floor(s / 3600),
        m = Math.floor((s % 3600) / 60),
        sec = s % 60;
      return `${sign}${h.toString().padStart(2,'0')}:${m.toString().padStart(2,'0')}:${sec.toString().padStart(2,'0')}`;
    };
    const lease = Number.isFinite(Number(data?.leasetime)) ? Number(data.leasetime) : 86400;
    const start = Number.isFinite(Number(data?.startOctet)) ? Number(data.startOctet) : 101;
    const last = Number.isFinite(Number(data?.lastOctet)) ? Number(data.lastOctet) : 200;
    return `Lease Time: ${toHHMMSS(lease)} Availability: ${start} - ${last}`;
  }
});