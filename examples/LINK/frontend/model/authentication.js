const passport = require('passport');
const LocalStrategy = require('passport-local').Strategy;
const fs = require('fs-extra');
var jsf = require('jsonfile');
var path = require('path');
var os = require('os');
var expressflash = require('express-flash');
const ejs = require('ejs');

const linkDir = path.join(os.homedir(),'rtx_link');
const authFile = path.join(linkDir,'auth.json');

let _auth = {};


if (process.env.LINK_LOGIN_USER) {
  console.log('using environtment variables for authentication info');
  _auth = {
    u: process.env.LINK_LOGIN_USER,
    p: process.env.LINK_LOGIN_PASS
  }
}
else {
  try {
    if (!fs.pathExistsSync(authFile)) {
      fs.ensureFileSync(authFile);
      jsf.writeFileSync(authFile,{u:'admin',p:'admin'});
    }
    _auth = jsf.readFileSync(authFile);
  } catch (e) {
    console.log('error reading/writing auth file');
    console.log(e);
  }
}




module.exports.init = (app) => {
  app.use(passport.initialize());
  app.use(passport.session());
  passport.use(new LocalStrategy((user,pass,done) => {
    if (user == _auth.u && pass == _auth.p) {
      console.log('authentication succeeded. access granted');
      return done(null,user);
    }
    else {
      console.log('authentication failed');
      return done(null, false, {message: 'Not Permitted'});
    }
  }));
  passport.serializeUser((user,done) => {done(null,user);});
  passport.deserializeUser((id,done) => {done(null,id);});

  // template engine and flash support
  app.use(expressflash());
  app.engine('.ejs', ejs.renderFile);

  app.all('/api/*',(req,res,next) => {
  	this.check(req,res,next);
  });
  app.get('/login',(req,res) => {
  	let flash = req.flash();
  	let templateLocation = path.join(__dirname, '..', 'components','login.ejs');
  	res.render(templateLocation,{flash});
  });
  // recieve login credentials
  app.post('/login', this.authenticate);
  // destroy logged in session
  app.get('/logout', (req,res) => {
    req.logout();
    res.redirect('/');
  });

};

module.exports.authenticate = function(req,res,next) {
  console.log(`attempting authentication`);
  passport.authenticate('local', {
		successRedirect: '/',
		failureRedirect: '/login',
    failureFlash: true
	})(req,res,next);
};

module.exports.check = function(req, res, next){
    if(req.isAuthenticated()){
      next();
    } else {
      res.redirect("/login");
    }
};
