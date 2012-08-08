describe("Map", function() {
  describe('delete', function() {
    it('with string', function() {
      var map = new Map();
      map.set('0', 0);
      expect(map.has('0')).toBe(true)
      map.delete('0');
      expect(map.has('0')).toBe(false)
    });

    it('with number', function() {
      var map = new Map();
      map.set(0, 0);
      expect(map.has(0)).toBe(true)
      map.delete(0);
      expect(map.has(0)).toBe(false)
    });

    it('with boolean', function() {
      var map = new Map();
      map.set(true, 0);
      expect(map.has(true)).toBe(true)
      map.delete(false);
      expect(map.has(true)).toBe(true)
      map.delete(true);
      expect(map.has(true)).toBe(false)
    });

    it('with NaN', function() {
      var map = new Map();
      expect(map.has(NaN)).toBe(false)
      map.set(NaN, 0);
      expect(map.has(NaN)).toBe(true)
      map.delete(NaN);
      expect(map.has(NaN)).toBe(false)
    });

    it('with object', function() {
      var map = new Map();
      var key = {};
      map.set(key, 0);
      expect(map.has(key)).toBe(true)
      expect(map.has({})).toBe(false)
      map.delete({});
      expect(map.has(key)).toBe(true)
      map.delete(key);
      expect(map.has(key)).toBe(false)
    });
  });
});
