
// on load:
console.log(' \
  WARNING::: NO LOGIN WILL BE REQUIRED \
')


let _auth = {
  u: 'admin'
};

module.exports.init = (app) => {
  app.get('/login',(req,res) => {
    res.redirect('/');
  });
};

module.exports.check = function(req,res,next) {
  next && next();
};
