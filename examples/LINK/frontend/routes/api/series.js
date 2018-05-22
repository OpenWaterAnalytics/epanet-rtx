var proxy = require('../../model/linkProxy.js');
var parse = require('csv-parse');
import fetch from 'node-fetch';

export function get(req, res, next) {
  proxy.get(req,res);
}

export function post(req, res, next) {
  if (req.files) {
    console.log('files are present');
    // parse the incoming csv and format it for LINK consumption
    var parser = parse();
    var tsJson = [];
    parser.on('readable', () => {
      let r = parser.read();
      if (!r) {
        return;
      }
      // console.log('reading line: ', r);
      if (r.length !== 4) {
        return;
      }

      let [sName,sUnits,dName,dUnits] = r;
      tsJson.push({
        _class: 'filter',
        name: dName.trim(),
        units: {_class:'units', unitString: dUnits.trim()},
        renamed_from_name: sName.trim(),
        renamed_from_units: {
          _class:'units',
          unitString: sUnits.trim()
        }
      });

    });

    parser.on('finish', () => {
      // console.log('finished with csv data: ', tsJson);
      const postOpts = {
    		method: 'post',
    		headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(tsJson)
    	};
      fetch('http://127.0.0.1:3131/series', postOpts)
      .then(r => {
        console.log('POST series response:')
        console.log(r)
        res.send(r);
      });
    });

    parser.write(req.files.file.data);
    parser.end();

  }
  else {
    proxy.post(req,res);
  }
}
