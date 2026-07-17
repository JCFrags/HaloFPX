(() => {
  const root = document.body.dataset.root || "";
  const input = document.getElementById("wiki-search");
  const results = document.getElementById("search-results");
  const index = window.WIKI_SEARCH_INDEX || [];
  const norm = (s) => (s || "").toLowerCase().replace(/\s+/g, " ").trim();

  if (input && results) {
    input.addEventListener("input", () => {
      const q = norm(input.value);
      if (q.length < 2) {
        results.classList.remove("active");
        results.innerHTML = "";
        return;
      }
      const terms = q.split(" ");
      const matches = index.filter(item => {
        const hay = norm(item.title + " " + item.text);
        return terms.every(t => hay.includes(t));
      }).slice(0, 12);
      results.innerHTML = matches.length
        ? matches.map(item => `<a class="search-result" href="${root}${item.url}"><strong>${item.title}</strong><span>${item.snippet}</span></a>`).join("")
        : `<div class="search-result"><strong>No match</strong><span>Try a path, commit prefix, type, or subsystem.</span></div>`;
      results.classList.add("active");
    });
    document.addEventListener("click", (e) => {
      if (!results.contains(e.target) && e.target !== input) results.classList.remove("active");
    });
  }

  const toggle = document.getElementById("mobile-toggle");
  if (toggle) toggle.addEventListener("click", () => document.body.classList.toggle("nav-open"));

  document.querySelectorAll('a[href^="http"]').forEach(a => {
    a.target = "_blank";
    a.rel = "noopener noreferrer";
  });
})();
