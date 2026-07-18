
const q=document.getElementById('q');
q?.addEventListener('input',()=>{
  const s=q.value.toLowerCase().trim();
  document.querySelectorAll('[data-search]').forEach(el=>{
    el.style.display=!s||el.textContent.toLowerCase().includes(s)?'':'none';
  });
});
const links=[...document.querySelectorAll('.nav a')];
const obs=new IntersectionObserver(entries=>{
  entries.forEach(e=>{if(e.isIntersecting){links.forEach(a=>a.classList.toggle('active',a.getAttribute('href')==='#'+e.target.id));}});
},{rootMargin:'-20% 0px -70% 0px'});
document.querySelectorAll('.section').forEach(s=>obs.observe(s));
