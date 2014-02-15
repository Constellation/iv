describe("Symbol", function() {
  describe('@@toPrimitive', function() {
    it('throw TypeError', function () {
      expect(function () {
        Symbol.prototype[Symbol.toPrimitive]();
      }).toThrow();
    });

    it('name', function () {
      expect(Symbol.prototype[Symbol.toPrimitive].name).toBe('[Symbol.toPrimitive]');
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
