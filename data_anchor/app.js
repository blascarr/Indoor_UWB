const statusEl = document.getElementById('status');
const feedbackEl = document.getElementById('feedback');
const form = document.getElementById('position-form');
const apiOutputEl = document.getElementById('api-output');

function initNav() {
  document.querySelectorAll('.nav-btn').forEach((btn) => {
    btn.addEventListener('click', () => {
      const panelId = btn.dataset.panel;
      document.querySelectorAll('.nav-btn').forEach((b) => b.classList.remove('active'));
      btn.classList.add('active');
      document.querySelectorAll('.panel').forEach((panel) => {
        const show = panel.id === `panel-${panelId}`;
        panel.hidden = !show;
        panel.classList.toggle('active', show);
      });
    });
  });
}

function showApiOutput(label, data, isError = false) {
  if (!apiOutputEl) {
    return;
  }
  const body =
    typeof data === 'string' ? data : JSON.stringify(data, null, 2);
  apiOutputEl.textContent = `${label}\n${body}`;
  apiOutputEl.style.color = isError ? '#fca5a5' : '#e2e8f0';
}

async function apiRaw(path, options = {}) {
  const res = await fetch(path, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  const type = res.headers.get('content-type') || '';
  const body = type.includes('json') ? await res.json() : await res.text();
  return { ok: res.ok, status: res.status, body };
}

function initApiExplorer() {
  document.querySelectorAll('.try-api').forEach((btn) => {
    btn.addEventListener('click', async () => {
      const path = btn.dataset.path;
      const method = btn.dataset.method || 'GET';
      let body;

      if (btn.dataset.bodyId) {
        const raw = document.getElementById(btn.dataset.bodyId)?.value?.trim();
        try {
          body = raw;
          JSON.parse(raw);
        } catch {
          showApiOutput('Error', 'JSON inválido en el cuerpo', true);
          return;
        }
      } else if (btn.dataset.body) {
        body = btn.dataset.body;
      }

      const options = { method };
      if (body !== undefined && method !== 'GET') {
        options.body = body;
      }

      try {
        const result = await apiRaw(path, options);
        showApiOutput(
          `${method} ${path} → HTTP ${result.status}`,
          result.body,
          !result.ok,
        );
        if (result.ok) {
          await refresh();
        }
      } catch (err) {
        showApiOutput('Error de red', err.message, true);
      }
    });
  });
}

async function api(path, options = {}) {
  const res = await fetch(path, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || res.statusText);
  }
  const type = res.headers.get('content-type') || '';
  return type.includes('json') ? res.json() : res.text();
}

function fillForm(data) {
  form.x.value = data.x ?? 0;
  form.y.value = data.y ?? 0;
  form.z.value = data.z ?? 0;
  form.offset.value = data.offset ?? 0;
  statusEl.textContent = `${data.name} · MAC ${data.mac} · IP ${data.ip}`;
}

async function refresh() {
  const data = await api('/api/position');
  fillForm(data);
}

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const payload = Object.fromEntries(new FormData(form));
  Object.keys(payload).forEach((key) => {
    payload[key] = Number(payload[key]);
  });
  await api('/api/position', {
    method: 'POST',
    body: JSON.stringify(payload),
  });
  feedbackEl.textContent = 'Posición guardada.';
  await refresh();
});

document.getElementById('sync').addEventListener('click', async () => {
  await api('/api/sync', { method: 'POST', body: '{}' });
  feedbackEl.textContent = 'Posición enviada al tag por ESP-NOW.';
});

initNav();
initApiExplorer();

refresh().catch((err) => {
  statusEl.textContent = `Error: ${err.message}`;
});
