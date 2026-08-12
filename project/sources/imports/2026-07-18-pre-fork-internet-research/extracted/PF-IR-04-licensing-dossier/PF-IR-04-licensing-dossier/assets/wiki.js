
const input = document.querySelector('#wiki-search');
if (input) {
  input.addEventListener('input', () => {
    const q = input.value.trim().toLowerCase();
    document.querySelectorAll('[data-searchable]').forEach(el => {
      const hit = q && el.textContent.toLowerCase().includes(q);
      el.classList.toggle('search-hit', !!hit);
      if (q && !hit && el.matches('tr')) el.style.display = 'none'; else if (el.matches('tr')) el.style.display = '';
    });
  });
}
