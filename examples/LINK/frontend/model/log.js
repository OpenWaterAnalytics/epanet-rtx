var CircularBuffer = require("circular-buffer");

const _maxLog = 1000;
const _logMessages = new CircularBuffer(_maxLog);

module.exports.logLine = (msg) => {
  console.log(msg);
  _logMessages.enq(msg);
};
module.exports.errLine = (msg) => {
  console.log(msg);
  _logMessages.enq(msg);
};

module.exports.linkLogs = () => {
  return {
    'log': _logMessages.toarray(),
  };
}
