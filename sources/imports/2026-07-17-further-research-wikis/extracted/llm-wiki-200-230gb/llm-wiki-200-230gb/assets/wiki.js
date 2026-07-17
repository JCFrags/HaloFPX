
const box=document.getElementById('search');
if(box){box.addEventListener('input',()=>{const q=box.value.trim().toLowerCase();document.querySelectorAll('.nav a[data-search]').forEach(a=>a.classList.toggle('hidden',q && !a.dataset.search.includes(q)));});}
