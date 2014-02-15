describe("Symbol", function() {
  describe('properties', function () {
    it('constructor properties', function() {
      var symbols = [
        'create',
        'hasInstance',
        'isConcatSpreadable',
        'isRegExp',
        'iterator',
        'toPrimitive',
        'toStringTag',
        'unscopables'
      ];
      for (var i = 0; i < symbols.length; ++i) {
        expect(Object(Symbol[symbols[i]]).toString()).toBe('Symbol(Symbol.' + symbols[i] + ')');
      }
      expect(Symbol.prototype).toBe(Object.getPrototypeOf(Object(Symbol())));
      expect(typeof Symbol[Symbol.create]).toBe('function');
    });

    it('prototype properties', function() {
      expect(Symbol.prototype.constructor).toBe(Symbol);
      expect(typeof Symbol.prototype.toString).toBe('function');
      expect(typeof Symbol.prototype.valueOf).toBe('function');
      expect(typeof Symbol.prototype[Symbol.toPrimitive]).toBe('function');
      expect(Symbol.prototype[Symbol.toStringTag]).toBe('Symbol');
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
