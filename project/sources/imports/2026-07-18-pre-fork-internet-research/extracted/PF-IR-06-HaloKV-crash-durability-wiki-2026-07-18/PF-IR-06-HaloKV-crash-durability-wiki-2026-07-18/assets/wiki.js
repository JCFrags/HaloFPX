
(() => {
  const box = document.querySelector('#nav-search');
  if (!box) return;
  box.addEventListener('input', () => {
    const q = box.value.trim().toLowerCase();
    document.querySelectorAll('.nav a').forEach(a => {
      a.style.display = !q || a.textContent.toLowerCase().includes(q) ? '' : 'none';
    });
  });
})();
