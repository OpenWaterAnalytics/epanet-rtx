var config = {
  source:      {_class: 'odbc', name: '', connectionString: ''},
  destination: {},
  options:       { window:12, interval:5, backfill:30, lag:0},
  series:      [],
  dash:        { proto:'http' },
  run:         { run: false },
  analytics: [ ]

};

module.exports.config = config;
