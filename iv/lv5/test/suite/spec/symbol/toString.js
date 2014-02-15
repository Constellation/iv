describe("Symbol", function() {
  describe('toString', function() {
    it('call with Symbol', function () {
      expect(function () {
        Symbol().toString();
      }).toThrow();
    });

    it('call with Symbol Object', function () {
      expect(function () {
        Object(Symbol('hello')).toString();
      }).not.toThrow();
    });

    it('description', function () {
      expect(Object(Symbol('hello')).toString()).toBe('Symbol(hello)');
      expect(Object(Symbol()).toString()).toBe('Symbol()');
      expect(Object(Symbol.create).toString()).toBe('Symbol(Symbol.create)');
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
