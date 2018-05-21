var respawn = require('respawn');
const logger = require('./log.js');
var configuration = require('./configuration.js');

let link_monitor;

module.exports.init = (linkExePath) => {
	link_monitor = respawn([linkExePath], {maxRestarts:10, sleep:2000});
	link_monitor.on('stdout', function(data) {
	  logger.logLine('stdout: ' + data);
	});
	link_monitor.on('stderr', function(data) {
	  logger.errLine('stderr: ' + data);
	});
	link_monitor.on('spawn', function(child) {
	  logger.logLine('LINK MONITOR is spawning');
	  logger.logLine('SENDING CONFIG to new instance');
	  setTimeout(function() {
	    ok = configuration.send();
	    if (!ok) {
	      // respawn?
	      logger.errLine('error sending config. respawning');
	      link_monitor.stop(function() {
	          link_monitor.start();
	      });
	    }
	  }, 2000);
	});
	link_monitor.on('warn', function(error) {
	  logger.logLine('LINK warning: ' + error);
	});
	link_monitor.on('exit', function(code,signal) {
	  logger.logLine('LINK exited with code: ' + code);
	});
	link_monitor.on('crash', function() {
	  throw new Error('SERVICE MONITOR HAS CRASHED');
	});

  link_monitor.start() // spawn and watch
}
