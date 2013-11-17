describe("Symbol", function() {
  describe('@@toStringTag', function() {
    it('value', function () {
      expect(Symbol.prototype[Symbol.toStringTag]).toBe('Symbol');
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
