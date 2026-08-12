
(function () {
  const root = document.documentElement;
  const saved = localStorage.getItem("pfir11-theme");
  if (saved) root.dataset.theme = saved;
  const toggle = document.getElementById("theme-toggle");
  if (toggle) {
    toggle.addEventListener("click", function () {
      const next = root.dataset.theme === "dark" ? "light" : "dark";
      root.dataset.theme = next;
      localStorage.setItem("pfir11-theme", next);
    });
  }

  const search = document.getElementById("nav-search");
  const links = Array.from(document.querySelectorAll(".nav-list li"));
  if (search) {
    search.addEventListener("input", function () {
      const q = search.value.trim().toLowerCase();
      links.forEach(function (li) {
        li.classList.toggle("hidden", q && !li.textContent.toLowerCase().includes(q));
      });
    });
    document.addEventListener("keydown", function (ev) {
      if (ev.key === "/" && !/input|textarea/i.test(document.activeElement.tagName)) {
        ev.preventDefault();
        search.focus();
      }
    });
  }

  const menu = document.getElementById("mobile-menu");
  if (menu) menu.addEventListener("click", () => document.body.classList.toggle("nav-open"));
  document.querySelectorAll(".sidebar a").forEach(a => a.addEventListener("click", () => document.body.classList.remove("nav-open")));

  document.querySelectorAll(".article table").forEach(function (table) {
    if (table.parentElement && !table.parentElement.classList.contains("table-wrap")) {
      const wrap = document.createElement("div");
      wrap.className = "table-wrap";
      table.parentNode.insertBefore(wrap, table);
      wrap.appendChild(table);
    }
  });

  document.querySelectorAll(".article h2, .article h3, .article h4").forEach(function (heading) {
    if (!heading.id) return;
    const a = document.createElement("a");
    a.className = "anchor-link";
    a.href = "#" + heading.id;
    a.setAttribute("aria-label", "Link to section");
    a.textContent = "#";
    heading.appendChild(a);
  });
})();
