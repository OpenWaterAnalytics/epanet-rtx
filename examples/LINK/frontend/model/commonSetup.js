const compression = require('compression');
const bodyParser = require('body-parser');
var session = require('express-session')
var cookieParser = require('cookie-parser')
const fileUpload = require('express-fileupload');
var MemoryStore = require('session-memory-store')(session);

module.exports.init = (app) => {
  app.use(compression({ threshold: 0 }));
  app.use(bodyParser.json({limit: '50mb'}))
  app.use(bodyParser.urlencoded({extended: false}))
  app.use(
  	session({
    	secret: 'fenestrate.winnow.snaketar',
  		store: new MemoryStore(),
    	resave: false,
    	saveUninitialized: false,
  	})
  );
  app.use(cookieParser());
  app.use(fileUpload());
}
