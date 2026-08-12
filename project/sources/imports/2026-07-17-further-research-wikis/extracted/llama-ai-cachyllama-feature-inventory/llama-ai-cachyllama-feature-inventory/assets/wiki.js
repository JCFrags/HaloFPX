(() => {
  const q = (s) => document.querySelector(s);
  const rows = q("#featureRows");
  const detail = q("#detail");
  const search = q("#search");
  const cat = q("#category");
  const dec = q("#decision");
  const mat = q("#maturity");
  const count = q("#visibleCount");

  const esc = (v) => String(v ?? "").replace(/[&<>"']/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;","\"":"&quot;","'":"&#039;"}[c]));
  const badge = (v) => `<span class="badge ${String(v).toLowerCase()}">${esc(v)}</span>`;
  const evLinks = (ids) => ids.map(id => {
    const e = window.EVIDENCE[id];
    return e ? `<a href="${esc(e.url)}" target="_blank" rel="noreferrer">${esc(id)}</a>` : esc(id);
  }).join(", ");

  const categories = [...new Set(window.FEATURES.map(f => f.category))].sort();
  categories.forEach(v => cat.insertAdjacentHTML("beforeend", `<option>${esc(v)}</option>`));

  function matches(f) {
    const needle = search.value.trim().toLowerCase();
    const hay = [f.id,f.category,f.feature,f.behavior,f.rationale,f.risks,...f.implementation_files,...f.dependencies].join(" ").toLowerCase();
    return (!needle || hay.includes(needle))
      && (!cat.value || f.category === cat.value)
      && (!dec.value || f.decision === dec.value)
      && (!mat.value || f.maturity === mat.value);
  }

  function render() {
    const list = window.FEATURES.filter(matches);
    count.textContent = `${list.length} / ${window.FEATURES.length}`;
    rows.innerHTML = list.map(f => `
      <tr data-id="${esc(f.id)}" tabindex="0">
        <td class="mono">${esc(f.id)}</td>
        <td>${esc(f.category)}</td>
        <td><strong>${esc(f.feature)}</strong><br><span style="color:var(--muted)">${esc(f.behavior)}</span></td>
        <td>${badge(f.decision)}</td>
        <td>${badge(f.maturity)}</td>
        <td>${esc(f.portability)}</td>
        <td>${evLinks(f.evidence)}</td>
      </tr>`).join("");
    rows.querySelectorAll("tr").forEach(tr => {
      const open = () => show(window.FEATURES.find(f => f.id === tr.dataset.id));
      tr.addEventListener("click", open);
      tr.addEventListener("keydown", e => { if (e.key === "Enter" || e.key === " ") open(); });
    });
  }

  function show(f) {
    if (!f) return;
    detail.classList.add("open");
    detail.innerHTML = `
      <div style="display:flex;justify-content:space-between;gap:1rem;align-items:flex-start">
        <div><div class="eyebrow">${esc(f.id)} · ${esc(f.category)}</div><h3>${esc(f.feature)}</h3></div>
        <div>${badge(f.decision)} ${badge(f.maturity)}</div>
      </div>
      <div class="detail-grid">
        <dl>
          <dt>Observed behavior</dt><dd>${esc(f.behavior)}</dd>
          <dt>Implementation files</dt><dd>${f.implementation_files.map(x=>`<code>${esc(x)}</code>`).join(" ")}</dd>
          <dt>Dependencies</dt><dd>${f.dependencies.map(esc).join("; ")}</dd>
          <dt>License</dt><dd>${esc(f.license)}</dd>
          <dt>Evidence</dt><dd>${evLinks(f.evidence)}</dd>
        </dl>
        <dl>
          <dt>ROCmFPX overlap</dt><dd>${esc(f.target_overlap)}</dd>
          <dt>Portability</dt><dd>${esc(f.portability)}</dd>
          <dt>Decision rationale</dt><dd>${esc(f.rationale)}</dd>
          <dt>Risks</dt><dd>${esc(f.risks || "No additional feature-specific risk recorded.")}</dd>
          <dt>Evidence confidence</dt><dd>${esc(f.confidence)}</dd>
        </dl>
      </div>`;
    detail.scrollIntoView({behavior:"smooth", block:"nearest"});
  }

  [search,cat,dec,mat].forEach(el => el.addEventListener("input", render));
  q("#clear").addEventListener("click", () => { search.value="";cat.value="";dec.value="";mat.value="";detail.classList.remove("open");render(); });
  render();

  const counts = window.WIKI_SUMMARY.decision_counts;
  const total = window.WIKI_SUMMARY.feature_count;
  ["RETAIN","REDESIGN","REJECT"].forEach(k => {
    const el = q(`#bar-${k.toLowerCase()}`);
    if (el) el.style.width = `${(counts[k] / total) * 100}%`;
  });
})();
