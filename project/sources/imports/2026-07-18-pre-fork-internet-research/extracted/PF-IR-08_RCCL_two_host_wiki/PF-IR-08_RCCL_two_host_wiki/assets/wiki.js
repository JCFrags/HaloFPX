
const q=document.getElementById('nav-search');
if(q){q.addEventListener('input',()=>{const s=q.value.toLowerCase();document.querySelectorAll('.nav a').forEach(a=>{a.style.display=a.textContent.toLowerCase().includes(s)?'block':'none';});});}
document.querySelectorAll('.article p,.article td,.article li').forEach(el=>{
  const labels=['NORMATIVE_API','NORMATIVE_DOC','SOURCE_IMPLEMENTATION','UPSTREAM_TEST','MAINTAINER_STATEMENT','MAINTAINER_REPORTED_MACHINE_EVIDENCE','COMMUNITY_REPORTED_MACHINE_EVIDENCE','ISSUE_COMMENTARY','INFERENCE','NEGATIVE_EVIDENCE','MACHINE_EVIDENCE_REQUIRED'];
  labels.forEach(label=>{const token='['+label+']'; if(el.innerHTML.includes(token)){let cls=label.includes('NORMATIVE')?'norm':label.includes('SOURCE')||label.includes('TEST')?'source':label.includes('MACHINE')||label.includes('MAINTAINER')||label.includes('COMMUNITY')?'machine':label.includes('NEGATIVE')?'negative':'infer';el.innerHTML=el.innerHTML.split(token).join('<span class="badge '+cls+'">'+token+'</span>');}})
});
