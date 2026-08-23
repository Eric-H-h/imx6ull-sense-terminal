(function () {
  var path = window.location.pathname;
  if (path.slice(-1) !== "/" && path.indexOf(".") === -1) {
    window.location.replace(path + "/" + window.location.search + window.location.hash);
    return;
  }

  window.setTimeout(function () {
    if (window.__labReady) {
      return;
    }
    var el = document.getElementById("lab-boot");
    if (el) {
      el.hidden = false;
    }
  }, 4000);
})();
