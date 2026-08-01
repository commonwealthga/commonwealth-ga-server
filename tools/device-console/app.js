(function () {
  var tabs = document.querySelectorAll('.tab');
  var panes = document.querySelectorAll('.classpane');
  function sel(name) {
    tabs.forEach(function (t) { t.classList.toggle('active', t.dataset.tab === name); });
    panes.forEach(function (p) { p.classList.toggle('hide', name !== 'All' && p.dataset.class !== name); });
  }
  tabs.forEach(function (t) { t.addEventListener('click', function () { sel(t.dataset.tab); }); });
  sel('All');
})();

