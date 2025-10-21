// WASM loader and interop
let Module = null;
window.TactileBrowserWasm = {
  loadUrl: function(url) {
    document.getElementById('status').textContent = 'Loading...';
    Module.ccall('load_url', null, ['string'], [url]);
  },
  setStatus: function(msg, color) {
    const status = document.getElementById('status');
    status.textContent = msg;
    if (color) status.style.color = color;
  }
};

// Emscripten Module config
Module = {
  canvas: document.getElementById('canvas'),
  onRuntimeInitialized: function() {
    window.TactileBrowserWasm.setStatus("Ready.", "#FFD93D");
  },
  print: function(text) { console.log(text); }
};
// need to build tactilebrowser_wasm.js and tactilebrowser_wasm.wasm with Emscripten, then load here