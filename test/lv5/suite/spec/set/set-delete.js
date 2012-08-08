describe("Set", function() {
  describe('delete', function() {
    it('with string', function() {
      var set = new Set();
      set.add('0');
      expect(set.has('0')).toBe(true)
      set.delete('0');
      expect(set.has('0')).toBe(false)
    });

    it('with number', function() {
      var set = new Set();
      set.add(0);
      expect(set.has(0)).toBe(true)
      set.delete(0);
      expect(set.has(0)).toBe(false)
    });

    it('with boolean', function() {
      var set = new Set();
      set.add(true);
      expect(set.has(true)).toBe(true)
      set.delete(false);
      expect(set.has(true)).toBe(true)
      set.delete(true);
      expect(set.has(true)).toBe(false)
    });

    it('with NaN', function() {
      var set = new Set();
      expect(set.has(NaN)).toBe(false)
      set.add(NaN);
      expect(set.has(NaN)).toBe(true)
      set.delete(NaN);
      expect(set.has(NaN)).toBe(false)
    });

    it('with object', function() {
      var set = new Set();
      var key = {};
      set.add(key);
      expect(set.has(key)).toBe(true)
      expect(set.has({})).toBe(false)
      set.delete({});
      expect(set.has(key)).toBe(true)
      set.delete(key);
      expect(set.has(key)).toBe(false)
    });
  });
});
