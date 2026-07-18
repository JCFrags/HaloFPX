(() => {
  const root = document.documentElement;
  const saved = localStorage.getItem('pf-ir-03-theme');
  if (saved) root.dataset.theme = saved;
  const toggle = document.getElementById('theme-toggle');
  if (toggle) toggle.addEventListener('click', () => {
    root.dataset.theme = root.dataset.theme === 'dark' ? 'light' : 'dark';
    localStorage.setItem('pf-ir-03-theme', root.dataset.theme);
  });
  const q = document.getElementById('wiki-search');
  if (q) q.addEventListener('input', () => {
    const needle = q.value.trim().toLowerCase();
    document.querySelectorAll('.nav a').forEach(a => {
      a.classList.toggle('search-hidden', needle && !a.textContent.toLowerCase().includes(needle));
    });
  });
  document.querySelectorAll('article code').forEach(el => {
    const t = el.textContent;
    let cls = 'claim-label label-gap';
    if (t === '[DOCUMENTED_SUPPORT]' || t === '[SIGNED_REPOSITORY_METADATA]' || t === '[SOURCE_PIN]') cls = 'claim-label label-support';
    else if (t === '[PREVIEW_AVAILABILITY]' || t === '[NOT_RELEASE_READY]' || t === '[MATURITY_CONFLICT]' || t === '[LOCAL_COMPARISON_ONLY]') cls = 'claim-label label-preview';
    else if (t === '[KNOWN_ISSUE]' || t === '[DO_NOT_MIX]' || t === '[UNVERIFIED_COMBINATION]') cls = 'claim-label label-issue';
    if (/^\[[A-Z0-9_-]+\]$/.test(t)) el.className = cls;
  });
})();
