// Grab an existing namespace object, or create a blank object
// if it doesn't exist
var Acme = window.Acme || {};

// Stick on the modules that need to be exported.
// You only need to require the top-level modules, browserify
// will walk the dependency graph and load everything correctly
// Acme.messages_pb = require('./messages_pb');
Acme.buffer_pb = require('./buffer_pb');

// Replace/Create the global namespace
window.Acme = Acme;