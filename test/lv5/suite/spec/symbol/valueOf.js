describe("Symbol", function() {
  describe('valueOf', function() {
    it('throw TypeError', function () {
      expect(function () {
        Symbol.prototype.valueOf.call({});
      }).toThrow();
    });

    it('extract symbol', function () {
      var sym = Symbol('value');
      expect(Object(sym).valueOf()).toBe(sym);
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
