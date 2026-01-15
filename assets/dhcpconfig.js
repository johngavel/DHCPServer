const leaseTimeform = document.getElementById('leaseTimeForm');
const leaseTimeInput = document.getElementById('leaseTimeInput');

if (leaseTimeform && leaseTimeInput) {
  leaseTimeform.addEventListener('submit', async (evt) => {
    evt.preventDefault();

    const raw = (leaseTimeInput.value || '').trim();
    const leaseTime = Number(raw);

    if (!raw || Number.isNaN(leaseTime) || !Number.isInteger(leaseTime) || leaseTime <= 0) {
      alert('Please enter a valid positive integer for lease time (e.g., 86400).');
      leaseTimeInput.focus();
      return;
    }

    try {
      const res = await fetch('/api/dhcp-info.json', {
        method: 'POST', // change to 'PUT' if your API expects it
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          leasetime: leaseTime
        })
      });

      if (!res.ok) {
        let detail = '';
        try {
          detail = await res.text();
        } catch {}
        throw new Error(`Server responded with ${res.status}${detail ? `: ${detail}` : ''}`);
      }

      alert('Lease time saved successfully.');
      // You can notify dependent components
      // window.dispatchEvent(new CustomEvent('dhcp:updated'));
    } catch (err) {
      console.error(err);
      alert('Failed to save lease time. Please try again.');
    }
  });
}

const rangeForm = document.getElementById('rangeForm');
const startInput = document.getElementById('startInput');
const lastInput = document.getElementById('lastInput');

if (rangeForm && startInput && lastInput) {
  rangeForm.addEventListener('submit', async (evt) => {
    evt.preventDefault();

    const raw = (startInput.value || '').trim();
    const start = Number(raw);

    if (!raw || Number.isNaN(start) || !Number.isInteger(start) || start <= 0 || start >= 256) {
      alert('Please enter a valid positive integer for last octet');
      moveToInput.focus();
      return;
    }

    const raw2 = (lastInput.value || '').trim();
    const last = Number(raw2);

    if (!raw || Number.isNaN(last) || !Number.isInteger(last) || last <= 0 || last >= 256) {
      alert('Please enter a valid positive integer for last octet');
      moveFromInput.focus();
      return;
    }

    try {
      const res = await fetch('/api/dhcp-info.json', {
        method: 'POST', // change to 'PUT' if your API expects it
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          startOctet: start,
          lastOctet: last
        })
      });

      if (!res.ok) {
        let detail = '';
        try {
          detail = await res.text();
        } catch {}
        throw new Error(`Server responded with ${res.status}${detail ? `: ${detail}` : ''}`);
      }

      alert('Configure Range completed successfully.');
      // You can notify dependent components
      // window.dispatchEvent(new CustomEvent('dhcp:updated'));
    } catch (err) {
      console.error(err);
      alert('Failed to configure range addresses. Please try again.');
    }
  });
}

const moveform = document.getElementById('moveForm');
const moveToInput = document.getElementById('moveToInput');
const moveFromInput = document.getElementById('moveFromInput');

if (moveform && moveToInput && moveFromInput) {
  moveform.addEventListener('submit', async (evt) => {
    evt.preventDefault();

    const raw = (moveToInput.value || '').trim();
    const moveTo = Number(raw);

    if (!raw || Number.isNaN(moveTo) || !Number.isInteger(moveTo) || moveTo <= 0 || moveTo >= 256) {
      alert('Please enter a valid positive integer for To last octet');
      moveToInput.focus();
      return;
    }

    const raw2 = (moveFromInput.value || '').trim();
    const moveFrom = Number(raw2);

    if (!raw || Number.isNaN(moveFrom) || !Number.isInteger(moveFrom) || moveFrom <= 0 || moveFrom >= 256) {
      alert('Please enter a valid positive integer for To last octet');
      moveFromInput.focus();
      return;
    }

    try {
      const res = await fetch('/api/dhcp-info.json', {
        method: 'POST', // change to 'PUT' if your API expects it
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          moveFrom: moveFrom,
          moveTo: moveTo
        })
      });

      if (!res.ok) {
        let detail = '';
        try {
          detail = await res.text();
        } catch {}
        throw new Error(`Server responded with ${res.status}${detail ? `: ${detail}` : ''}`);
      }

      alert('Move completed successfully.');
      // You can notify dependent components
      // window.dispatchEvent(new CustomEvent('dhcp:updated'));
    } catch (err) {
      console.error(err);
      alert('Failed to move ip addresses. Please try again.');
    }
  });
}

const deleteform = document.getElementById('deleteForm');
const deleteInput = document.getElementById('deleteInput');

if (deleteform && deleteInput) {
  deleteform.addEventListener('submit', async (evt) => {
    evt.preventDefault();

    const raw = (deleteInput.value || '').trim();
    const deleteIP = Number(raw);

    if (!raw || Number.isNaN(deleteIP) || !Number.isInteger(deleteIP) || deleteIP <= 0 || deleteIP >= 256) {
      alert('Please enter a valid positive integer for last octet.');
      deleteInput.focus();
      return;
    }

    try {
      const res = await fetch('/api/dhcp-info.json', {
        method: 'POST', // change to 'PUT' if your API expects it
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          delete: deleteIP
        })
      });

      if (!res.ok) {
        let detail = '';
        try {
          detail = await res.text();
        } catch {}
        throw new Error(`Server responded with ${res.status}${detail ? `: ${detail}` : ''}`);
      }

      alert('Delete IP completed successfully.');
      // You can notify dependent components
      // window.dispatchEvent(new CustomEvent('dhcp:updated'));
    } catch (err) {
      console.error(err);
      alert('Failed to delete ip. Please try again.');
    }
  });
}

const deleteAllform = document.getElementById('deleteAllForm');

if (deleteAllform) {
  deleteAllform.addEventListener('submit', async (evt) => {
    evt.preventDefault();
    const confirmed = window.confirm(
      'Are you sure you want to delete All Ip Addresses?\n\nIP Addresses will be reassigned.'
    );
    if (!confirmed) return;
    try {
      const res = await fetch('/api/dhcp-info.json', {
        method: 'POST', // change to 'PUT' if your API expects it
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          deleteAll: 0
        })
      });

      if (!res.ok) {
        let detail = '';
        try {
          detail = await res.text();
        } catch {}
        throw new Error(`Server responded with ${res.status}${detail ? `: ${detail}` : ''}`);
      }

      alert('Delete All IP completed successfully.');
      // You can notify dependent components
      // window.dispatchEvent(new CustomEvent('dhcp:updated'));
    } catch (err) {
      console.error(err);
      alert('Failed to delete all IPs. Please try again.');
    }
  });
}