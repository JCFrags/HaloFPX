(() => {
  const root = document.body.dataset.root || '';
  const themeButton = document.getElementById('theme-toggle');
  const savedTheme = localStorage.getItem('halokv-theme');
  if (savedTheme) document.documentElement.dataset.theme = savedTheme;
  if (themeButton) {
    themeButton.addEventListener('click', () => {
      const next = document.documentElement.dataset.theme === 'light' ? 'dark' : 'light';
      document.documentElement.dataset.theme = next;
      localStorage.setItem('halokv-theme', next);
    });
  }

  const mobile = document.getElementById('mobile-menu');
  if (mobile) mobile.addEventListener('click', () => document.body.classList.toggle('nav-open'));
  document.addEventListener('click', (e) => {
    if (window.innerWidth <= 980 && document.body.classList.contains('nav-open') &&
        !e.target.closest('.sidebar') && !e.target.closest('#mobile-menu')) {
      document.body.classList.remove('nav-open');
    }
  });

  document.querySelectorAll('pre').forEach((pre) => {
    const button = document.createElement('button');
    button.className = 'copy-code';
    button.textContent = 'Copy';
    button.addEventListener('click', async () => {
      try {
        await navigator.clipboard.writeText(pre.innerText.replace(/^Copy\n/, ''));
        button.textContent = 'Copied';
        setTimeout(() => button.textContent = 'Copy', 1200);
      } catch (_) { button.textContent = 'Unavailable'; }
    });
    pre.appendChild(button);
  });

  const input = document.getElementById('nav-search');
  const results = document.getElementById('search-results');
  let indexPromise = null;
  function loadIndex() {
    if (!indexPromise) indexPromise = fetch(root + 'assets/search-index.json').then(r => r.json()).catch(() => []);
    return indexPromise;
  }
  if (input) {
    input.addEventListener('input', async () => {
      const q = input.value.trim().toLowerCase();
      document.querySelectorAll('.nav-link').forEach((a) => {
        a.classList.toggle('hidden', q && !a.textContent.toLowerCase().includes(q));
      });
      if (!results) return;
      if (q.length < 2) { results.classList.remove('active'); results.innerHTML = ''; return; }
      const idx = await loadIndex();
      const hits = idx.filter(x => (x.title + ' ' + x.summary + ' ' + x.section).toLowerCase().includes(q)).slice(0, 8);
      results.innerHTML = hits.length ? hits.map(x =>
        `<a class="search-result" href="${root}${x.path}"><strong>${escapeHtml(x.title)}</strong><small>${escapeHtml(x.section)} · ${escapeHtml(x.summary)}</small></a>`
      ).join('') : '<div class="search-result"><small>No matching page</small></div>';
      results.classList.add('active');
    });
  }
  function escapeHtml(s) {
    return String(s).replace(/[&<>'"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;',"'":'&#39;','"':'&quot;'}[c]));
  }
})();
