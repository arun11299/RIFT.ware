var p = require('../../lib/platform/platform.js');
window.platform = p;
var c = require('compatibilityChecker');
describe('Platform Checker', function(){

  it('Should approve IE 10', function(){
    window.platform.name = 'IE';
    window.platform.version = '10';
    expect(c.check(null, true)).toBe(true);
  });

  it('Should should hate IE 9', function(){
    window.platform.name = 'IE';
    window.platform.version = '9';
    expect(c.check(null, true)).toBe(false);
  });

  it('Should approve Chrome 40', function(){
    window.platform.name = 'Chrome';
    window.platform.version = '40';
    expect(c.check(null, true)).toBe(true);
  });

  it('Should should hate Chrome 9', function(){
    window.platform.name = 'Chrome';
    window.platform.version = '9';
    expect(c.check(null, true)).toBe(false);
  });

  it('Should approve Firefox 40', function(){
    window.platform.name = 'Firefox';
    window.platform.version = '40';
    expect(c.check(null, true)).toBe(true);
  });

  it('Should should hate Firefox 9', function(){
    window.platform.name = 'Firefox';
    window.platform.version = '9';
    expect(c.check(null, true)).toBe(false);
  });

  it('Should approve Safari 40', function(){
    window.platform.name = 'Safari';
    window.platform.version = '40';
    expect(c.check(null, true)).toBe(true);
  });

  it('Should should hate Safari 7', function(){
    window.platform.name = 'Firefox';
    window.platform.version = '7';
    expect(c.check(null, true)).toBe(false);
  });
});