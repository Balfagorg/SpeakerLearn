/* Stub for Electron compatibility - avoids native build failures.
   naudiodon works without segfault-handler; it only provides crash logging. */
module.exports = { registerHandler: function () {} };
