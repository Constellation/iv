describe("Map", function() {
  describe('get', function() {
    it('with string', function() {
      var map = new Map();
      map.set('0', 0);
      expect(map.get('0')).toBe(0)
    });

    it('with number', function() {
      var map = new Map();
      map.set(0, 0);
      expect(map.get(0)).toBe(0)
    });

    it('with boolean', function() {
      var map = new Map();
      map.set(true, 0);
      expect(map.get(true)).toBe(0)
      expect(map.get(false)).toBe(undefined)
    });

    it('with NaN', function() {
      var map = new Map();
      map.set(NaN, 0);
      expect(map.get(NaN)).toBe(0)
    });

    it('with object', function() {
      var map = new Map();
      var key = {};
      map.set(key, 0);
      expect(map.get(key)).toBe(0)
      expect(map.get({})).toBe(undefined)
    });
  });
});
