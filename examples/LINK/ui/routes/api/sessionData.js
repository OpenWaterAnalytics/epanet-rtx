import {config} from '../../model/configuration.js';

export function get(req, res, next) {
  if (!('config' in req.session)) {
    console.log('building config state for session id: ', req.session.id);
    req.session.config = config;
  }
  res.json(req.session.config);
};

export function post(req,res,next) {
  req.session.config = req.body;
  console.log('posted config: ', req.session.config);
  res.type('json');
  res.json({status:'ok'});
};
