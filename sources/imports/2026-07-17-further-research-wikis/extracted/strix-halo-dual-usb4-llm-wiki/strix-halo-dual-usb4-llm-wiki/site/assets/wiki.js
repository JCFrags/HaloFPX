(() => {
  const root = document.documentElement;
  const stored = localStorage.getItem('wiki-theme');
  if (stored) root.dataset.theme = stored;
  const toggle = document.getElementById('theme-toggle');
  toggle?.addEventListener('click', () => {
    root.dataset.theme = root.dataset.theme === 'light' ? 'dark' : 'light';
    localStorage.setItem('wiki-theme', root.dataset.theme);
  });

  const sidebar = document.getElementById('sidebar');
  document.getElementById('nav-toggle')?.addEventListener('click', () => sidebar?.classList.toggle('open'));

  const panel = document.getElementById('search-panel');
  const input = document.getElementById('wiki-search');
  const results = document.getElementById('search-results');
  const closeSearch = () => { if (panel) panel.hidden = true; };
  const openSearch = () => { if (panel) { panel.hidden = false; setTimeout(() => input?.focus(), 0); } };
  document.getElementById('search-toggle')?.addEventListener('click', openSearch);
  panel?.addEventListener('click', e => { if (e.target === panel) closeSearch(); });
  document.addEventListener('keydown', e => {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); openSearch(); }
    if (e.key === 'Escape') closeSearch();
  });

  const escapeHtml = value => value.replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  input?.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    if (!results) return;
    if (!q) { results.innerHTML = '<small>Type at least two characters. Ctrl/⌘+K opens search.</small>'; return; }
    const terms = q.split(/\s+/).filter(Boolean);
    const hits = (window.WIKI_SEARCH_INDEX || []).map(item => {
      const hay = (item.title + ' ' + item.text).toLowerCase();
      const score = terms.reduce((s, t) => s + (item.title.toLowerCase().includes(t) ? 8 : 0) + (hay.includes(t) ? 1 : -20), 0);
      return {...item, score};
    }).filter(x => x.score >= terms.length).sort((a,b) => b.score-a.score).slice(0, 20);
    results.innerHTML = hits.length ? hits.map(hit => {
      const href = window.WIKI_PAGE_ROOT + hit.path;
      return `<a class="search-hit" href="${escapeHtml(href)}"><b>${escapeHtml(hit.title)}</b><small>${escapeHtml(hit.section)} · ${escapeHtml(hit.snippet)}</small></a>`;
    }).join('') : '<small>No matching pages.</small>';
  });
})();
