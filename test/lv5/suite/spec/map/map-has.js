describe("Map", function() {
  describe('has', function() {
    it('with string', function() {
      var map = new Map();
      map.set('0', 0);
      expect(map.has('0')).toBe(true)
    });

    it('with number', function() {
      var map = new Map();
      map.set(0, 0);
      expect(map.has(0)).toBe(true)
      expect(map.has(-0)).toBe(false)
    });

    it('with boolean', function() {
      var map = new Map();
      map.set(true, 0);
      expect(map.has(true)).toBe(true)
      expect(map.has(false)).toBe(false)
    });

    it('with NaN', function() {
      var map = new Map();
      map.set(NaN, 0);
      expect(map.has(NaN)).toBe(true)
    });

    it('with object', function() {
      var map = new Map();
      var key = {};
      map.set(key, 0);
      expect(map.has(key)).toBe(true)
      expect(map.has({})).toBe(false)
    });
  });
});
