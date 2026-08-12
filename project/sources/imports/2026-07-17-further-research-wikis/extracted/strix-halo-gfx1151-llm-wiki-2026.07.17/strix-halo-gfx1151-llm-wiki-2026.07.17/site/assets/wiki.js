(() => {
  const doc = document.documentElement;
  const root = document.body.dataset.root || '';
  const saved = localStorage.getItem('strix-wiki-theme');
  if (saved) doc.dataset.theme = saved;
  document.getElementById('themeButton')?.addEventListener('click', () => {
    const next = doc.dataset.theme === 'light' ? 'dark' : 'light';
    doc.dataset.theme = next; localStorage.setItem('strix-wiki-theme', next);
  });
  const sidebar = document.getElementById('sidebar');
  document.getElementById('menuButton')?.addEventListener('click', () => sidebar?.classList.toggle('open'));
  document.querySelectorAll('.sidebar a').forEach(a => a.addEventListener('click', () => sidebar?.classList.remove('open')));

  document.querySelectorAll('pre').forEach(pre => {
    const b = document.createElement('button'); b.className='copy-code'; b.textContent='copy';
    b.addEventListener('click', async () => {
      const code = pre.querySelector('code')?.innerText || pre.innerText;
      await navigator.clipboard.writeText(code.replace(/copy$/, ''));
      b.textContent='copied'; setTimeout(() => b.textContent='copy', 1200);
    }); pre.appendChild(b);
  });

  const dialog = document.getElementById('searchDialog');
  const input = document.getElementById('searchInput');
  const results = document.getElementById('searchResults');
  let index = null;
  async function openSearch() {
    dialog?.showModal(); input?.focus();
    if (!index) index = await fetch(root + 'search-index.json').then(r => r.json());
  }
  document.getElementById('searchButton')?.addEventListener('click', openSearch);
  window.addEventListener('keydown', e => {
    if (e.key === '/' && !['INPUT','TEXTAREA'].includes(document.activeElement?.tagName)) { e.preventDefault(); openSearch(); }
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') { e.preventDefault(); openSearch(); }
  });
  input?.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    if (!q) { results.innerHTML='<p>Type a component, version, flag, or regression.</p>'; return; }
    const terms = q.split(/\s+/).filter(Boolean);
    const hits = (index || []).map(item => {
      const hay = (item.title+' '+item.headings.join(' ')+' '+item.text).toLowerCase();
      let score = 0; terms.forEach(t => { if (item.title.toLowerCase().includes(t)) score+=8; if (item.headings.join(' ').toLowerCase().includes(t)) score+=4; score += Math.min(3,(hay.split(t).length-1)); });
      return {item,score};
    }).filter(x => x.score>0).sort((a,b)=>b.score-a.score).slice(0,20);
    if (!hits.length) { results.innerHTML='<p>No matches.</p>'; return; }
    results.innerHTML = hits.map(({item}) => `<a class="search-result" href="${root}${item.path}"><strong>${escapeHtml(item.title)}</strong><span>${escapeHtml(item.section)} · ${escapeHtml(item.snippet)}</span></a>`).join('');
  });
  function escapeHtml(s){ return s.replace(/[&<>'"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;',"'":'&#39;','"':'&quot;'}[c])); }
})();
