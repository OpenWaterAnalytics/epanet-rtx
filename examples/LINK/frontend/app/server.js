const fs = require('fs');
const app = require('express')();
const compression = require('compression');
const sapper = require('sapper');
const serve = require('serve-static');
const bodyParser = require('body-parser');
var session = require('express-session')
var cookieParser = require('cookie-parser')
const fileUpload = require('express-fileupload');

import { routes } from './manifest/server.js';

const { PORT = 3000 } = process.env;

app.use(compression({ threshold: 0 }));
app.use(bodyParser.json())
app.use(bodyParser.urlencoded({extended: false}))
app.use(serve('assets'));
app.use(
	session({
  	secret: 'fenestrate.winnow.snaketar',
  	resave: false,
  	saveUninitialized: false,
	})
);
app.use(cookieParser());
app.use(fileUpload());

app.use(sapper({routes}));

app.listen(PORT, () => {
	console.log(`listening on port ${PORT}`);
});
