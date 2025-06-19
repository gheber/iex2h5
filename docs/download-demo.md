# 🎬 Terminal Demo with IEX-DOWNLOAD
<div id="asciicast-player" style="margin-top: 2em;"></div>
<script>
  document.addEventListener("DOMContentLoaded", function () {
    function waitForAsciinemaPlayer(attempts = 10) {
      if (typeof AsciinemaPlayer !== "undefined") {
        AsciinemaPlayer.create('../assets/iex-download-demo.cast', document.getElementById('asciicast-player'), {
          cols: 125, rows: 40, autoPlay: true,  loop: true, theme: 'solarized-light'
        });
      } else if (attempts > 0) {
        setTimeout(() => waitForAsciinemaPlayer(attempts - 1), 200);
      } else {
        console.error("AsciinemaPlayer failed to load.");
      }
    }
    waitForAsciinemaPlayer();
});
</script>

