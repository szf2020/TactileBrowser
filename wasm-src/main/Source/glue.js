// WASM loader and interop (glue for modularized Emscripten output)

(function() {
  let ModuleInstance = null;

  function setStatus(msg, color) {
    const status = document.getElementById('status');
    if (!status) return;
    status.textContent = msg;
    if (color) status.style.color = color;
  }

  // Public API used by C/C++ via EM_ASM and by the UI
  window.TactileBrowserWasm = {
    // Called from index.html after the modularized Emscripten module is instantiated.
    init: function(modInstance) {
      ModuleInstance = modInstance;

      // Provide a global Module pointer for any code that still expects it (best-effort).
      // This helps older code that uses Module.ccall directly. Prefer using ModuleInstance.
      try {
        window.Module = ModuleInstance;
      } catch (e) {
        // ignore
      }

      // Notify runtime ready (C++ may override onRuntimeInitialized; this is just UI)
      setStatus('Ready.', '#FFD93D');

      // If the module provided an onRuntimeInitialized callback attribute, make sure we still show Ready
      if (typeof ModuleInstance.onRuntimeInitialized === 'function') {
        // If runtime hasn't initialized yet, wrap the existing callback to update status when it runs.
        const existing = ModuleInstance.onRuntimeInitialized;
        ModuleInstance.onRuntimeInitialized = function() {
          try { existing(); } catch (e) { console.error(e); }
          setStatus('Ready.', '#FFD93D');
        };
      }
    },

    loadUrl: function(url) {
      setStatus('Loading...', '#999');

      if (!ModuleInstance) {
        setStatus('WASM not initialized', '#FF6B6B');
        console.error('Attempted to call loadUrl before WASM module was initialized');
        return;
      }

      // ccall should be available on the module instance returned by the factory.
      if (typeof ModuleInstance.ccall !== 'function') {
        setStatus('WASM API missing', '#FF6B6B');
        console.error('Module instance does not expose ccall. ModuleInstance:', ModuleInstance);
        return;
      }

      try {
        ModuleInstance.ccall('load_url', // C function name
                             'void',     // return type
                             ['string'], // arg types
                             [url]);     // args
      } catch (e) {
        setStatus('Error invoking WASM', '#FF6B6B');
        console.error('Error calling load_url via ccall:', e);
      }
    },

    setStatus: function(msg, color) {
      setStatus(msg, color);
    }
  };
})();