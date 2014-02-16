describe("Set", function() {
  describe('add', function() {
    it('with string', function() {
      var set = new Set();
      set.add('0');
      expect(set.has('0')).toBe(true);
      expect(set.has('-0')).toBe(false);
      expect(set.has(0)).toBe(false);
      expect(set.has(-0)).toBe(false);
    });

    it('with number', function() {
      var set = new Set();
      set.add(0);
      expect(set.has(0)).toBe(true);
      expect(set.has(-0)).toBe(true);  // SameValueZero
      expect(set.has('0')).toBe(false);
    });

    it('with boolean', function() {
      var set = new Set();
      set.add(true);
      expect(set.has(true)).toBe(true);
      expect(set.has(false)).toBe(false);
      expect(set.has(null)).toBe(false);
      expect(set.has(undefined)).toBe(false);
    });

    it('with NaN', function() {
      var set = new Set();
      set.add(NaN);
      expect(set.has(NaN)).toBe(true);
      expect(set.has(Infinity)).toBe(false);
      expect(set.has(false)).toBe(false);
    });

    it('with object', function() {
      var set = new Set();
      var key = {};
      set.add(key);
      expect(set.has(key)).toBe(true);
      expect(set.has({})).toBe(false);
    });
  });
});
