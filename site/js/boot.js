window.setTimeout(function () {
  if (!window.__labReady) {
    var el = document.getElementById("lab-boot");
    if (el) {
      el.hidden = false;
    }
  }
}, 800);
