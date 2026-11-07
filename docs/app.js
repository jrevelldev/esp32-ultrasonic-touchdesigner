'use strict';
(() => {
  const $ = (s) => document.querySelector(s);
  const steps = [...document.querySelectorAll('.step')];
  const storageKey = 'distancia-guide-v1';
  let saved = {done: [], checks: {}, notes: {}};
  let storageAvailable = true;
  try {
    const parsed = JSON.parse(localStorage.getItem(storageKey) || 'null');
    if (parsed && typeof parsed === 'object') {
      saved.done = Array.isArray(parsed.done) ? parsed.done.filter(id => steps.some(s => s.id === id)) : [];
      saved.checks = parsed.checks && typeof parsed.checks === 'object' ? parsed.checks : {};
      saved.notes = parsed.notes && typeof parsed.notes === 'object' ? parsed.notes : {};
    }
  } catch { storageAvailable = false; }
  let current = 0;
  let toastTimer;
  let firmwarePromise;
  const nav = $('#step-nav');
  const select = $('#mobile-steps');
  $('.skip-link').addEventListener('click', event => {
    event.preventDefault();
    $('#pas').focus();
  });
  steps.forEach((s, i) => {
    const a = document.createElement('a');
    a.href = '#' + s.id;
    a.className = 'nav-step';
    a.innerHTML = `<span class="nav-num">${String(i + 1).padStart(2, '0')}</span><span>${s.dataset.short}</span>`;
    nav.append(a);
    const option = document.createElement('option');
    option.value = s.id;
    option.textContent = `${String(i + 1).padStart(2, '0')} — ${s.dataset.title}`;
    select.append(option);
  });
  function notify(message) {
    $('#toast').textContent = message;
    $('#toast').classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => $('#toast').classList.remove('show'), 3000);
  }
  function persist() {
    try { localStorage.setItem(storageKey, JSON.stringify(saved)); storageAvailable = true; }
    catch { storageAvailable = false; notify('El navegador no pot desar el progrés. Es manté mentre la guia estigui oberta.'); }
  }
  function updateProgress() {
    const count = saved.done.length;
    $('#progress').value = count;
    $('#progress-label').textContent = `${count} de ${steps.length} passos completats`;
    [...nav.children].forEach((a, i) => {
      const done = saved.done.includes(steps[i].id);
      a.classList.toggle('done', done);
      a.querySelector('.nav-num').textContent = done ? '✓' : String(i + 1).padStart(2, '0');
    });
    $('#step-complete').checked = saved.done.includes(steps[current].id);
  }
  function loadFirmware() {
    if (!firmwarePromise) firmwarePromise = fetch('downloads/ESP32_OSC_Demo.ino')
      .then(r => { if (!r.ok) throw new Error('download'); return r.text(); })
      .then(code => {
        if (!code.includes('void setup()') || !code.includes('void loop()')) throw new Error('invalid');
        $('#firmware-code').textContent = code;
        return code;
      }).catch(error => {
        firmwarePromise = null;
        $('#firmware-code').textContent = 'No s’ha pogut carregar el codi. Descarrega el ZIP o el fitxer .ino amb els botons de dalt.';
        throw error;
      });
    return firmwarePromise;
  }
  function render(scroll = false) {
    const id = location.hash.slice(1);
    const index = steps.findIndex(s => s.id === id);
    current = index >= 0 ? index : 0;
    steps.forEach((s, i) => { s.hidden = i !== current; });
    [...nav.children].forEach((a, i) => {
      a.classList.toggle('active', i === current);
      if (i === current) a.setAttribute('aria-current', 'step');
      else a.removeAttribute('aria-current');
    });
    select.value = steps[current].id;
    $('#eyebrow').textContent = `${String(current + 1).padStart(2, '0')} / ${steps[current].dataset.category}`;
    document.title = `${steps[current].dataset.title} · Distància`;
    $('#prev-step').disabled = current === 0;
    $('#next-step').textContent = current === steps.length - 1 ? 'Torna a l’inici ↗' : 'Següent pas →';
    updateProgress();
    if (steps[current].id === 'programa') loadFirmware().catch(() => {});
    if (scroll) { window.scrollTo({top: 0, behavior: 'instant'}); $('#pas').focus({preventScroll: true}); }
  }
  window.addEventListener('hashchange', () => render(true));
  select.addEventListener('change', () => { location.hash = select.value; });
  $('#prev-step').addEventListener('click', () => { if (current > 0) location.hash = steps[current - 1].id; });
  $('#next-step').addEventListener('click', () => { location.hash = steps[(current + 1) % steps.length].id; });
  $('#step-complete').addEventListener('change', e => {
    const id = steps[current].id;
    saved.done = saved.done.filter(x => x !== id);
    if (e.target.checked) saved.done.push(id);
    persist(); updateProgress();
  });
  $('#reset-progress').addEventListener('click', () => {
    if (!confirm('Vols desmarcar els passos i les comprovacions? Les notes del dispositiu es conservaran.')) return;
    saved.done = []; saved.checks = {};
    document.querySelectorAll('[data-check]').forEach(el => { el.checked = false; });
    persist(); updateProgress(); notify('Progrés reiniciat.');
  });
  document.querySelectorAll('[data-check]').forEach(el => {
    el.checked = saved.checks[el.dataset.check] === true;
    el.addEventListener('change', () => { saved.checks[el.dataset.check] = el.checked; persist(); });
  });
  async function copyText(value) {
    try { await navigator.clipboard.writeText(value); }
    catch {
      const input = document.createElement('textarea');
      input.value = value;
      input.style.cssText = 'position:fixed;left:-9999px;top:0';
      document.body.append(input); input.focus(); input.select();
      const ok = document.execCommand('copy'); input.remove();
      if (!ok) throw new Error('copy');
    }
  }
  document.querySelectorAll('[data-copy]').forEach(button => button.addEventListener('click', async () => {
    try { await copyText(document.getElementById(button.dataset.copy).textContent); notify('Adreça copiada.'); }
    catch { notify('No s’ha pogut copiar. Selecciona el text manualment.'); }
  }));
  $('#copy-firmware').addEventListener('click', async () => {
    try { await copyText(await loadFirmware()); $('#code-state').textContent = 'Programa complet copiat. Enganxa’l en un sketch buit d’Arduino.'; notify('Programa complet copiat.'); }
    catch { $('#code-state').textContent = 'No s’ha pogut copiar. Descarrega el ZIP o obre el codi complet i selecciona’l manualment.'; }
  });
  document.querySelectorAll('[data-network]').forEach(b => b.addEventListener('click', () => {
    const mode = b.dataset.network;
    document.querySelectorAll('[data-network]').forEach(other => { const active = other === b; other.classList.toggle('active', active); other.setAttribute('aria-pressed', String(active)); });
    $('#network-config').hidden = mode !== 'config';
    $('#network-normal').hidden = mode !== 'normal';
  }));
  function isIPv4(value) {
    const p = value.split('.');
    return p.length === 4 && p.every(x => /^\d{1,3}$/.test(x) && Number(x) <= 255) && Number(p[0]) > 0 && Number(p[0]) < 224 && Number(p[0]) !== 127;
  }
  for (const key of ['name','esp','computer','port']) {
    if (typeof saved.notes[key] === 'string') document.getElementById('note-' + key).value = saved.notes[key];
  }
  function readNotes() {
    return Object.fromEntries(['name','esp','computer','port'].map(key => [key, document.getElementById('note-' + key).value.trim()]));
  }
  function validateNotes(notes) {
    if (notes.esp && !isIPv4(notes.esp)) return 'Revisa la IP de l’ESP: cal una adreça IPv4, com 192.168.1.50.';
    if (notes.computer && !isIPv4(notes.computer)) return 'Revisa la IP de l’ordinador: cal una adreça IPv4.';
    if (!/^\d+$/.test(notes.port) || +notes.port < 1024 || +notes.port > 65535) return 'El port ha d’estar entre 1024 i 65535.';
    return '';
  }
  function saveNotes() {
    const notes = readNotes();
    const error = validateNotes(notes);
    if (error) { $('#notes-status').textContent = error; return null; }
    saved.notes = notes; persist();
    $('#notes-status').textContent = storageAvailable ? 'Notes desades en aquest navegador.' : 'Notes disponibles en aquesta sessió. Exporta-les per conservar-les.';
    return notes;
  }
  $('#device-notes').addEventListener('submit', e => { e.preventDefault(); saveNotes(); });
  $('#open-esp').addEventListener('click', () => {
    const notes = saveNotes(); if (!notes) return;
    if (!notes.esp) { $('#notes-status').textContent = 'Escriu primer la IP de l’ESP.'; return; }
    window.open('http://' + notes.esp, '_blank', 'noopener,noreferrer');
  });
  $('#export-notes').addEventListener('click', () => {
    const notes = saveNotes(); if (!notes) return;
    const text = `Dispositiu: ${notes.name || '—'}\nIP de l’ESP: ${notes.esp || '—'}\nIP de l’ordinador: ${notes.computer || '—'}\nPort OSC: ${notes.port}\n\nWeb: ${notes.esp ? 'http://' + notes.esp : 'pendent'}\nPortal: http://192.168.4.1 (connectat al Wi-Fi de l’ESP)\n`;
    const url = URL.createObjectURL(new Blob([text], {type:'text/plain;charset=utf-8'}));
    const a = document.createElement('a'); a.href = url; a.download = 'notes-esp32.txt'; a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  });
  const dialog = $('#image-dialog');
  document.querySelectorAll('[data-zoom]').forEach(button => button.addEventListener('click', () => {
    $('#zoom-image').src = button.dataset.zoom;
    $('#zoom-image').alt = button.querySelector('img').alt;
    dialog.showModal();
  }));
  $('#close-image').addEventListener('click', () => dialog.close());
  dialog.addEventListener('click', e => { if (e.target === dialog) dialog.close(); });
  $('#print-guide').addEventListener('click', () => window.print());
  const beforePrint = new Map();
  window.addEventListener('beforeprint', () => {
    document.querySelectorAll('details').forEach(d => { beforePrint.set(d, d.open); if (d.id !== 'code-details') d.open = true; });
    document.querySelectorAll('img[loading="lazy"]').forEach(i => { i.loading = 'eager'; });
  });
  window.addEventListener('afterprint', () => { beforePrint.forEach((open, d) => { d.open = open; }); beforePrint.clear(); });
  render();
})();
