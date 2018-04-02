var config = {
  source:      {_class: 'odbc', name: '', connectionString: ''},
  destination: {_class: 'influx', name: '', connectionString: ''},
  options:       { window:12, interval:5, backfill:30, smart:true},
  series:      [],
  dash:        { proto:'http' },
  run:         { run: false },
  analytics: [ ]

};

module.exports.config = config;
