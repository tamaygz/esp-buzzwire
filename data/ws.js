// ── WebSocket client — shared across all pages ───────────────────────────────
(function () {
  let ws, reconnectTimer;
  const dot   = () => document.getElementById('wsDot');
  const label = () => document.getElementById('wsLabel');

  // Registered per-event handlers from each page
  const handlers = {};

  window.wsOn = function (event, fn) { handlers[event] = fn; };

  window.wsSend = function (cmd, data) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ cmd, ...(data || {}) }));
    }
  };

  function setStatus(connected) {
    const d = dot(), l = label();
    if (!d) return;
    d.className = 'dot' + (connected ? ' on' : '');
    l.textContent = connected ? 'Live' : 'Offline';
  }

  function connect() {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`${proto}://${location.host}/ws`);

    ws.onopen = () => {
      setStatus(true);
      clearTimeout(reconnectTimer);
    };

    ws.onmessage = (e) => {
      let msg;
      try { msg = JSON.parse(e.data); } catch { return; }
      const fn = handlers[msg.event];
      if (fn) fn(msg.data);
    };

    ws.onclose = () => {
      setStatus(false);
      reconnectTimer = setTimeout(connect, 3000);
    };

    ws.onerror = () => { ws.close(); };
  }

  document.addEventListener('DOMContentLoaded', connect);
})();
