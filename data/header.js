// ── Shared header / navbar ───────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', function () {
  const pages = [
    { href: 'index.html',  label: 'Game'    },
    { href: 'config.html', label: 'Config'  },
    { href: 'sounds.html', label: 'Sounds'  },
    { href: 'wifi.html',   label: 'WiFi'    },
    { href: 'system.html', label: 'System'  },
  ];

  const current = location.pathname.split('/').pop() || 'index.html';

  const nav = document.createElement('nav');
  nav.innerHTML = `
    <span class="nav-brand">&#9889; Buzzwire</span>
    ${pages.map(p =>
      `<a href="${p.href}" class="${current === p.href ? 'active' : ''}">${p.label}</a>`
    ).join('')}
    <div class="nav-status">
      <span class="dot" id="wsDot"></span>
      <span id="wsLabel">Connecting…</span>
    </div>`;
  document.body.prepend(nav);

  // Toast helper
  const toastEl = document.createElement('div');
  toastEl.id = 'toast';
  document.body.appendChild(toastEl);
  let toastTimer;
  window.showToast = function (msg, isErr) {
    toastEl.textContent = msg;
    toastEl.className   = 'show' + (isErr ? ' err' : '');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { toastEl.className = ''; }, 2800);
  };
});
