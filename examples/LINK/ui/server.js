const fs = require('fs');
const app = require('express')();
const compression = require('compression');
const sapper = require('sapper');
const static = require('serve-static');
const bodyParser = require('body-parser');
var session = require('express-session')
var cookieParser = require('cookie-parser')


const { PORT = 3000 } = process.env;

// this allows us to do e.g. `fetch('/api/blog')` on the server
const fetch = require('node-fetch');
global.fetch = (url, opts) => {
	if (url[0] === '/') url = `http://localhost:${PORT}${url}`;
	return fetch(url, opts);
};

app.use(compression({ threshold: 0 }));
app.use(bodyParser.json())
app.use(bodyParser.urlencoded({extended: false}))
app.use(static('assets'));
app.use(
	session({
  	secret: 'fenestrate.winnow.snaketar',
  	resave: false,
  	saveUninitialized: false,
	})
);
app.use(cookieParser());

app.use(sapper());

app.listen(PORT, () => {
	console.log(`listening on port ${PORT}`);
});
